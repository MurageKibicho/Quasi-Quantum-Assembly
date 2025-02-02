#include "ff_asm_runtime.h"
typedef struct iris_dataset_struct
{
   double inputs[4];
   double output[3];
} iris_dataset_holder;

iris_dataset_holder iris_dataset_holder_array[ ] = 
{
//{ Sepal Length, Sepal Width, Petal Length, Petal Width} {Iris-setosa,Iris-versicolor, Iris-virginica}
{ { 5.1, 3.5, 1.4, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 4.9, 3.0, 1.4, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 4.7, 3.2, 1.3, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 4.6, 3.1, 1.5, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 5.0, 3.6, 1.4, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 5.4, 3.9, 1.7, 0.4 }, { 1.0, 0.0, 0.0 } },
{ { 4.6, 3.4, 1.4, 0.3 }, { 1.0, 0.0, 0.0 } },
{ { 5.0, 3.4, 1.5, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 4.4, 2.9, 1.4, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 4.9, 3.1, 1.5, 0.1 }, { 1.0, 0.0, 0.0 } },
{ { 5.4, 3.7, 1.5, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 4.8, 3.4, 1.6, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 4.8, 3.0, 1.4, 0.1 }, { 1.0, 0.0, 0.0 } },
{ { 4.3, 3.0, 1.1, 0.1 }, { 1.0, 0.0, 0.0 } },
{ { 5.8, 4.0, 1.2, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 5.7, 4.4, 1.5, 0.4 }, { 1.0, 0.0, 0.0 } },
{ { 5.4, 3.9, 1.3, 0.4 }, { 1.0, 0.0, 0.0 } },
{ { 5.1, 3.5, 1.4, 0.3 }, { 1.0, 0.0, 0.0 } },
{ { 5.7, 3.8, 1.7, 0.3 }, { 1.0, 0.0, 0.0 } },
{ { 5.1, 3.8, 1.5, 0.3 }, { 1.0, 0.0, 0.0 } },
{ { 5.4, 3.4, 1.7, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 5.1, 3.7, 1.5, 0.4 }, { 1.0, 0.0, 0.0 } },
{ { 4.6, 3.6, 1.0, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 5.1, 3.3, 1.7, 0.5 }, { 1.0, 0.0, 0.0 } },
{ { 4.8, 3.4, 1.9, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 5.0, 3.0, 1.6, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 5.0, 3.4, 1.6, 0.4 }, { 1.0, 0.0, 0.0 } },
{ { 5.2, 3.5, 1.5, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 5.2, 3.4, 1.4, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 4.7, 3.2, 1.6, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 4.8, 3.1, 1.6, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 5.4, 3.4, 1.5, 0.4 }, { 1.0, 0.0, 0.0 } },
{ { 5.2, 4.1, 1.5, 0.1 }, { 1.0, 0.0, 0.0 } },
{ { 5.5, 4.2, 1.4, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 4.9, 3.1, 1.5, 0.1 }, { 1.0, 0.0, 0.0 } },
{ { 5.0, 3.2, 1.2, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 5.5, 3.5, 1.3, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 4.9, 3.1, 1.5, 0.1 }, { 1.0, 0.0, 0.0 } },
{ { 4.4, 3.0, 1.3, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 5.1, 3.4, 1.5, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 5.0, 3.5, 1.3, 0.3 }, { 1.0, 0.0, 0.0 } },
{ { 4.5, 2.3, 1.3, 0.3 }, { 1.0, 0.0, 0.0 } },
{ { 4.4, 3.2, 1.3, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 5.0, 3.5, 1.6, 0.6 }, { 1.0, 0.0, 0.0 } },
{ { 5.1, 3.8, 1.9, 0.4 }, { 1.0, 0.0, 0.0 } },
{ { 4.8, 3.0, 1.4, 0.3 }, { 1.0, 0.0, 0.0 } },
{ { 5.1, 3.8, 1.6, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 4.6, 3.2, 1.4, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 5.3, 3.7, 1.5, 0.2 }, { 1.0, 0.0, 0.0 } },
{ { 5.0, 3.3, 1.4, 0.2 }, { 1.0, 0.0, 0.0 } },
                          // Iris-versicolor
{ { 7.0, 3.2, 4.7, 1.4 }, { 0.0, 1.0, 0.0 } },
{ { 6.4, 3.2, 4.5, 1.5 }, { 0.0, 1.0, 0.0 } },
{ { 6.9, 3.1, 4.9, 1.5 }, { 0.0, 1.0, 0.0 } },
{ { 5.5, 2.3, 4.0, 1.3 }, { 0.0, 1.0, 0.0 } },
{ { 6.5, 2.8, 4.6, 1.5 }, { 0.0, 1.0, 0.0 } },
{ { 5.7, 2.8, 4.5, 1.3 }, { 0.0, 1.0, 0.0 } },
{ { 6.3, 3.3, 4.7, 1.6 }, { 0.0, 1.0, 0.0 } },
{ { 4.9, 2.4, 3.3, 1.0 }, { 0.0, 1.0, 0.0 } },
{ { 6.6, 2.9, 4.6, 1.3 }, { 0.0, 1.0, 0.0 } },
{ { 5.2, 2.7, 3.9, 1.4 }, { 0.0, 1.0, 0.0 } },
{ { 5.0, 2.0, 3.5, 1.0 }, { 0.0, 1.0, 0.0 } },
{ { 5.9, 3.0, 4.2, 1.5 }, { 0.0, 1.0, 0.0 } },
{ { 6.0, 2.2, 4.0, 1.0 }, { 0.0, 1.0, 0.0 } },
{ { 6.1, 2.9, 4.7, 1.4 }, { 0.0, 1.0, 0.0 } },
{ { 5.6, 2.9, 3.6, 1.3 }, { 0.0, 1.0, 0.0 } },
{ { 6.7, 3.1, 4.4, 1.4 }, { 0.0, 1.0, 0.0 } },
{ { 5.6, 3.0, 4.5, 1.5 }, { 0.0, 1.0, 0.0 } },
{ { 5.8, 2.7, 4.1, 1.0 }, { 0.0, 1.0, 0.0 } },
{ { 6.2, 2.2, 4.5, 1.5 }, { 0.0, 1.0, 0.0 } },
{ { 5.6, 2.5, 3.9, 1.1 }, { 0.0, 1.0, 0.0 } },
{ { 5.9, 3.2, 4.8, 1.8 }, { 0.0, 1.0, 0.0 } },
{ { 6.1, 2.8, 4.0, 1.3 }, { 0.0, 1.0, 0.0 } },
{ { 6.3, 2.5, 4.9, 1.5 }, { 0.0, 1.0, 0.0 } },
{ { 6.1, 2.8, 4.7, 1.2 }, { 0.0, 1.0, 0.0 } },
{ { 6.4, 2.9, 4.3, 1.3 }, { 0.0, 1.0, 0.0 } },
{ { 6.6, 3.0, 4.4, 1.4 }, { 0.0, 1.0, 0.0 } },
{ { 6.8, 2.8, 4.8, 1.4 }, { 0.0, 1.0, 0.0 } },
{ { 6.7, 3.0, 5.0, 1.7 }, { 0.0, 1.0, 0.0 } },
{ { 6.0, 2.9, 4.5, 1.5 }, { 0.0, 1.0, 0.0 } },
{ { 5.7, 2.6, 3.5, 1.0 }, { 0.0, 1.0, 0.0 } },
{ { 5.5, 2.4, 3.8, 1.1 }, { 0.0, 1.0, 0.0 } },
{ { 5.5, 2.4, 3.7, 1.0 }, { 0.0, 1.0, 0.0 } },
{ { 5.8, 2.7, 3.9, 1.2 }, { 0.0, 1.0, 0.0 } },
{ { 6.0, 2.7, 5.1, 1.6 }, { 0.0, 1.0, 0.0 } },
{ { 5.4, 3.0, 4.5, 1.5 }, { 0.0, 1.0, 0.0 } },
{ { 6.0, 3.4, 4.5, 1.6 }, { 0.0, 1.0, 0.0 } },
{ { 6.7, 3.1, 4.7, 1.5 }, { 0.0, 1.0, 0.0 } },
{ { 6.3, 2.3, 4.4, 1.3 }, { 0.0, 1.0, 0.0 } },
{ { 5.6, 3.0, 4.1, 1.3 }, { 0.0, 1.0, 0.0 } },
{ { 5.5, 2.5, 4.0, 1.3 }, { 0.0, 1.0, 0.0 } },
{ { 5.5, 2.6, 4.4, 1.2 }, { 0.0, 1.0, 0.0 } },
{ { 6.1, 3.0, 4.6, 1.4 }, { 0.0, 1.0, 0.0 } },
{ { 5.8, 2.6, 4.0, 1.2 }, { 0.0, 1.0, 0.0 } },
{ { 5.0, 2.3, 3.3, 1.0 }, { 0.0, 1.0, 0.0 } },
{ { 5.6, 2.7, 4.2, 1.3 }, { 0.0, 1.0, 0.0 } },
{ { 5.7, 3.0, 4.2, 1.2 }, { 0.0, 1.0, 0.0 } },
{ { 5.7, 2.9, 4.2, 1.3 }, { 0.0, 1.0, 0.0 } },
{ { 6.2, 2.9, 4.3, 1.3 }, { 0.0, 1.0, 0.0 } },
{ { 5.1, 2.5, 3.0, 1.1 }, { 0.0, 1.0, 0.0 } },
{ { 5.7, 2.8, 4.1, 1.3 }, { 0.0, 1.0, 0.0 } },
                          // Iris-virginica
{ { 6.3, 3.3, 6.0, 2.5 }, { 0.0, 0.0, 1.0 } },
{ { 5.8, 2.7, 5.1, 1.9 }, { 0.0, 0.0, 1.0 } },
{ { 7.1, 3.0, 5.9, 2.1 }, { 0.0, 0.0, 1.0 } },
{ { 6.3, 2.9, 5.6, 1.8 }, { 0.0, 0.0, 1.0 } },
{ { 6.5, 3.0, 5.8, 2.2 }, { 0.0, 0.0, 1.0 } },
{ { 7.6, 3.0, 6.6, 2.1 }, { 0.0, 0.0, 1.0 } },
{ { 4.9, 2.5, 4.5, 1.7 }, { 0.0, 0.0, 1.0 } },
{ { 7.3, 2.9, 6.3, 1.8 }, { 0.0, 0.0, 1.0 } },
{ { 6.7, 2.5, 5.8, 1.8 }, { 0.0, 0.0, 1.0 } },
{ { 7.2, 3.6, 6.1, 2.5 }, { 0.0, 0.0, 1.0 } },
{ { 6.5, 3.2, 5.1, 2.0 }, { 0.0, 0.0, 1.0 } },
{ { 6.4, 2.7, 5.3, 1.9 }, { 0.0, 0.0, 1.0 } },
{ { 6.8, 3.0, 5.5, 2.1 }, { 0.0, 0.0, 1.0 } },
{ { 5.7, 2.5, 5.0, 2.0 }, { 0.0, 0.0, 1.0 } },
{ { 5.8, 2.8, 5.1, 2.4 }, { 0.0, 0.0, 1.0 } },
{ { 6.4, 3.2, 5.3, 2.3 }, { 0.0, 0.0, 1.0 } },
{ { 6.5, 3.0, 5.5, 1.8 }, { 0.0, 0.0, 1.0 } },
{ { 7.7, 3.8, 6.7, 2.2 }, { 0.0, 0.0, 1.0 } },
{ { 7.7, 2.6, 6.9, 2.3 }, { 0.0, 0.0, 1.0 } },
{ { 6.0, 2.2, 5.0, 1.5 }, { 0.0, 0.0, 1.0 } },
{ { 6.9, 3.2, 5.7, 2.3 }, { 0.0, 0.0, 1.0 } },
{ { 5.6, 2.8, 4.9, 2.0 }, { 0.0, 0.0, 1.0 } },
{ { 7.7, 2.8, 6.7, 2.0 }, { 0.0, 0.0, 1.0 } },
{ { 6.3, 2.7, 4.9, 1.8 }, { 0.0, 0.0, 1.0 } },
{ { 6.7, 3.3, 5.7, 2.1 }, { 0.0, 0.0, 1.0 } },
{ { 7.2, 3.2, 6.0, 1.8 }, { 0.0, 0.0, 1.0 } },
{ { 6.2, 2.8, 4.8, 1.8 }, { 0.0, 0.0, 1.0 } },
{ { 6.1, 3.0, 4.9, 1.8 }, { 0.0, 0.0, 1.0 } },
{ { 6.4, 2.8, 5.6, 2.1 }, { 0.0, 0.0, 1.0 } },
{ { 7.2, 3.0, 5.8, 1.6 }, { 0.0, 0.0, 1.0 } },
{ { 7.4, 2.8, 6.1, 1.9 }, { 0.0, 0.0, 1.0 } },
{ { 7.9, 3.8, 6.4, 2.0 }, { 0.0, 0.0, 1.0 } },
{ { 6.4, 2.8, 5.6, 2.2 }, { 0.0, 0.0, 1.0 } },
{ { 6.3, 2.8, 5.1, 1.5 }, { 0.0, 0.0, 1.0 } },
{ { 6.1, 2.6, 5.6, 1.4 }, { 0.0, 0.0, 1.0 } },
{ { 7.7, 3.0, 6.1, 2.3 }, { 0.0, 0.0, 1.0 } },
{ { 6.3, 3.4, 5.6, 2.4 }, { 0.0, 0.0, 1.0 } },
{ { 6.4, 3.1, 5.5, 1.8 }, { 0.0, 0.0, 1.0 } },
{ { 6.0, 3.0, 4.8, 1.8 }, { 0.0, 0.0, 1.0 } },
{ { 6.9, 3.1, 5.4, 2.1 }, { 0.0, 0.0, 1.0 } },
{ { 6.7, 3.1, 5.6, 2.4 }, { 0.0, 0.0, 1.0 } },
{ { 6.9, 3.1, 5.1, 2.3 }, { 0.0, 0.0, 1.0 } },
{ { 5.8, 2.7, 5.1, 1.9 }, { 0.0, 0.0, 1.0 } },
{ { 6.8, 3.2, 5.9, 2.3 }, { 0.0, 0.0, 1.0 } },
{ { 6.7, 3.3, 5.7, 2.5 }, { 0.0, 0.0, 1.0 } },
{ { 6.7, 3.0, 5.2, 2.3 }, { 0.0, 0.0, 1.0 } },
{ { 6.3, 2.5, 5.0, 1.9 }, { 0.0, 0.0, 1.0 } },
{ { 6.5, 3.0, 5.2, 2.0 }, { 0.0, 0.0, 1.0 } },
{ { 6.2, 3.4, 5.4, 2.3 }, { 0.0, 0.0, 1.0 } },
{ { 5.9, 3.0, 5.1, 1.8 }, { 0.0, 0.0, 1.0 } }
};

