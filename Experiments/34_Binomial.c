#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <gmp.h>
#include "ff_asm_primes.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define INDEX(x, y, cols) ((x) * (cols) + (y))
#define TRUE 1
#define FALSE 0
typedef struct square_struct *Square;
typedef struct exponent_struct *ExponentLookup;
struct square_struct
{
	int modulo;
	int length;
	ExponentLookup *square;
	ExponentLookup *discreteLogInput; 
	ExponentLookup *regularModInput;
};

struct exponent_struct
{
	/*2^x = y or 2^x + 2^y = 2 ^z*/
	int x;
	int y;
	int z;
};
int ModularExponentiation(int base, int exp, int mod){int result = 1;base = base % mod;while(exp > 0){if(exp % 2 == 1){result = (result * base) % mod;}exp = exp >> 1;base = (base * base) % mod;}return result;}
int FastCheck_2IsAGenerator(int primeNumber){int result = FALSE;if(primeNumber == 2){result = TRUE;}if(primeNumber % 8 == 3 || primeNumber % 8 == 5){int primeNumberMinusOne = primeNumber - 1;int n = primeNumberMinusOne;int q = 0;if(n % 2 == 0){q = 2;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}while(n % 2 == 0){n /= 2;}}for(int i = 3; i * i <= primeNumberMinusOne; i += 2){if(n % i == 0){q = i;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}while(n % i == 0){n /= i;}}}if(n > 1){q = n;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}}result = TRUE;}return result;}
void FreeExponentLookup(ExponentLookup exponentLookup){if(exponentLookup){free(exponentLookup);}}
void FreeSquare(Square square)
{
	if(square)
	{
		assert(arrlen(square->discreteLogInput) == arrlen(square->regularModInput));
		for(size_t i = 0; i < arrlen(square->discreteLogInput); i++)
		{
			FreeExponentLookup(square->discreteLogInput[i]);
			FreeExponentLookup(square->regularModInput[i]);
		}
		for(size_t i = 0; i < arrlen(square->square); i++)
		{
			FreeExponentLookup(square->square[i]);
		}
		arrfree(square->discreteLogInput);
		arrfree(square->regularModInput);
		arrfree(square->square);
		free(square);
	}
}
int CompareExponentLookupX(const void *a, const void *b){ExponentLookup expA = *(ExponentLookup *)a;ExponentLookup expB = *(ExponentLookup *)b;return expA->x - expB->x;}
int CompareExponentLookupY(const void *a, const void *b){ExponentLookup expA = *(ExponentLookup *)a;ExponentLookup expB = *(ExponentLookup *)b;return expA->y - expB->y;}
int CompareExponentLookupXY(const void *a, const void *b){ExponentLookup expA = *(ExponentLookup *)a;ExponentLookup expB = *(ExponentLookup *)b;return expA->y - expB->y;}
void ChineseRemainderTheorem(int fieldOrder, int *finiteField, int *mods, mpz_t result){mpz_set_ui(result,0);mpz_t productHolder; mpz_init(productHolder);mpz_set_ui(productHolder, 1);mpz_t modularInverseHolder; mpz_init(modularInverseHolder);mpz_t temporary0; mpz_init(temporary0);mpz_t temporary1; mpz_init(temporary1);for(int i = 0; i < fieldOrder; i++){assert(finiteField[i] > 0);mpz_mul_ui(productHolder, productHolder, finiteField[i]);}for(int i = 0; i < fieldOrder; i++){mpz_set_ui(temporary0, finiteField[i]);mpz_divexact(temporary1, productHolder, temporary0);mpz_invert(modularInverseHolder, temporary1, temporary0);mpz_mul(temporary1, temporary1,modularInverseHolder);mpz_mul_ui(temporary0, temporary1, mods[i]);mpz_add(result,result, temporary0);}mpz_mod(result, result,productHolder);mpz_clear(temporary0);mpz_clear(temporary1);mpz_clear(productHolder);mpz_clear(modularInverseHolder);}
ExponentLookup CreateExponentLookup(int x, int y, int z){ExponentLookup exponentLookup  = malloc(sizeof(struct exponent_struct));exponentLookup->x = x;exponentLookup->y = y;exponentLookup->z = z;return exponentLookup;	}
void PrintSquares(Square *squares)
{
	size_t squaresLength = arrlen(squares);
	for(size_t i = 0; i < squaresLength; i++)
	{
		assert(arrlen(squares[i]->discreteLogInput) == arrlen(squares[i]->regularModInput));
		printf("|%3d (%3d)|\n", squares[i]->modulo,squares[i]->length);	
		for(size_t j = 0; j < arrlen(squares[i]->discreteLogInput); j++){printf("2 ^%3d = %3d || %3d = 2^%3d(mod %3d)\n", squares[i]->discreteLogInput[j]->x,squares[i]->discreteLogInput[j]->y, squares[i]->regularModInput[j]->y,squares[i]->regularModInput[j]->x,squares[i]->modulo);}printf("\n");
	}
}
Square CreateSquare(int modulo)
{
	if(modulo <= 0){assert(modulo > 0);}
	Square square  = malloc(sizeof(struct square_struct));
	square->modulo = modulo;
	square->length = ((modulo - 1) / 2) + 1;
	square->regularModInput = NULL;
	square->discreteLogInput = NULL;
	square->square =  NULL;

	int antiLog = 1;
	ExponentLookup undefinedLog = CreateExponentLookup(-1,0,-1);
	ExponentLookup undefinedMod = CreateExponentLookup(-1,0,-1);
	arrput(square->discreteLogInput, undefinedLog);
	arrput(square->regularModInput, undefinedMod);
	for(int i = 0; i < modulo-1; i++)
	{
		ExponentLookup exponentLookup = CreateExponentLookup(i,antiLog,-1);
		ExponentLookup regularLookup  = CreateExponentLookup(i,antiLog,-1);
		arrput(square->regularModInput, regularLookup);
		arrput(square->discreteLogInput, exponentLookup);
		assert(antiLog < modulo);
		antiLog = (antiLog * 2) % modulo;
	}
	qsort(square->regularModInput, arrlen(square->regularModInput), sizeof(ExponentLookup), CompareExponentLookupY);
	
	//Fill squares
	for(int i = 0; i < square->length ; i++)
	{
		int value = (int) ((long) i *  (long) i) % (long) modulo;

		{
			ExponentLookup squareLookup = CreateExponentLookup(value,-1,-1);
			arrput(square->square, squareLookup);
		}
	}
	square->length = arrlen(square->square);
	assert(arrlen(square->square) == square->length);
	qsort(square->square, arrlen(square->square), sizeof(ExponentLookup), CompareExponentLookupX);

	return square;
}

