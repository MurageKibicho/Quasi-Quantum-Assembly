#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <gmp.h>
#include "29_2_Iris.h"
#include "ff_asm_primes.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define kb_PRIME_ARRAY_LENGTH 5239
#define kb_ENABLE_ERRORS 1
#define kb_TRUE 1
#define kb_FALSE 0
#define kb_INDEX(x, y, cols) ((x) * (cols) + (y))
#define PRINT_ERROR(msg) \
do { \
if (kb_ENABLE_ERRORS) { \
    fprintf(stderr, "\033[31mERROR: %s\033[0m\n", msg); \
    exit(1); \
} \
} while (0)
typedef struct kibicho_brocot_struct *kb;
typedef struct kibicho_brocot_layer_struct *kbLayer;
typedef struct kibicho_brocot_activation_struct *kbActivation;
struct kibicho_brocot_struct
{
	float number;
	float gradient;
	int sign;
	int numerator;
	int denominator;
	kb *children;
	void (*backward)(kb);
};

struct kibicho_brocot_activation_struct
{
	kb *weight;
};

struct kibicho_brocot_layer_struct
{
	size_t rowCount;
	size_t columnCount;
	int hasBias;
	kb *weight;
	kb *bias;
};
//Check if (a*b + c) overflows int32_t
int CheckAddMulOverflow(int32_t a, int32_t b, int32_t c) 
{
	if(a == 0 || b == 0){return 1;}
	if(a > 0){if(b > 0){if(a > (INT32_MAX - c) / b) return 0;}else{if(b < (INT32_MIN - c) / a) return 0;}}
	else{if(b > 0){if(a < (INT32_MIN - c) / b) return 0;}else{if(b < (INT32_MAX - c) / a) return 0;}}
	return 1;
}
void kb_initBack(kb output){}


kb kb_init(float number)
{
	if(isnan(number) || isinf(number)){PRINT_ERROR("kb_init Error: input is NaN or Infinity");}	
	kb result = malloc(sizeof(struct kibicho_brocot_struct));
	result->number = number;
	result->gradient = 0.0f;
	result->children = NULL;
	float remainder = number;
	int sign = (number < 0) ? -1 : 1;
	result->sign = sign;
	remainder = fabs(number);
	int32_t a, h[3] = {0, 1, 0}, k[3] = {1, 0, 0};
	for(int i = 0; i < 100; i++)//Prevent infinite loops
	{
		a = (int32_t)remainder;
		remainder = 1.0 / (remainder - a);
		if(!CheckAddMulOverflow(a, h[1], h[0]) || !CheckAddMulOverflow(a, k[1], k[0])){break;}
		h[2] = a * h[1] + h[0];
		k[2] = a * k[1] + k[0];
		if (k[2] < 0) break;
		h[0] = h[1]; h[1] = h[2];
       	k[0] = k[1]; k[1] = k[2];
       	if(remainder == 0) break;
	}
	//printf("%ld %ld : %.3f\n", h[1], k[1], (float) h[1]/ (float) k[1]);
	result->numerator = h[1];
	result->denominator = k[1];
	result->backward = kb_initBack;
	return result;
}

void kb_print(kb a)
{
	if(a == NULL){PRINT_ERROR("kb_print Error: kb is NULL");}
	printf("%.3f : %.3f (%c %3d %3d)\n", a->number, a->gradient, a->sign < 0 ? '-': '+', a->numerator, a->denominator);
}

void kb_clear(kb a)
{
	if(a)
	{
		if(a->children)
		{
			arrfree(a->children);
		}
		free(a);
	}
}


void kb_addBack(kb output)
{
	if(arrlen(output->children) < 2){PRINT_ERROR("kb_addBack Error: children < 2");}
	output->children[0]->gradient += output->gradient;
	output->children[1]->gradient += output->gradient;
}

kb kb_add(kb a, kb b)
{
	kb c = kb_init(a->number + b->number);
	c->backward = kb_addBack;
	arrput(c->children, a);
	arrput(c->children, b);
	return c;
}

void kb_mulBack(kb output)
{
	if(arrlen(output->children) < 2){PRINT_ERROR("kb_mulBack Error: children < 2");}
	output->children[0]->gradient += output->children[1]->number * output->gradient;
	output->children[1]->gradient += output->children[0]->number * output->gradient;
}

kb kb_mul(kb a, kb b)
{
	kb c = kb_init(a->number * b->number);
	c->backward = kb_mulBack;
	arrput(c->children, a);
	arrput(c->children, b);
	return c;
}

