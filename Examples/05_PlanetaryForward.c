#include "ff_asm_primes.h"
#include "../Datasets/mnist/mnistIO.h"
#include <gmp.h>
typedef struct model_struct *Model;
typedef struct layer_struct *Layer;
struct layer_struct
{
	int inputFeatures;
	int outputFeatures;
	int bias;
	int *input; 
	mpz_t *weights;
	Layer next;
	int layerIndex;
};
struct model_struct
{
	int layerCount;
	Layer layers;
};

Model CreateModel()
{
	Model model = malloc(sizeof(struct model_struct));
	model->layerCount = 0;
	model->layers = NULL;
	return model;
}

Layer CreateLayer(int inputFeatures, int outputFeatures, int bias)
{
	Layer layer = malloc(sizeof(struct layer_struct));
	layer->inputFeatures = inputFeatures;
	layer->outputFeatures = outputFeatures;
	layer->bias = bias;
	layer->input = calloc(inputFeatures, sizeof(int));
	layer->weights = malloc(inputFeatures * sizeof(mpz_t));
	layer->layerIndex = 0;
	gmp_randstate_t state;
	gmp_randinit_mt(state);
	//gmp_randseed_ui(state, seed);
	for(int i = 0; i < layer->inputFeatures; i++)
	{
		mpz_init(layer->weights[i]);
		mpz_urandomb(layer->weights[i], state, inputFeatures * 8);
	}
	layer->next = NULL;
	gmp_randclear(state);
	return layer;
}

void AddLinearLayer(Model model, int inputFeatures, int outputFeatures, int bias)
{
	assert(model != NULL);
	Layer layer = CreateLayer(inputFeatures, outputFeatures, bias);
	layer->layerIndex = model->layerCount;
	if(model->layers == NULL)
	{
		model->layers = layer;
	}
	else
	{
		Layer temp = model->layers;
		while(temp->next != NULL)
		{
			temp = temp->next;
		}
		assert(temp->outputFeatures == inputFeatures);
		temp->next = layer;
	}
	model->layerCount += 1;
}

void PrintModel(Model model)
{
	printf("\nModel Summary: %d layers\n", model->layerCount);
	Layer temp = model->layers;
	int i = 0;
	while(temp != NULL)
	{
		printf("Layer %d: inputFeatures=%d, outputFeatures=%d, bias=%d\n", i++, temp->inputFeatures, temp->outputFeatures, temp->bias);
		temp = temp->next;
	}
}

void FreeModel(Model model)
{
	Layer temp = model->layers;
	while(temp != NULL)
	{
		Layer next_layer = temp->next;
		for(int i = 0; i < temp->inputFeatures; i++){mpz_clear(temp->weights[i]);}
		free(temp->weights);
		free(temp->input);
		free(temp);
		temp = next_layer;
	}
	free(model);
}

void ChineseRemainderTheorem(int fieldOrder, int *finiteField, int *mods, mpz_t result)
{
	/*Chinese Remainder Theorem*/
	mpz_set_ui(result,0);
	mpz_t productHolder; mpz_init(productHolder);mpz_set_ui(productHolder, 1);
	mpz_t modularInverseHolder; mpz_init(modularInverseHolder);
	mpz_t temporary0; mpz_init(temporary0);
	mpz_t temporary1; mpz_init(temporary1);
	
	for(int i = 0; i < fieldOrder; i++)
	{
		assert(finiteField[i] > 0);
		mpz_mul_ui(productHolder, productHolder, finiteField[i]);
	}
	
	for(int i = 0; i < fieldOrder; i++)
	{
		mpz_set_ui(temporary0, finiteField[i]);
		mpz_divexact(temporary1, productHolder, temporary0);
		mpz_invert(modularInverseHolder, temporary1, temporary0);
		mpz_mul(temporary1, temporary1,modularInverseHolder);
		mpz_mul_ui(temporary0, temporary1, mods[i]);
		mpz_add(result,result, temporary0);
	}
	mpz_mod(result, result,productHolder);
	mpz_clear(temporary0);
	mpz_clear(temporary1);
	mpz_clear(productHolder);
	mpz_clear(modularInverseHolder);
}


void LoadInputOutput_Mnist(Model model, unsigned char *trainImages, unsigned char *trainLabels, int inputIndex, int trainImagesCount, int *target)
{
	assert(inputIndex >= 0);
	assert(inputIndex < trainImagesCount);
	for(int i = 0; i < 784; i++)
	{
		model->layers->input[i] = (int) trainImages[(inputIndex * 784) + i];
	}
	*target = trainLabels[inputIndex];
}

