#include "ff_asm_primes.h"
#include "../Datasets/mnist/mnistIO.h"
#include <gmp.h>
typedef struct model_struct *Model;
typedef struct layer_struct *Layer;
typedef struct blankinship_row_struct *BlankinshipRow;
typedef BlankinshipRow* BlankinshipMatrix;
typedef struct mpz_index_struct *MpzWithIndex;

struct layer_struct
{
	int inputFeatures;
	int outputFeatures;
	int bias;
	int *input; 
	mpz_t output;
	MpzWithIndex *weights;
	Layer next;
	int layerIndex;
};
struct model_struct
{
	int modStart;
	int layerCount;
	Layer layers;
};

struct mpz_index_struct
{	
	mpz_t integer;
	int index;
	mpz_t *generalSolution; 
};
struct blankinship_row_struct
{
	mpz_t leader;
	int numberOfColumns;
	mpz_t *columns;
};

Model CreateModel()
{
	Model model = malloc(sizeof(struct model_struct));
	model->layerCount = 0;
	model->modStart = 15;
	model->layers = NULL;
	return model;
}

Layer CreateLayer(int inputFeatures, int outputFeatures, int bias)
{
	Layer layer = malloc(sizeof(struct layer_struct));
	layer->inputFeatures = inputFeatures;
	layer->outputFeatures = outputFeatures;
	layer->bias = bias;
	mpz_init(layer->output);
	mpz_set_ui(layer->output, 0);
	layer->input = calloc(inputFeatures, sizeof(int));
	layer->weights = malloc(inputFeatures * sizeof(MpzWithIndex));
	layer->layerIndex = 0;
	gmp_randstate_t state;
	gmp_randinit_mt(state);
	for(int i = 0; i < layer->inputFeatures; i++)
	{
		layer->weights[i] = malloc(sizeof(struct mpz_index_struct));
		layer->weights[i]->index = i;
		mpz_init(layer->weights[i]->integer);
		layer->weights[i]->generalSolution = malloc(layer->inputFeatures * sizeof(mpz_t));
		layer->input[i] = rand() % 10;
		for(int j = 0; j < layer->inputFeatures; j++)
		{
			mpz_init(layer->weights[i]->generalSolution[j]);
		}
		mpz_urandomb(layer->weights[i]->integer, state, 10);
	}
	layer->next = NULL;
	gmp_randclear(state);
	return layer;
}