void kb_tanhBack(kb output)
{
	if(arrlen(output->children) < 1){PRINT_ERROR("kb_tanhBack Error: children < 1");}
	output->children[0]->gradient += (1 - pow(output->number, 2)) * output->gradient;
}
kb kb_tanh(kb a)
{
	kb c = kb_init(tanh(a->number));
	c->backward = kb_tanhBack;
	arrput(c->children, a);
	return c;
}

void kb_TopologicalSort(kb v, kb* topo, int* topo_size, kb* visited, int* visited_size)
{
	for(int i = 0; i < *visited_size; ++i) {if (visited[i] == v) return;}
	visited[*visited_size] = v;
	(*visited_size)++;
	for(size_t i = 0; i < arrlen(v->children); ++i){kb_TopologicalSort(v->children[i], topo, topo_size, visited, visited_size);}
	topo[*topo_size] = v;
	(*topo_size)++;
}

float random_uniform(float min, float max) 
{
	return min + (max - min) * ((float)rand() / RAND_MAX);
}

kbLayer kbLayer_init(size_t rowCount, size_t columnCount, int bias)
{
	kbLayer result = malloc(sizeof(struct kibicho_brocot_layer_struct));
	result->rowCount = rowCount;
	result->columnCount = columnCount;
	result->hasBias = bias;
	if(bias == kb_TRUE)
	{
		result->bias = malloc(rowCount * columnCount * sizeof(kb));
		result->weight = malloc(rowCount * columnCount * sizeof(kb));
		for(size_t i = 0; i < rowCount * columnCount; i++)
		{
			result->weight[i] = kb_init(random_uniform(-1.0f, 1.0f));
			result->bias[i] = kb_init(random_uniform(-1.0f, 1.0f));
		}
	}
	else
	{
		result->bias = NULL;
		result->weight = malloc(rowCount * columnCount * sizeof(kb));
		for(size_t i = 0; i < rowCount * columnCount; i++)
		{
			result->weight[i] = kb_init(random_uniform(-1.0f, 1.0f));	
		}
	}
	return result;
}

void kbActivation_clear(kbActivation activation)
{
	if(activation)
	{
		if(activation->weight)
		{
			for(size_t i = 0; i < arrlen(activation->weight); i++)
			{
				if(activation->weight[i]){kb_clear(activation->weight[i]);}
			}
			arrfree(activation->weight);
		}
		free(activation);
	}
}

void kbLayer_clear(kbLayer layer)
{
	if(layer)
	{
		if(layer->bias && layer->weight)
		{
			for(size_t i = 0; i < layer->rowCount * layer->columnCount; i++)
			{
				if(layer->weight[i]){kb_clear(layer->weight[i]);}
				if(layer->bias[i]){kb_clear(layer->bias[i]);}
			}
			free(layer->bias);
			free(layer->weight);
		}
		else if(layer->weight)
		{
			for(size_t i = 0; i < layer->rowCount * layer->columnCount; i++)
			{
				kb_clear(layer->weight[i]);
			}
			free(layer->weight);
		}
		free(layer);
	}
}


void TestLinearExpression()
{
	kb a = kb_init(2.0);
	kb b = kb_init(-3.0);
	kb c = kb_init(10.0);
	kb d = kb_mul(a, b);
	kb e = kb_add(d,c);
	kb f = kb_init(-2.0);
	kb L = kb_mul(e,f);
	kb_print(a);
	kb_print(b);
	kb_print(c);
	kb_print(d);
	kb_print(e);
	kb_print(f);
	kb_print(L);
	kb_clear(a);
	kb_clear(b);
	kb_clear(c);
	kb_clear(d);
	kb_clear(e);
	kb_clear(f);
	kb_clear(L);
}