int largestPossible = 1 << 30;
double Find_Log_MPZ_Double(mpz_t x){if(mpz_cmp_ui(x, 0) == 0){return 0;}signed long int ex;const double di = mpz_get_d_2exp(&ex, x);return ( (log(di) + log(2) * (double) ex) /log(2));}
double FileForma_GammaApproximation(double n, double k){double result = 0;result = (lgamma(n+1) -lgamma(n-k+1) -lgamma(k+1)) / log(2);return result;}
int FindLargestNLogarithm(double searchLog, int base){int max = largestPossible;int min = 0;int mid = 0;double currentLog = 0;double k = (double)base;while(min <= max){mid = min + (max - min) / 2;currentLog = FileForma_GammaApproximation((double)mid, k);if(currentLog < searchLog){min = mid + 1;}else{max = mid - 1;}}return max;}
int FindLargestNoLog(int number, int base, mpz_t binomialCoefficient){int currentValue = 0;int k = base;int n = 0;while(currentValue <= number){mpz_bin_uiui(binomialCoefficient, n, k);currentValue = mpz_get_ui(binomialCoefficient);n++;}return n-2;}
void PrintIntArray(int length, int*array){for(int i = 0; i < length; i++){printf("%4d ", array[i]);}printf("\n");}
void PrintFloatArray(int length, float*array){for(int i = 0; i < length; i++){printf("(%3d: %.3f)  ",i, array[i]);}printf("\n");}
void DimensionToInteger(mpz_t binomialCoefficient, mpz_t integer, int oneCount, int *array){assert(oneCount > 0);mpz_set_ui(integer, 0);for(int i = 0; i < oneCount; i++){mpz_bin_uiui(binomialCoefficient, array[i], i+1);mpz_add(integer, integer, binomialCoefficient);}}
void ChangeDimension(mpz_t binomialCoefficient, mpz_t integer, int oneCount, int *array){assert(oneCount > 0);double currentLogValue = 0.0f; int n = 0; int k = oneCount;while(mpz_cmp_ui(integer, 0) >= 0){if(k <= 1){break;}currentLogValue = Find_Log_MPZ_Double(integer);if(currentLogValue > 25.0){n = FindLargestNLogarithm(currentLogValue, k);}else{int number = mpz_get_ui(integer); n = FindLargestNoLog(number, k,binomialCoefficient);}if(n < 0){break;}mpz_bin_uiui(binomialCoefficient, n, k);mpz_sub(integer, integer, binomialCoefficient);array[k-1] = n;k -=1;}if(k == 1){array[k-1] = mpz_get_ui(integer);}}

enum sign_enum{ADD, SUB, MUL};
typedef struct binomial_struct *Binomial;
typedef Binomial* BinomialArray;
struct binomial_struct
{
	int n;
	int k;
	int sign;
	mpz_t value;
};

Binomial CreateBinomial(int n, int k)
{
	Binomial binomial = malloc(sizeof(struct binomial_struct));
	binomial->n = n;
	binomial->k = k;
	binomial->sign = ADD;
	mpz_init(binomial->value);
	return binomial;
}

void FreeBinomial(Binomial binomial)
{
	mpz_clear(binomial->value);
	free(binomial);
}

