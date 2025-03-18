#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>
#include <math.h>
#include "Binomial.h"

void FindNumberBase(int numberToFactor, int freeParameterPolynomialDegree, int *freeParameterBase)
{
	double power = 0.0f;
	*freeParameterBase = (int) pow(numberToFactor, 1.0 / (double) freeParameterPolynomialDegree);
}

int FindBaseExpansion(int number, int base)
{
	int polynomialDegree = 0;
	int sum = 0;
	while(number > 0)
	{
		int numberInBase = number % base;
		printf("(%3d * %3d ^%3d)\n",numberInBase, base, polynomialDegree);
		polynomialDegree += 1;
		number /= base;	
	}
}
//1 15 29 8 = (1000) for 45113. Idk where i got 1000
//0 0  29 9 = ( 908)

// 111 8 46 = () for 181418309
//   1 8 47

void Test(int numberToFactor, int *n, int *k)
{
	int max = 10000;
	int l =0;
	mpz_t bino; mpz_init(bino);
	for(int i = 1400; i < max; i++)
	{
		for(int j = 2; j < max-1 && j < i; j++)
		{
			mpz_bin_uiui(bino, i, j);
			mpz_mod_ui(bino, bino, numberToFactor);
			if(mpz_cmp_ui(bino, 0) == 0)
			{
				//gmp_printf("%d : %3d %3d : %Zd\n", l, i, j, bino);
				*n = i;
				*k = j;
				i = max;
				break; 
				l += 1;
			}
		}
	}
	mpz_clear(bino);
}

/*
1.48
1.50
//Table 3
//3.9
//3.34
//3.38
//3.68
*/
int test1()
{
	int numberToFactor = 1418309;
	
	/*Number base approximately equal to d_root(numberToFactor)
	The quotient of n divided by m^d should be exactly one*/
	int freeParameterBase = 0;
	/*Polynomial degree 3 changes
	50 to 80  digits = 3
	80 to 100 digits = 4
	*/
	int freeParameterPolynomialDegree = 3;
	int targetN = 0;
	int targetK = 0;
	FindNumberBase(numberToFactor, freeParameterPolynomialDegree, &freeParameterBase);
	freeParameterBase = 113;
	FindBaseExpansion(numberToFactor, freeParameterBase);
	Test(numberToFactor, &targetN, &targetK);
	printf("Number base : %d %d %d\n", freeParameterBase, targetN, targetK);
	return 0;
}



int main()
{
	BinomialArray binomials = NULL;
	arrput(binomials, CreateBinomial(1400, 368));
	
	printf("Stored binomials:\n");
	for (size_t i = 0; i < arrlen(binomials); i++)
	{
		printf("Binomial(%d, %d)\n", binomials[i]->n, binomials[i]->k);
	}
	TestIdentity();
	for (size_t i = 0; i < arrlen(binomials); i++) 
	{
		FreeBinomial(binomials[i]);
	}
	arrfree(binomials);
	return 0;
	return 0;
}
