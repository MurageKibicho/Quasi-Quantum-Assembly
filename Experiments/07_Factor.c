#include "ff_asm_primes.h"
#include <gmp.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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
	int discreteLog; 
	mpz_t *input; 
	mpz_t biasValue;
	mpz_t discreteLogValue;
	mpz_t discreteLogMod;
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

Layer CreateLayer(int inputFeatures, int outputFeatures, int bias, int discreteLog)
{	
	assert(inputFeatures > 0);
	gmp_randstate_t state;
	gmp_randinit_mt(state);
	Layer layer = malloc(sizeof(struct layer_struct));
	layer->inputFeatures = inputFeatures;
	layer->outputFeatures = outputFeatures;
	layer->bias = bias;
	layer->discreteLog = discreteLog;
	if(layer->bias > 0)
	{
		mpz_init(layer->biasValue);
		mpz_urandomb(layer->biasValue, state, 10);
	}
	if(layer->discreteLog > 0)
	{
		mpz_init(layer->discreteLogValue);
		mpz_init(layer->discreteLogMod);
		mpz_urandomb(layer->discreteLogValue, state, 10);
		mpz_urandomb(layer->discreteLogMod, state, 100);
	}

	mpz_init(layer->output);
	mpz_set_ui(layer->output, 0);
	layer->input = malloc(inputFeatures * sizeof(mpz_t));
	layer->weights = malloc(inputFeatures * sizeof(MpzWithIndex));
	layer->layerIndex = 0;
	
	for(int i = 0; i < layer->inputFeatures; i++)
	{
		layer->weights[i] = malloc(sizeof(struct mpz_index_struct));
		layer->weights[i]->index = i;
		mpz_init(layer->weights[i]->integer);
		mpz_init(layer->input[i]);
		mpz_set_ui(layer->input[i], 0);
		layer->weights[i]->generalSolution = malloc(layer->inputFeatures * sizeof(mpz_t));
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

void AddLinearLayer(Model model, int inputFeatures, int outputFeatures, int bias, int discreteLog)
{
	assert(model != NULL);
	Layer layer = CreateLayer(inputFeatures, outputFeatures, bias, discreteLog);
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
		printf("Layer %d: inputFeatures=%d, outputFeatures=%d, bias=%d, discreteLog=%d\n", i++, temp->inputFeatures, temp->outputFeatures, temp->bias, temp->discreteLog);
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
			mpz_clear(temp->input[i]);
			free(temp->weights[i]);
		}
		if(temp->bias > 0){mpz_clear(temp->biasValue);}
		if(temp->discreteLog > 0){mpz_clear(temp->discreteLogValue);mpz_clear(temp->discreteLogMod);}
		
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

void Forward(Model model)
{
	assert(model != NULL);
	assert(model->layerCount >= 2);
	Layer layer = model->layers;
	mpz_t temporaryMultiply;mpz_init(temporaryMultiply);
	while(layer != NULL)
	{	
		//printf("|%d %d|\n", layer->inputFeatures, layer->outputFeatures);
		mpz_set_ui(layer->output, 0);
		for(int i = 0; i < layer->inputFeatures; i++)
		{
			mpz_mul(temporaryMultiply, layer->weights[i]->integer, layer->input[i]);
			mpz_add(layer->output, layer->output, temporaryMultiply);
			//gmp_printf("%Zd %Zd %Zd\n", layer->input[i],layer->weights[i]->integer, layer->output);
		}
		if(layer->bias > 0)
		{
			mpz_add(layer->output, layer->output, layer->biasValue);
			//gmp_printf("\tBias :%Zd : %Zd \n", layer->biasValue, layer->output);
		}
		if(layer->discreteLog > 0)
		{
			mpz_powm(layer->output, layer->output, layer->discreteLogValue, layer->discreteLogMod);
			//gmp_printf("\tDiscreteLogValue :(%Zd %.3f) %Zd\n", layer->discreteLogValue, Find_Log_MPZ_Double(layer->discreteLogMod),layer->output);
		}
		if(layer->next != NULL)
		{
			assert(layer->outputFeatures == layer->next->inputFeatures);
			for(int i = 0; i < layer->next->inputFeatures; i++)
			{
				 mpz_mod_ui(layer->next->input[i], layer->output, (unsigned long int) first5239[i]);
			}
		}
		else
		{
			//gmp_printf("Output: %Zd\n", layer->output);
		}
		layer = layer->next;
	} 
	mpz_clear(temporaryMultiply);
}
int globalPass = 0;
void PlanetaryPropPrime(Model model, int outputLength, mpz_t *target)
{
	assert(model != NULL);
	assert(outputLength >= 0);
	Layer layer = model->layers;
	int targetLayer = model->layerCount - 1;while(layer->next != NULL){layer = layer->next;}
	int currentOutput = mpz_fdiv_ui(layer->output, (unsigned long int) first5239[0]);
	int isEqual = mpz_cmp_ui(target[0], (unsigned long int) currentOutput);
	if(isEqual == 0)
	{
		gmp_printf("Pass (%Zd %d): %d %d\n",target[0], currentOutput, isEqual, globalPass);
		globalPass += 1;
		//maybe add 2
		
	}
	else
	{
		gmp_printf("Fail (%Zd %d): %d %d\n",target[0], currentOutput, isEqual, globalPass);
		mpz_add_ui(layer->output, layer->output, 1);
		int index = -1;
		for(int i = 0; i < layer->inputFeatures; i++)
		{
			if(mpz_fdiv_ui(layer->input[i], (unsigned long int) first5239[0]) == 1)
			{
				index = i;
				break;
			}
		}
		if(index > -1 && index < layer->inputFeatures)
		{
			//mpz_add_ui(layer->weights[index]->integer,layer->weights[index]->integer, 1);
		}
	}

	
}


int IdentifyPrimes()
{
	Model model = CreateModel();
	int inputLength = 1;
	int outputLength= 1;
	int layer0 =295;
	mpz_t *targetOutput = malloc(outputLength * sizeof(mpz_t));for(int i = 0; i < outputLength; i++){mpz_init(targetOutput[i]);}
	AddLinearLayer(model, inputLength, layer0, 1,0);
	AddLinearLayer(model, layer0, outputLength, 0, 1);
	
	PrintModel(model);
	
	for(int i = 0; i < 3000; i++)
	{
		mpz_set_ui(model->layers->input[0], (unsigned long int) first5239[i]);
		mpz_set_ui(targetOutput[0], 1);
		Forward(model);
		PlanetaryPropPrime(model, outputLength, targetOutput); 
	}

	FreeModel(model);
	for(int i = 0; i < outputLength; i++){mpz_clear(targetOutput[i]);}free(targetOutput);
	return 0;
}

int main()
{	
	IdentifyPrimes();
	return 0;
}
