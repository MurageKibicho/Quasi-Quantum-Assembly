#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <gmp.h>

double Log(double number, double base)
{
	return log2(number) / log2(base);
}

int *FindBaseExpansion(double number, double base, int *length)
{
	*length = floor(Log(number, base))+1;
	int currentIndex = *length -1 ;
	int *result = calloc(*length, sizeof(int));
	int polynomialDegree = 0;
	int sum = 0;
	while(currentIndex > 0)
	{
		int numberInBase = (int)fmod(number, base);
		result[currentIndex] = numberInBase;
		polynomialDegree += 1;
		number /= base;
		currentIndex -= 1;	
	}
	return result;
}

/*
We want to solve for x and k in : 2 log_n (x) - log_n (k) = log_n(smoothNumber) + 1
eg, when n = 84923, 2 log_n (x) - log_n (k) = log_n(8400) + 1
*/

void LogSearch(double n, double target)
{
	double x = 0.0f;
	double k = 0.0f;
	int expansionLength = 0;
	double targetLog = Log(target, n);
	int *targetDigits = FindBaseExpansion(target, n, &expansionLength);
	
	printf("%.3f\n",n);
	
	free(targetDigits);
}
int main()
{
	double number = 84923.0f;
	double base = 84923.0f;
	double logValue = Log(number, base);
	double target = 8400.0f;
	printf("%.3f %.3f\n", 2*Log(513, base)-Log(3, base)-1, Log(8400, base));
	//LogSearch(base, target);
	return 0;
}
