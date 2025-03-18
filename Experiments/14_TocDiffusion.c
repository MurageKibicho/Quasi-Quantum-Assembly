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

typedef struct group_struct *Group;
typedef struct square_struct *Square;
typedef struct exponent_struct *ExponentLookup;

struct square_struct
{
	int modulo;
	int length;
	int *square;
	ExponentLookup *lookupDiscreteLog; 
	ExponentLookup *exponentLookup;
};

struct exponent_struct
{
	/*2^x = y or 2^x + 2^y = 2 ^z*/
	int x;
	int y;
	int z;
};

struct group_struct
{
	int congruence;
	int modulo;
};
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
		mpz_tdiv_q(temporary1, productHolder, temporary0);
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
int GCD(int a, int b){while(b != 0){int temp = b;b = a % b;a = temp;}return a;}
int ModularExponentiation(int base, int exp, int mod){int result = 1;base = base % mod;while(exp > 0){if(exp % 2 == 1){result = (result * base) % mod;}exp = exp >> 1;base = (base * base) % mod;}return result;}
int FastCheck_2IsAGenerator(int primeNumber){int result = FALSE;if(primeNumber == 2){result = TRUE;}if(primeNumber % 8 == 3 || primeNumber % 8 == 5){int primeNumberMinusOne = primeNumber - 1;int n = primeNumberMinusOne;int q = 0;if(n % 2 == 0){q = 2;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}while(n % 2 == 0){n /= 2;}}for(int i = 3; i * i <= primeNumberMinusOne; i += 2){if(n % i == 0){q = i;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}while(n % i == 0){n /= i;}}}if(n > 1){q = n;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}}result = TRUE;}return result;}
void FreeExponentLookup(ExponentLookup exponentLookup){if(exponentLookup){free(exponentLookup);}}
void FreeGroup(Group group){if(group){free(group);}}
void FreeSquare(Square square){if(square){free(square->square);for(size_t i = 0; i < arrlen(square->lookupDiscreteLog); i++){FreeExponentLookup(square->lookupDiscreteLog[i]);}for(size_t i = 0; i < arrlen(square->exponentLookup); i++){FreeExponentLookup(square->exponentLookup[i]);}arrfree(square->exponentLookup);arrfree(square->lookupDiscreteLog);free(square);}}
int CompareExponentLookupY(const void *a, const void *b){ExponentLookup expA = *(ExponentLookup *)a;ExponentLookup expB = *(ExponentLookup *)b;return expA->y - expB->y;}
int CompareExponentLookupXY(const void *a, const void *b){ExponentLookup expA = *(ExponentLookup *)a;ExponentLookup expB = *(ExponentLookup *)b;
return expA->y - expB->y;
}
int IsPairwiseCoprime(int size, int *arr){for(int i = 0; i < size; i++){for(int j = i + 1; j < size; j++){if(GCD(arr[i], arr[j]) != 1){return 0;}}}return 1;}
ExponentLookup CreateExponentLookup(int x, int y, int z){ExponentLookup exponentLookup  = malloc(sizeof(struct exponent_struct));exponentLookup->x = x;exponentLookup->y = y;exponentLookup->z = z;return exponentLookup;	}
Group CreateGroup(int congruence, int modulo){Group group  = malloc(sizeof(struct group_struct));group->congruence = congruence;group->modulo = modulo;return group;}
Square CreateSquare(int modulo)
{
	if(modulo <= 0){assert(modulo > 0);}
	Square square  = malloc(sizeof(struct square_struct));
	square->modulo = modulo;
	square->length = ((modulo - 1) / 2) + 1;
	square->exponentLookup = NULL;
	square->lookupDiscreteLog = NULL;
	square->square =  calloc(square->length , sizeof(int));;
	mpz_t squareHolder, modHolder;mpz_inits(squareHolder, modHolder, NULL);mpz_set_si(modHolder, modulo);
	for(int i = 0; i < square->length; i++){mpz_set_ui(squareHolder, i);mpz_powm_ui(squareHolder, squareHolder, (unsigned long)2, modHolder);square->square[i] = (int) mpz_get_si(squareHolder);}
	int antiLog = 1;
	int *lookupTableValue = calloc(modulo, sizeof(int));
	ExponentLookup undefined = CreateExponentLookup(-1,0,-1);
	arrput(square->lookupDiscreteLog, undefined);
	for(int i = 0; i < modulo-1; i++)
	{
		ExponentLookup exponentLookup = CreateExponentLookup(i,antiLog,-1);
		arrput(square->lookupDiscreteLog, exponentLookup);
		assert(antiLog < modulo);
		lookupTableValue[antiLog] = i;
		antiLog = (antiLog * 2) % modulo;
	}
	for(int left0 = 0; left0 < modulo-1; left0++){for(int left1 = 0; left1 < modulo-1; left1++)
	{
		if(left0 >= left1)
		{
			int left0RegularMod = square->lookupDiscreteLog[left0]->y;
			int left1RegularMod = square->lookupDiscreteLog[left1]->y;
			int rightRegularMod = (left0RegularMod + left1RegularMod) % modulo;
			int rightExponent   = lookupTableValue[rightRegularMod];
			if(rightExponent % 2 == 0)
			{
				ExponentLookup exponentLookup = CreateExponentLookup(left0,left1,rightExponent);
				arrput(square->exponentLookup, exponentLookup);
				//printf("2^%2d + 2^%2d = 2 ^%2d | (%2d + %2d = %2d) mod %d\n", left0,left1,rightExponent, left0RegularMod, left1RegularMod, rightRegularMod, modulo);
			}	
		}
	}}
	mpz_clears(squareHolder, modHolder, NULL);
	free(lookupTableValue);
	return square;
}

