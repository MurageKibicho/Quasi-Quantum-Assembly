#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpz_factor.h>

typedef struct pell_curve_struct *PellCurve;
struct pell_curve_struct
{
	fmpz_t D;
	fmpz_t n;
	fmpz_t groupOrder;
	fmpz_t prime;
	fmpz_t primePlusOne;
	fmpz_t primeMinusOne;
	fmpz_t temp0;
	fmpz_t temp1;
	fmpz_t temp2;
	fmpz_factor_t factorizationPlusOne;
	fmpz_factor_t factorizationMinusOne;
	int legendreSymbolD;
	int legendreSymbolN;
};

typedef struct pell_point_struct *PellPoint;
struct pell_point_struct
{
	fmpz_t x;
	fmpz_t y;
};

PellCurve PellCurve_CreateCurveAuto(fmpz_t prime, fmpz_t D, fmpz_t n)
{	
	PellCurve pellCurve = malloc(sizeof(struct pell_curve_struct));
	fmpz_init(pellCurve->D);
	fmpz_init(pellCurve->n);
	fmpz_init(pellCurve->groupOrder);
	fmpz_init(pellCurve->prime);
	fmpz_init(pellCurve->primePlusOne);
	fmpz_init(pellCurve->primeMinusOne);
	fmpz_init(pellCurve->temp0);
	fmpz_init(pellCurve->temp1);
	fmpz_init(pellCurve->temp2);
	fmpz_factor_init(pellCurve->factorizationPlusOne);
	fmpz_factor_init(pellCurve->factorizationMinusOne);
	
	fmpz_set(pellCurve->D, D);
	fmpz_set(pellCurve->n, n);
	fmpz_set(pellCurve->prime, prime);
	fmpz_sub_ui(pellCurve->primeMinusOne, pellCurve->prime, 1);
	fmpz_add_ui(pellCurve->primePlusOne, pellCurve->prime, 1);
	fmpz_factor(pellCurve->factorizationMinusOne, pellCurve->primeMinusOne);
	fmpz_factor(pellCurve->factorizationPlusOne, pellCurve->primePlusOne);
	pellCurve->legendreSymbolD = fmpz_jacobi(D, prime);
	pellCurve->legendreSymbolN = fmpz_jacobi(n, prime);
	if(pellCurve->legendreSymbolD == 0)
	{
		fmpz_set_ui(pellCurve->groupOrder, 0);
		if(pellCurve->legendreSymbolN == 1 && fmpz_cmp_ui(n, 0) != 0)
		{
			fmpz_set_ui(pellCurve->groupOrder, 2);
		}

	}
	else if(pellCurve->legendreSymbolD == 1)
	{
		fmpz_set(pellCurve->groupOrder, pellCurve->primeMinusOne);
		if(fmpz_cmp_ui(n,0) == 0)
		{	
			//Order becomes 2*prime - 1
			fmpz_mul_ui(pellCurve->groupOrder, pellCurve->prime, 2);	
			fmpz_sub_ui(pellCurve->groupOrder, pellCurve->groupOrder, 1);
			fmpz_factor(pellCurve->factorizationMinusOne, pellCurve->primeMinusOne);	
		}
	}
	else
	{	
		fmpz_set(pellCurve->groupOrder, pellCurve->primePlusOne);
	}
	return pellCurve;
}

void PellCurve_Clear(PellCurve pellCurve)
{
	if(pellCurve)
	{
		fmpz_clear(pellCurve->D);
		fmpz_clear(pellCurve->n);
		fmpz_clear(pellCurve->groupOrder);
		fmpz_clear(pellCurve->prime);
		fmpz_clear(pellCurve->primePlusOne);
		fmpz_clear(pellCurve->primeMinusOne);
		fmpz_clear(pellCurve->temp0);
		fmpz_clear(pellCurve->temp1);
		fmpz_clear(pellCurve->temp2);
		fmpz_factor_clear(pellCurve->factorizationPlusOne);
		fmpz_factor_clear(pellCurve->factorizationMinusOne);
	
		free(pellCurve);
	}
}