BinomialArray CreateBinomialArray(int length)
{
	BinomialArray BinomialArray= malloc(length* sizeof(Binomial));
	for(int i = 0; i < length; i++)
	{
		BinomialArray[i] = CreateBinomial(length-i, i+1);
	}
	return BinomialArray;
}

void FreeBinomialArray(int length, BinomialArray BinomialArray)
{	
	for(int i = 0; i < length; i++)
	{
		FreeBinomial(BinomialArray[i]);
	}
	free(BinomialArray);
}

double GammaApproximation(double n, double k, double base)
{
	double result = 0;
	result = (lgamma(n+1) -lgamma(n-k+1) -lgamma(k+1)) / log(base);
	return result;
}

void ThreePointOne(BinomialArray array)
{
	
}

void TestBinomials()
{
	int lengthX0 = 2;
	int lengthX1 = 2;
	int lengthY0 = 2;
	int lengthY1 = 2;
	BinomialArray x0 = CreateBinomialArray(lengthX0);
	BinomialArray x1 = CreateBinomialArray(lengthX1);
	BinomialArray y0 = CreateBinomialArray(lengthY0);
	BinomialArray y1 = CreateBinomialArray(lengthY1);
	FreeBinomialArray(lengthX0, x0); 
	FreeBinomialArray(lengthX1, x1); 
	FreeBinomialArray(lengthY0, y0); 
	FreeBinomialArray(lengthY1, y1); 
}

//1.81

int TestSTB()
{
	BinomialArray binomials = NULL;
	arrput(binomials, CreateBinomial(5, 2));
	arrput(binomials, CreateBinomial(6, 3));
	arrput(binomials, CreateBinomial(7, 4));
	printf("Stored binomials:\n");
	for (size_t i = 0; i < arrlen(binomials); i++)
	{
		printf("Binomial(%d, %d)\n", binomials[i]->n, binomials[i]->k);
	}

	for (size_t i = 0; i < arrlen(binomials); i++) 
	{
		FreeBinomial(binomials[i]);
	}
	arrfree(binomials);
	return 0;
}

void TestBin()
{
	int oneCount = 16;
	int currentPrimeIndex = 0;double logValue = 0.0f;int bSmooth = 1;
	Square *squares  = NULL;
	mpz_t compositeNumber,temporary0, temporary1,temporary2,temporary3;//x^2 - y ^ 2 = compositeNumber * k;
	mpz_inits(compositeNumber, temporary0,temporary1,temporary2, temporary3, NULL);
	mpz_set_str(compositeNumber, "1522605027922533360535618378132637429718068114961380688657908494580122963258952897654000350692006139", 10);
	//mpz_set_str(compositeNumber,"37975227936943673922808872755445627854565536638199",10);
	mpz_set(temporary0, compositeNumber);
	size_t blockSizeBytes = (mpz_sizeinbase(compositeNumber, 2) / 8) + 1;
	largestPossible = FindLargestNLogarithm((double)blockSizeBytes*8,oneCount)+1; assert(largestPossible > 0);
	int *binomialHolder = calloc(oneCount, sizeof(int));
	printf("%ld\n", blockSizeBytes *8);
	while(currentPrimeIndex < 5239)
	{
		if(first5239[currentPrimeIndex] > bSmooth && FastCheck_2IsAGenerator(first5239[currentPrimeIndex]) == TRUE)
		{
			Square square = CreateSquare(first5239[currentPrimeIndex]);
			arrput(squares, square);
			logValue += log2(first5239[currentPrimeIndex]);
			if(logValue > blockSizeBytes*8){break;}
		}
		currentPrimeIndex += 1;
	}
	
	
	ChangeDimension(temporary1, temporary0, oneCount, binomialHolder);
	DimensionToInteger(temporary1, temporary0,oneCount, binomialHolder);
	assert(mpz_cmp(temporary0, compositeNumber) == 0);
	for(int i = 0; i < oneCount; i++)
	{
		printf("%2d %2d", i+1, binomialHolder[i]);
		mpz_bin_uiui(temporary0, binomialHolder[i], i+1);
		//for(size_t j = 0; j < arrlen(squares); j++)
		for(size_t j = 0; j < 5; j++)
		{
			int mod = mpz_mod_ui(temporary1,temporary0, squares[j]->modulo);
			int generatorExponent = squares[j]->regularModInput[mod]->x;
			printf("(%3d %3d %3d)", mod, generatorExponent, squares[j]->modulo);
			//printf("(%3d %3d)", generatorExponent, squares[j]->modulo);
		}
		printf("\n");
	}
	//PrintSquares(squares);
	for(size_t i = 0; i < arrlen(squares); i++){FreeSquare(squares[i]);}
	arrfree(squares);
	free(binomialHolder);
	mpz_clears(compositeNumber, temporary0,temporary1,temporary2, temporary3, NULL);
}
int main()
{
	TestBin();
	return 0;
}