void PrintSquares(Square *squares)
{
	size_t squaresLength = arrlen(squares);
	for(size_t i = 0; i < squaresLength; i++)
	{
		printf("|%3d|\n", squares[i]->modulo);	
		for(size_t j = 0; j < arrlen(squares[i]->lookupDiscreteLog); j++)
		{
			//printf("2 ^%3d = %3d (mod %3d)\n", squares[i]->lookupDiscreteLog[j]->x,squares[i]->lookupDiscreteLog[j]->y,  squares[i]->modulo);
		}
		printf("\n");
		for(size_t j = 0; j < arrlen(squares[i]->exponentLookup); j++)
		{
			printf("2 ^%3d + 2 ^%3d = 2 ^%3d mod %3d\n", squares[i]->exponentLookup[j]->x, squares[i]->exponentLookup[j]->y, squares[i]->exponentLookup[j]->z, squares[i]->modulo);
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

void LoadConstants(int rowCount, int columnCount, int *board, Square *squares, int compositeNumber, int smoothNumber)
{
	int l = 2;
	for(int i = 0; i < columnCount; i++)
	{
		qsort(squares[i]->lookupDiscreteLog, arrlen(squares[i]->lookupDiscreteLog), sizeof(ExponentLookup), CompareExponentLookupY);
		/*1st row is 2^x = compositeNumber (mod p)*/
		board[INDEX(0,i,columnCount)] = squares[i]->lookupDiscreteLog[compositeNumber % squares[i]->modulo]->x;
		/*2nd row is 2^x = smoothNumber (mod p) */
		board[INDEX(1,i,columnCount)] = squares[i]->lookupDiscreteLog[smoothNumber % squares[i]->modulo]->x;
		board[INDEX(2,i,columnCount)] = squares[i]->lookupDiscreteLog[l % squares[i]->modulo]->x;
		//board[INDEX(2,i,columnCount)] = l % squares[i]->modulo;
		assert(board[INDEX(0,i,columnCount)] != -1);
		assert(board[INDEX(1,i,columnCount)] != -1);
	}
}

void PrintCurrentRadixIndex(int length, int *currentIndex)
{
	for(size_t i = 0; i < length; i++)
	{
		printf("%d, ", currentIndex[i]);
	}
	printf("\n");
}
int FindNextRadixIndex(int length, int startIndex, int *currentIndex, int *maxIndex)
{
	int stop = 0;
	assert(startIndex >= 0);
	assert(startIndex <= length - 1);
	
	for(int i = startIndex; i >= 0; i--)
	{
		currentIndex[i] += 1;
		if(maxIndex[i] <= 0){assert(maxIndex[i] > 0);}
		//printf("|%3ld %3ld|", currentIndex[i], maxIndex[i]);
		if(currentIndex[i] == maxIndex[i]) 
		{
			currentIndex[i] = 0;
			if(i == 0){stop = 1;}
		}
		else{break;}
	}
	return stop;
}

int ResetRadixIndex(int length, int startIndex, int *currentIndex, int *maxIndex)
{
	int stop = 0;
	assert(startIndex >= 0);
	assert(startIndex <= length - 1);
	
	for(int i = startIndex; i >= 0; i--)
	{
		currentIndex[i] += 1;for(int j = i+1; j < length; j++){currentIndex[j] = 0;}
		if(maxIndex[i] <= 0){assert(maxIndex[i] > 0);}
		//printf("|%3ld %3ld|", currentIndex[i], maxIndex[i]);
		if(currentIndex[i] == maxIndex[i]) 
		{
			currentIndex[i] = 0;
			if(i == 0){stop = 1;}
		}
		else{break;}
	}
	return stop;
}

void GroupMe(int rowCount, int groupsLength, Group **groups, Square *squares, int *board)
{
	assert(groupsLength > 2);
	int *currentIndex = calloc(groupsLength, sizeof(int));
	int *maxIndex = calloc(groupsLength, sizeof(int));
	//maxIndex[0] = 2;maxIndex[1] = 3;maxIndex[2] = 1;maxIndex[3] = 4;maxIndex[4] = 2;maxIndex[5] = 5;
	for(int i = 0; i < groupsLength; i++){maxIndex[i] = arrlen(groups[i]);}
	int *moduli = calloc(groupsLength, sizeof(int));
	int *congruences = calloc(groupsLength, sizeof(int));
	int *modSumHolder = calloc(groupsLength, sizeof(int));
	int *modExponentHolder = calloc(groupsLength, sizeof(int));
	int *smoothExponentHolder = calloc(groupsLength, sizeof(int));
	int *compositeExponentHolder = calloc(groupsLength, sizeof(int));

	mpz_t crtResult;mpz_init(crtResult);
	mpz_t sumResult;mpz_init(sumResult);
	int crtCount  = 0;
	int evenCount = 0;
	int stop = 0;
	int k = 0;
	int validCount = 0;
	int validEvenCount = 0;
	while(stop == 0)
	{
		for(int i = 0; i < groupsLength; i++)
		{
			Group currentGroup = groups[i][currentIndex[i]];
			moduli[i] = currentGroup->modulo;
			assert(moduli[i] > 0);
			//printf("%2d,", moduli[i]);
		}
		//printf("\n");
		for(int i = 0; i < groupsLength; i++)
		{
			for(int j = i + 1; j < groupsLength; j++)
			{
				int gcd = GCD(moduli[i],moduli[j]);
				if(gcd != 1)
				{
					//printf("(%2d %2d : %2d | %2d %2d)\n", moduli[i],moduli[j], gcd, i, j);
					stop = ResetRadixIndex(groupsLength,j, currentIndex,maxIndex);
					//printf("\t%3d (%3d) : ",k, stop);PrintCurrentRadixIndex(groupsLength, currentIndex);
					i = groupsLength;
					break;
				}
			}
		}
		for(int i = 0; i < groupsLength; i++)
		{
			Group currentGroup = groups[i][currentIndex[i]];
			moduli[i] = currentGroup->modulo;
			assert(moduli[i] > 0);

		}
		int valid = IsPairwiseCoprime(groupsLength, moduli);
		if(valid)
		{	
			crtCount = 0;
			evenCount = 0;
			for(int i = 0; i < groupsLength; i++)
			{
				Group currentGroup = groups[i][currentIndex[i]];
				if(currentGroup->congruence > -1)
				{
					congruences[crtCount] = currentGroup->congruence;
					moduli[crtCount] = currentGroup->modulo;
					//printf("(%2d %2d),",congruences[crtCount], moduli[crtCount]);
					crtCount += 1;
				}
			}
			ChineseRemainderTheorem(crtCount, moduli, congruences, crtResult);
			for(int i = 0; i < groupsLength; i++)
			{
				int compositeExponent= board[INDEX(0,i,groupsLength)];
				int smoothExponent   = board[INDEX(1,i,groupsLength)];
				int smoothMod = ModularExponentiation(2, smoothExponent, squares[i]->modulo);
				mpz_add_ui(sumResult,crtResult, compositeExponent);
				int newCompositeExponent = mpz_mod_ui(sumResult, sumResult, squares[i]->modulo - 1);
				int newCompositeMod = ModularExponentiation(2, newCompositeExponent, squares[i]->modulo);
				int modSum = (newCompositeMod + smoothMod) % squares[i]->modulo;
				int modExponent = squares[i]->lookupDiscreteLog[modSum]->x;
				if(modExponent % 2 == 0){evenCount += 1;}
				modSumHolder[i] = modSum;
				modExponentHolder[i] = modExponent;
				smoothExponentHolder[i] = smoothExponent;
				compositeExponentHolder[i] = newCompositeExponent;
				//printf("(%d,%d %d)", squares[i]->modulo, compositeExponent, newCompositeExponent);	
				//printf("(2^%d+2^%d = 2^%d:%d)", newCompositeExponent, smoothExponent, modExponent, squares[i]->modulo);	
			}
			if(evenCount == groupsLength)
			{
				for(int i = 0; i < groupsLength; i++)
				{
					moduli[i] = squares[i]->modulo;
					//printf("(%d)", modExponentHolder[i]);	
					printf("(2^%d+2^%d=2^%d:%d)", compositeExponentHolder[i], smoothExponentHolder[i], modExponentHolder[i], modSumHolder[i]);	
				}
				mpz_set(sumResult, crtResult);
				ChineseRemainderTheorem(groupsLength, moduli, modSumHolder, sumResult);
				int perfectCheck = mpz_perfect_square_p(crtResult);
				//if(perfectCheck == 1)
				gmp_printf("(%d %d %d):%Zd\n", crtCount,validEvenCount, perfectCheck, crtResult);
				validEvenCount += 1;
			}
			//exit(1);
			validCount += 1;
		}
		if(stop == 1){break;}
		stop = FindNextRadixIndex(groupsLength,groupsLength-1, currentIndex,maxIndex);
		//printf("%3d (%3d) : ",k, stop);PrintCurrentRadixIndex(groupsLength, currentIndex);
		if(stop == 1){break;}
		k+=1;
	}
	free(maxIndex);
	free(currentIndex);
	free(modExponentHolder);free(smoothExponentHolder);free(compositeExponentHolder);free(modSumHolder);
	free(moduli);free(congruences);mpz_clear(crtResult);mpz_clear(sumResult);
}


void FindPerfectSquare(int rowCount, int columnCount, Square *squares, int *board)
{
	int possibleExponent = 0;
	Group **allGroups = malloc(columnCount * sizeof(Group*));
	for(int i = 0; i < columnCount; i++)
	{
		int compositeExponent= board[INDEX(0,i,columnCount)];
		int smoothExponent   = board[INDEX(1,i,columnCount)];
		Group *columnGroup = NULL;
		Group nullGroup = CreateGroup(-1, 1);
		arrput(columnGroup, nullGroup);//4,6,8,14,18,25
		//printf("2 ^(%3d+x) + 2 ^%3d\n",compositeExponent, smoothExponent);
		for(int j = 0; j < arrlen(squares[i]->exponentLookup); j++)
		{
			if(squares[i]->exponentLookup[j]->x == smoothExponent || squares[i]->exponentLookup[j]->y == smoothExponent)
			{
				possibleExponent =  squares[i]->exponentLookup[j]->x;
				if(smoothExponent != squares[i]->exponentLookup[j]->y){possibleExponent = squares[i]->exponentLookup[j]->y;}
				int modded = possibleExponent - compositeExponent;
				if(modded < 0){modded = modded + squares[i]->modulo-1;}
				int gcd = GCD(modded, squares[i]->modulo-1);
				Group group = CreateGroup(modded / gcd, (squares[i]->modulo-1)/gcd);
				arrput(columnGroup, group);
				//if((squares[i]->modulo-1)/gcd == 1)
				//printf("\t2 ^%3d + 2 ^%3d | (%3d+x) ≡ %3d | x ≡ %3d (mod %3d)\n",squares[i]->exponentLookup[j]->x, squares[i]->exponentLookup[j]->y, compositeExponent ,possibleExponent, modded / gcd, (squares[i]->modulo-1)/gcd);
			}
		}
		allGroups[i] = columnGroup;
	}
	GroupMe(rowCount, columnCount, allGroups, squares, board);
	for(int i = 0; i < columnCount; i++)
	{
		for(size_t j = 0; j < arrlen(allGroups[i]); j++){FreeGroup(allGroups[i][j]);}
		arrfree(allGroups[i]);
	}
	free(allGroups);
}
/*
(6 31063):215
(6 31064):425
(6 31065):383
(6 31066):110
(6 31067):131
(6 31068):509
(6 31069):551
(6 31070):47
(6 31071):761
(6 31072):1735065202102
(6 31073):971
(6 31074):152
*/
int main()
{
	int rowCount = 3;
	int compositeNumber = 84923;
	int smoothNumber    = 8400;
	int currentPrimeIndex = 0;
	double logValue = 0.0f;
	int bSmooth = 2;
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
	//PrintSquares(squares);
	FindPerfectSquare(rowCount , arrlen(squares),squares,board);
	PrintBoard(rowCount , arrlen(squares),squares,board);
	for(size_t i = 0; i < arrlen(squares); i++){FreeSquare(squares[i]);}
	arrfree(squares);
	free(board);
	return 0;
}
/*
8400 : 23,275, 59,3083
33600: 18,198, 25,61, 277, 457,8017
84293^2 * 2(23+275|298)    : 8400
84293^2 * 2(23+59|82)      : 8400
84293^2 * 2(59+275|334)    : 8400
84293^2 * 2(275+3083|3358) : 8400
84293^2 * 2(23+3083|3106)  : 8400
84293^2 * 2(59+3083|3136)  : 8400

8400 + 84923*3 = 513^2  | 513
8400 + 84923^2 = 2^82   | 2^41
8400 + 84923^2 = 2^334  | 2^167
8400 + 84923^2 = 2^3358 | 2^1679
8400 + 84923^2 = 2^3106 | 2^1553
8400 + 84923^2 = 2^3136 | 2^1568
*/
