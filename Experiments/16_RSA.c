#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
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
typedef struct crt_struct *CRT;
struct crt_struct
{	
	mpz_t temporary1;
};
struct square_struct
{
	int modulo;
	int length;
	mpz_t sumOfQuotientsInverse;
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
uint32_t rngState[1] = {1975546};  
uint32_t XorShift(uint32_t state[]){uint32_t x = state[0];x ^= x >> 17;x ^= x << 5;state[0] = x;return x;}
int GCD(int a, int b){while(b != 0){int temp = b;b = a % b;a = temp;}return a;}
int ModularExponentiation(int base, int exp, int mod){int result = 1;base = base % mod;while(exp > 0){if(exp % 2 == 1){result = (result * base) % mod;}exp = exp >> 1;base = (base * base) % mod;}return result;}
int FastCheck_2IsAGenerator(int primeNumber){int result = FALSE;if(primeNumber == 2){result = TRUE;}if(primeNumber % 8 == 3 || primeNumber % 8 == 5){int primeNumberMinusOne = primeNumber - 1;int n = primeNumberMinusOne;int q = 0;if(n % 2 == 0){q = 2;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}while(n % 2 == 0){n /= 2;}}for(int i = 3; i * i <= primeNumberMinusOne; i += 2){if(n % i == 0){q = i;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}while(n % i == 0){n /= i;}}}if(n > 1){q = n;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}}result = TRUE;}return result;}
double Find_Log_MPZ_Double(mpz_t x){if(mpz_cmp_ui(x, 0) == 0){return 0;}signed long int ex;const double di = mpz_get_d_2exp(&ex, x);return ( (log(di) + log(2) * (double) ex) /log(2));}
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
		mpz_clear(square->sumOfQuotientsInverse);
		arrfree(square->discreteLogInput);
		arrfree(square->regularModInput);
		arrfree(square->square);
		free(square);
	}
}
void RandomIndexArray(int arrayLength, int *currentIndex, int *maxIndex)
{
	for(int i = 0; i < arrayLength; i++)
	{
		currentIndex[i] = XorShift(rngState) % maxIndex[i];
	}
}

