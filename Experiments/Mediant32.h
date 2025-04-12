#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#define m32_ENABLE_ERRORS 1
#define m32_TRUE 1
#define m32_FALSE 0
#define PRINT_ERROR(msg) \
do { \
if (m32_ENABLE_ERRORS) { \
    fprintf(stderr, "\033[31mERROR: %s\033[0m\n", msg); \
    exit(1); \
} \
} while (0)
typedef struct SternBrocotFraction_rational_struct *m32;
struct SternBrocotFraction_rational_struct
{
	int numerator;
	int denominator;
};

m32 m32_Init()
{
	m32 result = malloc(sizeof(struct SternBrocotFraction_rational_struct));	
	result->numerator = 0;
	result->denominator = 0;
	return result;
}

void m32_Clear(m32 a)
{
	if(a)
	{
		free(a);
	}
}
void m32_Multiply(m32 result, m32 left, m32 right)
{
	result->numerator = left->numerator * right->numerator;
	result->denominator = left->denominator * right->denominator;
}

void m32_Divide(m32 result, m32 left, m32 right)
{
	result->numerator = left->numerator * right->denominator;
	result->denominator = left->denominator * right->numerator;
}

void m32_Add(m32 result, m32 left, m32 right)
{
	result->numerator = left->numerator + right->numerator;
	result->denominator = left->denominator + right->denominator;
}

void m32_Subtract(m32 result, m32 left, m32 right)
{
	result->numerator = left->numerator + (-1 * right->numerator);
	result->denominator = left->denominator + right->denominator;
}

void m32_MulInverse(m32 result)
{
	int temp = result->numerator;
	result->numerator = result->denominator;
	result->denominator = temp;
}

void m32_AddInverse(m32 result)
{
	result->numerator = result->numerator * -1;
}

void m32_IntegerExponent(int n, m32 result)
{
	int temporaryNumerator = result->numerator;
	int temporaryDenominator = result->denominator;
	for(int i = 0; i < n-1; i++)
	{
		result->numerator = result->numerator * temporaryNumerator;
		result->denominator = result->denominator * temporaryDenominator;	
	}
}

void m32_IntegerNthRoot(int n, m32 result)
{
	//Choose how many steps you want to take
	int ITERATIONS = 3;
	/*Set a and b values*/
	int a = result->numerator;
	int b = result->denominator;
	/*Start recurrence with a guess of 1*/
	m32 yValue = m32_Init();
	yValue->numerator = 1;
	yValue->denominator = 1;
	
	/*Create temporary yValues*/
	m32 yValuePowN = m32_Init();
	m32 yValuePowNMinus1 = m32_Init();
	for(int i = 0; i < ITERATIONS; i++)
	{
		yValuePowN->numerator = yValue->numerator;
		yValuePowN->denominator = yValue->denominator;
		m32_IntegerExponent(n, yValuePowN);

		// y^{n-1}
		yValuePowNMinus1->numerator = yValue->numerator;
		yValuePowNMinus1->denominator = yValue->denominator;
		m32_IntegerExponent(n-1, yValuePowNMinus1);

		// numerator = b*(n-1)*y^n * y^{n-1}.den + a * y^{n-1}.den * y^n.den
		int term1 = b * (n - 1) * yValuePowN->numerator * yValuePowNMinus1->denominator;
		int term2 = a * yValuePowNMinus1->denominator * yValuePowN->denominator;
		int new_numerator = term1 + term2;

		int new_denominator = b * n * yValuePowNMinus1->numerator * yValuePowNMinus1->denominator * yValuePowN->denominator;
		//printf("%d %d\n", new_numerator, new_denominator);
		yValue->numerator = new_numerator;
		yValue->denominator = new_denominator;
	}
	result->numerator = yValue->numerator;
	result->denominator = yValue->denominator;
	m32_Clear(yValue);
	m32_Clear(yValuePowN);
	m32_Clear(yValuePowNMinus1);
}
//Check if (a*b + c) overflows int32_t
int CheckAddMulOverflow(int32_t a, int32_t b, int32_t c) 
{
	if(a == 0 || b == 0){return 1;}
	if(a > 0){if(b > 0){if(a > (INT32_MAX - c) / b) return 0;}else{if(b < (INT32_MIN - c) / a) return 0;}}
	else{if(b > 0){if(a < (INT32_MIN - c) / b) return 0;}else{if(b < (INT32_MAX - c) / a) return 0;}}
	return 1;
}
m32 m32_InitFloat(float number)
{
	if(isnan(number) || isinf(number)){PRINT_ERROR("m32_InitFloat Error: number is NaN or Infinity");}	
	m32 result = malloc(sizeof(struct SternBrocotFraction_rational_struct));
	result->numerator = 0;
	result->denominator = 0;
	float remainder = number;
	int sign = (number < 0) ? -1 : 1;
	remainder = fabs(number);
	int32_t a, h[3] = {0, 1, 0}, k[3] = {1, 0, 0};
	for(int i = 0; i < 100; i++)//Prevent infinite loops
	{
		a = (int32_t)remainder;
		remainder = 1.0 / (remainder - a);
		if(!CheckAddMulOverflow(a, h[1], h[0]) || !CheckAddMulOverflow(a, k[1], k[0])){break;}
		h[2] = a * h[1] + h[0];
		k[2] = a * k[1] + k[0];
		if (k[2] < 0) break;
		h[0] = h[1]; h[1] = h[2];
       	k[0] = k[1]; k[1] = k[2];
       	if(remainder == 0) break;
	}
	
	result->numerator = h[1] * sign;
	result->denominator = k[1];
	//printf("|%d /%d|\n", h[1], k[1]);
	return result;	
}

void m32_Print(m32 a)
{
	printf("(%d / %d)\n", a->numerator, a->denominator);
}


void TestFloatInit()
{
	m32 a = m32_InitFloat(467.97757443);
	m32_Print(a);
	m32_Clear(a);
}
/*
int main()
{
	return 0;
}*/
