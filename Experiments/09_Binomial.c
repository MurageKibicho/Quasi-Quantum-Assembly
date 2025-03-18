#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <gmp.h>
#include "ff_asm_primes.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
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

int main()
{
	return 0;
}
