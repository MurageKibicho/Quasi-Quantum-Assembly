#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PI 3.141592653589793

void ComputeChebyshevPolynomials(double x, int degree, double *T)
{
	T[0] = 1;
	T[1] = x;
	for(int n = 2; n <= degree; n++)
	{
		T[n] = 2 * x * T[n - 1] - T[n - 2];
	}
}

// Compute Chebyshev coefficients using least-squares fitting
void ComputeChebyshevCoefficients(int degree, double modulus, double *coeffs)
{
	int sampleCount = 1000;
	for(int n = 0; n <= degree; n++)
	{
		double sum = 0.0;
		for(int k = 0; k < sampleCount; k++)
		{
			double x = cos(PI * (k + 0.5) / sampleCount);  
			double mod_x = fmod((x + 1) * modulus / 2, modulus);
			double Tn = cos(n * acos(x));
			sum += mod_x * Tn;
		}
		coeffs[n] = (2.0 / sampleCount) * sum;
	}
}


double ChebyshevMod(double x, int modulus, int degree, double *coeffs)
{
	double scaledX = (2 * x / modulus) - 1; // Map x to [-1,1]
	double T[degree + 1];
	ComputeChebyshevPolynomials(scaledX, degree, T);

	double result = coeffs[0] / 2;  // First coefficient has half weight
	for(int n = 1; n <= degree; n++)
	{
		result += coeffs[n] * T[n];
	}
	return result;
}

void PrintChebyshevPolynomial(int degree, double modulus, double *coeffs)
{
	printf("\nf(x) ≈ ");
	for(int n = 0; n <= degree; n++)
	{
		if(fabs(coeffs[n]) > 1e-6) 
		{  // Ignore near-zero coefficients
		    printf("%+lf T_%d(x) ", coeffs[n], n);
		}
	}
	printf("\n\nwhere T_n(x) are Chebyshev polynomials.\n");
}

int main() 
{
	int modulus = 70000;
	int degree = 10;  
	double coeffs[degree + 1];


	ComputeChebyshevCoefficients(degree, modulus, coeffs);
	PrintChebyshevPolynomial(degree, modulus, coeffs);
	// Test Chebyshev approximation
	for(int i = 0; i <= 20; i++)
	{
		double x = i * (modulus / 20.0);
		printf("%7.0lf mod %d ≈ %7.0lf (Chebyshev)  %7.0lf (Exact)\n", x, modulus, ChebyshevMod(x, modulus, degree, coeffs), fmod(x, modulus));
	}

	return 0;
}

