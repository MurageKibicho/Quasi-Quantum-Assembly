#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <gmp.h>
#include "ff_asm_primes.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define kb_PRIME_ARRAY_LENGTH 5239
#define kb_ENABLE_ERRORS 1
#define kb_TRUE 1
#define kb_FALSE 0
#define PRINT_ERROR(msg) \
do { \
if (kb_ENABLE_ERRORS) { \
    fprintf(stderr, "\033[31mERROR: %s\033[0m\n", msg); \
    exit(1); \
} \
} while (0)
/*
Assumptions:
1. Finite field is a prime number
*/
typedef struct kibicho_brocot_struct *kb;
typedef struct kibicho_brocot_finite_fields_struct *kbField;
typedef struct kibicho_crt_struct *kbCRT;
struct kibicho_brocot_struct
{
	int *numerator;
	int *denominator;
};
struct kibicho_crt_struct
{	
	mpz_t temporary0;
};
struct kibicho_brocot_finite_fields_struct
{
	int *finiteField;
	int floatingPointBits;
	int bitCountGoal;
	int smoothnessFactor;
	mpz_t numberRange;
	mpz_t numberRangeHalfFloor;
	kbCRT *cachedCRTConstants;
};
int ModularExponentiation(int base, int exp, int mod){int result = 1;base = base % mod;while(exp > 0){if(exp % 2 == 1){result = (result * base) % mod;}exp = exp >> 1;base = (base * base) % mod;}return result;}
int FastCheck_2IsAGenerator(int primeNumber){int result = kb_FALSE;if(primeNumber == 2){result = kb_TRUE;}if(primeNumber % 8 == 3 || primeNumber % 8 == 5){int primeNumberMinusOne = primeNumber - 1;int n = primeNumberMinusOne;int q = 0;if(n % 2 == 0){q = 2;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = kb_FALSE;return result;}while(n % 2 == 0){n /= 2;}}for(int i = 3; i * i <= primeNumberMinusOne; i += 2){if(n % i == 0){q = i;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = kb_FALSE;return result;}while(n % i == 0){n /= i;}}}if(n > 1){q = n;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = kb_FALSE;return result;}}result = kb_TRUE;}return result;}
uint32_t XorShift(uint32_t state[]){uint32_t x = state[0];x ^= x >> 17;x ^= x << 5;state[0] = x;return x;}

