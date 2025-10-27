#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpz_factor.h>


typedef struct elliptic_simple_point_struct *EllipticSecp;
struct elliptic_simple_point_struct
{
	fmpz_t x;
	fmpz_t y;
	int infinity;
};

EllipticSecp EllipticSecp_CreatePoint()
{
	EllipticSecp point = malloc(sizeof(struct elliptic_simple_point_struct));
	fmpz_init(point->x);
	fmpz_init(point->y);
	point->infinity = 0;
	return point;
}

void EllipticSecp_CopyPoint(EllipticSecp source, EllipticSecp destination)
{
	fmpz_set(destination->x, source->x);
	fmpz_set(destination->y, source->y);
	destination->infinity = source->infinity;
}

void EllipticSecp_FindPointInverse(EllipticSecp source, EllipticSecp destination, fmpz_t primeNumber)
{
	fmpz_set(destination->x, source->x);
	fmpz_sub(destination->y, primeNumber,source->y);
	fmpz_mod(destination->y,destination->y,primeNumber);
}

int EllipticSecp_TestPointEquality(EllipticSecp a, EllipticSecp b)
{
	if(a->infinity && b->infinity) return 1;
	if(a->infinity || b->infinity) return 0;
	return fmpz_cmp(a->x, b->x) == 0 && fmpz_cmp(a->y, b->y) == 0;	
}

void EllipticSecp_PrintPoint(EllipticSecp point)
{
	fmpz_print(point->x);printf(" ");fmpz_print(point->y);printf("\n");
}

void EllipticSecp_DestroyPoint(EllipticSecp point)
{
	if(point)
	{
		fmpz_clear(point->x);
		fmpz_clear(point->y);
		free(point);
	}
}

bool EllipticSecp_AddCurvePoints(EllipticSecp R, EllipticSecp P, EllipticSecp Q, fmpz_t primeNumber)
{
	//Case 0: Handle Points at infinity
	if(P->infinity != 0){EllipticSecp_CopyPoint(Q,R);return true;}
	if(Q->infinity != 0){EllipticSecp_CopyPoint(P,R);return true;}
	
	//Case 1: Handle P->x == Q->x
	if(fmpz_cmp(P->x, Q->x) == 0) 
	{
		//Case 1.1: X values similar but Y differ or 0
		if(fmpz_cmp(P->y, Q->y) != 0 || fmpz_cmp_ui(P->y, 0) == 0){R->infinity = 1;return true;}
		
		//Case 1.2: Point Doubling
		fmpz_t s, num, den, denominatorInverse, tmp;
		fmpz_init(s);
		fmpz_init(num);
		fmpz_init(den);
		fmpz_init(denominatorInverse);
		fmpz_init(tmp);

		//num = x^2
		fmpz_mul(num, P->x, P->x);
		//num = 3x^2   
		fmpz_mul_ui(num, num, 3);       

		//den = 2y
		fmpz_mul_ui(den, P->y, 2); 
		//Find denominator inverse     
		if(!fmpz_invmod(denominatorInverse, den, primeNumber))
		{
			R->infinity = 1;
			fmpz_clear(s);
			fmpz_clear(num);
			fmpz_clear(den);
			fmpz_clear(denominatorInverse);
			fmpz_clear(tmp);
			return true;
		}

		//s = (3x^2)/(2y)
		fmpz_mul(s, num, denominatorInverse);     
		fmpz_mod(s, s, primeNumber);

		//x3 = s^2 - 2x
		fmpz_mul(tmp, s, s);
		fmpz_sub(tmp, tmp, P->x);
		fmpz_sub(tmp, tmp, Q->x);
		fmpz_mod(R->x, tmp, primeNumber);

		//y3 = s*(x - x3) - y
		fmpz_sub(tmp, P->x, R->x);
		fmpz_mul(tmp, s, tmp);
		fmpz_sub(tmp, tmp, P->y);
		fmpz_mod(R->y, tmp, primeNumber);

		R->infinity = 0;

		fmpz_clear(s);
		fmpz_clear(num);
		fmpz_clear(den);
		fmpz_clear(denominatorInverse);
		fmpz_clear(tmp);
		return true;

	}
	//Case 2: Handle P != Q (Point Addition)
	else
	{
		fmpz_t s, num, den, denominatorInverse, tmp;
		fmpz_init(s);
		fmpz_init(num);
		fmpz_init(den);
		fmpz_init(denominatorInverse);
		fmpz_init(tmp);
		
		//num = y2 - y1
		fmpz_sub(num, Q->y, P->y);
		//den = x2 - x1   
		fmpz_sub(den, Q->x, P->x); 
		//Find inverse of denominator mod p    
		if(!fmpz_invmod(denominatorInverse, den, primeNumber))
		{
			R->infinity = 1;
			fmpz_clear(s);
			fmpz_clear(num);
			fmpz_clear(den);
			fmpz_clear(denominatorInverse);
			fmpz_clear(tmp);
			return true;
		}

		//s = (y2 - y1)/(x2 - x1) mod p
		fmpz_mul(s, num, denominatorInverse);fmpz_mod(s, s, primeNumber);

		//x3 = s^2 - x1 - x2 mod p
		fmpz_mul(tmp, s, s);fmpz_sub(tmp, tmp, P->x);fmpz_sub(tmp, tmp, Q->x);fmpz_mod(R->x, tmp, primeNumber);

		//y3 = s*(x1 - x3) - y1
		fmpz_sub(tmp, P->x, R->x);fmpz_mul(tmp, s, tmp);fmpz_sub(tmp, tmp, P->y);fmpz_mod(R->y, tmp, primeNumber);
		R->infinity = 0;
		//Free memory
		fmpz_clear(s);
		fmpz_clear(num);
		fmpz_clear(den);
		fmpz_clear(denominatorInverse);
		fmpz_clear(tmp);
		return true;
	}
}


