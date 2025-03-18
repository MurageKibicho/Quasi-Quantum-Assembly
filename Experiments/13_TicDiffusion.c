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

typedef struct board_struct *Board;
typedef struct square_struct *Square;
typedef struct exponent_struct *ExponentLookup;

struct board_struct
{
	int *board;
	int *playerMoves;
	int visitCount;
	double wins;
	Board *children;
};

struct square_struct
{
	int modulo;
	int length;
	int *square;
	int *generators;
	int *lookupTableValue; 
	int *lookupTableLog; 
	ExponentLookup *exponentLookup;
};

struct exponent_struct
{
	/*2^x + 2^y = 2 ^z*/
	int x;
	int y;
	int z;
};

int ModularExponentiation(int base, int exp, int mod) 
{
	int result = 1;
	base = base % mod;
	while(exp > 0)
	{
		if(exp % 2 == 1)
		{
			result = (result * base) % mod;
		}
		exp = exp >> 1;
		base = (base * base) % mod;
	}
	return result;
}
int FastCheck_2IsAGenerator(int primeNumber)
{
	int result = FALSE;
	//2 is a generator in GF(2)
	if(primeNumber == 2){result = TRUE;}
	if(primeNumber % 8 == 3 || primeNumber % 8 == 5)
	{
		//Check that 2 ^ prime factor,q, of (primeNumber - 1) is not 1 mod primeNumber
		int primeNumberMinusOne = primeNumber - 1;
		int n = primeNumberMinusOne;
		int q = 0;
		//Check if 2 is a prime factor
		if(n % 2 == 0)
		{
			q = 2;
			int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);
			if(checkvalue == 1){result = FALSE;return result;}
			while(n % 2 == 0){n /= 2;}
		}
		//Check if odd primes are a factor
		for(int i = 3; i * i <= primeNumberMinusOne; i += 2)
		{
			if(n % i == 0)
			{
				q = i;
				int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);
				if(checkvalue == 1){result = FALSE;return result;}
				while(n % i == 0){n /= i;}
			}
		}
		//n is prime itself
		if(n > 1)
		{
			q = n;
			int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);
			if(checkvalue == 1){result = FALSE;return result;}
		}
		//Passed all checks
		result = TRUE;
	}
	return result;
}

ExponentLookup CreateExponentLookup(int x, int y, int z)
{
	ExponentLookup exponentLookup  = malloc(sizeof(struct exponent_struct));
	exponentLookup->x = x;
	exponentLookup->y = y;
	exponentLookup->z = z;
	return exponentLookup;	
}

int CompareExponentLookup(const void *a, const void *b)
{
	ExponentLookup expA = *(ExponentLookup *)a;
	ExponentLookup expB = *(ExponentLookup *)b;

	if(expA->x != expB->x)
	{
		return expA->x - expB->x;
	}

	if(expA->y != expB->y)
	{
		return expA->y - expB->y;
	}

	return expA->z - expB->z;
}

int CompareExponentLookupZ(const void *a, const void *b)
{
	ExponentLookup expA = *(ExponentLookup *)a;
	ExponentLookup expB = *(ExponentLookup *)b;

	if(expA->z != expB->z)
	{
		return expA->z - expB->z;
	}

	if(expA->x != expB->x)
	{
		return expA->x - expB->x;
	}

	return expA->y - expB->y;
}
Board CreateBoard(int rowCount, int columnCount, int movesPerTurn)
{	
	Board board  = malloc(sizeof(struct board_struct));
	board->board = calloc(rowCount * columnCount, sizeof(int));
	board->playerMoves = calloc(movesPerTurn, sizeof(int));
	board->visitCount = 0;
	board->wins = 0.0f;
	board->children = NULL;
	return board;
}