//Check if (a*b + c) overflows int64_t
int CheckAddMulOverflow(int64_t a, int64_t b, int64_t c) 
{
	if(a == 0 || b == 0){return 1;}
	if(a > 0){if(b > 0){if(a > (INT64_MAX - c) / b) return 0;}else{if(b < (INT64_MIN - c) / a) return 0;}}
	else{if(b > 0){if(a < (INT64_MIN - c) / b) return 0;}else{if(b < (INT64_MAX - c) / a) return 0;}}
	return 1;
}
kbField CreateField(size_t bitCount, size_t smoothnessFactor)
{
	kbField result = malloc(sizeof(struct kibicho_brocot_finite_fields_struct));
	if(result == NULL){PRINT_ERROR("kbField CreateField Error: field is NULL");}
	if(smoothnessFactor < 2){PRINT_ERROR("kbField CreateField Error: smoothnessFactor must be >= 0");}
	if(smoothnessFactor > 45000){PRINT_ERROR("kbField CreateField Error: smoothnessFactor must be < 45000");}
	
	result->finiteField = NULL;
	result->cachedCRTConstants = NULL;
	result->bitCountGoal = bitCount;
	result->smoothnessFactor = smoothnessFactor;
	mpz_init(result->numberRange);mpz_set_ui(result->numberRange,1);
	mpz_init(result->numberRangeHalfFloor);
	int currentPrimeIndex = 0;
	double logValue = 0.0f;
	while(currentPrimeIndex < kb_PRIME_ARRAY_LENGTH)
	{
		if(first5239[currentPrimeIndex] >= smoothnessFactor && FastCheck_2IsAGenerator(first5239[currentPrimeIndex]) == kb_TRUE)
		{	
			kbCRT crtConstant = malloc(sizeof(struct kibicho_crt_struct));
			mpz_init(crtConstant->temporary0);
			arrput(result->cachedCRTConstants, crtConstant);
			arrput(result->finiteField, first5239[currentPrimeIndex]);
			logValue += log2(first5239[currentPrimeIndex]);
			mpz_mul_ui(result->numberRange, result->numberRange, first5239[currentPrimeIndex]);
			if(logValue > (double) bitCount){break;}
		}
		currentPrimeIndex += 1;
	}
	if(mpz_cmp_ui(result->numberRange, 0) <= 0){PRINT_ERROR("kbCRT_FillCRTConstants Error: setSize is 0");}
	if(arrlen(result->finiteField) < 1){PRINT_ERROR("kbCRT_FillCRTConstants Error: arrlen(finiteField) is less than 1");}
	if(arrlen(result->cachedCRTConstants) != arrlen(result->finiteField)){PRINT_ERROR("kbCRT_FillCRTConstants Error: length cachedCRTConstants != length finiteField");}
	mpz_t temporary0,temporary1, temporary2, productHolder,modularInverseHolder;
	mpz_inits(temporary0, temporary1, temporary2,productHolder,modularInverseHolder,NULL);
	mpz_set(productHolder, result->numberRange);
	for(size_t i = 0; i < arrlen(result->finiteField); i++)
	{
		mpz_set_ui(temporary0, result->finiteField[i]);
		mpz_tdiv_q(temporary1, productHolder, temporary0);
		mpz_invert(modularInverseHolder, temporary1, temporary0);
		mpz_mul(temporary1, temporary1,modularInverseHolder);
		mpz_set(result->cachedCRTConstants[i]->temporary0, temporary1);			
		//gmp_printf("||%Zd %d||\n", tempConstant->fixedPointInverse,finiteField[i]);
	}
	mpz_clears(temporary0, temporary1, temporary2, productHolder,modularInverseHolder, NULL);
	mpz_tdiv_q_2exp(result->numberRangeHalfFloor,result->numberRange, 1);
	return result;
}
kb kb_init(double number, kbField field)
{
	if(field == NULL){PRINT_ERROR("kb_init Error: kbField is NULL");}	
	if(isnan(number) || isinf(number)){PRINT_ERROR("kb_init Error: input is NaN or Infinity");}	
	kb result = malloc(sizeof(struct kibicho_brocot_struct));
	double remainder = number;
	int sign = (number < 0) ? -1 : 1;
	remainder = fabs(number);
	int64_t a, h[3] = {0, 1, 0}, k[3] = {1, 0, 0};
	for(int i = 0; i < 100; i++)//Prevent infinite loops
	{
		a = (int64_t)remainder;
		remainder = 1.0 / (remainder - a);
		if(!CheckAddMulOverflow(a, h[1], h[0]) || !CheckAddMulOverflow(a, k[1], k[0])){break;}
		h[2] = a * h[1] + h[0];
		k[2] = a * k[1] + k[0];
		if (k[2] < 0) break;
		h[0] = h[1]; h[1] = h[2];
       	k[0] = k[1]; k[1] = k[2];
       	if(remainder == 0) break;
	}
	//printf("%ld %ld : %.3f\n", h[1], k[1], (double) h[1]/ (double) k[1]);
	result->numerator = NULL;
	result->denominator = NULL;
	int currentNumerator   = 0;
	int currentDenominator = 0;
	int currentField = 0;
	gmp_printf("|%ld /%ld|\n", h[1], k[1]);
	if(sign == 1)
	{
		for(size_t i = 0; i < arrlen(field->finiteField); i++)
		{
			currentField = field->finiteField[i];
			currentNumerator   = h[1] % currentField;
			currentDenominator = k[1] % currentField;
			arrput(result->numerator, currentNumerator);
			arrput(result->denominator, currentDenominator);
		}
	}
	else
	{
		for(size_t i = 0; i < arrlen(field->finiteField); i++)
		{
			currentField = field->finiteField[i];
			currentNumerator   = currentField - (h[1] % currentField);
			currentDenominator = k[1] % currentField;
			arrput(result->numerator, currentNumerator);
			arrput(result->denominator, currentDenominator);
		}
	}
	return result;
}

