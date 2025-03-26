#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define MAX_PREVS 3
#define MAX_ARGS 5
#define MAX_PARAM_TENSORS 10
// op codes
#define MATMUL 0
#define MEAN 1
#define MUL 2

typedef struct ffasm_array_struct *ffasmArray;
typedef struct ffasm_tensor_struct *ffasmTensor;
struct ffasm_array_struct
{
	int size;
	int numberOfDimensions;
	int *shape;
	int *strides;
	float *data;
};
typedef union
{
	int ival;
	float fval;
}Arg;

struct ffasm_tensor_struct
{
	ffasmArray data;
	ffasmArray grad;
	int op;
	struct ffasm_tensor_struct* prevs[MAX_PREVS]; 
	int num_prevs;
	Arg args[MAX_ARGS];
};


void LogSoftmax(ffasmArray input, ffasmArray output)
{
	// input and output are both (B,C)
	for(int b = 0; b < input->shape[0]; b++)
	{
		float maxV = input->data[b * input->strides[0]];
		for(int c = 1; c < input->shape[1]; c++)
		{
			int index = b * input->strides[0] + c * input->strides[1];
			if(maxV < input->data[index])
			{
				maxV = input->data[index];
			}
		}
		float sumExp = 0.0f;
		for(int c = 0; c < input->shape[1]; c++)
		{
			int index = b * input->strides[0] + c * input->strides[1];
			float expVal = expf(input->data[index] - maxV);
			sumExp += expVal;
		}
		for(int c = 0; c < input->shape[1]; c++)
		{
			int index = b * input->strides[0] + c * input->strides[1];
			output->data[index] = input->data[index] - maxV - logf(sumExp);
		}
	}
}

void LogSoftmaxBack(ffasmArray dInput, ffasmArray dOutput, ffasmArray output)
{
	for(int b = 0; b < output->shape[0]; b++)
	{
		float gradSum = 0.0f;
		for(int c = 0; c < output->shape[1]; c++)
		{
			gradSum += dOutput->data[b * output->shape[1] + c];
		}
		for(int c = 0; c < output->shape[1]; c++)
		{
			int index = b * output->shape[1] + c;
			dInput->data[index] = dOutput->data[index] - expf(output->data[index]) *gradSum;
		}
	}
}

void Relu(ffasmArray output, ffasmArray input)
{
	for(int i = 0; i < input->size; i++)
	{
		output->data[i] = (input->data[i] > 0) ? input->data[i] : 0;
	}
}

void ReluBack(ffasmArray dInput, ffasmArray dOutput, ffasmArray input)
{
	for(int i = 0; i < input->size; i++)
	{
		dInput->data[i] = (input->data[i] > 0) ? dOutput->data[i] : 0;	
	}
}

ffasmArray ffasmArray_CreateArrayZeros(int numberOfDimensions, int *shape)
{
	ffasmArray array = malloc(sizeof(struct ffasm_array_struct));
	array->numberOfDimensions = numberOfDimensions;
	array->shape   = calloc(numberOfDimensions, sizeof(int));
	array->strides = calloc(numberOfDimensions, sizeof(int));
	memcpy(array->shape, shape, numberOfDimensions * sizeof(int));
	array->size = 1;
	for(int i = numberOfDimensions - 1; i >= 0; i--)
	{
		array->strides[i] = array->size;
		array->size *= shape[i];
	}
	array->data = calloc(array->size, sizeof(float));
	return array;
}

ffasmArray ffasmArray_CreateArray(int numberOfDimensions, int *shape, float *data)
{
	ffasmArray array = ffasmArray_CreateArrayZeros(numberOfDimensions, shape);
	memcpy(array->data, data, array->size * sizeof(float));
	return array;
}

void ffasmArray_Free(ffasmArray array)
{
	if(array)
	{
		free(array->data);
		free(array->shape);
		free(array->strides);
		free(array);
	}
}