Square CreateSquare(int modulo)
{
	if(modulo <= 0){assert(modulo > 0);}
	Square square  = malloc(sizeof(struct square_struct));
	square->modulo = modulo;
	square->length = ((modulo - 1) / 2) + 1;
	square->generators = NULL;
	square->exponentLookup = NULL;
	square->square =  calloc(square->length , sizeof(int));;
	square->lookupTableValue = calloc(modulo, sizeof(int));
	square->lookupTableLog = calloc(modulo, sizeof(int));
	mpz_t squareHolder, modHolder;
	mpz_inits(squareHolder, modHolder, NULL);mpz_set_si(modHolder, modulo);

	int result = FastCheck_2IsAGenerator(modulo);
	if(result == FALSE)
	{
		//printf("2 is NOT a generator in GF(%d)\n", modulo);
		arrput(square->generators, -1);
	}
	else
	{
		//printf("2 is a generator in GF(%d)\n", modulo);
		arrput(square->generators, 2);
		/*Squaring is symmetric : x^2 == (-x)^2 or Every square has exactly two roots.log(a-c)=log(a) + log(1 - c/a).Restrict to modulus where 2 is a generator.if |A = g^i| mod m and |B = g^j| mod m then |AB| mod m = |g ^ |(i+j)mod(m-1)|| mod m */
		for(int i = 0; i < square->length; i++)
		{
			mpz_set_ui(squareHolder, i);
			mpz_powm_ui(squareHolder, squareHolder, (unsigned long)2, modHolder);
			square->square[i] = (int) mpz_get_si(squareHolder);
		}
		for(int i = 0; i < modulo; i++){square->lookupTableValue[i]=-1;square->lookupTableLog[i]  = -1;}
		
		int value = 1;
		//2^i = value;
		for(int i = 0; i < modulo-1; i++)
		{	
			square->lookupTableValue[value] = i;
			square->lookupTableLog[i] = value;
			//printf("\t2^%d MOD %d : %d\n", i, modulo, value);
			value = (value * 2) % modulo;
		}
		
		for(int i = 1; i < modulo; i++)
		{
			if(square->lookupTableValue[i] == -1)
			{
				//printf("-1 at %d %d\n", i, modulo);
				assert(square->lookupTableValue[i] != -1);
			}
		}
		
		//Fill exponent lookup
		for(int x = 0; x < modulo-1; x++)
		{
			for(int y = 0; y < modulo-1; y++)
			{
				if(x >= y)
				{
					int xNoLog = square->lookupTableLog[x];
					int yNoLog = square->lookupTableLog[y];
					int zNoLog = (xNoLog + yNoLog) % modulo;
					int z = square->lookupTableValue[zNoLog];
					if(z % 2 == 0)
					{
						ExponentLookup exponentLookup = CreateExponentLookup(x,y,z);
						arrput(square->exponentLookup, exponentLookup);
						//printf("2^%2d + 2^%2d = 2 ^%2d | (%2d + %2d = %2d) mod %d\n", x,y,z, xNoLog, yNoLog, zNoLog, modulo);
					}
				}
			}
		}
		//qsort(square->exponentLookup, arrlen(square->exponentLookup), sizeof(ExponentLookup), CompareExponentLookupZ);

	}
	
	mpz_clears(squareHolder, modHolder, NULL);
	return square;
}

void PrintSquares(int squaresLength, Square *squares)
{
	for(int i = 0; i < squaresLength; i++)
	{
		printf("|%3d|\n", squares[i]->modulo);	
		for(size_t j = 0; j < arrlen(squares[i]->exponentLookup); j++)
		{
			printf("2^%3d + 2^%3d = 2^%3d\n", squares[i]->exponentLookup[j]->x, squares[i]->exponentLookup[j]->y, squares[i]->exponentLookup[j]->z);
		}
		printf("\n");
	}
}

void PrintBoard(int rowCount, int columnCount, int *columnField, Board board)
{
	for(int j = 0; j < columnCount; j++)
	{
		printf("%3d|", columnField[j]);
	}
	printf("\n");
	for(int i = 0; i < rowCount; i++)
	{
		for(int j = 0; j < columnCount; j++)
		{
			int cell = board->board[INDEX(i,j,columnCount)];
			printf("%3d,", cell);
		}
		printf("\n");
	}	
	printf("\n");
}

void FreeBoard(Board board)
{
	if(board)
	{
		for(size_t i = 0; i < arrlen(board->children) ; i++)
		{
			FreeBoard(board->children[i]);
		}
		arrfree(board->children);
		free(board->playerMoves);
		free(board->board);
		free(board);
	}
}

void FreeExponentLookup(ExponentLookup exponentLookup)
{
	if(exponentLookup)
	{
		free(exponentLookup);
	}
}

