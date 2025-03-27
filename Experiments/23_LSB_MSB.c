#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <gmp.h>
#include <sys/mman.h>
#include "ff_asm_primes.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define TRUE 1
#define FALSE 0
#define INDEX(x, y, cols) ((x) * (cols) + (y))
typedef struct field_struct *Field;
typedef struct mod_struct *Mod;
struct mod_struct
{
	int *probability;
};

struct field_struct 
{
	int primeNumber;
	int *modToPower;
	int *powerToMod;
	Mod *mod;
};
int ModularExponentiation(int base, int exp, int mod){int result = 1;base = base % mod;while(exp > 0){if(exp % 2 == 1){result = (result * base) % mod;}exp = exp >> 1;base = (base * base) % mod;}return result;}
int FastCheck_2IsAGenerator(int primeNumber){int result = FALSE;if(primeNumber == 2){result = TRUE;}if(primeNumber % 8 == 3 || primeNumber % 8 == 5){int primeNumberMinusOne = primeNumber - 1;int n = primeNumberMinusOne;int q = 0;if(n % 2 == 0){q = 2;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}while(n % 2 == 0){n /= 2;}}for(int i = 3; i * i <= primeNumberMinusOne; i += 2){if(n % i == 0){q = i;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}while(n % i == 0){n /= i;}}}if(n > 1){q = n;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}}result = TRUE;}return result;}
size_t GetFileSize(char *fileName)
{
	FILE *fp = fopen(fileName, "rb");assert(fp != NULL);
	fseek(fp, 0L, SEEK_END);
	size_t currentFileSize = ftell(fp);rewind(fp);
	fclose(fp);
	return currentFileSize;
}
Field CreateField(int primeNumber)
{
	assert(primeNumber > 1);
	Field field = malloc(sizeof(struct field_struct));
	field->primeNumber = primeNumber;
	field->mod = malloc(primeNumber * sizeof(Mod));
	field->modToPower = calloc(primeNumber, sizeof(int));
	field->powerToMod = calloc(primeNumber, sizeof(int));
	field->powerToMod[0] = -1;
	for(int i = 0; i < field->primeNumber; i++)
	{
		field->powerToMod[i] = ModularExponentiation(2, i, primeNumber);
		field->modToPower[field->powerToMod[i]] = i;
		field->mod[i] = malloc(sizeof(struct mod_struct));
		field->mod[i]->probability = calloc(primeNumber * primeNumber , sizeof(int));
		for(int j = 0; j < field->primeNumber; j++)
		{
			field->mod[i]->probability[j] = 1;
		}
	}	
	return field;
}

void PrintField(Field field)
{
	printf("Prime: %d\n", field->primeNumber);
	for(int i = 0; i < field->primeNumber; i++)
	{
		printf("%d : ", i);
		for(int j = 0; j < field->primeNumber; j++)
		{
			printf("(%d %d),", j, field->mod[i]->probability[j]);
		}
		printf("\n");
	}
	printf("\n");
}
void DestroyField(Field field)
{
	if(field)
	{
		for(int i = 0; i < field->primeNumber; i++)
		{
			free(field->mod[i]->probability);
			free(field->mod[i]);
		}
		free(field->modToPower);
		free(field->powerToMod);
		free(field->mod);
		free(field);
	}
}

void UpdateFieldProbabilities(unsigned char input, mpz_t integerLeft, mpz_t integerRight, mpz_t temporary0, Field *fieldHolder)
{
	assert(input > -1);
	int currentModLeft  = 0;
	int currentModRight  = 0;
	for(size_t i = 0; i < arrlen(fieldHolder); i++)
	{
		//gmp_printf("%Zd %Zd\n", integerLeft, integerRight);
		currentModLeft  = mpz_mod_ui(temporary0, integerLeft, fieldHolder[i]->primeNumber);
		currentModRight = mpz_mod_ui(temporary0, integerRight, fieldHolder[i]->primeNumber);
		fieldHolder[i]->mod[currentModLeft]->probability[currentModRight] += 1;
	}
}


void TrainNetwork(size_t inputLength, unsigned char *input, Field *fieldHolder)
{
	mpz_t integerLeft, integerRight, temporary0;
	mpz_inits(integerLeft, integerRight, temporary0, NULL);
	int inputLog = 0;
	size_t maxCount = 0;
	for(size_t i = inputLength; i-- > maxCount; )
	{
		inputLog = input[i] == 0 ? 1 : (int) ceil(log2(input[i])) + 1;
		mpz_mul_2exp(integerRight,integerRight, inputLog);
		mpz_add_ui(integerRight, integerRight, input[i]);
		//gmp_printf("%d %d, %Zd\n", input[i], inputLog, integerRight);
	}
	//printf("\n");
	for(size_t i = maxCount; i < inputLength; i++)
	{
		assert(input[i] > -1);
		inputLog = input[i] == 0 ? 1 : (int) ceil(log2(input[i])) + 1;
		mpz_mul_2exp(integerLeft,integerLeft, inputLog);
		mpz_add_ui(integerLeft, integerLeft, input[i]);
		UpdateFieldProbabilities(input[i], integerLeft, integerRight, temporary0, fieldHolder);
		//gmp_printf("%d %d, %Zd %Zd\n", input[i], inputLog, integerLeft,integerRight);	
		mpz_fdiv_q_2exp(integerRight, integerRight, inputLog);
	}
	for(size_t i = 0; i < arrlen(fieldHolder); i++)
	{
		PrintField(fieldHolder[i]);
	}
	mpz_clears(integerLeft, integerRight, temporary0, NULL);
}
void TrainNetworkBits(size_t inputLength, unsigned char *input, Field *fieldHolder)
{
	mpz_t integerLeft, integerRight, temporary0;
	mpz_inits(integerLeft, integerRight, temporary0, NULL);
	int inputLog = 0;
	size_t maxCount = 0;
	for(size_t i = inputLength; i-- > maxCount; )
	{
		inputLog = input[i] == 0 ? 1 : (int) ceil(log2(input[i])) + 1;
		mpz_mul_2exp(integerRight,integerRight, inputLog);
		mpz_add_ui(integerRight, integerRight, input[i]);
		//gmp_printf("%d %d, %Zd\n", input[i], inputLog, integerRight);
	}
	//printf("\n");
	for(size_t i = maxCount; i < inputLength; i++)
	{
		assert(input[i] > -1);
		inputLog = input[i] == 0 ? 1 : (int) ceil(log2(input[i])) + 1;
		mpz_mul_2exp(integerLeft,integerLeft, inputLog);
		mpz_add_ui(integerLeft, integerLeft, input[i]);

		UpdateFieldProbabilities(input[i], integerLeft, integerRight, temporary0, fieldHolder);
		//gmp_printf("%d %d, %Zd %Zd\n", input[i], inputLog, integerLeft,integerRight);	
		mpz_fdiv_q_2exp(integerRight, integerRight, inputLog);
	}
	for(size_t i = 0; i < arrlen(fieldHolder); i++)
	{
		PrintField(fieldHolder[i]);
	}
	mpz_clears(integerLeft, integerRight, temporary0, NULL);
}