void Forward(Model model, mpz_t output)
{
	assert(model != NULL);
	assert(model->layerCount >= 2);
	Layer layer = model->layers;
	mpz_t temporaryMultiply;mpz_init(temporaryMultiply);
	mpz_t temporaryAdd;mpz_init(temporaryAdd);
	while(layer != NULL)
	{
		mpz_set_ui(temporaryMultiply, 0);
		mpz_set_ui(temporaryAdd, 0);
		for(int i = 0; i < layer->inputFeatures; i++)
		{
			mpz_mul_ui(temporaryMultiply, layer->weights[i], layer->input[i]);
			mpz_add(temporaryAdd, temporaryAdd, temporaryMultiply);
		}
		layer = layer->next;
		if(layer != NULL)
		{	
			for(int i = 0; i < layer->inputFeatures; i++)
			{
				layer->input[i] = mpz_fdiv_ui(temporaryAdd, (unsigned long int) first5239[i]);
			}
		}
	}
	mpz_set(output, temporaryAdd);
	mpz_clear(temporaryMultiply);
	mpz_clear(temporaryAdd);
}

void PlanetaryProp(Model model, mpz_t output, int outputLength, int target)
{
	printf("%d\n", target);
	int *outputHolder = calloc(outputLength, sizeof(int));
	int *change = calloc(outputLength, sizeof(int));
	int *newOutput = calloc(outputLength, sizeof(int));
	int *finiteField = calloc(outputLength, sizeof(int));
	
	float *probability = calloc(outputLength, sizeof(float));
	mpz_t newOutputHolder;mpz_init(newOutputHolder);
	float learningRate = 0.10;
	int learningIterations = 3;
	int modStart = 15;
	for(int i = 0; i < outputLength; i++)
	{
		finiteField[i] = first5239[i + modStart];
		outputHolder[i] = mpz_fdiv_ui(output, (unsigned long int) finiteField[i]);
		probability[i] = (float) outputHolder[i] / (float) finiteField[i]; 
		change[i] = (int) ((float)finiteField[i] * learningRate);
		if(change[i] == 0){change[i] = 1;}
		newOutput[i] = outputHolder[i];
	}
	for(int k = 0; k < learningIterations; k++)
	{
		for(int i = 0; i < outputLength; i++)
		{
			if(newOutput[i] - change[i] > 0 && i != target){newOutput[i] = newOutput[i] - change[i];}
			else if(newOutput[i] + change[i] < finiteField[i] && i == target){newOutput[i] = newOutput[i] + change[i];}
			else{newOutput[i] = newOutput[i];}
			probability[i] = (float) newOutput[i] / (float) finiteField[i]; 
			if(k == learningIterations-1)printf("%d : %2d %2d = %2d %2d %.3f\n", i, outputHolder[i], finiteField[i], change[i],newOutput[i], probability[i]);
		}
	}
	Layer layer = model->layers;
	int targetLayer = model->layerCount - 1;
	while(layer != NULL && layer->layerIndex != targetLayer){layer = layer->next;}
	
	ChineseRemainderTheorem(outputLength, finiteField, newOutput, newOutputHolder);
	
	
	gmp_printf("%d %d  %Zd\n", targetLayer, layer->layerIndex, newOutputHolder);
	free(newOutput);
	free(outputHolder);
	free(probability);
	free(change);
	free(finiteField);
	mpz_clear(newOutputHolder);
}

int main()
{
	srand(65784359);
	char *trainImagesPath = "../Datasets/mnist/train-images.idx3-ubyte";
	char *trainLabelsPath = "../Datasets/mnist/train-labels.idx1-ubyte";
	int trainImagesCount = 0;
	int trainLabelsCount = 0;
	unsigned char *trainImages = ReadMnistImages(trainImagesPath, &trainImagesCount);
	unsigned char *trainLabels = ReadMnistLabels(trainLabelsPath, &trainLabelsCount);
	
	assert(trainLabels != NULL);
	assert(trainImagesCount == trainLabelsCount);
	ShuffleData(trainImages, trainLabels, trainImagesCount);
	
	int inputIndex = 0;
	int inputLength = 784;
	int outputLength = 10;
	int mnistTarget = 0;
	mpz_t output;mpz_init(output);
	Model model = CreateModel();
	AddLinearLayer(model, inputLength, 128, 0);
	AddLinearLayer(model, 128, 64, 0);
	AddLinearLayer(model, 64, outputLength, 0);
	
	{
		LoadInputOutput_Mnist(model, trainImages,trainLabels, inputIndex, trainImagesCount, &mnistTarget);
		Forward(model, output);
		PlanetaryProp(model, output, outputLength, mnistTarget);
		inputIndex += outputLength;
	}
	
	mpz_clear(output);
	free(trainLabels);
	free(trainImages);
	FreeModel(model);
	return 0;
}
/*
           | 1            
             
1   0   0  | 1
0   1   0  |-1
0   0   1  | 0
437 391 323|68 
*/
