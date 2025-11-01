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

typedef struct elliptic_secp_curve_struct *EllipticSecpCurve;
typedef struct elliptic_secp_point_struct *EllipticSecp;
#ifdef SECP_HAS_EXPONENT
struct elliptic_secp_point_struct
{
	fmpz_t x;
	fmpz_t y;
	fmpz_t exponent;
	fmpz_t privateKey;
	int infinity;
};
#else
struct elliptic_secp_point_struct
{
	fmpz_t x;
	fmpz_t y;
	int infinity;
};
#endif

int mersenneExponents [] = {2, 3, 5, 7, 13, 17, 19, 31, 61, 89, 107, 127, 521, 607, 1279, 2203, 2281, 3217, 4253, 4423, 9689, 9941, 11213, 19937, 21701, 23209, 44497, 86243, 110503, 132049, 216091, 756839, 859433, 1257787, 1398269, 2976221, 3021377, 6972593, 13466917, 20996011, 24036583, 25964951, 30402457, 32582657, 37156667, 42643801, 43112609, 57885161, 74207281, 77232917, 82589933};
int mersenneLength = sizeof(mersenneExponents) / sizeof(int);

struct elliptic_secp_curve_struct
{
	int mersenneExponent;
	int bitCount;
	EllipticSecp generator;
	EllipticSecp result0;
	EllipticSecp temp0;
	EllipticSecp temp1;
	EllipticSecp *cachedDoubles;
	flint_rand_t randomState;
	fmpz_factor_t factorization;
	fmpz_t pointOrder;
	fmpz_t pointOrderMinusOne;
	fmpz_t testPrime;
	fmpz_t primeNumber;
	fmpz_t primeNumberMinusOne;
	fmpz_t primitiveRoot;
};
EllipticSecp EllipticSecp_CreatePoint()
{
	EllipticSecp point = malloc(sizeof(struct elliptic_secp_point_struct));
	fmpz_init(point->x);
	fmpz_init(point->y);
	point->infinity = 0;
	#ifdef SECP_HAS_EXPONENT
		fmpz_init(point->exponent);
		fmpz_init(point->privateKey);
	#endif
	return point;
}

void EllipticSecp_CopyPoint(EllipticSecp source, EllipticSecp destination)
{
	fmpz_set(destination->x, source->x);
	fmpz_set(destination->y, source->y);
	destination->infinity = source->infinity;
	#ifdef SECP_HAS_EXPONENT
		fmpz_set(destination->exponent, source->exponent);
		fmpz_set(destination->privateKey, source->privateKey);
	#endif
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
	fmpz_print(point->x);printf(" ");fmpz_print(point->y);
	#ifdef SECP_HAS_EXPONENT
		printf(" e: ");fmpz_print(point->exponent);printf(" key: ");fmpz_print(point->privateKey);
	#endif
	printf("\n");
}
void EllipticSecp_PrintPointTab(EllipticSecp point)
{
	printf("(");fmpz_print(point->x);printf(",");fmpz_print(point->y);printf(") ");
}

