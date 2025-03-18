#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <gmp.h>
#include "ff_asm_primes.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define PASSTEST 1
#define FAILTEST 0
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


int ThreePoint24(int top, int bottom)
{
	int n = bottom / 2;
	int x = top - bottom + 1;
	if(bottom % 2 == 1){return FAILTEST;}
	if(x < 0){return FAILTEST;}
	printf("%d %d\n", n, x);
	BinomialArray leftArray = NULL;
	BinomialArray rightArray = NULL;
	//mpz_t sum; mpz_init(sum);mpz_t binoL; mpz_init(binoL);mpz_t binoR; mpz_init(binoR);
	for(int k = 0; k <= n; k++)
	{
		arrput(leftArray, CreateBinomial(x, 2 * k));
		arrput(rightArray,CreateBinomial(x + n - k - 1, n -k));
		printf("(%d, %d) * (%d, %d) \n", leftArray[k]->n, leftArray[k]->k, rightArray[k]->n, rightArray[k]->k);
		//mpz_bin_uiui(binoL, leftArray[k]->n, leftArray[k]->k);
		//mpz_bin_uiui(binoR, rightArray[k]->n, rightArray[k]->k);
		//mpz_mul(binoL, binoL, binoR);
		//mpz_add(sum, sum,binoL);
	}
	//mpz_bin_uiui(binoL, top, bottom);
	//if(mpz_cmp(binoL, sum) != 0){printf("No 3.24\n");}
	
	for(size_t i = 0; i < arrlen(leftArray); i++) 
	{
		FreeBinomial(leftArray[i]);
		FreeBinomial(rightArray[i]);
	}
	arrfree(leftArray);
	arrfree(rightArray);
	//mpz_clear(sum);mpz_clear(binoL);mpz_clear(binoR);
}

void TestIdentity()
{
	int r324 = ThreePoint24(1400, 18);
	printf("%d\n", r324);
	
}
