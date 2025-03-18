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
typedef struct hashtable_struct IntHashTable;
struct hashtable_struct
{
    int key;
    int value;
};
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
double Find_Log_MPZ_Double(mpz_t x){if(mpz_cmp_ui(x, 0) == 0){return 0;}signed long int ex;const double di = mpz_get_d_2exp(&ex, x);return ( (log(di) + log(2) * (double) ex) /log(2));}
int GCD(int a, int b){while(b != 0){int temp = b;b = a % b;a = temp;}return a;}
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
IntHashTable *HashSearch(int modulo){IntHashTable *hashTable = NULL;mpz_t base, mod, exponent, modResult;mpz_inits(base, mod, exponent, modResult, NULL);mpz_set_ui(mod, modulo);int moduloResult = 0;for(int i = 0; i < modulo; i++){mpz_set_ui(base, i);mpz_powm_ui(modResult, base,(unsigned long) 2, mod);	moduloResult = mpz_get_ui(modResult);hmput(hashTable, moduloResult, 1);}mpz_clears(base, mod, exponent, modResult, NULL);return hashTable;}
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
		printf("|%3d, %3d solutions|\n", squares[i]->modulo,squares[i]->length);	
		for(int j = 0; j < squares[i]->length ; j++)
		{
			int regularMultiple = squares[i]->discreteLogInput[squares[i]->square[j]->y + 1]->y;
		
			printf("%4d: %3d, %3d (%3d)\n", j, squares[i]->square[j]->x,squares[i]->square[j]->y, regularMultiple);	
		}
		for(size_t j = 0; j < arrlen(squares[i]->discreteLogInput); j++)
		{
			//printf("2 ^%3d = %3d || %3d = 2^%3d(mod %3d)\n", squares[i]->discreteLogInput[j]->x,squares[i]->discreteLogInput[j]->y, squares[i]->regularModInput[j]->y,squares[i]->regularModInput[j]->x,squares[i]->modulo);
		}
		printf("\n");
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
void LoadConstants(int rowCount, int columnCount, int *board, Square *squares, int compositeNumber, int smoothNumber)
{	
	int l = 3;
	int m = 537*537;
	for(int i = 0; i < columnCount; i++)
	{
		/*1st row is 2^x = compositeNumber (mod p)*/
		board[INDEX(0,i,columnCount)] = squares[i]->regularModInput[compositeNumber % squares[i]->modulo]->x;
		/*2nd row is smoothNumber (mod p) */
		board[INDEX(1,i,columnCount)] = smoothNumber % squares[i]->modulo;
		board[INDEX(2,i,columnCount)] = squares[i]->regularModInput[l % squares[i]->modulo]->x;
		board[INDEX(3,i,columnCount)] = m % squares[i]->modulo;
		printf("%d %d : %d\n", m, board[INDEX(3,i,columnCount)], squares[i]->modulo);
		assert(board[INDEX(0,i,columnCount)] != -1);
		assert(board[INDEX(1,i,columnCount)] != -1);
	}
	//exit(1);
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

int ResetRadixIndex(int length, int startIndex, int *currentIndex, int *maxIndex)
{
	int stop = 0;
	assert(startIndex >= 0);
	assert(startIndex <= length - 1);
	
	for(int i = startIndex; i >= 0; i--)
	{
		currentIndex[i] += 1;for(int j = i+1; j < length; j++){currentIndex[j] = 0;}
		if(currentIndex[i] == maxIndex[i]) 
		{
			currentIndex[i] = 0;
			if(i == 0){stop = 1;}
		}
		else{break;}
	}
	return stop;
}

void SumOfQuotientsDX(int length, int *congruences0, int *congruences1, int *moduli)
{
	mpz_t setSize, sumOfQuotients, mDividend, modulus, modInverse, dX, dY, temporary0;
	mpz_inits(setSize,sumOfQuotients,mDividend,modulus,modInverse, dX, dY,temporary0, NULL);
	mpz_set_ui(setSize, 1);mpz_set_ui(sumOfQuotients, 0);
	mpz_set_ui(dX, 0);mpz_set_ui(dY, 0);
	for(int i = 0; i < length; i++){mpz_mul_ui(setSize, setSize, moduli[i]);}
	for(int i = 0; i < length; i++)
	{
		mpz_set_ui(modulus, moduli[i]);
		mpz_divexact(mDividend,setSize, modulus);
		mpz_add(sumOfQuotients, sumOfQuotients, mDividend);
	}
	for(int i = 0; i < length; i++)
	{
		mpz_set_ui(modulus, moduli[i]);
		mpz_invert(modInverse, modulus, sumOfQuotients);
		mpz_sub(modInverse, sumOfQuotients, modInverse);
		mpz_mul_ui(temporary0, modInverse, congruences0[i]);
		mpz_add(dX, dX, temporary0);
		mpz_mul_ui(temporary0, modInverse, congruences1[i]);
		mpz_add(dY, dY, temporary0);
		mpz_mod(dX, dX, sumOfQuotients);
		mpz_mod(dY, dY, sumOfQuotients);
		gmp_printf("%Zd %Zd %Zd\n", modInverse, dX, dY);
	}
	
	
	gmp_printf("Setsize : %Zd\nSum of Quotients : %Zd\n", setSize, sumOfQuotients);
	if(mpz_cmp(dX,dY) > 0)
	{
		printf("0 is bigger\n");
	}
	else if(mpz_cmp(dX,dY) == 0)
	{
		printf("Equal size\n");
	}
	else
	{
		printf("1 is bigger\n");
	}
	mpz_clears(setSize,sumOfQuotients,mDividend,modulus,modInverse,dX, dY,temporary0, NULL);
}

void BaseExtension(int length, int *congruences, int *moduli, int base)
{

}

void BruteForceSolutionsDMP(int rowCount, int columnCount, int *board, Square *squares, double compositeNumberBits)
{
	mpz_t setSize,base, mod, modResult, exponentResult;
	mpz_inits(setSize,base, mod, modResult,exponentResult,NULL);
	mpz_set_ui(setSize, 1);mpz_set_ui(base, 2);
	int *currentIndex = calloc(columnCount, sizeof(int));
	int *maxIndex = calloc(columnCount, sizeof(int));
	int *moduli = calloc(columnCount, sizeof(int));
	int *congruences = calloc(columnCount, sizeof(int));
	int *expos = calloc(columnCount, sizeof(int));
	int **holder = malloc(columnCount * sizeof(int*));
	for(int i = 0; i < columnCount; i++){maxIndex[i] = squares[i]->length;moduli[i]=squares[i]->modulo;mpz_mul_ui(setSize, setSize, squares[i]->length);holder[i] = calloc(7, sizeof(int));}
	
	gmp_printf("Set size : %.3f\nComposite Bits : %.3f\n", Find_Log_MPZ_Double(setSize), compositeNumberBits);
	int stop = 0; int breakCheck = 0;int exponent = 0;
	while(1)
	{
		//RandomIndexArray(columnCount, currentIndex, maxIndex);
		for(int j = 0; j < columnCount; j++)
		{
			assert(currentIndex[j] < squares[j]->length);
			assert(currentIndex[j] >= 0);
			assert(squares[j]->square[currentIndex[j]]->y + 1 < arrlen(squares[j]->discreteLogInput));
			mpz_set_ui(mod, moduli[j]);
			
			int regularMultiple = squares[j]->discreteLogInput[squares[j]->square[currentIndex[j]]->y + 1]->y;
			int originalMod = squares[j]->square[currentIndex[j]]->x;
			
			holder[j][0] = board[INDEX(0,j,columnCount)];
			holder[j][1] = squares[j]->square[currentIndex[j]]->y;
			holder[j][2] = regularMultiple;
			holder[j][3] = board[INDEX(1,j,columnCount)];
			holder[j][4] = moduli[j];
			holder[j][5] = squares[j]->square[currentIndex[j]]->x;
			congruences[j] = squares[j]->square[currentIndex[j]]->x;
			expos[j] = regularMultiple;
			//printf("2^(%4d + %4d (%4d)) + %4d (mod %4d) = %4d\n", holder[j][0],holder[j][1],holder[j][2],holder[j][3],holder[j][4],holder[j][5]);
			exponent = board[INDEX(0,j,columnCount)] + squares[j]->square[currentIndex[j]]->y;
			mpz_powm_ui(modResult, base,(unsigned long) exponent, mod);
			//mpz_add_ui(modResult, modResult, );
		}
		ChineseRemainderTheorem(columnCount, moduli, congruences, modResult);
		ChineseRemainderTheorem(columnCount, moduli, expos, exponentResult);
		double squareNumberBits = Find_Log_MPZ_Double(modResult);
		double expoNumberBits = Find_Log_MPZ_Double(exponentResult);
		double leftSideBits = expoNumberBits + compositeNumberBits;
		int perfectCheck = mpz_perfect_square_p(modResult);
		if(perfectCheck > 0)gmp_printf("%d : %.3f %.3f\n",perfectCheck, leftSideBits, squareNumberBits);
		//gmp_printf("%Zd %Zd\n", exponentResult, modResult);
		if(perfectCheck > 0 && leftSideBits < squareNumberBits){break;}
		//if(leftSideBits < squareNumberBits){break;}
	}
		
	for(int i = 0; i < columnCount; i++){free(holder[i]);}
	free(holder);free(currentIndex);free(maxIndex);free(moduli);free(congruences);free(expos);
	mpz_clears(setSize,base,mod, modResult,exponentResult, NULL);
}

int FindNextRadixIndex(int length, int startIndex, int *currentIndex, int *maxIndex){int stop = 0;assert(startIndex >= 0);assert(startIndex <= length - 1);for(int i = startIndex; i >= 0; i--){currentIndex[i] += 1;if(maxIndex[i] <= 0){assert(maxIndex[i] > 0);}if(currentIndex[i] == maxIndex[i]) {currentIndex[i] = 0;if(i == 0){stop = 1;}}else{break;}}return stop;}
void PrintCurrentRadixIndex(int length, int *currentIndex){for(int i = 0; i < length; i++){printf("%d, ", currentIndex[i]);}printf("\n");}
void BruteForceSolutions(int rowCount, int columnCount, int *board, Square *squares)
{
	mpz_t setSize,base, mod, modResult, exponentResult;
	mpz_inits(setSize,base, mod, modResult,exponentResult,NULL);
	mpz_set_ui(setSize, 1);mpz_set_ui(base, 2);
	int exponent = 0;
	int *currentIndex = calloc(columnCount, sizeof(int));int *maxIndex = calloc(columnCount, sizeof(int));
	int *moduli = calloc(columnCount, sizeof(int));
	int *congruences = calloc(columnCount, sizeof(int));
	int *expos = calloc(columnCount, sizeof(int));
	for(int i = 0; i < columnCount; i++){maxIndex[i] = squares[i]->length;moduli[i]=squares[i]->modulo;mpz_mul_ui(setSize, setSize, squares[i]->length);}
	int **holder = malloc(columnCount * sizeof(int*));for(int i = 0; i < columnCount; i++){holder[i] = calloc(7, sizeof(int));}
	gmp_printf("Set size : %.3f\n", Find_Log_MPZ_Double(setSize));
	int stop = 0;
	int breakCheck = 0;
	int z = 0;int totalCount = 0;
	while(stop == 0)
	{
		breakCheck = 0;
		for(int j = 0; j < columnCount; j++)
		{
			assert(currentIndex[j] < squares[j]->length);
			assert(currentIndex[j] >= 0);
			assert(squares[j]->square[currentIndex[j]]->y + 1 < arrlen(squares[j]->discreteLogInput));
			mpz_set_ui(mod, moduli[j]);
			
			int regularMultiple = squares[j]->discreteLogInput[squares[j]->square[currentIndex[j]]->y + 1]->y;
			int originalMod = squares[j]->square[currentIndex[j]]->x;
			
			holder[j][0] = board[INDEX(0,j,columnCount)];
			holder[j][1] = squares[j]->square[currentIndex[j]]->y;
			holder[j][2] = regularMultiple;
			holder[j][3] = board[INDEX(1,j,columnCount)];
			holder[j][4] = moduli[j];
			holder[j][5] = squares[j]->square[currentIndex[j]]->x;
			congruences[j] = squares[j]->square[currentIndex[j]]->x;
			expos[j] = regularMultiple;
			//printf("2^(%2d + %2d (%2d)) + %d (mod %d) = %d %d \n", board[INDEX(0,j,columnCount)],squares[j]->square[currentIndex[j]]->y,regularMultiple, board[INDEX(1,j,columnCount)], moduli[j], mod4, mod4Result);
			exponent = board[INDEX(0,j,columnCount)] + squares[j]->square[currentIndex[j]]->y;
			mpz_powm_ui(modResult, base,(unsigned long) exponent, mod);
			//mpz_add_ui(modResult, modResult, );

		}
		if(breakCheck == 0)
		{
			ChineseRemainderTheorem(columnCount, moduli, congruences, modResult);
			ChineseRemainderTheorem(columnCount, moduli, expos, exponentResult);
			int perfectCheck = mpz_perfect_square_p(modResult);
			//if(perfectCheck == 1)
			//if(totalCount == 1088684)
			//if(mpz_cmp_ui(exponentResult, 3) == 0)
			{		
				printf("%3d : ", totalCount);
				for(int j = 0; j < columnCount; j++)
				{
					//printf("2^(%2d + %2d (%2d)) + %d (mod %d) = %d\n", holder[j][0],holder[j][1],holder[j][2],holder[j][3],holder[j][4],holder[j][5]);
				}
				for(int j = 0; j < columnCount; j++){printf("%3d,", congruences[j]);}
				//mpz_sqrt(modResult, modResult);
				gmp_printf("%Zd %Zd\n", exponentResult, modResult);
				z += 1;
			}
		}
		if(stop == 0){stop = FindNextRadixIndex(columnCount,columnCount-1, currentIndex,maxIndex);}
		totalCount += 1;
		if(totalCount == 10)break;;
	}
	//for(int i = 0; i < columnCount; i++){printf("%3d\n", maxIndex[i]);}
	for(int i = 0; i < columnCount; i++){free(holder[i]);}
	free(currentIndex);free(maxIndex);free(moduli);
	mpz_clears(setSize,base,mod, modResult,exponentResult, NULL);
	free(congruences);
	free(expos);
	free(holder);
}


void TestSumOfQuotientsCompare()
{
	int moduli[] = {5, 11, 14, 17, 9};
	int congruences0[] = {3,5,11,8,7};
	int congruences1[] = {0,0,10,1,2};
	int length = sizeof(moduli) / sizeof(int);
	
	SumOfQuotientsDX(length, congruences0, congruences1, moduli);
	
}

void TestBaseExtension()
{
	int newBase = 3;
	int moduli[] = {5, 7};
	int congruences[] = {2,3};
	int length = sizeof(moduli) / sizeof(int);
	
	BaseExtension(length, congruences, moduli, newBase);
	
}



int main()
{
	int rowCount = 4;
	int compositeNumber = 84923;
	int smoothNumber    = 33600;
	int currentPrimeIndex = 0;double logValue = 0.0f;
	int bSmooth = 10;
	Square *squares  = NULL;
	while(logValue < 26.0f && currentPrimeIndex < 5239)
	{
		if(first5239[currentPrimeIndex] > bSmooth && 
		FastCheck_2IsAGenerator(first5239[currentPrimeIndex]) == TRUE &&
		compositeNumber % first5239[currentPrimeIndex] != 0 &&
		smoothNumber % first5239[currentPrimeIndex] != 0
		)
		{
			Square square = CreateSquare(first5239[currentPrimeIndex]);
			arrput(squares, square);
			logValue += log2(first5239[currentPrimeIndex]);
		}
		currentPrimeIndex += 1;
	}
	int *board = calloc(rowCount * arrlen(squares), sizeof(int));
	LoadConstants(rowCount, arrlen(squares), board, squares, compositeNumber, smoothNumber);
	FindSquareSolutions(rowCount, arrlen(squares), board, squares);

	TestSumOfQuotientsCompare();
	TestBaseExtension();
	//BruteForceSolutions(rowCount, arrlen(squares), board, squares);
	PrintSquares(squares);

	//PrintBoard(rowCount , arrlen(squares),squares,board);
	for(size_t i = 0; i < arrlen(squares); i++){FreeSquare(squares[i]);}
	arrfree(squares);free(board);
	return 0;
}