uint64_t modular_exponentiation_iris(uint64_t base, uint64_t exp, uint64_t mod)
{
	uint64_t result = 1; 
	base = base % mod;

	while(exp > 0)
	{
		if (exp % 2 == 1)
		{
			result = (result * base) % mod;
		}
		exp = exp >> 1;
		base = (base * base) % mod;
	}

	return result;
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

void LoadIrisInput(int batchSize, int layerIntegerCount, int startIndex, int finiteField[4], mpz_t *layerIntegers)
{
	int mods[4] = {0};
	int integerIndex = 0;
	for(int i = startIndex; i < startIndex + batchSize && i < 150; i++)
	{
		for(int j = 0; j < 4; j++)
		{
			mods[j] = (int)(iris_dataset_holder_array[i].inputs[j] * 10);
		}
		ChineseRemainderTheorem(4, finiteField, mods, layerIntegers[integerIndex]);
		integerIndex += 1;
	}
}

//Run : clear && gcc 04_irisClassification.c -lgmp -lm -o m.o && ./m.o
int main()
{
	ShuffleIrisDataset(iris_dataset_holder_array, 150);
	srand(3456);
	int batchSize = 4;//Left is 1 * batchSize
	int hiddenLayerNeurons[] = {4, 3,5, 4};
	int hiddenLayerCount = sizeof(hiddenLayerNeurons) / sizeof(int);
	int weightArrayLength = 0;
	int leftColumnCount = batchSize;
	int rightRowCount[hiddenLayerCount];
	int rightColumnCount[hiddenLayerCount];
	int layerIntegerCount = batchSize;
	mpz_t temporary; mpz_init(temporary);
	mpz_t fieldOrder; mpz_init(fieldOrder);mpz_set_ui(fieldOrder, 1);
	for(int i = 0; i < hiddenLayerCount; i++)
	{
		rightRowCount[i] = leftColumnCount;
		rightColumnCount[i] = hiddenLayerNeurons[i];
		weightArrayLength += rightRowCount[i] * rightColumnCount[i]; 
		layerIntegerCount += hiddenLayerNeurons[i];
		leftColumnCount = hiddenLayerNeurons[i];
	}
	
	int *weights = calloc(weightArrayLength, sizeof(int));for(int i = 0; i < weightArrayLength; i++){weights[i] = rand() % 10;}
	mpz_t *layerIntegers = malloc(layerIntegerCount * sizeof(mpz_t));for(int i = 0; i < layerIntegerCount; i++){mpz_init(layerIntegers[i]);}
	mpz_t *activations = malloc(hiddenLayerCount * sizeof(mpz_t));for(int i = 0; i < hiddenLayerCount; i++){mpz_init(activations[i]);}
	int finiteField[4] = {89, 83, 79, 73};for(int i = 0; i < 4; i++){mpz_mul_ui(fieldOrder, fieldOrder, finiteField[i]);}
	int loadStartIndex = 0;
	LoadIrisInput(batchSize,layerIntegerCount,loadStartIndex,finiteField, layerIntegers);

	int integerStoreIndex = 0;
	int integerCurrentIndex = 0;
	int weightCurrentIndex = 0;
	for(int k = 0; k < hiddenLayerCount; k++)
	{
		integerStoreIndex += rightRowCount[k];
		printf("%d %d %d\n", rightRowCount[k], rightColumnCount[k],integerStoreIndex);
		for(int i = 0; i < rightColumnCount[k]; i++)
		{
			printf(" %2d :  ", integerStoreIndex+i);
			mpz_set_ui(layerIntegers[integerStoreIndex+i], 0);
			for(int j = 0; j < rightRowCount[k]; j++)
			{
				printf(" (%2d, %2d) ", weightCurrentIndex, integerCurrentIndex+j);
				mpz_mul_ui(temporary, layerIntegers[integerCurrentIndex+j], weights[weightCurrentIndex]);
				mpz_add(layerIntegers[integerStoreIndex+i], layerIntegers[integerStoreIndex+i], temporary);
				assert(weightCurrentIndex < weightArrayLength);
				assert(integerCurrentIndex+j < layerIntegerCount);
				weightCurrentIndex += 1;
			}
			mpz_powm_ui(layerIntegers[integerStoreIndex+i], layerIntegers[integerStoreIndex+i], 5,fieldOrder);
			printf("\n");
			//integerStoreIndex += 1;
		}
		integerCurrentIndex += rightRowCount[k];
	}
	gmp_printf("%Zd\n%Zd\n%Zd\n%Zd\n",layerIntegers[16],layerIntegers[17],layerIntegers[18],layerIntegers[19]);
	
	//Backwards
	
	
	
	for(int i = 0; i < layerIntegerCount; i++){mpz_clear(layerIntegers[i]);}
	for(int i = 0; i < hiddenLayerCount; i++){mpz_clear(activations[i]);}
	free(layerIntegers);free(activations);
	free(weights);mpz_clear(temporary);mpz_clear(fieldOrder);
	return 0;
}