void TestField()
{
	unsigned char  input[] = {'b','a','n','a','n','a',1};
	double lookupBits = 10;
	int bSmooth = 2;
	size_t inputLength = sizeof(input) / sizeof(unsigned char );
	int currentPrimeIndex = 0;
	double logValue = 0.0f;
	Field *fieldHolder = NULL;
	while(currentPrimeIndex < 5239)
	{
		if(first5239[currentPrimeIndex] >= bSmooth && FastCheck_2IsAGenerator(first5239[currentPrimeIndex]) == TRUE)
		{
			Field field = CreateField(first5239[currentPrimeIndex]);
			arrput(fieldHolder, field);
			logValue += log2(first5239[currentPrimeIndex]);
			if(logValue > lookupBits){break;}
		}
		currentPrimeIndex += 1;
	}
	TrainNetwork(inputLength, input, fieldHolder);
	for(size_t i = 0; i < arrlen(fieldHolder); i++){DestroyField(fieldHolder[i]);}
	arrfree(fieldHolder);
}

void TestShakespeare()
{
	char *inputFileName = "../Datasets/shakespeare/input.txt";
	size_t inputLength = GetFileSize(inputFileName);
	FILE *fp = fopen(inputFileName, "rb");assert(fp != NULL);
	int fileNumber = fileno(fp);
	unsigned char *input = mmap(NULL,inputLength, PROT_READ|PROT_WRITE, MAP_PRIVATE, fileNumber, 0);assert(input != NULL);
	double lookupBits = 10;
	int bSmooth = 2;
	int currentPrimeIndex = 0;
	double logValue = 0.0f;
	Field *fieldHolder = NULL;
	while(currentPrimeIndex < 5239)
	{
		if(first5239[currentPrimeIndex] >= bSmooth && FastCheck_2IsAGenerator(first5239[currentPrimeIndex]) == TRUE)
		{
			Field field = CreateField(first5239[currentPrimeIndex]);
			arrput(fieldHolder, field);
			logValue += log2(first5239[currentPrimeIndex]);
			if(logValue > lookupBits){break;}
		}
		currentPrimeIndex += 1;
	}
	TrainNetwork(inputLength, input, fieldHolder);
	for(size_t i = 0; i < arrlen(fieldHolder); i++){DestroyField(fieldHolder[i]);}
	assert(munmap(input, inputLength) != -1);
	arrfree(fieldHolder);fclose(fp);
}

void TestNumber()
{
	mpz_t factor0, factor1, num;
	mpz_inits(factor0, factor1, num, NULL);
	mpz_set_str(factor0, "37975227936943673922808872755445627854565536638199", 10);
	mpz_set_str(factor1, "40094690950920881030683735292761468389214899724061", 10);
	mpz_mul(num, factor0, factor1);
	size_t inputLength = (mpz_sizeinbase(num, 256) + 1); 
	unsigned char *input = calloc(inputLength, sizeof(unsigned char));
	size_t written = 0;
	mpz_export(input, &written, 1, sizeof(unsigned char), 1, 0, num);
	
	double lookupBits = 10;
	int bSmooth = 2;
	int currentPrimeIndex = 0;
	double logValue = 0.0f;
	Field *fieldHolder = NULL;
	while(currentPrimeIndex < 5239)
	{
		if(first5239[currentPrimeIndex] >= bSmooth && FastCheck_2IsAGenerator(first5239[currentPrimeIndex]) == TRUE)
		{
			Field field = CreateField(first5239[currentPrimeIndex]);
			arrput(fieldHolder, field);
			logValue += log2(first5239[currentPrimeIndex]);
			if(logValue > lookupBits){break;}
		}
		currentPrimeIndex += 1;
	}
	TrainNetwork(written, input, fieldHolder);
	for(size_t i = 0; i < arrlen(fieldHolder); i++){DestroyField(fieldHolder[i]);}
	arrfree(fieldHolder);
	free(input);
	mpz_clears(factor0, factor1, num, NULL);
}


int main()
{
	//TestField();
	//TestShakespeare();//change to this size_t maxCount = inputLength - 100 in TrainNetwork
	TestNumber();
	return 0;
}