ffasmTensor ffasmTensor_CreateTensor(int numberOfDimensions, int *shape, float *data)
{	
	ffasmTensor tensor = malloc(sizeof(struct ffasm_tensor_struct));
	tensor->data = ffasmArray_CreateArray(numberOfDimensions, shape, data);
	tensor->grad = ffasmArray_CreateArrayZeros(numberOfDimensions, shape);
	tensor->op = -1;
	tensor->num_prevs = 0;
	return tensor;	
}
ffasmTensor ffasmTensor_CreateZeroTensor(int numberOfDimensions, int *shape)
{	
	ffasmTensor tensor = malloc(sizeof(struct ffasm_tensor_struct));
	tensor->data = ffasmArray_CreateArrayZeros(numberOfDimensions, shape);
	tensor->grad = ffasmArray_CreateArrayZeros(numberOfDimensions, shape);
	tensor->op = -1;
	tensor->num_prevs = 0;
	return tensor;	
}
ffasmTensor ffasmTensor_Matmul(ffasmTensor a, ffasmTensor b)
{
	//(P,Q) x (Q,R) = (P,R)
	int P = a->data->shape[0];
	int Q = a->data->shape[1];
	int R = b->data->shape[1];
	ffasmTensor c = ffasmTensor_CreateZeroTensor(2, (int[]){P,R});
	for(int i = 0; i < P; i++)
	{
		for(int j = 0; j < R; j++)
		{
			float temp = 0.0f;
			for(int k = 0; k < Q; k++)
			{
				int aIndex = i * a->data->strides[0] + k * a->data->strides[1];
				int bIndex = k * b->data->strides[0] + j * b->data->strides[1];
				temp += a->data->data[aIndex] * b->data->data[bIndex];
			}
			int cIndex = i * c->data->strides[0] + j * c->data->strides[1];
			c->data->data[cIndex] = temp; 
		}
	}
	c->op = MATMUL;
	c->num_prevs = 2;
	c->prevs[0] = a;
	c->prevs[1] = b;
	return c;
}

void ffasmTensor_MatmulBack(ffasmTensor output)
{
	// a (P,Q), b (Q,R), c (P, R)	
	int P = output->prevs[0]->data->shape[0];
	int Q = output->prevs[0]->data->shape[1];
	int R = output->prevs[1]->data->shape[1];
	// dc x b.T  (P,R) x (R,Q) => (P,Q)
	for(int i = 0; i < P; i++)
	{
		for(int j = 0; j < Q; j++)
		{
			float temp = 0.0f;
			for(int k = 0; k < R; k++)
			{
				// (k,j) in b.T is (j,k) in b
				int bIndex = j * output->prevs[1]->data->strides[0] + k * output->prevs[1]->data->strides[1];
				temp += output->grad->data[i * R + k] * output->prevs[1]->data->data[bIndex];
			}
			int daIndex = i * Q + j;
			output->prevs[0]->grad->data[daIndex] = temp;
		}
	}
	
	// a.T x dc  (Q,P) x (P,R) => (Q,R)
	for(int i = 0; i < Q; i++)
	{
		for(int j = 0; j < R; j++)
		{
			float temp = 0.0f;
			for(int k = 0; k < P; k++)
			{
				// (i,k) in a.T is (k,i) in a
				int aIndex = k * output->prevs[0]->data->strides[0] + i * output->prevs[0]->data->strides[1];
				temp += output->grad->data[k * R + j] * output->prevs[0]->data->data[aIndex];
			}
			int dbIndex = i * R + j;
			output->prevs[1]->grad->data[dbIndex] = temp;
		}
	}
	
}