void kbField_clear(kbField a)
{
	if(a)
	{
		if(a->cachedCRTConstants)
		{
			for(size_t i = 0; i < arrlen(a->cachedCRTConstants); i++)
			{
				if(a->cachedCRTConstants[i]){mpz_clear(a->cachedCRTConstants[i]->temporary0);free(a->cachedCRTConstants[i]);}
			}
			arrfree(a->cachedCRTConstants);
		}
		if(a->finiteField){arrfree(a->finiteField);}
		mpz_clear(a->numberRange);
		mpz_clear(a->numberRangeHalfFloor);
		free(a);
	}
}

void kb_clear(kb a)
{
	if(a)
	{
		if(a->numerator)arrfree(a->numerator);
		if(a->denominator)arrfree(a->denominator);
		free(a);
	}
}

void kbField_PrintField(kbField field)
{
	if(field == NULL){PRINT_ERROR("kbField_PrintField Error: field is NULL");}
	if(arrlen(field->finiteField) < 0){PRINT_ERROR("kbField_PrintField Error: field->finiteField is NULL");}
	printf("Smoothness : %d\n", field->smoothnessFactor);
	printf("Bit count goal : %d , Actual range(bits) %ld\n", field->bitCountGoal, mpz_sizeinbase(field->numberRange, 2));
	printf("Prime Number Range : %d to %d\n", field->finiteField[0], field->finiteField[arrlen(field->finiteField)-1]);
	gmp_printf("Range : %Zd\n", field->numberRange);
}

void kb_Print(kb a)
{
	if(a == NULL){PRINT_ERROR("kb_Print Error: kb is NULL");}
	if(a->numerator == NULL){PRINT_ERROR("kb_Print Error: kb->numerator is NULL");}
	if(a->denominator == NULL){PRINT_ERROR("kb_Print Error: kb->denominator is NULL");}
	for(size_t i = 0; i < arrlen(a->denominator); i++)
	{
		printf("(%d / %d)",a->numerator[i], a->denominator[i]);
	}
	printf("\n");
}

double kb_ToDouble(kb a, kbField field)
{
	if(a == NULL){PRINT_ERROR("kb_ToDouble Error: kb is NULL");}
	if(field == NULL){PRINT_ERROR("kb_ToDouble Error: field is NULL");}
	if(a->numerator == NULL){PRINT_ERROR("kb_Print Error: kb->numerator is NULL");}
	if(a->denominator == NULL){PRINT_ERROR("kb_Print Error: kb->denominator is NULL");}
	if(arrlen(field->finiteField) < 0){PRINT_ERROR("kbField_PrintField Error: field->finiteField is NULL");}
	
	double result = 0.0f;
	mpf_t f_num, f_den, f_result;
	mp_bitcnt_t precision = 256;
	mpf_set_default_prec(precision);
	mpf_inits(f_num, f_den, f_result, NULL);	
	mpz_t cachedCrtNum, cachedCrtDen, temporary0;
	mpz_inits(cachedCrtNum, cachedCrtDen, temporary0, NULL);
	for(size_t i = 0; i < arrlen(a->denominator); i++)
	{
		mpz_mul_ui(temporary0, field->cachedCRTConstants[i]->temporary0, a->numerator[i]);
		mpz_add(cachedCrtNum,cachedCrtNum,temporary0);
		mpz_mul_ui(temporary0, field->cachedCRTConstants[i]->temporary0, a->denominator[i]);
		mpz_add(cachedCrtDen,cachedCrtDen,temporary0);
		//gmp_printf("(%d / %d :(%Zd %Zd)%Zd %d)\n",a->numerator[i], a->denominator[i], cachedCrtNum, temporary0, field->cachedCRTConstants[i]->temporary0, field->finiteField[i]);
	}
	mpz_mod(cachedCrtNum, cachedCrtNum, field->numberRange);
	mpz_mod(cachedCrtDen, cachedCrtDen, field->numberRange);
	if(mpz_cmp(cachedCrtNum,field->numberRangeHalfFloor) > 0){mpz_sub(cachedCrtNum, cachedCrtNum, field->numberRange);}
	if(mpz_cmp(cachedCrtDen,field->numberRangeHalfFloor) > 0){mpz_sub(cachedCrtDen, cachedCrtDen, field->numberRange);}
	//if(mpz_sgn(cachedCrtDen) == 0){PRINT_ERROR("kb_ToDouble Error: kb->denominator is 0");}
	mpz_add_ui(cachedCrtDen,cachedCrtDen, 1);
	mpf_set_z(f_num, cachedCrtNum);
	mpf_set_z(f_den, cachedCrtDen);
	mpf_div(f_result, f_num, f_den);
	result = mpf_get_d(f_result);
	gmp_printf("%Zd /%Zd\n", cachedCrtNum, cachedCrtDen);
	mpf_clears(f_num, f_den, f_result, NULL);
	mpz_clears(cachedCrtNum, cachedCrtDen, temporary0, NULL);
	return result;
}