void PellCurve_PrettyPrint(PellCurve pellCurve)
{
	printf("LegendreD: %d\n D: ", pellCurve->legendreSymbolD);fmpz_print(pellCurve->D);printf("\n");
	printf("LegendreN: %d\n N: ", pellCurve->legendreSymbolN);fmpz_print(pellCurve->n);printf("\n");
	printf("Prime : ");fmpz_print(pellCurve->prime);printf("\n");
	printf("Group Order : ");fmpz_print(pellCurve->groupOrder);printf("\n");
	printf("Prime+1 : ");fmpz_print(pellCurve->primePlusOne);printf("\n");
	printf("Prime+1 Factorization: ");
	for(slong i = 0; i < pellCurve->factorizationPlusOne->num; i++)
	{
		printf("(");
		fmpz_print(&pellCurve->factorizationPlusOne->p[i]); printf(" ^ ");
		fmpz_print(&pellCurve->factorizationPlusOne->exp[i]); printf(")");
	}
	printf("\n");
	printf("Prime-1 : ");fmpz_print(pellCurve->primeMinusOne);printf("\n");
	printf("Prime-1 Factorization: ");
	for(slong i = 0; i < pellCurve->factorizationMinusOne->num; i++)
	{
		printf("(");
		fmpz_print(&pellCurve->factorizationMinusOne->p[i]); printf(" ^ ");
		fmpz_print(&pellCurve->factorizationMinusOne->exp[i]); printf(")");
	}
	printf("\n");
}

PellPoint PellCurve_CreateEmptyPoint()
{
	PellPoint pellPoint = malloc(sizeof(struct pell_point_struct));
	fmpz_init(pellPoint->x);
	fmpz_init(pellPoint->y);
	return pellPoint;
}

void PellCurve_ResetPoint(PellPoint point)
{
	fmpz_set_ui(point->x, 0);
	fmpz_set_ui(point->y, 0);
}

void PellCurve_CopyPoint(PellPoint source, PellPoint destination)
{
	fmpz_set(destination->x, source->x);
	fmpz_set(destination->y, source->y);
}

int PellCurve_TestPointEquality(PellPoint a, PellPoint b)
{
	return fmpz_cmp(a->x, b->x) == 0 && fmpz_cmp(a->y, b->y) == 0;	
}

bool PellCurve_IsValidPellPoint(fmpz_t x, fmpz_t y, fmpz_t D, fmpz_t n, fmpz_t p)
{
	fmpz_t xSquare, DySquare, tmp;
	fmpz_init(xSquare);
	fmpz_init(DySquare);
	fmpz_init(tmp);
	//x² mod p
	fmpz_mul(xSquare, x, x);
	fmpz_mod(xSquare, xSquare, p);
	//D * y² mod p
	fmpz_mul(DySquare, y, y);
	fmpz_mul(DySquare, DySquare, D);
	fmpz_mod(DySquare, DySquare, p);
	
	//Check if (x² - Dy²) ≡ n mod p
	fmpz_sub(tmp, xSquare, DySquare);
	fmpz_mod(tmp, tmp, p);
	
	bool isOnCurve = (fmpz_cmp(tmp, n) == 0);
	fmpz_clear(xSquare);
	fmpz_clear(DySquare);
	fmpz_clear(tmp);
	return isOnCurve;
}


void PellCurve_PrintPoint(PellPoint pellPoint)
{
	printf("x: ");fmpz_print(pellPoint->x);printf(" ");
	printf("y: ");fmpz_print(pellPoint->y);printf("\n");
}

void PellCurve_ClearPoint(PellPoint pellPoint)
{
	fmpz_clear(pellPoint->x);
	fmpz_clear(pellPoint->y);
	free(pellPoint);
}