ffasmTensor ffasmTensor_Multiply(ffasmTensor a, ffasmTensor b)
{
	assert(a->data->size == b->data->size);
	float *floatHolder = malloc(a->data->size * sizeof(float));
	for(int i = 0; i < a->data->size; i++)
	{
		floatHolder[i] = a->data->data[i] * b->data->data[i];
	}
	ffasmTensor result = ffasmTensor_CreateTensor(a->data->numberOfDimensions, a->data->shape, floatHolder);
	result->op = MUL;
	result->num_prevs = 2;
	result->prevs[0]  = a;
	result->prevs[1]  = b;
	free(floatHolder);
	return result;
}


void ffasmTensor_MultiplyBack(ffasmTensor output)
{
	for(int i = 0; i < output->data->size; i++)
	{
		output->prevs[0]->grad->data[i] += output->grad->data[i] * output->prevs[1]->data->data[i];
        	output->prevs[1]->grad->data[i] += output->grad->data[i] * output->prevs[0]->data->data[i];
	}
}


ffasmTensor ffasmTensor_Free(ffasmTensor tensor)
{
	if(tensor)
	{
		if(tensor->data){ffasmArray_Free(tensor->data);}
		if(tensor->grad){ffasmArray_Free(tensor->grad);}
		free(tensor);
	}
}

void ffasmTensor_Print(ffasmTensor tensor)
{
	printf("Tensor(\n");
	printf("\tdata: ");
	for(int i = 0; i < tensor->data->size; i++) printf("%.1f,", tensor->data->data[i]);
	printf("\n\tshape: ");
	for (int i = 0; i < tensor->data->numberOfDimensions; i++) printf("%d,", tensor->data->shape[i]);
	printf("\n\tgrad: ");
	for (int i = 0; i < tensor->data->size; i++) printf("%.1f,", tensor->grad->data[i]);
	printf("\n)\n");
}

void ffasmTensor_Back(ffasmTensor tensor)
{
	if(tensor->op == MUL)
	{
		ffasmTensor_MultiplyBack(tensor);
	}
	else if(tensor->op == MATMUL)
	{
		ffasmTensor_MatmulBack(tensor);
	}
	for(int i = 0; i < tensor->num_prevs; i++){ffasmTensor_Back(tensor->prevs[i]);}
}
	

void TestTensor()
{
	ffasmTensor a = ffasmTensor_CreateTensor(2, (int []){2,2}, (float[]){1.0, 2.0, 3.0, 4.0});
	ffasmTensor b = ffasmTensor_CreateTensor(2, (int []){2,2}, (float[]){2.0, 2.0, 2.0, 2.0});
	ffasmTensor c = ffasmTensor_Multiply(a, b);
	c->grad->data[0] = 1.0f;
	c->grad->data[1] = 1.0f;
	c->grad->data[2] = 1.0f;
	c->grad->data[3] = 1.0f;
	ffasmTensor_Back(c);
	
	/*Test Multiply*/
	printf("\nTest Multiply\n");
	ffasmTensor_Print(a);
	ffasmTensor_Print(b);
	ffasmTensor_Print(c);
	
	ffasmTensor a1 = ffasmTensor_CreateTensor(2, (int []){2,2}, (float[]){1.0, 6.0, -3.0, 44.0});
	ffasmTensor b1 = ffasmTensor_CreateTensor(2, (int []){2,2}, (float[]){-2.0, 2.0, 12.0, 3.0});
	ffasmTensor c1 = ffasmTensor_Matmul(a1, b1);
	c1->grad->data[0] = -5.0f;
	c1->grad->data[1] = -1.0f;
	c1->grad->data[2] = 240.0f;
	c1->grad->data[3] = 37.0f;
	ffasmTensor_Back(c1);
	/*Test Matmul*/
	printf("\nTest Matmul\n");
	ffasmTensor_Print(a1);
	ffasmTensor_Print(b1);
	ffasmTensor_Print(c1);
	
	
	ffasmTensor_Free(a);
	ffasmTensor_Free(b);
	ffasmTensor_Free(c);
	ffasmTensor_Free(a1);
	ffasmTensor_Free(b1);
	ffasmTensor_Free(c1);
}


int main()
{
	TestTensor();
	return 0;
}