void FreeSquare(Square square)
{
	if(square)
	{
		free(square->square);
		free(square->lookupTableValue);
		free(square->lookupTableLog);
		arrfree(square->generators);
		for(size_t i = 0; i < arrlen(square->exponentLookup); i++)
		{
			FreeExponentLookup(square->exponentLookup[i]);
		}
		arrfree(square->exponentLookup);
		free(square);
	}
}



void LoadConstants(int rowCount, int columnCount, Board board, Square *squares, int compositeNumber, int smoothNumber)
{
	assert(columnCount == arrlen(squares));
	/*2^1 = 2 for all*/
	for(int i = 0; i < columnCount; i++)
	{
		/*1st row is 2^x(n * k (mod p)): multiples of compositeNumber */
		board->board[INDEX(0,i,columnCount)] = squares[i]->lookupTableValue[compositeNumber % squares[i]->modulo];
		/*2nd row is 2^x(smoothNumber (mod p)) */
		//board->board[INDEX(1,i,columnCount)] = squares[i]->lookupTableValue[smoothNumber % squares[i]->modulo];
		board->board[INDEX(1,i,columnCount)] = squares[i]->lookupTableValue[smoothNumber % squares[i]->modulo];
		/*3rd row is x^2 */
		board->board[INDEX(2,i,columnCount)] = squares[i]->lookupTableValue[squares[i]->square[0]];
	}
}
int GCD(int a, int b){if(a==0 || b == 0){return 1;} while(b != 0){int temp = b;b = a % b;a = temp;}return a;}
void FindMatch(int rowCount, int columnCount, Board board, Square *squares)
{
	assert(columnCount == arrlen(squares));
	for(int i = 0; i < columnCount; i++)
	{
		int cell = board->board[INDEX(1,i,columnCount)];
		int original = board->board[INDEX(0,i,columnCount)];;
		int mod = squares[i]->modulo - 1;
		printf("%d\n", squares[i]->modulo);
		for(size_t j = 0; j < arrlen(squares[i]->exponentLookup); j++)
		{
			int x = squares[i]->exponentLookup[j]->x;
			int y = squares[i]->exponentLookup[j]->y;
			int modded = (y - original) >= 0 ? (y - original) : mod - (y - original) * -1 ;
			int gcd = GCD(modded, mod);
			if(x > cell){break;}
			if(x == cell)
			{
				printf("x = %3d (mod %d)\n", modded/gcd, mod / gcd);
				//printf("x + %3d = %3d (mod %d)\n", board->board[INDEX(0,i,columnCount)], squares[i]->exponentLookup[j]->y,squares[i]->modulo - 1);
				//printf("\t2^%3d + 2^%3d = 2^%3d\n", squares[i]->exponentLookup[j]->x, squares[i]->exponentLookup[j]->y, squares[i]->exponentLookup[j]->z);
			}
		}
		//printf("\n");
	}
}

int main()
{
	srand(5567);
	int rowCount = 3;
	int columnCount = 0;
	int movesPerTurn = 1;
	double logValue = 0.0f;
	int *columnField = NULL;
	Square *squares  = NULL;
	int currentPrimeIndex = 0;
	int n = 84923;//  6,  3, 10,  2, 29, 27, 18, 21, 38, 81, 58,

	int l = 8400;
	int bSmooth = 7;
	while(logValue < 26.0f && currentPrimeIndex < 5239)
	{
		Square square = CreateSquare(first5239[currentPrimeIndex]);
		if(square->generators[0] == 2 && first5239[currentPrimeIndex] > bSmooth)
		{
			arrput(columnField, first5239[currentPrimeIndex]);
			arrput(squares, square);
			logValue += log2(first5239[currentPrimeIndex]);
			columnCount += 1;
		}
		else{FreeSquare(square);}
		currentPrimeIndex += 1;
	}
	Board root = CreateBoard(rowCount, columnCount, movesPerTurn);
	LoadConstants(rowCount, columnCount, root, squares, n, l);
	
	{
		FindMatch(rowCount, columnCount, root, squares);
	}
	//PrintSquares(columnCount, squares);
	PrintBoard(rowCount, columnCount,columnField, root);
	

	FreeBoard(root);
	for(size_t i = 0; i < arrlen(squares); i++)
	{
		FreeSquare(squares[i]);
	}
	arrfree(squares);
	arrfree(columnField);
	return 0;
}