PellPoint *PellCurve_GenerateAllPoints(PellCurve pellCurve)
{	
	PellPoint *allPoints;
	if(pellCurve->legendreSymbolD == 0 && pellCurve->legendreSymbolN != 1)
	{
		/*no. of points is zero. Malloc 1 to avoid NULLS*/
		allPoints = malloc(1 * sizeof(PellPoint));
	}
	else if(pellCurve->legendreSymbolD == 0 && pellCurve->legendreSymbolN == 1)
	{
		/*Only two possible x values*/
		allPoints = malloc(2 * sizeof(PellPoint));
		allPoints[0] = PellCurve_CreateEmptyPoint();
		fmpz_sqrtmod(allPoints[0]->x, pellCurve->n, pellCurve->prime);
		allPoints[1] = PellCurve_CreateEmptyPoint();
		fmpz_sub(allPoints[1]->x, pellCurve->prime, allPoints[0]->x);
		
	}
	else if(pellCurve->legendreSymbolD == 1)
	{
		if(fmpz_cmp_ui(pellCurve->n,0) == 0)
		{
			//allocate 2*(p-1) + 1 points
			allPoints = malloc(fmpz_get_ui(pellCurve->groupOrder) * sizeof(PellPoint));
			fmpz_sqrtmod(pellCurve->temp0, pellCurve->D, pellCurve->prime);
			int index = 0;
			//Add the origin
			allPoints[index] = PellCurve_CreateEmptyPoint();
			fmpz_set_ui(allPoints[index]->x, 0);
			fmpz_set_ui(allPoints[index]->y, 0);
			index += 1;
			for(int i = 1; i < fmpz_get_ui(pellCurve->prime); i++)
			{
				//x = +sqrt(D)*y
				allPoints[index] = PellCurve_CreateEmptyPoint();
				fmpz_mul_ui(allPoints[index]->x, pellCurve->temp0, i);
				fmpz_mod(allPoints[index]->x, allPoints[index]->x, pellCurve->prime);
				fmpz_set_ui(allPoints[index]->y, i);
				index += 1;
				//x = -sqrt(D)*y
				allPoints[index] = PellCurve_CreateEmptyPoint();
				fmpz_sub(allPoints[index]->x, pellCurve->prime, allPoints[index-1]->x);
				fmpz_set_ui(allPoints[index]->y, i);
				index += 1;
			}
		}
		else
		{
			//allocate (p-1) points
			allPoints = malloc(fmpz_get_ui(pellCurve->groupOrder) * sizeof(PellPoint));	
			//Find the modular inverse of 2 mod prime
			fmpz_set_ui(pellCurve->temp1, 2);
			fmpz_invmod(pellCurve->temp0, pellCurve->temp1, pellCurve->prime);
			
			//Find the modular inverse of 2D mod prime
			fmpz_sqrtmod(pellCurve->temp1, pellCurve->D, pellCurve->prime);
			fmpz_mul_ui(pellCurve->temp1, pellCurve->temp1, 2);
			fmpz_invmod(pellCurve->temp1, pellCurve->temp1, pellCurve->prime);
			
			//Remember 0 has no modular inverse
			for(int i = 1; i < fmpz_get_ui(pellCurve->groupOrder)+1; i++)
			{
				//Find n/a
				fmpz_set_ui(pellCurve->temp2, i);
				fmpz_invmod(pellCurve->temp2, pellCurve->temp2, pellCurve->prime);
				fmpz_mul(pellCurve->temp2, pellCurve->temp2, pellCurve->n);
				
				//x = (a + (n/a)) / 2
				allPoints[i-1] = PellCurve_CreateEmptyPoint(); 
				fmpz_add_ui(allPoints[i-1]->x, pellCurve->temp2, i);
				fmpz_mul(allPoints[i-1]->x, allPoints[i-1]->x, pellCurve->temp0);
				fmpz_mod(allPoints[i-1]->x, allPoints[i-1]->x, pellCurve->prime);
				//y = (a - (n/a)) / 2 * rootD
				fmpz_sub(allPoints[i-1]->y, pellCurve->prime, pellCurve->temp2);
				fmpz_add_ui(allPoints[i-1]->y, allPoints[i-1]->y, i);
				fmpz_mul(allPoints[i-1]->y, allPoints[i-1]->y, pellCurve->temp1);
				fmpz_mod(allPoints[i-1]->y, allPoints[i-1]->y, pellCurve->prime);
			}
		}
	}
	return allPoints;
}

void PellCurve_DestroyAllPoints(PellCurve pellCurve, PellPoint *allPoints)
{
	if(allPoints && pellCurve)
	{
		if(pellCurve->legendreSymbolD == 0 && pellCurve->legendreSymbolN != 1)
		{
			free(allPoints);
		}
		else if(pellCurve->legendreSymbolD == 0 && pellCurve->legendreSymbolN == 1)
		{
			free(allPoints[0]);
			free(allPoints[1]);
			free(allPoints);
		}
		else
		{
			for(int i = 0; i < fmpz_get_ui(pellCurve->groupOrder); i++)
			{
				PellCurve_ClearPoint(allPoints[i]);	
			}
			free(allPoints);
		}
		
	}
}
