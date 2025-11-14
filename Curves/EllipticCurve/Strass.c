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
//clear && gcc Strass.c -lm -lgmp -lmpfr -lflint -o m.o && ./m.o
typedef struct elliptic_strass_curve_struct *EllipticStrassCurve;
typedef struct elliptic_strass_point_struct *EllipticStrassPoint;
struct elliptic_strass_point_struct 
{
	fmpz_t x;
	fmpz_t y;
	fmpz_t z;
	bool infinity;
};

struct elliptic_strass_curve_struct 
{
	EllipticStrassPoint generator;
	fmpz_t a1;
	fmpz_t a2;
	fmpz_t a3;
	fmpz_t a4;
	fmpz_t a5;
	fmpz_t a6;
	
	fmpz_t b2;
	fmpz_t b4;
	fmpz_t b6;
	fmpz_t b8;
	fmpz_t fieldCharacteristic;
	fmpz_t pointOrder;
	bool affineCoordinates;
};

EllipticStrassPoint EllipticStrassCurve_NewPoint()
{
	EllipticStrassPoint point = malloc(sizeof(struct elliptic_strass_point_struct));
	fmpz_init(point->x);
	fmpz_init(point->y);
	fmpz_init(point->z);
	point->infinity = true;
	return point;
}

void EllipticStrassCurve_ClearPoint(EllipticStrassPoint point)
{
	fmpz_clear(point->x);
	fmpz_clear(point->y);
	fmpz_clear(point->z);
	free(point);
}

EllipticStrassCurve EllipticStrassCurve_AllocateCurve()
{
	EllipticStrassCurve curve = malloc(sizeof(struct elliptic_strass_curve_struct));
	curve->generator = EllipticStrassCurve_NewPoint();
	fmpz_init(curve->a1);
	fmpz_init(curve->a2);
	fmpz_init(curve->a3);
	fmpz_init(curve->a4);
	fmpz_init(curve->a5);
	fmpz_init(curve->a6);
	
	fmpz_init(curve->b2);
	fmpz_init(curve->b4);
	fmpz_init(curve->b6);
	fmpz_init(curve->b8);
	fmpz_init(curve->fieldCharacteristic);
	fmpz_init(curve->pointOrder);
	curve->affineCoordinates = true;
	return curve;
}

void EllipticStrassCurve_ClearCurve(EllipticStrassCurve curve)
{
	EllipticStrassCurve_ClearPoint(curve->generator);
	fmpz_clear(curve->a1);
	fmpz_clear(curve->a2);
	fmpz_clear(curve->a3);
	fmpz_clear(curve->a4);
	fmpz_clear(curve->a5);
	fmpz_clear(curve->a6);
	
	fmpz_clear(curve->b2);
	fmpz_clear(curve->b4);
	fmpz_clear(curve->b6);
	fmpz_clear(curve->b8);
	fmpz_clear(curve->fieldCharacteristic);
	fmpz_clear(curve->pointOrder);
	free(curve);
}

void EllipticStrassCurve_FindPointInverse(EllipticStrassPoint result, EllipticStrassPoint point, EllipticStrassCurve curve)
{
	if(point->infinity)
	{
		result->infinity = true;
	}
	else
	{
		result->infinity = false;
		//Set x and z similar
		fmpz_set(result->x, point->x);
		fmpz_set(result->z, point->z);
		
		//yr = −(y+a1x1+a3)
		fmpz_mul(result->y, curve->a1, point->x);
		fmpz_add(result->y, result->y, curve->a3);
		fmpz_sub(result->y, result->y, point->y);
		fmpz_mul_si(result->y, result->y, -1);
	}
}

void EllipticStrassPoint_CopyPoint(EllipticStrassPoint source, EllipticStrassPoint destination)
{
	fmpz_set(destination->x, source->x);
	fmpz_set(destination->y, source->y);
	fmpz_set(destination->z, source->z);
	destination->infinity = source->infinity;
}

int EllipticStrassPoint_TestPointEquality(EllipticStrassPoint a, EllipticStrassPoint b)
{
	if(a->infinity && b->infinity) return 1;
	if(a->infinity || b->infinity) return 0;
	return fmpz_cmp(a->x, b->x) == 0 && fmpz_cmp(a->y, b->y) == 0 && fmpz_cmp(a->z, b->z) == 0;	
}

bool EllipticStrassPoint_AddCurvePoints(EllipticStrassPoint R, EllipticStrassPoint P, EllipticStrassPoint Q, EllipticStrassCurve curve)
{
	//Case 0: Handle Points at infinity
	if(P->infinity != 0){EllipticStrassPoint_CopyPoint(Q,R);return true;}
	if(Q->infinity != 0){EllipticStrassPoint_CopyPoint(P,R);return true;}
	
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
		fmpz_init(mu);
		fmpz_init(denominatorInverse);
		fmpz_init(tmp);
		
		//x2
		fmpz_mul(tmp, P->x, P->x);
		//3x2
		fmpz_mul_ui(num, tmp, 3);
		//a2x1
		fmpz_mul(tmp, curve->a2, P->x);
		//2*a2x1
		fmpz_mul_ui(tmp, tmp, 2);
		fmpz_add(num, num, tmp);
		fmpz_add(num, num, curve->a4);
		//a1y1
		fmpz_mul(tmp, curve->a1, P->y1);
		fmpz_add(num, num, tmp);
		
		//a1x1
		fmpz_mul(tmp, curve->a1, P->x1);
		fmpz_mul_ui(den, P->y1, 2);
		fmpz_add(den, den, tmp);
		fmpz_add(den, den, curve->a3);
		
		if(!fmpz_invmod(den, den, curve->fieldCharacteristic))
		{
			R->infinity = 1;
			fmpz_clear(s);
			fmpz_clear(num);
			fmpz_clear(den);
			fmpz_clear(denominatorInverse);
			fmpz_clear(tmp);
			return true;
		}
		/*//Find gradient and store in num
		fmpz_mul(num, num, den);
		fmpz_mul(R->x, num, num);
		fmpz_mul(tmp, curve->a1, num);
		fmpz_add(R->x, tmp, tmp);
		fmpz_sub(R->x, R->x, curve->a2);
		fmpz_sub(R->x, R->x, curve->a2);
		fmpz_sub(R->x, R->x, curve->a2);*/


	

		R->infinity = 0;
		fmpz_clear(mu);
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


void TestPrimeFieldCurve()
{
	EllipticStrassCurve primeField = EllipticStrassCurve_AllocateCurve();
	
	EllipticStrassCurve_ClearCurve(primeField);
}


int main()
{
	TestIntegerRingCurve();
	flint_cleanup();
	return 0;
}
