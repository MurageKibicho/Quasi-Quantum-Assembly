#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gmp.h>
#include "ff_asm_primes.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define TRUE 1
#define FALSE 0
#define INDEX(x, y, cols) ((x) * (cols) + (y))
typedef struct int_index_struct *IntWithIndex;
struct int_index_struct
{
	mp_bitcnt_t a;
	mp_bitcnt_t b;
	mp_bitcnt_t c;
	int difference;
};
typedef struct polynomial_term_struct *Polynomial;
struct polynomial_term_struct
{
	int sourceIndex0;
	int sourceIndex1;
	int original0;
	int original1;
	int coefficient;
	int sum;
};


int ModularExponentiation(int base, int exp, int mod){int result = 1;base = base % mod;while(exp > 0){if(exp % 2 == 1){result = (result * base) % mod;}exp = exp >> 1;base = (base * base) % mod;}return result;}
int FastCheck_2IsAGenerator(int primeNumber){int result = FALSE;if(primeNumber == 2){result = TRUE;}if(primeNumber % 8 == 3 || primeNumber % 8 == 5){int primeNumberMinusOne = primeNumber - 1;int n = primeNumberMinusOne;int q = 0;if(n % 2 == 0){q = 2;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}while(n % 2 == 0){n /= 2;}}for(int i = 3; i * i <= primeNumberMinusOne; i += 2){if(n % i == 0){q = i;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}while(n % i == 0){n /= i;}}}if(n > 1){q = n;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}}result = TRUE;}return result;}

double GammaApproximation(double n, double k)
{
	double result = 0;
	result = (lgamma(n+1) -lgamma(n-k+1) -lgamma(k+1)) / log(2);
	return result;
}
void PrintIntWithIndex(int length, IntWithIndex *array)
{
	for(int i = 0; i < length; i++)
	{
		printf("(%4lu %4lu : %4lu (%4d))\n", array[i]->a,array[i]->b,array[i]->c, array[i]->difference);
	}
	printf("\n");
}
int CompareAB(const void *x, const void *y)
{
	const IntWithIndex ix = *(const IntWithIndex *)x;
	const IntWithIndex iy = *(const IntWithIndex *)y;
	if (ix->a != iy->a)return (ix->a > iy->a) - (ix->a < iy->a);  
	if (ix->b != iy->b)return (ix->b > iy->b) - (ix->b < iy->b);
	return (ix->c > iy->c) - (ix->c < iy->c);   
}
int CompareDifference(const void *x, const void *y)
{
	const IntWithIndex ix = *(const IntWithIndex *)x;
	const IntWithIndex iy = *(const IntWithIndex *)y;
	return (ix->difference) - (iy->difference);   
}
int **CreateTable(int tableHeight, size_t tableWidth)
{
	int **table = malloc(tableHeight * sizeof(int *));
	for(int i = 0; i < tableHeight; i++)
	{
		table[i] = calloc(tableWidth, sizeof(int));
	}
	return table;
}

void SetTableResult(int tableHeight, size_t width, int **table, mpz_t number)
{
	for(size_t j = 0, k = width-1; j < width && k >= 0; j++, k--)
	{
		table[tableHeight-1][k] = mpz_tstbit(number,j);
	}	
	gmp_printf("%Zd\n", number);
}

void PrintTable(int tableHeight, size_t width, int **table)
{
	for(int i = 0; i < tableHeight-1; i++)
	{
		for(size_t j = 0; j < width; j++)
		{
			printf("%d|", table[i][j]);
		}
		printf("\n");
	}
	for(size_t j = 0; j < width; j++)
	{
		printf("__");
	}
	printf("\n");
	for(size_t j = 0; j < width; j++)
	{
		printf("%d|", table[tableHeight-1][j]);
	}
	printf("\n");
}

void DestroyTable(int tableHeight, int **table)
{
	for(int i = 0; i < tableHeight; i++)
	{
		free(table[i]);
	}
	free(table);
}

int compare_polynomials(const void *a, const void *b)
{
	const Polynomial polyA = *(const Polynomial *)a;
	const Polynomial polyB = *(const Polynomial *)b;
	return polyA->sum - polyB->sum;  
}