void kb_Reciprocal(kb a)
{
	if(a == NULL){PRINT_ERROR("kb_Reciprocal Error: kb is NULL");}
	if(a->numerator == NULL){PRINT_ERROR("kb_Reciprocal Error: kb->numerator is NULL");}
	if(a->denominator == NULL){PRINT_ERROR("kb_Reciprocal Error: kb->denominator is NULL");}
	int *temp = a->numerator;
	a->numerator = a->denominator;
	a->denominator = temp;
}

void kb_Scale(kb a, size_t scalingFactor, kbField field)
{
	if(a == NULL){PRINT_ERROR("kb_Scale Error: kb is NULL");}
	if(field == NULL){PRINT_ERROR("kb_Scale Error: field is NULL");}
	if(a->numerator == NULL){PRINT_ERROR("kb_Scale Error: kb->numerator is NULL");}
	if(a->denominator == NULL){PRINT_ERROR("kb_Scale Error: kb->denominator is NULL");}
	for(size_t i = 0; i < arrlen(a->numerator) ; i++)
	{
		int primeNumber = field->finiteField[i];
		int scalingFactorModularInverse = ModularExponentiation(scalingFactor, primeNumber - 2, primeNumber);
		
		int numberA = a->numerator[i];
		int numberB = a->denominator[i];
		int numberModScalingFactorA = numberA % scalingFactor;
		int numberModScalingFactorB = numberB % scalingFactor;
		numberA = (numberA - numberModScalingFactorA) < 0 ? primeNumber - (numberA - numberModScalingFactorA): (numberA - numberModScalingFactorA);
		numberB = (numberB - numberModScalingFactorB) < 0 ? primeNumber - (numberB - numberModScalingFactorB): (numberB - numberModScalingFactorB);
		numberA *= scalingFactorModularInverse;
		numberB *= scalingFactorModularInverse;
		a->numerator[i] = numberA % primeNumber;
		a->denominator[i] = numberB % primeNumber;
		printf("%2d %2d (%2d mod %2d)\n", a->numerator[i], a->denominator[i], scalingFactorModularInverse, primeNumber);
	}
}

void TestScale()
{
	size_t smoothnessFactor = 2;
	size_t bitCount = 64;
	double value = 65;
	size_t scalingFactor = 2;
	kbField field = CreateField(bitCount, smoothnessFactor);
	kb a = kb_init(value, field);
	kb_Scale(a, scalingFactor, field);
	double aReverse = kb_ToDouble(a, field);
	
	printf("\nScale Test\n%.13f %.13f\n", floor(value/(float)scalingFactor), aReverse);
	
	kb_clear(a);
	kbField_clear(field);
}

void TestReciprocal()
{
	size_t smoothnessFactor = 2;
	size_t bitCount = 64;
	double value = 3.1406456053;
	kbField field = CreateField(bitCount, smoothnessFactor);
	/*kb a = kb_init(value, field);
	kb_Reciprocal(a);
	double aReverse = kb_ToDouble(a, field);
	
	printf("\nReciprocal Test\n%.13f %.13f\n",1.0f / value, aReverse);
	
	kb_clear(a);*/
	kbField_clear(field);
}

void TestReverse()
{
	size_t smoothnessFactor = 2;
	size_t bitCount = 64;
	double value = 3.1406456053;
	kbField field = CreateField(bitCount, smoothnessFactor);
	kb a = kb_init(value, field);
	double aReverse = kb_ToDouble(a, field);
	kb b = kb_init(-value, field);
	double bReverse = kb_ToDouble(b, field);
	
	printf("\nReverse Test\n%.13f %.13f\n%.13f %.13f\n",value, aReverse, -value, bReverse);
	
	kb_clear(a);
	kbField_clear(field);
}


/*
int main()
{
	TestReverse();
	TestReciprocal();
	return 0;
}*/