void EllipticSecp_MSBScalarMultiplication(EllipticSecp resultant, EllipticSecp generator, fmpz_t privateKey, fmpz_t primeNumber)
{
	//Create pointAtInfinity, temp0
	EllipticSecp pointAtInfinity = EllipticSecp_CreatePoint();
	pointAtInfinity->infinity = 1;  
	
	EllipticSecp temp0 = EllipticSecp_CreatePoint();
	size_t binaryLength = fmpz_sizeinbase(privateKey, 2);
	
	//loop from MSB to LSB
	for(ssize_t i = binaryLength - 1; i >= 0; --i)
	{
		//temp0 = 2*pointAtInfinity
		EllipticSecp_AddCurvePoints(temp0, pointAtInfinity, pointAtInfinity, primeNumber);
		EllipticSecp_CopyPoint(temp0, pointAtInfinity);
		
		//Test the current bit's parity
		if(fmpz_tstbit(privateKey, i) != 0)
		{
			//temp0 = pointAtInfinity + generator
			EllipticSecp_AddCurvePoints(temp0, pointAtInfinity, generator, primeNumber);
			EllipticSecp_CopyPoint(temp0, pointAtInfinity);	
		}
	}
	//Save pointAtInfinity to the result variable
	EllipticSecp_CopyPoint(pointAtInfinity, resultant);

	//Free memory
	EllipticSecp_DestroyPoint(pointAtInfinity);
	EllipticSecp_DestroyPoint(temp0);
}

void EllipticSecp_LSBScalarMultiplication(EllipticSecp resultant, EllipticSecp generator, fmpz_t privateKey, fmpz_t primeNumber)
{
	//Create pointAtInfinity, temp0, temp1
	EllipticSecp pointAtInfinity = EllipticSecp_CreatePoint();
	pointAtInfinity->infinity = 1;  
	
	EllipticSecp temp0 = EllipticSecp_CreatePoint();
	//Save generator to temp0
	EllipticSecp_CopyPoint(generator, temp0); 
	EllipticSecp temp1 = EllipticSecp_CreatePoint();

	size_t binaryLength = fmpz_sizeinbase(privateKey, 2);
	//loop from LSB to MSB
	for(size_t i = 0; i < binaryLength; ++i)
	{
		int bit = fmpz_tstbit(privateKey, i);
		if(bit)
		{
			//temp1 = pointAtInfinity + currentGenerator
			EllipticSecp_AddCurvePoints(temp1, pointAtInfinity, temp0, primeNumber);
			EllipticSecp_CopyPoint(temp1, pointAtInfinity);
		}
		//Update temp0(currentGenerator) = 2*temp0(previousGenerator)
		EllipticSecp_AddCurvePoints(temp1, temp0, temp0, primeNumber);
		EllipticSecp_CopyPoint(temp1, temp0);
	}
	//Save pointAtInfinity to the result variable
	EllipticSecp_CopyPoint(pointAtInfinity, resultant);
	EllipticSecp_DestroyPoint(pointAtInfinity);
	EllipticSecp_DestroyPoint(temp0);
	EllipticSecp_DestroyPoint(temp1);
}