void PrintPolynomial(int length, Polynomial *polynomial, int *generators, mpz_t compositeNumber)
{
	int sum = 0;
	mpz_t sumHolder, powerHolder;
	mpz_inits(sumHolder, powerHolder, NULL);mpz_set_ui(sumHolder, 0);
	for(int i = 0; i < length; i++)
	{
		sum = polynomial[i]->original0 + polynomial[i]->original1;
		mpz_set_ui(powerHolder, 0);
		mpz_setbit(powerHolder, sum);
		mpz_add(sumHolder, sumHolder, powerHolder);
		//printf("(%3d %3d), (%3d %3d : %3d)",polynomial[i]->sourceIndex0,polynomial[i]->sourceIndex1,polynomial[i]->original0,polynomial[i]->original1, sum);
		//printf("\n");
	}
	for(size_t j = 0; j < arrlen(generators); j++)
	{
		int currentMod = mpz_mod_ui(powerHolder, sumHolder,generators[j]);
		int targetMod  = mpz_mod_ui(powerHolder, compositeNumber,generators[j]);
		printf("(%2d, %2d : %2d)\n", currentMod, targetMod, generators[j]);
	}
	//gmp_printf("\n%Zd\n", sumHolder);
	mpz_clears(sumHolder, powerHolder, NULL);
}
//707329
void Rummage(mpz_t compositeNumber, mpz_t factor0, mpz_t factor1, mpz_t temporary0, int *generators)
{
	srand(6543);
	IntWithIndex *bitState = NULL;
	gmp_randstate_t state;
	gmp_randinit_mt(state);
	gmp_randseed_ui(state, 1085);
	int *modHolder = calloc(arrlen(generators), sizeof(int));
	int iterations = 1;
	for(int i = 0; i < iterations; i++)
	{
		int index0 = 1000;//rand() % 2048;
		int index1 = 1000;//rand() % 2048;
		//mpz_set_ui(factor0, 57);
		//mpz_set_ui(factor1, 123);
		//mpz_urandomb(factor0, state, index0);
		//mpz_urandomb(factor1, state, index1);
		mpz_set_str(factor0, "37975227936943673922808872755445627854565536638199", 10);
		mpz_set_str(factor1, "40094690950920881030683735292761468389214899724061", 10);
		if(mpz_mod_ui(temporary0, factor0, 2) == 0){mpz_add_ui(factor0, factor0,1);}
		if(mpz_mod_ui(temporary0, factor1, 2) == 0){mpz_add_ui(factor1, factor1,1);}
		mpz_mul(compositeNumber, factor0, factor1);
		IntWithIndex element = malloc(sizeof(struct int_index_struct));assert(element != NULL);
		element->a = mpz_popcount(factor0);
		element->b = mpz_popcount(factor1);
		element->c = mpz_popcount(compositeNumber);
		element->difference = (int)(element->a + element->b) - element->c;
		arrput(bitState, element);
		size_t tableWidth = mpz_sizeinbase(compositeNumber,2);
		int maxPossibleLength  = tableWidth - element->a;
		double setSize = GammaApproximation((double)maxPossibleLength , (double)element->b);
		int **table = CreateTable(element->a + 1,tableWidth);
		int powerLength = 0;
		int *powerHolderA = calloc(element->a, sizeof(int));for(int j = 0; j < element->a; j++){powerHolderA[j] = j;}
		int *powerHolderB = calloc(element->b, sizeof(int));for(int j = 0; j < element->b; j++){powerHolderB[j] = j;}
		Polynomial *polynomial = malloc(element->a * element->b * sizeof(Polynomial));
		SetTableResult(element->a + 1,tableWidth,table, compositeNumber);
		for(int j = 0; j < element->a; j++)
		{
			for(int k = 0; k < element->b; k++)
			{
				polynomial[INDEX(j,k,element->b)] = malloc(sizeof(struct polynomial_term_struct));
				polynomial[INDEX(j,k,element->b)]->sourceIndex0 = j;
				polynomial[INDEX(j,k,element->b)]->sourceIndex1 = k;
				polynomial[INDEX(j,k,element->b)]->original0    = powerHolderA[j];
				polynomial[INDEX(j,k,element->b)]->original1    = powerHolderB[k];
				polynomial[INDEX(j,k,element->b)]->coefficient  = 0;
				polynomial[INDEX(j,k,element->b)]->sum = polynomial[INDEX(j,k,element->b)]->original0 + polynomial[INDEX(j,k,element->b)]->original1;
			}
		}
		//qsort(polynomial, element->a * element->b, sizeof(Polynomial), compare_polynomials);
		PrintPolynomial(element->a * element->b, polynomial,generators,compositeNumber);
		//printf("%ld (%d choose %ld : %.3f bits)\n", tableWidth, maxPossibleLength, element->b, setSize);
		//PrintTable(element->a + 1,tableWidth, table);
		printf("%ld : (%d) %lu %lu\n", tableWidth, maxPossibleLength, element->a, element->b);
		for(size_t j = 0; j < arrlen(generators); j++)
		{
			int mod = mpz_mod_ui(temporary0, compositeNumber, generators[j]);
			modHolder[j] = mod;
			//printf("%4d(%4d) %4d\n", generators[j], generators[j]-1, mod);
		}
		DestroyTable(element->a + 1, table);
		free(powerHolderA);free(powerHolderB);
		for(int j = 0; j < element->a; j++)
		{
			for(int k = 0; k < element->b; k++)
			{
				free(polynomial[INDEX(j,k,element->b)]);
			}
		}
		free(polynomial);

	}
	qsort(bitState, arrlen(bitState), sizeof(IntWithIndex), CompareDifference);
	//PrintIntWithIndex(arrlen(bitState), bitState);
	for(size_t i = 0; i < arrlen(bitState); i++)
	{
		free(bitState[i]);
	}
	arrfree(bitState);
	gmp_randclear(state);
	free(modHolder);
}
int main()
{
	int *generators = NULL;
	int currentPrimeIndex = 0;
	double logValue = 0.0f;
	double compositeNumberBits = 20;
	int bSmooth = 2;
	while(currentPrimeIndex < 5239)
	{
		if(first5239[currentPrimeIndex] > bSmooth && FastCheck_2IsAGenerator(first5239[currentPrimeIndex]) == TRUE)
		{
			arrput(generators, first5239[currentPrimeIndex]);
			logValue += log2(first5239[currentPrimeIndex]);
			if(logValue > compositeNumberBits){break;}
		}
		currentPrimeIndex += 1;
	}
	mpz_t compositeNumber, factor0, factor1, temporary0;
	mpz_inits(compositeNumber, factor0, factor1, temporary0, NULL);
	mpz_set_ui(compositeNumber, 84923);
	size_t tableWidth = mpz_sizeinbase(compositeNumber, 2);
	Rummage(compositeNumber, factor0, factor1, temporary0, generators);
	mpz_clears(compositeNumber, factor0, factor1, temporary0, NULL);
	arrfree(generators);
	return 0;
}
/*
/*
Hello Justice Chigiti
Here's a summary of the meeting between the contractors.
Let me know what you think and I'll share your feedback with my father.
1. The contractors can NOT work concurrently.
- Your contractor will finish first then my contractor comes in.

2. My contractor will provide a modified quotation for renovating House 9.
-  The quotation will indicate the labor plus materials cost for painting inside the house.
-  The quotation will leave out the costs my father does not approve.
-  My father will follow this quotation. He will pay the new owner's contractor to paint inside the house.

3. My contractor will do the timber floor, electricals, and gypsum ceiling.
4. My contractor will paint outside the house.
5. My contractor will clean the terrazzo.
6. My contractor will provide a seating toilet for the SQ.


7. The new owner will clean the roof at their own expense.
8. The new owner will clean the garage cabro at their own expense.
9. The new owner will handle all plumbing and painting in the SQ, including the cost of installing the seating toilet.
10. The new owner will cover the cost of adding an electric fence.
7011 = 123 * 57
7011 mod 37 = 18 mod 37 = (2 ^ 17) mod 37 = (2 ^ (17+36k) ) mod 37
(2^a)+(2^b)+(2^c)+(2^d)+(2^e)+(2^f) * (2^A)+(2^B)+(2^C)+(2^D) = (2 ^ (17+36k) ) 
(2^a)+(2^b)+(2^c)+(2^d)+(2^e)+(2^f) * (2^A)+(2^B)+(2^C)+(2^D) = ((2^19)^-1) * 1 mod 37
2^19 * ((2^a)+(2^b)+(2^c)+(2^d)+(2^e)+(2^f) * (2^A)+(2^B)+(2^C)+(2^D)) = 1 mod 37
 
(2^6)+(2^5)+(2^4)+(2^3)+(2^1)+(2^0) * (2^5)+(2^4)+(2^3)+(2^0)
*/
