#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include "ff_asm_primes.h"
#include "29_2_Iris.h"
#include "../Datasets/mnist/mnistIO.h"
#include <gmp.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define m32_ENABLE_ERRORS 1
#define m32_TRUE 1
#define m32_FALSE 0
#define m32_INDEX(x, y, cols) ((x) * (cols) + (y))
#define PRINT_ERROR(msg) \
do { \
if (m32_ENABLE_ERRORS) { \
    fprintf(stderr, "\033[31mERROR: %s\033[0m\n", msg); \
    exit(1); \
} \
} while (0)
typedef struct m32_rational_struct *m32;
typedef struct model_struct *Model;
typedef struct linear_layer_struct *LinearLayer;
struct linear_layer_struct
{
	size_t rowCount;
	size_t columnCount;
	m32 *bias; 
	m32 *weight;
	m32 *output;
};
struct model_struct
{
	size_t batchSize;
	gmp_randstate_t state;
	LinearLayer *linearLayers;
};

struct m32_rational_struct
{
	mpz_t numerator;
	mpz_t denominator;
};

m32 m32_init(gmp_randstate_t state)
{
	m32 result = malloc(sizeof(struct m32_rational_struct));

	mpz_init(result->numerator);
	mpz_init(result->denominator);
	
	mpz_urandomb(result->numerator, state, 4);
	mpz_urandomb(result->denominator, state, 4);
	
	int numSign = rand() % 2 ? -1 : 1;
	int denSign = rand() % 2 ? -1 : 1;
	mpz_mul_si(result->numerator, result->numerator, numSign);
	mpz_mul_si(result->denominator, result->denominator, denSign);
	return result;
}

void m32_print(m32 a)
{
	if(a)
	{
		mpf_t fn, fd, result;
		mpf_inits(fn, fd, result, NULL);
		mpf_set_ui(fd, 1);
		mpf_set_z(fn, a->numerator);
		if(mpz_sgn(a->denominator) != 0){mpf_set_z(fd, a->denominator);}
		mpf_div(result, fn, fd);
		gmp_printf("(%2Zd %2Zd  %.1Ff)", a->numerator, a->denominator, result);
		mpf_clears(fn, fd, result, NULL);
	}
}

void m32_clear(m32 a)
{
	if(a)
	{
		mpz_clear(a->numerator);
		mpz_clear(a->denominator);
		free(a);
	}
}

void LoadInputOutput_Mnist(Model model, unsigned char *trainImages, unsigned char *trainLabels, int inputIndex, int trainImagesCount, int *target)
{
	//batchSize is 1
	assert(inputIndex >= 0);
	assert(inputIndex < trainImagesCount);
	assert(model != NULL);
	assert(model->linearLayers[0] != NULL);
	assert(arrlen(model->linearLayers[0]->output) == 784);
	for(int i = 0; i < 784; i++)
	{
		mpz_set_ui(model->linearLayers[0]->output[i]->numerator, trainImages[(inputIndex * 784) + i]);
		//Normalize by dividing by 255
		mpz_set_ui(model->linearLayers[0]->output[i]->denominator, 255);
	}
	*target = trainLabels[inputIndex];
}

void LoadInputOutput_Iris(Model model, size_t inputIndex, size_t inputLength, size_t batchSize, size_t datasetSize, iris_dataset_holder iris_dataset_holder_array[], int *target)
{
	assert(batchSize > 0);assert(model != NULL);
	assert(inputIndex + batchSize < datasetSize);
	assert(model->linearLayers[0] != NULL);
	assert(arrlen(model->linearLayers[0]->output) == batchSize * inputLength);
	size_t localInputIndex = 0;
	
	for(size_t i = 0; i < batchSize; i++)
	{
		for(size_t j = 0; j < inputLength; j++)
		{
			assert(localInputIndex < arrlen(model->linearLayers[0]->output));
			int numerator = iris_dataset_holder_array[inputIndex + i].inputs[j] * 10;
			mpz_set_si(model->linearLayers[0]->output[localInputIndex]->numerator, numerator);
			mpz_set_ui(model->linearLayers[0]->output[localInputIndex]->denominator, 10);
			localInputIndex += 1;
		}
		for(int j = 0; j < 3; j++)
		{
			float value = iris_dataset_holder_array[inputIndex + i].output[j];
			if(value == 1.0){target[i] = j;break;}
		}
	}
	
}