void EllipticSecp_BenchMarkMSB()
{
	int keyCount = 1000;
	char *primeNumberHexadecimal = "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F";	
	char *generatorXHexadecimal  = "79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798";
	char *generatorYHexadecimal  = "483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8";
	
	fmpz_t primeNumber, privateKey;
	fmpz_init(primeNumber);
	fmpz_init(privateKey);
	EllipticSecp generator = EllipticSecp_CreatePoint();
	EllipticSecp result = EllipticSecp_CreatePoint();
	fmpz_set_str(primeNumber, primeNumberHexadecimal, 16);
	fmpz_set_str(generator->x, generatorXHexadecimal, 16);
	fmpz_set_str(generator->y, generatorYHexadecimal, 16);
	
	clock_t start, end;
	double cpuTimeCount;
	printf("Benchmarking MySecp public key generation...\n");
	printf("Iterations: %d\n", keyCount);
	start = clock();
	for(int i = 1; i < keyCount; i++)
	{
		fmpz_set_ui(privateKey, i);
		EllipticSecp_MSBScalarMultiplication(result, generator, privateKey, primeNumber);	
		//EllipticSecp_PrintPoint(result);
	}
	end = clock(); 

	cpuTimeCount = ((double)(end - start)) / CLOCKS_PER_SEC;
	double iterationsPerSecond = keyCount / cpuTimeCount;

	printf("Total time MSB: %.4f seconds\n", cpuTimeCount);
	
	EllipticSecp_DestroyPoint(generator);
	EllipticSecp_DestroyPoint(result);
	fmpz_clear(primeNumber);
	fmpz_clear(privateKey);
}

EllipticSecp *EllipticSecp_CacheDoubles(EllipticSecp generator, fmpz_t primeNumber, int bitCount)
{
	EllipticSecp *doubles = malloc(bitCount * sizeof(EllipticSecp));
	assert(doubles != NULL);

	EllipticSecp temp = EllipticSecp_CreatePoint();
	EllipticSecp temp2 = EllipticSecp_CreatePoint();
	EllipticSecp_CopyPoint(generator, temp);

	for(int i = 0; i < bitCount; i++)
	{
		doubles[i] = EllipticSecp_CreatePoint();
		//Store current power of 2 * G
		EllipticSecp_CopyPoint(temp, doubles[i]); 

		//Compute next power of 2: temp = 2 * temp
	
		EllipticSecp_AddCurvePoints(temp2, temp, temp, primeNumber);
		EllipticSecp_CopyPoint(temp2, temp);
	}
	EllipticSecp_DestroyPoint(temp2);
	EllipticSecp_DestroyPoint(temp);
	return doubles;
}

void EllipticSecp_LSBCachedMultiplication(int bitCount, EllipticSecp *cachedDoubles, EllipticSecp resultant, EllipticSecp generator, fmpz_t privateKey, fmpz_t primeNumber)
{
	//Create pointAtInfinity, temp0, temp1
	EllipticSecp pointAtInfinity = EllipticSecp_CreatePoint();
	pointAtInfinity->infinity = 1;  
	
	EllipticSecp temp0 = EllipticSecp_CreatePoint();
	//Save generator to temp0
	EllipticSecp_CopyPoint(generator, temp0); 
	EllipticSecp temp1 = EllipticSecp_CreatePoint();

	size_t binaryLength = fmpz_sizeinbase(privateKey, 2);
	//loop from LSB to MSB
	for(size_t i = 0; i < binaryLength; ++i)
	{
		int bit = fmpz_tstbit(privateKey, i);
		if(bit)
		{
			//temp1 = pointAtInfinity + currentGenerator
			EllipticSecp_AddCurvePoints(temp1, pointAtInfinity, cachedDoubles[i], primeNumber);
			EllipticSecp_CopyPoint(temp1, pointAtInfinity);
		}
		
	}
	//Save pointAtInfinity to the result variable
	EllipticSecp_CopyPoint(pointAtInfinity, resultant);
	EllipticSecp_DestroyPoint(pointAtInfinity);
	EllipticSecp_DestroyPoint(temp0);
	EllipticSecp_DestroyPoint(temp1);
}

bool EllipticSecp_FindRandomSecpPoint(EllipticSecp result, fmpz_t primeNumber, flint_rand_t randomState)
{
	fmpz_t x, y, rhs, lhs, tmp;
	fmpz_init(x);
	fmpz_init(y);
	fmpz_init(rhs);
	fmpz_init(lhs);
	fmpz_init(tmp);

	bool found = false;
	int attempts = 0;
	int maxAttempts = 1000;

	while(!found && attempts < maxAttempts)
	{
		attempts++;
		//Generate random x coordinate
		fmpz_randm(x, randomState, primeNumber);

		// Compute right side: x^3 + 7 mod primeNumber
		fmpz_powm_ui(rhs, x, 3, primeNumber);      // rhs = x^3 mod p
		fmpz_add_ui(rhs, rhs, 7);        // rhs = x^3 + 7
		fmpz_mod(rhs, rhs, primeNumber);           // rhs = (x^3 + 7) mod p

		// Check if rhs is a quadratic residue mod p
		int legendre = fmpz_jacobi(rhs, primeNumber);

		if(legendre == 1) 
		{
			if(fmpz_sqrtmod(y, rhs, primeNumber))
			{
				fmpz_set(result->x, x);
				fmpz_set(result->y, y);
				found = true;
			}
		}
	}
	fmpz_clear(x);
	fmpz_clear(y);
	fmpz_clear(rhs);
	fmpz_clear(lhs);
	fmpz_clear(tmp);

	return found;
}