int FindNextRadixIndex(int length, int startIndex, int *currentIndex, int *maxIndex){int stop = 0;assert(startIndex >= 0);assert(startIndex <= length - 1);for(int i = startIndex; i >= 0; i--){currentIndex[i] += 1;if(maxIndex[i] <= 0){assert(maxIndex[i] > 0);}if(currentIndex[i] == maxIndex[i]) {currentIndex[i] = 0;if(i == 0){stop = 1;}}else{break;}}return stop;}
void PrintCurrentRadixIndex(int length, int *currentIndex){for(int i = 0; i < length; i++){printf("%d, ", currentIndex[i]);}printf("\n");}
int CompareArray(const void *a, const void *b){int int_a = *((int*)a);int int_b = *((int*)b);return int_a - int_b;}
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
		//for(int j = 0; j < squares[i]->length ; j++){printf("|%3d, %3d|", squares[i]->square[j]->x,squares[i]->square[j]->y);	}printf("\n");
		//for(size_t j = 0; j < arrlen(squares[i]->discreteLogInput); j++){printf("2 ^%3d = %3d || %3d = 2^%3d(mod %3d)\n", squares[i]->discreteLogInput[j]->x,squares[i]->discreteLogInput[j]->y, squares[i]->regularModInput[j]->y,squares[i]->regularModInput[j]->x,squares[i]->modulo);}printf("\n");
	}
}
void PrintBoard(int rowCount, int columnCount, Square *squares, int *board)
{
	for(int j = 0; j < columnCount; j++)
	{
		printf("%3d|", squares[j]->modulo);
	}
	printf("\n");
	for(int i = 0; i < rowCount; i++)
	{
		for(int j = 0; j < columnCount; j++)
		{
			int cell = board[INDEX(i,j,columnCount)];
			printf("%3d,", cell);
		}
		printf("\n");
	}	
	printf("\n");
}
Square CreateSquare(int modulo)
{
	if(modulo <= 0){assert(modulo > 0);}
	Square square  = malloc(sizeof(struct square_struct));
	square->modulo = modulo;
	square->length = ((modulo - 1) / 2) + 1;
	mpz_init(square->sumOfQuotientsInverse);
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

int JacobiSymbol(mpz_t x, mpz_t n)
{
	int result = -1;
	mpz_t temp0, temp1;
	mpz_inits(temp0, temp1, NULL);
	//In libgmp, the Jacobi symbol is defined only for n odd.
	assert(mpz_mod_ui(temp0, n, 2) == 1);
	mpz_set(temp0, x);
	mpz_set(temp1, n);
	result = mpz_jacobi(temp0, temp1);

	mpz_clears(temp0, temp1, NULL);
	return result;
}


int IsXaQuadraticResidueModN(mpz_t x, mpz_t n)
{
	int result = FALSE;
	//If x ≡ 0 mod n then true
	//if(mpz_divisible_p(x, n) > 0){return TRUE;}
	
	//Check Jacobi Symbol : if -1 then failed
	int jacobiSymbol = JacobiSymbol(x, n);
	if(jacobiSymbol == -1){return FALSE;}
	else if(jacobiSymbol == 0)
	{
		//x and n share a factor
		printf("X and N share a factor\n");
		exit(1);
	}
	
	//Maybe baby
	return TRUE;
}
void LoadConstants(int rowCount, int columnCount, int *board, Square *squares, mpz_t compositeNumber, mpz_t smoothNumber)
{	
	mpz_t temp0;mpz_init(temp0);
	for(int i = 0; i < columnCount; i++)
	{
		/*1st row is 2^x = compositeNumber (mod p)*/
		board[INDEX(0,i,columnCount)] = squares[i]->regularModInput[mpz_mod_ui(temp0, compositeNumber, squares[i]->modulo)]->x;
		/*2nd row is smoothNumber (mod p) */
		board[INDEX(1,i,columnCount)] =  mpz_mod_ui(temp0, smoothNumber, squares[i]->modulo);
		//board[INDEX(2,i,columnCount)] = squares[i]->regularModInput[l % squares[i]->modulo]->x;
		//board[INDEX(3,i,columnCount)] = m % squares[i]->modulo;
		assert(board[INDEX(0,i,columnCount)] != -1);
		assert(board[INDEX(1,i,columnCount)] != -1);
	}
	mpz_clear(temp0);
}
void LoadSumofQuotientsInverse(Square *squares, mpz_t returnValue)
{
	mpz_t setSize, sumOfQuotients, mDividend, modulus, modInverse, dX, dY, temporary0;
	mpz_inits(setSize,sumOfQuotients,mDividend,modulus,modInverse, dX, dY,temporary0, NULL);
	mpz_set_ui(setSize, 1);mpz_set_ui(sumOfQuotients, 0);
	mpz_set_ui(dX, 0);mpz_set_ui(dY, 0);
	for(int i = 0; i < arrlen(squares); i++){mpz_mul_ui(setSize, setSize, squares[i]->modulo);}	
	for(int i = 0; i < arrlen(squares); i++)
	{
		mpz_set_ui(modulus, squares[i]->modulo);
		mpz_divexact(mDividend,setSize, modulus);
		mpz_add(sumOfQuotients, sumOfQuotients, mDividend);
	}
	for(int i = 0; i < arrlen(squares); i++)
	{
		mpz_set_ui(modulus, squares[i]->modulo);
		mpz_invert(modInverse, modulus, sumOfQuotients);
		mpz_sub(modInverse, sumOfQuotients, modInverse);
		mpz_set(squares[i]->sumOfQuotientsInverse, modInverse);
	}
	//printf("%.3f %.3f\n", Find_Log_MPZ_Double(setSize), Find_Log_MPZ_Double(sumOfQuotients));
	//gmp_printf("Setsize : %Zd\nSum of Quotients : %Zd\n", setSize, sumOfQuotients);
	mpz_set(returnValue, sumOfQuotients);
	mpz_clears(setSize,sumOfQuotients,mDividend,modulus,modInverse,dX, dY,temporary0, NULL);
}

void FindSquareSolutions(int rowCount, int columnCount, int *board, Square *squares)
{
	for(int i = 0; i < columnCount; i++)
	{
		int logCompositeNumberModP = board[INDEX(0,i,columnCount)];
		int smoothNumberModP = board[INDEX(1,i,columnCount)];
		//printf("|%3d|", squares[i]->modulo);
		for(int j = 0; j < squares[i]->length ; j++)
		{
			int smoothNumberModPRight = ((squares[i]->square[j]->x - smoothNumberModP) < 0) ? (squares[i]->square[j]->x - smoothNumberModP) +  squares[i]->modulo : squares[i]->square[j]->x - smoothNumberModP; 
			int logSmoothNumberModPRight = squares[i]->regularModInput[smoothNumberModPRight]->x;
			if(logSmoothNumberModPRight < 0)
			{
				//printf("\nUnhandled Square solution mod (%3d)\n", squares[i]->modulo);
				squares[i]->square[j]->y = -1;
			}
			{
				//Find x as log
				squares[i]->square[j]->y = ((logSmoothNumberModPRight - logCompositeNumberModP) < 0) ? (logSmoothNumberModPRight - logCompositeNumberModP + squares[i]->modulo - 1) : (logSmoothNumberModPRight - logCompositeNumberModP);
				//printf("(%2d, %2d)", squares[i]->square[j]->x,squares[i]->square[j]->y);	
			}
		}
		//printf("\n");
	}
}



void FreeCRTConstants(int fieldOrder, CRT *constants)
{
	for(int i = 0; i < fieldOrder; i++)
	{
		mpz_clear(constants[i]->temporary1);
		free(constants[i]);
	}
	free(constants);
}

CRT *FillCRTConstants(int fieldOrder, int *finiteField, mpz_t setSize)
{
	CRT *constants = malloc(fieldOrder * sizeof(CRT));
	mpz_t temporary0,temporary1, productHolder,modularInverseHolder;
	mpz_inits(temporary0, temporary1,productHolder,modularInverseHolder,NULL);
	mpz_set(productHolder, setSize);
	for(int i = 0; i < fieldOrder; i++)
	{
		constants[i] = malloc(sizeof(struct crt_struct));
		mpz_init(constants[i]->temporary1);
		mpz_set_ui(temporary0, finiteField[i]);
		mpz_tdiv_q(temporary1, productHolder, temporary0);
		mpz_invert(modularInverseHolder, temporary1, temporary0);
		mpz_mul(temporary1, temporary1,modularInverseHolder);
		mpz_set(constants[i]->temporary1, temporary1);
	}
	mpz_clears(temporary0, temporary1, productHolder,modularInverseHolder, NULL);
	return constants;
}


void BruteForceSolutionsMain(int rowCount, int columnCount, int *board, Square *squares, double compositeNumberBits, mpz_t sumOfQuotients)
{
	mpz_t setSize,temporary0, temporary1, cachedCRT,productHolder;
	mpz_inits(setSize,temporary0,temporary1,cachedCRT,productHolder,NULL);
	mpz_set_ui(setSize, 1);mpz_set_ui(productHolder, 1);
	int *currentIndex = calloc(columnCount, sizeof(int));int *maxIndex = calloc(columnCount, sizeof(int));
	int *finiteField = calloc(columnCount, sizeof(int));
	for(int i = 0; i < columnCount; i++){finiteField[i] = squares[i]->modulo;maxIndex[i] = squares[i]->length;mpz_mul_ui(productHolder,productHolder, finiteField[i]);mpz_mul_ui(setSize, setSize, squares[i]->length);}
	CRT *constants = FillCRTConstants(columnCount, finiteField, productHolder);
	int k = 0;
	int resultMod = 0;
	while(k < 10)
	{
		mpz_set_ui(cachedCRT, 0);
		RandomIndexArray(columnCount, currentIndex, maxIndex);
		resultMod = 0;
		for(int j = 0; j < columnCount ; j++)
		{
			int regularMultiple = squares[j]->discreteLogInput[squares[j]->square[currentIndex[j]]->y + 1]->y;
			int rightSideRegular = squares[j]->square[currentIndex[j]]->x;
			int leftSideExponent = (board[INDEX(0,j,columnCount)] + squares[j]->square[currentIndex[j]]->y) % (squares[j]->modulo - 1);
			int leftSideRegular = squares[j]->discreteLogInput[leftSideExponent+1]->y;
			
			mpz_mul_ui(temporary0, constants[j]->temporary1, rightSideRegular);
			int modMul = mpz_mod_ui(temporary1, temporary0, 4);
			int modRightSideRegular = rightSideRegular % 4;
			int cachedMod = mpz_mod_ui(temporary1, constants[j]->temporary1, 4);
			mpz_add(cachedCRT,cachedCRT, temporary0);
			resultMod += modMul;
			printf("(2^((%4d + %4d (%4d)) = %4d)) %4d + %4d (mod %4d) = %4d|%d ⋅ %d = %d|\n", board[INDEX(0,j,columnCount)],squares[j]->square[currentIndex[j]]->y, regularMultiple,leftSideExponent,leftSideRegular,board[INDEX(1,j,columnCount)], squares[j]->modulo,rightSideRegular,modRightSideRegular,cachedMod,modMul);
		}
		mpz_mod(cachedCRT, cachedCRT,productHolder);
		//mpz_mod_ui(cachedCRT, cachedCRT,4);
		resultMod %= 4;
		gmp_printf("%Zd %d\n", cachedCRT, resultMod);
		k += 1;	
		
	}
	free(currentIndex);free(maxIndex);free(finiteField);
	mpz_clears(setSize,temporary0,temporary1,cachedCRT,productHolder,NULL);
	FreeCRTConstants(columnCount, constants);
}

void BruteForceSolutions(int rowCount, int columnCount, int *board, Square *squares, double compositeNumberBits, mpz_t sumOfQuotients)
{	
	mpz_t setSize,temporary0, temporary1, cachedCRT,productHolder;
	mpz_inits(setSize,temporary0,temporary1,cachedCRT,productHolder,NULL);
	mpz_set_ui(setSize, 1);mpz_set_ui(productHolder, 1);
	int *currentIndex = calloc(columnCount, sizeof(int));int *maxIndex = calloc(columnCount, sizeof(int));
	int *finiteField = calloc(columnCount, sizeof(int));
	for(int i = 0; i < columnCount; i++){finiteField[i] = squares[i]->modulo;maxIndex[i] = 4;mpz_mul_ui(productHolder,productHolder, finiteField[i]);mpz_mul_ui(setSize, setSize, squares[i]->length);}
	CRT *constants = FillCRTConstants(columnCount, finiteField, productHolder);
	int k = 0;
	printf("%d\n", columnCount);
	int stop = 0;
	while(stop == 0)
	{
		RandomIndexArray(columnCount, currentIndex, maxIndex);
		PrintCurrentRadixIndex(columnCount, currentIndex);
		mpz_set_ui(cachedCRT, 0);
		for(int j = 0; j < columnCount ; j++)
		{
			int rightSideRegular = currentIndex[j];
			mpz_mul_ui(temporary0, constants[j]->temporary1, rightSideRegular);
			mpz_add(cachedCRT,cachedCRT, temporary0);
		}
		//mpz_mod_ui(cachedCRT, cachedCRT,4);
		gmp_printf("%Zd\n", cachedCRT);
		k += 1;	
		if(stop == 0){stop = FindNextRadixIndex(columnCount,columnCount-1, currentIndex,maxIndex);}
		stop = 1;
	}
	free(currentIndex);free(maxIndex);free(finiteField);
	mpz_clears(setSize,temporary0,temporary1,cachedCRT,productHolder,NULL);
	FreeCRTConstants(columnCount, constants);
}

void Smoothen(int rowCount, int columnCount, int *board, Square *squares, mpz_t compositeNumber, mpz_t smoothNumber)
{
	mpz_t sumOfQuotients; mpz_init(sumOfQuotients);
	double compositeNumberBits = Find_Log_MPZ_Double(compositeNumber);
	LoadConstants(rowCount, columnCount, board, squares, compositeNumber, smoothNumber);
	//PrintBoard(rowCount , columnCount,squares,board);
	FindSquareSolutions(rowCount, columnCount, board, squares);
	LoadSumofQuotientsInverse(squares, sumOfQuotients);
	BruteForceSolutionsMain(rowCount, columnCount, board, squares, compositeNumberBits, sumOfQuotients);
	mpz_clear(sumOfQuotients);
}
void FactorRSA(mpz_t compositeNumber)
{
	double compositeNumberBits = Find_Log_MPZ_Double(compositeNumber);
	int currentPrimeIndex = 0;double logValue = 0.0f;
	int bSmooth = 10;
	int rowCount = 4;
	Square *squares  = NULL;
	while(currentPrimeIndex < 5239)
	{
		if(first5239[currentPrimeIndex] > bSmooth && FastCheck_2IsAGenerator(first5239[currentPrimeIndex]) == TRUE)
		{
			Square square = CreateSquare(first5239[currentPrimeIndex]);
			arrput(squares, square);
			logValue += log2(first5239[currentPrimeIndex]);
			if(logValue > compositeNumberBits){break;}
		}
		currentPrimeIndex += 1;
	}
	int *board = calloc(rowCount * arrlen(squares), sizeof(int));
	
	mpz_t smoothNumber;
	mpz_init(smoothNumber);
	
	for(int i = 336000; i < 336001; i++)
	{
		mpz_set_ui(smoothNumber, i);
		if(IsXaQuadraticResidueModN(smoothNumber, compositeNumber) == TRUE)
		{
			Smoothen(rowCount, arrlen(squares), board, squares, compositeNumber, smoothNumber);
		}
		else
		{
			gmp_printf("%Zd Failed\n", smoothNumber);
		}
	}
	
	//PrintSquares(squares);
	for(size_t i = 0; i < arrlen(squares); i++){FreeSquare(squares[i]);}
	arrfree(squares);free(board);
	mpz_clear(smoothNumber);
}

int main()
{
	mpz_t compositeNumber,factor0,factor1, multiple;
	mpz_inits(compositeNumber,factor0,factor1,multiple, NULL);
	mpz_set_ui(compositeNumber, 84923);
	//mpz_set_str(compositeNumber, "1522605027922533360535618378132637429718068114961380688657908494580122963258952897654000350692006139", 10);
	mpz_set_str(factor0, "37975227936943673922808872755445627854565536638199", 10);
	mpz_set_str(factor1, "40094690950920881030683735292761468389214899724061", 10);	
	
	mpz_set_ui(factor0, 57820);//16349,29376,10394,29693,6891,32381,15258,18182,37179,12513,1601,18962,22157,28759,36818,6765
	mpz_mul(multiple,compositeNumber,factor0);
	//mpz_add_ui(multiple, multiple, 140);
	int perfectCheck = mpz_perfect_square_p(multiple);
	mpz_sqrt(factor1, multiple);
	mpz_mul(factor1, factor1,factor1);
	mpz_sub(factor1, multiple,factor1);
	gmp_printf("%Zd | %.3f \n",multiple,factor1,Find_Log_MPZ_Double(factor1));

	//assert(mpz_cmp(multiple, compositeNumber) == 0);
	
	
	FactorRSA(compositeNumber);
	
	mpz_clears(compositeNumber,factor0,factor1,multiple, NULL);
	return 0;
}