void EllipticSecp_DestroyPoint(EllipticSecp point)
{
	if(point)
	{
		fmpz_clear(point->x);
		fmpz_clear(point->y);
		#ifdef SECP_HAS_EXPONENT
			fmpz_clear(point->exponent);
			fmpz_clear(point->privateKey);
		#endif
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

bool EllipticSecp_IsValidSecpPoint(fmpz_t x, fmpz_t y, fmpz_t p)
{
	fmpz_t lhs, rhs, x_cubed,b;
	fmpz_init(lhs);
	fmpz_init(rhs);
	fmpz_init(x_cubed);
	fmpz_init(b);
	fmpz_set_ui(b, 7);
	//Compute y^2 mod p
	fmpz_powm_ui(lhs, y, 2, p);

	//Compute x^3 mod p
	fmpz_powm_ui(x_cubed, x, 3, p);

	//Compute x^3 + b mod p
	fmpz_add(rhs, x_cubed, b);
	fmpz_mod(rhs, rhs, p);

	//Compare lhs and rhs
	bool result = (fmpz_cmp(lhs, rhs) == 0);

	//Clean up
	fmpz_clear(b);
	fmpz_clear(lhs);
	fmpz_clear(rhs);
	fmpz_clear(x_cubed);
	return result;
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
void EllipticSecp_FindNextMersennePoint(int mersenneExponent, EllipticSecp current, EllipticSecp temp0, EllipticSecp temp1, fmpz_t primeNumber)
{
	//Double previous point p times
	EllipticSecp_CopyPoint(current, temp0);
	for(int p = 0; p < mersenneExponent; p++)
	{
		EllipticSecp_AddCurvePoints(temp1, temp0, temp0, primeNumber);
		EllipticSecp_CopyPoint(temp1, temp0);
	}
	//Find Previous point's inverse
	EllipticSecp_FindPointInverse(current, temp1, primeNumber);
	//Add doubled point to inverse
	EllipticSecp_AddCurvePoints(current, temp0, temp1, primeNumber);
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

EllipticSecpCurve EllipticSecpCurve_Allocate()
{
	EllipticSecpCurve curve = malloc(sizeof(struct elliptic_secp_curve_struct));
	curve->generator = EllipticSecp_CreatePoint();
	curve->result0 = EllipticSecp_CreatePoint();
	curve->temp0 = EllipticSecp_CreatePoint();
	curve->temp1 = EllipticSecp_CreatePoint();
	curve->cachedDoubles = NULL;
	fmpz_init(curve->pointOrderMinusOne);
	fmpz_init(curve->pointOrder);
	fmpz_init(curve->primeNumber);
	fmpz_init(curve->primitiveRoot);
	fmpz_init(curve->testPrime);
	fmpz_init(curve->primeNumberMinusOne);
	fmpz_factor_init(curve->factorization);
	flint_rand_init(curve->randomState);
	return curve;
}

EllipticSecpCurve EllipticSecpCurve_CreateSmallCurve(int primeNumberHolder, int groupOrderHolder, int bitCount)
{
	assert(primeNumberHolder % 3 == 1);
	EllipticSecpCurve curve = EllipticSecpCurve_Allocate();
	fmpz_set_ui(curve->primeNumber, primeNumberHolder);
	fmpz_sub_ui(curve->primeNumberMinusOne, curve->primeNumber, 1);

	bool foundPrimePointOrder = false;
	int attempts = 1000;
	while(foundPrimePointOrder == false)
	{
		bool foundSecpPoint = EllipticSecp_FindRandomSecpPoint(curve->generator, curve->primeNumber, curve->randomState);
		assert(foundSecpPoint);
		bool foundPointOrder= EllipticSecp_BruteforcePointOrder(curve->generator, curve->primeNumber,curve->pointOrder);
		assert(foundPointOrder);
		if(fmpz_cmp_ui(curve->pointOrder, groupOrderHolder) == 0)
		{
			foundPrimePointOrder = true;
			break;
		}
		attempts -= 1;
		if(attempts == 0){break;}
	}
	assert(foundPrimePointOrder == true);
	fmpz_set_ui(curve->pointOrder, groupOrderHolder);
	fmpz_sub_ui(curve->pointOrderMinusOne, curve->pointOrder, 1);
	fmpz_factor(curve->factorization, curve->pointOrderMinusOne);
	
	//Check Mersenne Exponents
	int mersenneFoundCount = 0;
	for(int i = 0; i < mersenneLength; i++)
	{
		fmpz_one(curve->testPrime);
		fmpz_mul_2exp(curve->testPrime, curve->testPrime, mersenneExponents[i]); 
		fmpz_sub_ui(curve->testPrime, curve->testPrime, 1);
		if(fmpz_cmp(curve->testPrime, curve->primeNumber) > 0){break;}
		bool testResult = EllipticSecp_RaiseGeneratorTest(curve->testPrime, curve->pointOrder, curve->pointOrderMinusOne, curve->factorization);
		if(testResult == true)
		{
			curve->mersenneExponent = mersenneExponents[i];
			fmpz_set(curve->primitiveRoot, curve->testPrime);
		}
	}
	curve->bitCount = bitCount;
	curve->cachedDoubles = EllipticSecp_CacheDoubles(curve->generator, curve->primeNumber, curve->bitCount);
	return curve;
}


void EllipticSecp_DestroyCurve(EllipticSecpCurve curve)
{
	if(curve)
	{
		EllipticSecp_DestroyPoint(curve->generator);
		EllipticSecp_DestroyPoint(curve->result0);
		EllipticSecp_DestroyPoint(curve->temp0);
		EllipticSecp_DestroyPoint(curve->temp1);
		for(int i = 0; i < curve->bitCount; i++){EllipticSecp_DestroyPoint(curve->cachedDoubles[i]);}free(curve->cachedDoubles);
		flint_rand_clear(curve->randomState);
		fmpz_factor_clear(curve->factorization);
		fmpz_clear(curve->pointOrder);
		fmpz_clear(curve->primitiveRoot);
		fmpz_clear(curve->pointOrderMinusOne);
		fmpz_clear(curve->primeNumber);
		fmpz_clear(curve->primeNumberMinusOne);
		fmpz_clear(curve->testPrime);
		free(curve);
	}
}


void EllipticSecp_PrintCurve(EllipticSecpCurve curve)
{
	if(curve)
	{
		printf("Prime Number: ");fmpz_print(curve->primeNumber);
		printf("\nGenerator   :(");fmpz_print(curve->generator->x);printf(", ");fmpz_print(curve->generator->y);printf(")");	
		printf("\nGenerator order: ");fmpz_print(curve->pointOrder);
		printf("\nMersenne: %d ", curve->mersenneExponent);fmpz_print(curve->primitiveRoot);
		printf("\n");
	}
}

EllipticSecp *EllipticSecp_GenerateAllPoints(EllipticSecpCurve curve)
{
	int pointCountLog = fmpz_sizeinbase(curve->pointOrder, 2);
	assert(pointCountLog < 20);
	int totalPoints = fmpz_get_ui(curve->pointOrder);
	EllipticSecp *points = malloc(totalPoints * sizeof(EllipticSecp));
	
	//Add Generator
	points[0] = EllipticSecp_CreatePoint();
	EllipticSecp_CopyPoint(curve->generator, points[0]);
	bool secpTest = EllipticSecp_IsValidSecpPoint(points[0]->x, points[0]->y, curve->primeNumber);
	assert(secpTest);
	#ifdef SECP_HAS_EXPONENT
		fmpz_set_ui(points[0]->exponent, 1);
		fmpz_powm(points[0]->privateKey,curve->primitiveRoot, points[0]->exponent, curve->pointOrder);
	#endif
	//Add first point
	points[1] = EllipticSecp_CreatePoint();
	EllipticSecp_LSBCachedMultiplication(curve->bitCount, curve->cachedDoubles, points[1], curve->generator, curve->primitiveRoot, curve->primeNumber);
	secpTest = EllipticSecp_IsValidSecpPoint(points[1]->x, points[1]->y, curve->primeNumber);
	assert(secpTest);
	#ifdef SECP_HAS_EXPONENT
		fmpz_set_ui(points[1]->exponent, 2);
		fmpz_powm(points[1]->privateKey,curve->primitiveRoot, points[1]->exponent, curve->pointOrder);
	#endif
	//Find next points
	for(int i = 2; i < totalPoints; i++)
	{
		points[i] = EllipticSecp_CreatePoint();
		EllipticSecp_CopyPoint(points[i-1], points[i]);
		EllipticSecp_FindNextMersennePoint(curve->mersenneExponent, points[i], curve->temp0, curve->temp1, curve->primeNumber);
		secpTest = EllipticSecp_IsValidSecpPoint(points[i]->x, points[i]->y, curve->primeNumber);
		assert(secpTest);
		#ifdef SECP_HAS_EXPONENT
			fmpz_set_ui(points[i]->exponent, i+1);
			fmpz_powm(points[i]->privateKey,curve->primitiveRoot, points[i]->exponent, curve->pointOrder);
		#endif
	}
	return points;
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