void PrintModel(Model model)
{
	assert(model != NULL);
	printf("Layer count : %ld\nBatch size : %ld\n\n", arrlen(model->linearLayers), model->batchSize);
	for(size_t i = 0; i < arrlen(model->linearLayers); i++)
	{
		size_t weightRow = model->linearLayers[i]->rowCount;
		size_t weightColumn = model->linearLayers[i]->columnCount;
		size_t biasRow = model->batchSize;
		size_t biasColumn = model->linearLayers[i]->columnCount;
		size_t outputRow = model->batchSize;
		size_t outputColumn = model->linearLayers[i]->columnCount;
		printf("\tLayer %2ld : Weight[%3ld,%3ld], Bias[%3ld,%3ld],Output[%3ld,%3ld]\n", i, weightRow, weightColumn, biasRow, biasColumn, outputRow, outputColumn);
	}
}

Model CreateModel(size_t batchSize)
{
	assert(batchSize != 0);
	Model model = malloc(sizeof(struct model_struct));
	model->linearLayers = NULL;
	model->batchSize = batchSize;
	gmp_randinit_mt(model->state);
	return model;
}

LinearLayer CreateLayer(size_t rowCount, size_t columnCount, size_t batchSize, gmp_randstate_t state)
{
		
	LinearLayer layer = malloc(sizeof(struct linear_layer_struct));
	layer->rowCount = rowCount;
	layer->columnCount = columnCount;
	layer->bias   = NULL;
	layer->output = NULL; 
	layer->weight= NULL;
	for(size_t i = 0; i < rowCount * columnCount; i++)
	{
		m32 temp0 = m32_init(state);
		arrput(layer->weight, temp0);
	}
	for(size_t i = 0; i < batchSize * columnCount; i++)
	{
		m32 temp0 = m32_init(state);
		m32 temp1 = m32_init(state);
		arrput(layer->bias, temp0);
		arrput(layer->output, temp1);
	}
	
	return layer;
}

void AddLinearLayer(Model model, size_t rowCount, size_t columnCount, size_t batchSize)
{
	assert(model != NULL);
	LinearLayer layer = CreateLayer(rowCount, columnCount, batchSize, model->state);
	arrput(model->linearLayers, layer);	
}

void PrintInputLayer(Model model, size_t batchSize, int *target)
{
	for(size_t i = 0; i < model->linearLayers[0]->rowCount; i++)
	{
		for(size_t j = 0; j < model->linearLayers[0]->columnCount; j++)
		{
			m32_print(model->linearLayers[0]->output[m32_INDEX(i,j, model->linearLayers[0]->columnCount)]);
			printf(",");	
		}
		printf(" : %d\n", target[i]);
	}
	printf("\n");
}
void TestOutputLayer(Model model, size_t layerIndex)
{
	for(size_t i = 0; i < model->batchSize; i++)
	{
		for(size_t j = 0; j < model->linearLayers[layerIndex]->columnCount; j++)
		{
			m32_print(model->linearLayers[layerIndex]->output[m32_INDEX(i,j, model->linearLayers[layerIndex]->columnCount)]);
			printf(",");	
		}
		printf("\n");
	}
}
void TestWeightLayer(Model model, size_t layerIndex)
{
	for(size_t i = 0; i < model->linearLayers[layerIndex]->rowCount; i++)
	{
		for(size_t j = 0; j < model->linearLayers[layerIndex]->columnCount; j++)
		{
			m32_print(model->linearLayers[layerIndex]->weight[m32_INDEX(i,j, model->linearLayers[layerIndex]->columnCount)]);
			printf(",");	
		}
		printf("\n");
	}
	printf("\n");
}
void PrintLayerWeights(Model model)
{
	for(size_t k = 1; k < arrlen(model->linearLayers); k++)
	{
		for(size_t i = 0; i < model->linearLayers[k]->rowCount; i++)
		{
			for(size_t j = 0; j < model->linearLayers[k]->columnCount; j++)
			{
				size_t index = m32_INDEX(i,j, model->linearLayers[k]->columnCount);
				assert(index < model->linearLayers[k]->rowCount * model->linearLayers[k]->columnCount);
				m32_print(model->linearLayers[k]->weight[index]);
				printf(",");	
			}
			printf("\n");
		}
		printf("\n");
	}
}