void AddLinearLayer(Model model, int inputFeatures, int outputFeatures, int bias)
{
	assert(model != NULL);
	Layer layer = CreateLayer(inputFeatures, outputFeatures, bias);
	//for(int i = 0; i < layer->inputFeatures; i++){printf("%d\n",layer->input[i] );}
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
		for(int i = 0; i < temp->inputFeatures; i++)
		{
			for(int j = 0; j < temp->inputFeatures; j++)
			{
				mpz_clear(temp->weights[i]->generalSolution[j]);
			}
			free(temp->weights[i]->generalSolution);
			mpz_clear(temp->weights[i]->integer);
			free(temp->weights[i]);
		}
		free(temp->weights);
		free(temp->input);
		mpz_clear(temp->output);
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

int BlankinshipCompareLeader(const void *a, const void *b)
{
	BlankinshipRow rowA = *(BlankinshipRow *)a;
	BlankinshipRow rowB = *(BlankinshipRow *)b;
	return mpz_cmp(rowB->leader, rowA->leader);
}

int MpzArrayCompare(const void *a, const void *b)
{
	mpz_t *ia = (mpz_t *)a;
	mpz_t *ib = (mpz_t *)b;
	return mpz_cmp(*ib, *ia);
}
double Find_Log_MPZ_Double(mpz_t x){signed long int ex;const double di = mpz_get_d_2exp(&ex, x);return ( (log(di) + log(2) * (double) ex) /log(2));}

void PrintMpzWithIndex(int length, MpzWithIndex *mpzArray)
{
	for(int i = 0; i < length; i++)
	{
		gmp_printf("%6d: %4Zd |",mpzArray[i]->index, mpzArray[i]->integer);
		for(int j = 0; j < length; j++)
		{
			gmp_printf("%4Zd ", mpzArray[i]->generalSolution[j]);
			//break;
		}
		printf("\n");
	}
	printf("\n");
}
void PrintMpzWithIndexLog(int length, MpzWithIndex *mpzArray){for(int i = 0; i < length; i++){printf("%6d: %.3f\n",mpzArray[i]->index, Find_Log_MPZ_Double(mpzArray[i]->integer));}printf("\n");}
int MpzWithIndexCompareMpz(const void *a, const void *b){const MpzWithIndex *mpz1 = (const MpzWithIndex *)a;const MpzWithIndex *mpz2 = (const MpzWithIndex *)b;return mpz_cmp((*mpz2)->integer, (*mpz1)->integer);}
int MpzWithIndexCompareIndex(const void *a, const void *b){const MpzWithIndex *mpz1 = (const MpzWithIndex *)a;const MpzWithIndex *mpz2 = (const MpzWithIndex *)b;return (*mpz1)->index - (*mpz2)->index;}


BlankinshipMatrix CreateBlankinshipMatrix(int variableCount, MpzWithIndex *variables)
{
	qsort(variables, variableCount, sizeof(MpzWithIndex), MpzWithIndexCompareMpz);
	int rows = variableCount;int columns = variableCount;
	BlankinshipMatrix matrix = malloc(rows * sizeof(BlankinshipRow));
	for(int i = 0; i < rows; i++)
	{
		matrix[i] = malloc(sizeof(struct blankinship_row_struct));
		mpz_init(matrix[i]->leader);
		mpz_set(matrix[i]->leader, variables[i]->integer);
		matrix[i]->numberOfColumns = columns;
		matrix[i]->columns = malloc(columns * sizeof(mpz_t));
		for(int j = 0; j < columns; j++)
		{
			mpz_init(matrix[i]->columns[j]);
			mpz_set_ui(matrix[i]->columns[j], 0);
		}
		mpz_set_ui(matrix[i]->columns[i], 1);
		
	}
	return matrix;
}


void SortBlankinshipMatrix(int variableCount, BlankinshipMatrix matrix)
{
	qsort(matrix, variableCount, sizeof(BlankinshipRow), BlankinshipCompareLeader);
}

void PrintMatrix(int variableCount, BlankinshipMatrix matrix)
{
	int rows = variableCount;int columns = variableCount;
	for(int i = 0; i < rows; i++)
	{
		gmp_printf("%Zd : ", matrix[i]->leader);
		for(int j = 0; j < columns; j++)
		{
			gmp_printf("%Zd ", matrix[i]->columns[j]);
		}
		printf("\n");
	}
}

void FreeBlankinshipMatrix(int variableCount, BlankinshipMatrix matrix)
{
	int rows = variableCount;int columns = variableCount;
	for(int i = 0; i < rows; i++)
	{
		for(int j = 0; j < columns; j++)
		{
			mpz_clear(matrix[i]->columns[j]);
		}
		free(matrix[i]->columns);
		mpz_clear(matrix[i]->leader);
		free(matrix[i]);
	}
	free(matrix);
}

int FindOperatorIndex(int variableCount, BlankinshipMatrix matrix)
{
	int operatorIndex = 0;
	for(int i = variableCount-1; i >= 0; i--)
	{
		if(mpz_cmp_ui(matrix[i]->leader, 0)> 0)
		{
			operatorIndex = i;
			break;
		}
	}
	return operatorIndex;
}

void ReduceRows(int operatorIndex, int variableCount, BlankinshipMatrix matrix)
{
	assert(operatorIndex < variableCount);
	mpz_t quotient; mpz_init(quotient);
	mpz_t multiple; mpz_init(multiple);
	for(int i = operatorIndex-1; i >= 0; i--)
	{
		mpz_tdiv_q(quotient, matrix[i]->leader, matrix[operatorIndex]->leader);
		mpz_mul(multiple, matrix[operatorIndex]->leader, quotient);
		mpz_sub(matrix[i]->leader, matrix[i]->leader, multiple);
		assert(mpz_cmp_ui(matrix[i]->leader, 0) >= 0);
		for(int j = 0; j < matrix[i]->numberOfColumns; j++)
		{
			mpz_mul(multiple, matrix[operatorIndex]->columns[j], quotient);
			mpz_sub(matrix[i]->columns[j], matrix[i]->columns[j], multiple);
		}
	}
	mpz_clear(quotient);mpz_clear(multiple);
}

void StoreSolutions(int variableCount, BlankinshipMatrix matrix, MpzWithIndex *variables)
{
	int rows = variableCount;int columns = variableCount;
	for(int i = 0; i < columns; i++)
	{
		for(int j = 0; j < columns; j++)
		{
			mpz_set(variables[i]->generalSolution[j], matrix[j]->columns[i]);
		}
	}
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

void Forward(Model model)
{
	assert(model != NULL);
	assert(model->layerCount >= 2);
	Layer layer = model->layers;
	mpz_t temporaryMultiply;mpz_init(temporaryMultiply);
	while(layer != NULL)
	{
		mpz_set_ui(layer->output, 0);
		for(int i = 0; i < layer->inputFeatures; i++)
		{
			//printf("|%d|\n", layer->input[i]);
			mpz_mul_si(temporaryMultiply, layer->weights[i]->integer, layer->input[i]);
			mpz_add(layer->output, layer->output, temporaryMultiply);
		}
		if(layer->next == NULL)
		{
			break;
		}
		else
		{
			layer = layer->next;
			for(int i = 0; i < layer->inputFeatures; i++)
			{
				layer->input[i] = mpz_fdiv_ui(layer->output, (unsigned long int) first5239[i]);
			}
		}
	}
	mpz_clear(temporaryMultiply);
}

void PlanetaryProp(Model model, int outputLength, int target)
{
	printf("%d\n", target);
	Layer layer = model->layers;
	int targetLayer = model->layerCount - 1;while(layer->next != NULL){layer = layer->next;}
	int *outputHolder = calloc(outputLength, sizeof(int));
	int *change = calloc(outputLength, sizeof(int));
	int *newOutput = calloc(outputLength, sizeof(int));
	int *finiteField = calloc(outputLength, sizeof(int));
	
	float *probability = calloc(outputLength, sizeof(float));
	mpz_t newOutputHolder;mpz_init(newOutputHolder);
	float learningRate = 0.10;
	int learningIterations = 5;
	int modStart = 15;
	for(int i = 0; i < outputLength; i++)
	{
		finiteField[i] = first5239[i + modStart];
		outputHolder[i] = mpz_fdiv_ui(layer->output, (unsigned long int) finiteField[i]);
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
			if(k == learningIterations-1)printf("%d %2d : %2d = %2d %.3f\n", i, finiteField[i], outputHolder[i],newOutput[i], probability[i]);
		}
	}

	ChineseRemainderTheorem(outputLength, finiteField, newOutput, newOutputHolder);
	
	BlankinshipMatrix matrix = CreateBlankinshipMatrix(layer->inputFeatures, layer->weights);
	for(int i = 0; i < 200; i++)
	{
		int operatorIndex = FindOperatorIndex(layer->inputFeatures, matrix);
		printf("|%3d %3d|\n", i, layer->inputFeatures);

		if(operatorIndex == 0)
		{
			//PrintMatrix(layer->inputFeatures, matrix);printf("\n");
			StoreSolutions(layer->inputFeatures, matrix, layer->weights);
			PrintMpzWithIndex(layer->inputFeatures, layer->weights);
			
			break;
		}
		ReduceRows(operatorIndex,layer->inputFeatures, matrix);
		SortBlankinshipMatrix(layer->inputFeatures, matrix);
	}
	
	FreeBlankinshipMatrix(layer->inputFeatures, matrix);
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
	PrintModel(model);

	{
		LoadInputOutput_Mnist(model, trainImages,trainLabels, inputIndex, trainImagesCount, &mnistTarget);
		//model->layers = model->layers->next;model->layers = model->layers->next;for(int i = 0; i < model->layers->inputFeatures; i++){printf("%3d : %d\n",i, model->layers->input[i] );}
		Forward(model);
		PlanetaryProp(model, outputLength, mnistTarget);
		inputIndex += outputLength;
	}
	
	mpz_clear(output);
	free(trainLabels);
	free(trainImages);
	FreeModel(model);
	return 0;
}