bool EllipticSecp_BruteforcePointOrder(EllipticSecp generator, fmpz_t primeNumber, fmpz_t pointOrder)
{
	int bitCount = 32;
	bool foundPointOrder = false;
	assert(generator->infinity != 1);
	assert(fmpz_sizeinbase(primeNumber, 2) < bitCount);
	EllipticSecp *cachedDoubles = EllipticSecp_CacheDoubles(generator, primeNumber, bitCount);
	fmpz_t privateKey;
	EllipticSecp result0 = EllipticSecp_CreatePoint();
	fmpz_init(privateKey);
	for(int i = 1; i < fmpz_get_ui(primeNumber); i++)
	{
		fmpz_set_ui(privateKey, i);
		EllipticSecp_LSBCachedMultiplication(bitCount, cachedDoubles, result0, generator, privateKey, primeNumber);
		if(result0->infinity == 1)
		{
			fmpz_set_ui(pointOrder, i);
			foundPointOrder = true;
			break;
		}
	}
	for(int i = 0; i < bitCount; i++){EllipticSecp_DestroyPoint(cachedDoubles[i]);}free(cachedDoubles);
	fmpz_clear(privateKey);
	EllipticSecp_DestroyPoint(result0);
	return foundPointOrder;
}

bool EllipticSecp_RaiseGeneratorTest(fmpz_t testPrime, fmpz_t pointOrder, fmpz_t pointOrderMinusOne, fmpz_factor_t factorization)
{
	bool result = false;
	fmpz_t testResult, exponent;
	fmpz_init(testResult);
	fmpz_init(exponent);
	for(slong i = 0; i < factorization->num; i++)
	{
		fmpz_mod(exponent, pointOrderMinusOne, &factorization->p[i]);
		assert(fmpz_cmp_ui(exponent, 0 ) == 0);
		fmpz_divexact(exponent, pointOrderMinusOne, &factorization->p[i]);
		fmpz_powm(testResult, testPrime, exponent, pointOrder);
		if(fmpz_cmp_ui(testResult, 1) == 0)
		{
			result = false;
			return result;
		}
	}
	
	fmpz_clear(exponent);
	fmpz_clear(testResult);
	result = true;
	return result;
}

void EllipticSecp_BenchMarkLSB()
{
	int keyCount = 100000;
	int bitCount = 20;
	char *primeNumberHexadecimal = "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F";	
	char *generatorXHexadecimal  = "79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798";
	char *generatorYHexadecimal  = "483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8";
	
	fmpz_t primeNumber, privateKey;
	fmpz_init(primeNumber);
	fmpz_init(privateKey);
	EllipticSecp generator = EllipticSecp_CreatePoint();
	EllipticSecp result0 = EllipticSecp_CreatePoint();
	EllipticSecp result1 = EllipticSecp_CreatePoint();
	fmpz_set_str(primeNumber, primeNumberHexadecimal, 16);
	fmpz_set_str(generator->x, generatorXHexadecimal, 16);
	fmpz_set_str(generator->y, generatorYHexadecimal, 16);
	
	EllipticSecp *cachedDoubles = EllipticSecp_CacheDoubles(generator, primeNumber, bitCount);
	clock_t start, end;
	double cpuTimeCount;
	printf("Benchmarking MySecp public key generation...\n");
	printf("Iterations: %d\n", keyCount);
	start = clock();
	for(int i = 1; i < keyCount; i++)
	{
		fmpz_set_ui(privateKey, i);
		EllipticSecp_LSBCachedMultiplication(bitCount, cachedDoubles, result1, generator, privateKey, primeNumber);
	}
	end = clock(); 

	cpuTimeCount = ((double)(end - start)) / CLOCKS_PER_SEC;
	double iterationsPerSecond = keyCount / cpuTimeCount;

	printf("Total time MSB: %.4f seconds\n", cpuTimeCount);
	
	EllipticSecp_DestroyPoint(generator);
	EllipticSecp_DestroyPoint(result0);
	EllipticSecp_DestroyPoint(result1);
	fmpz_clear(primeNumber);
	fmpz_clear(privateKey);
	for(int i = 0; i < bitCount; i++){EllipticSecp_DestroyPoint(cachedDoubles[i]);}free(cachedDoubles);
}