void PrintLinearLayer(LinearLayer layer)
{
	assert(layer != NULL);
	for(size_t i = 0; i < arrlen(layer->output); i++)
	{
		m32_print(layer->output[i]);
	}
	printf("\n");
}


void FreeLayer(LinearLayer layer)
{
	if(layer)
	{
		if(layer->output)
		{
			for(size_t i = 0; i < arrlen(layer->output); i++)
			{
				m32_clear(layer->output[i]);
			}
			arrfree(layer->output);
		}
		if(layer->weight)
		{
			for(size_t i = 0; i < arrlen(layer->weight); i++)
			{
				m32_clear(layer->weight[i]);
			}
			arrfree(layer->weight);
		}
		if(layer->bias)
		{
			for(size_t i = 0; i < arrlen(layer->bias); i++)
			{
				m32_clear(layer->bias[i]);
			}
			arrfree(layer->bias);
		}
		free(layer);
	}
}

void FreeModel(Model model)
{
	if(model)
	{
		for(size_t i = 0; i < arrlen(model->linearLayers); i++)
		{
			FreeLayer(model->linearLayers[i]);
		}
		arrfree(model->linearLayers);
		gmp_randclear(model->state);
		free(model);
	}
}

struct fullyconnected
{
	m32 *input;
	m32 *weight;
	m32 *bias;
	m32 *output;
	size_t inputRowCount;
	size_t inputColumnCount;
	size_t outputRowCount;
	size_t outputColumnCount;
	size_t batchSize;
};

void MultiplyLinearLayer(struct fullyconnected *layer)
{
	assert(layer->inputColumnCount == layer->outputRowCount);
	size_t matrixIndex = 0;
	size_t outputIndex = 0;
	size_t weightIndex = 0;
	mpz_t temp0, temp1;
	mpz_inits(temp0, temp1, NULL);
	for(size_t b = 0; b < layer->batchSize; b++)
	{
		//Add Bias to output
		for(size_t i = 0; i < layer->outputColumnCount; i++)
		{
			matrixIndex = m32_INDEX(b, i, layer->outputColumnCount);
			assert(matrixIndex < arrlen(layer->output));
			assert(matrixIndex < arrlen(layer->bias));
			mpz_set(layer->output[matrixIndex]->numerator, layer->bias[i]->numerator);
			mpz_set(layer->output[matrixIndex]->denominator, layer->bias[i]->denominator);
			//mpz_set_ui(layer->output[matrixIndex]->numerator, 0);
			//mpz_set_ui(layer->output[matrixIndex]->denominator, 0);
		}
		//Matmul
		for(size_t i = 0; i < layer->inputColumnCount; i++)
		{
			matrixIndex = m32_INDEX(b, i, layer->inputColumnCount);
			assert(matrixIndex < arrlen(layer->input));
			m32 currentInput = layer->input[matrixIndex];
			for(size_t j = 0; j < layer->outputColumnCount; j++)
			{
				outputIndex = m32_INDEX(b, j, layer->outputColumnCount);
				weightIndex = m32_INDEX(i, j, layer->outputColumnCount);
				assert(outputIndex < arrlen(layer->output));
				assert(weightIndex < arrlen(layer->weight));
				mpz_mul(temp0, currentInput->numerator, layer->weight[weightIndex]->numerator);
				mpz_mul(temp1, currentInput->denominator, layer->weight[weightIndex]->denominator);
				mpz_add(layer->output[outputIndex]->numerator,layer->output[outputIndex]->numerator,temp0);
				mpz_add(layer->output[outputIndex]->denominator,layer->output[outputIndex]->denominator,temp1);
			}
		}
	}
	mpz_clears(temp0, temp1, NULL);
}

