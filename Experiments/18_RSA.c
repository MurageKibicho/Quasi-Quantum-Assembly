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

double *GenerateLookupTable(int size)
{
	int length = 1 << size;
	double increase = 1/(double)length;
	double *table = calloc(length, sizeof(double));
	for(int i = 1; i < length; i++)
	{
		table[i] = log2(1 + increase*i);
	}
	return table;	
}
double GammaApproximation(double n, double k, double base)
{
	double result = 0;
	result = (lgamma(n+1) -lgamma(n-k+1) -lgamma(k+1)) / log(base);
	return result;
}

void FindBestSquare()
{
	double n = 400.0f;
	double k = 40.0f;
	double base = 10.0f;
	double gamma = GammaApproximation(n, k, base);
	printf("%.3f\n", gamma);
}
int main()
{
	mpz_t compositeNumber,factor0,factor1, multiple0, multiple1;
	mpz_inits(compositeNumber,factor0,factor1,multiple0, multiple1, NULL);
	mpz_set_str(compositeNumber, "22112825529529666435281085255026230927612089502470015394413748319128822941402001986512729726569746599085900330031400051170742204560859276357953757185954298838958709229238491006703034124620545784566413664540684214361293017694020846391065875914794251435144458199", 10);
	mpz_set_str(factor0, "22112825529529666435281085255026230927612089502470015394413748319128822941402001986512729726569746599085900330031400051170742204560859276357953757185954298838958709229238491006703034124620545784566413664540684214361293017794020846391065875914794251435144459199", 10);
	mpz_set_str(factor1, "22112825529529666435281085255026230927612089502470015394413748319128822941402001986512729726569746599085900330031400051170742204560859276357953757185954298838958709229238491006703034124620545784566413664540684214361293017694020846391065875914794251435144458199", 10);
	
	//mpz_set_ui(compositeNumber, 84923);
	//mpz_set_ui(factor0, 537);
	//mpz_set_ui(factor1, 513);
	//mpz_mul(multiple0, factor0, factor0);
	//mpz_mul(multiple1, factor1, factor1);
	//mpz_sub(multiple0, multiple0,multiple1);
	//mpz_mod(multiple0, multiple0, compositeNumber);

	//gmp_printf("%Zd\n",multiple0);

	FindBestSquare();

	mpz_clears(compositeNumber,factor0,factor1,multiple0, multiple1, NULL);
	return 0;
}