void TestNeuron()
{
	/*inputs : x1, x2*/
	kb x1 = kb_init(2.0);
	kb x2 = kb_init(0.0);
	/*weights : w1, w2*/
	kb w1 = kb_init(-3.0);
	kb w2 = kb_init(1.0);
	/*bias*/
	kb bias = kb_init(6.8813735870195432);

	kb x1w1 = kb_mul(x1,w1);
	kb x2w2 = kb_mul(x2,w2);
	kb x1w1x2w2 = kb_add(x1w1,x2w2);
	kb n = kb_add(x1w1x2w2, bias);
	kb o = kb_tanh(n);
	o->gradient = 1.0;
	
	o->backward(o);
	n->backward(n);
	x1w1x2w2->backward(x1w1x2w2);
	x2w2->backward(x2w2);
	x1w1->backward(x1w1);

	kb sortedList[100];
	int sortedListSize = 0;
	kb visited[1000];
	int visitedSize = 0;
	kb_TopologicalSort(o, sortedList, &sortedListSize, visited, &visitedSize);

	for(int i = 0; i < 10; i++)
	{
		sortedList[i]->gradient = 0.0f;
		kb_print(sortedList[i]);
	}
	o->gradient = 1.0;
	for(int i = sortedListSize - 1; i >= 0; --i)
	{
		if(sortedList[i]->backward){sortedList[i]->backward(sortedList[i]);}
	}
	printf("\nEnd Topo Print\n");
	kb_print(x1);
	kb_print(w1);
	kb_print(x2);
	kb_print(w2);
	kb_print(bias);
	kb_print(x1w1);
	kb_print(x2w2);
	kb_print(x1w1x2w2);
	kb_print(n);
	kb_print(o);
	
	kb_clear(x1);
	kb_clear(w1);
	kb_clear(x2);
	kb_clear(w2);
	kb_clear(bias);
	kb_clear(x1w1);
	kb_clear(x2w2);
	kb_clear(x1w1x2w2);
	kb_clear(n);
	kb_clear(o);
}

kbActivation ForwardPass(size_t r0, size_t c0, size_t c1, kbLayer left, kbLayer right)
{
	kbActivation result = malloc(sizeof(struct kibicho_brocot_activation_struct));
	result->weight = NULL;
	for(size_t i = 0; i < r0; i++)
	{
		for(size_t j = 0; j < c1; j++)
		{
			kb sum = kb_init(0.0f);
			arrput(result->weight, sum);
			for(size_t k = 0; k < c0; k++)
			{
				size_t leftIndex = kb_INDEX(i,k, c0);
				size_t rightIndex = kb_INDEX(k,j, c1);
				kb product = kb_mul(left->weight[leftIndex], right->weight[rightIndex]);
				kb newSum = kb_add(sum, product);
				arrput(result->weight, product);
				arrput(result->weight, newSum);
				sum = newSum;
				//printf("(%ld %ld : %ld %ld )\n", i, k, k, j);
			}	
		}
	}
	return result;
}


void PrintActivation(kbActivation activation)
{
	for(size_t i = 0; i < arrlen(activation->weight); i++)
	{
		printf("%ld : %.3f\n", i, activation->weight[i]->number);
	}
	printf("\n");
}
void PrintLayer(kbLayer layer)
{
	for(size_t i = 0; i < layer->rowCount; i++)
	{
		for(size_t j = 0; j < layer->columnCount; j++)
		{
			printf("%.6f,", layer->weight[kb_INDEX(i,j,layer->columnCount)]->number);
		}
		printf("\n");
	}
	printf("\n");
}

void ShuffleIrisDataset(iris_dataset_holder *array, size_t n)
{
	srand(4567870);
	for(size_t i = n - 1; i > 0; i--)
	{
		size_t j = rand() % (i + 1);
		iris_dataset_holder temp = array[i];
		array[i] = array[j];
		array[j] = temp;
	}
}

void TestLayer()
{
	size_t irisDataSetSize = 150;
	ShuffleIrisDataset(iris_dataset_holder_array, irisDataSetSize);
	srand(3456);
	size_t trainSize = (150*8)/10;
	size_t testSize  = irisDataSetSize - trainSize;
	
	size_t batchSize = 2;
	size_t inputDimension = 2;
	size_t outputDimension = 3;
	size_t hiddenDimension0 = 3;
	size_t hiddenDimension1 = 3;
	
	kbLayer input   = kbLayer_init(batchSize, inputDimension, kb_FALSE);
	kbLayer hidden0 = kbLayer_init(inputDimension, hiddenDimension0, kb_TRUE);
	kbLayer hidden1 = kbLayer_init(hiddenDimension0, hiddenDimension1, kb_TRUE);
	kbLayer output  = kbLayer_init(hiddenDimension1, outputDimension, kb_FALSE);

	kbActivation activation0 = ForwardPass(batchSize, inputDimension, hiddenDimension0, input, hidden0);
	PrintLayer(input);
	PrintLayer(hidden0);
	PrintActivation(activation0);

	kbActivation_clear(activation0);
	kbLayer_clear(input);
	kbLayer_clear(hidden0);
	kbLayer_clear(hidden1);
	kbLayer_clear(output);
}

int main()
{
	//TestLinearExpression();
	//TestNeuron();
	TestLayer();
	return 0;
}