void Forward(Model model)
{
	for(size_t i = 1; i < arrlen(model->linearLayers); i++)
	{
		//PrintLinearLayer(model->linearLayers[i]);
		assert(arrlen(model->linearLayers[i]->output) == arrlen(model->linearLayers[i]->bias));
		TestOutputLayer(model, i-1);
		TestWeightLayer(model, i);printf("\n");
		MultiplyLinearLayer(&(struct fullyconnected)
		{
			.inputRowCount = model->linearLayers[i-1]->rowCount,
			.inputColumnCount = model->linearLayers[i-1]->columnCount,
			.outputRowCount = model->linearLayers[i]->rowCount,
			.outputColumnCount = model->linearLayers[i]->columnCount,
			.batchSize = model->batchSize,
			.input = model->linearLayers[i-1]->output,
			.weight = model->linearLayers[i]->weight,
			.bias = model->linearLayers[i]->bias,
			.output = model->linearLayers[i]->output
		});
	}	
	//TestOutputLayer(model, arrlen(model->linearLayers)-1);
}

float Backward(Model model, int *target)
{
	TestOutputLayer(model, arrlen(model->linearLayers)-1);
}

void TestIris()
{
	srand(1608096);
	
	ShuffleIrisDataset(iris_dataset_holder_array, 150);
	size_t hiddenLayerNeurons[] = {3,4,1,2};

	size_t loadInputIndex = 0;
	size_t trainLength = 0.7 * 150;
	size_t testLength  = 150 - trainLength;
	size_t batchSize = 4;
	size_t inputLength = 4;
	size_t predictionLength = 3;
	float loss = 0.0f;
	int *target = calloc(batchSize, sizeof(int));
	Model model = CreateModel(batchSize);
	AddLinearLayer(model, batchSize, inputLength, batchSize);
	for(size_t i = 0; i < sizeof(hiddenLayerNeurons) / sizeof(size_t); i++)
	{
		assert(hiddenLayerNeurons[i] != 0);
		AddLinearLayer(model, model->linearLayers[i]->columnCount, hiddenLayerNeurons[i], batchSize);
	}
	AddLinearLayer(model, model->linearLayers[arrlen(model->linearLayers) - 1]->columnCount, predictionLength, batchSize);
	
	PrintModel(model);
	
	{
		LoadInputOutput_Iris(model, loadInputIndex, inputLength, batchSize, trainLength, iris_dataset_holder_array, target);
		PrintInputLayer(model, batchSize, target);
		//PrintLayerWeights(model);
		Forward(model);
		loss = Backward(model, target);
		loadInputIndex += batchSize;
	}
	
	free(target);
	FreeModel(model);

}

int main()
{
	TestIris();
	return 0;
}


int TestMnist()
{
	srand(657845);
	char *trainImagesPath = "../Datasets/mnist/train-images.idx3-ubyte";
	char *trainLabelsPath = "../Datasets/mnist/train-labels.idx1-ubyte";
	int trainImagesCount = 0;
	int trainLabelsCount = 0;
	unsigned char *trainImages = ReadMnistImages(trainImagesPath, &trainImagesCount);
	unsigned char *trainLabels = ReadMnistLabels(trainLabelsPath, &trainLabelsCount);
	
	assert(trainLabels != NULL);
	assert(trainImagesCount == trainLabelsCount);
	ShuffleData(trainImages, trainLabels, trainImagesCount);
	
	size_t batchSize   = 1;
	size_t inputLength = 784;
	size_t outputLength = 10;
	int mnistTarget = 0;
	int inputIndex = 0;
	size_t hiddenLayer[] = {128};
	Model model = CreateModel(batchSize);
	AddLinearLayer(model, batchSize, inputLength, batchSize);
	AddLinearLayer(model, inputLength, hiddenLayer[0], batchSize);
	AddLinearLayer(model, hiddenLayer[0], outputLength, batchSize);

	{
		LoadInputOutput_Mnist(model, trainImages,trainLabels, inputIndex, trainImagesCount, &mnistTarget);
		Forward(model);
		//PrintLinearLayer(model->linearLayers[0]);
	}
	FreeModel(model);
	free(trainLabels);
	free(trainImages);
	return 0;
}

