#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpz_factor.h>
#define PellCurve_MATRIX_INDEX(x, y, cols) ((x) * (cols) + (y))
typedef struct pell_curve_struct *PellCurve;
typedef struct pell_point_struct *PellPoint;
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
	flint_rand_t randomState;
	fmpz_factor_t factorizationPlusOne;
	fmpz_factor_t factorizationMinusOne;
	PellPoint fundamentalSolution;
	PellPoint tempPoint0;
	PellPoint tempPoint1;
	bool foundGenerator;
	int legendreSymbolD;
	int legendreSymbolN;
};


struct pell_point_struct
{
	fmpz_t x;
	fmpz_t y;
};

PellPoint PellCurve_CreateEmptyPoint()
{
	PellPoint pellPoint = malloc(sizeof(struct pell_point_struct));
	fmpz_init(pellPoint->x);
	fmpz_init(pellPoint->y);
	return pellPoint;
}

void PellCurve_ResetPoint(PellPoint point)
{
	fmpz_set_ui(point->x, 1);
	fmpz_set_ui(point->y, 0);
}

void PellCurve_CopyPoint(PellPoint source, PellPoint destination)
{
	fmpz_set(destination->x, source->x);
	fmpz_set(destination->y, source->y);
}

void PellCurve_PrintPoint(PellPoint pellPoint)
{
	printf("x: ");fmpz_print(pellPoint->x);printf(" ");
	printf("y: ");fmpz_print(pellPoint->y);printf("\n");
}
void PellCurve_PrintPointTab(PellPoint pellPoint)
{
	printf("(");fmpz_print(pellPoint->x);printf(",");
	fmpz_print(pellPoint->y);printf(") ");
}


int PellCurve_TestPointEquality(PellPoint a, PellPoint b)
{
	return fmpz_cmp(a->x, b->x) == 0 && fmpz_cmp(a->y, b->y) == 0;	
}

void PellCurve_ClearPoint(PellPoint pellPoint)
{
	fmpz_clear(pellPoint->x);
	fmpz_clear(pellPoint->y);
	free(pellPoint);
}

void PellCurve_Multiply(PellPoint result, PellPoint a, PellPoint b, fmpz_t D, fmpz_t primeNumber)
{
	fmpz_t temp1;
	fmpz_init(temp1);
	//result->x
	fmpz_mul(temp1, D, a->y);
	fmpz_mul(result->x, temp1, b->y);
	fmpz_mul(temp1, a->x, b->x);
	fmpz_add(result->x, result->x, temp1);
	fmpz_mod(result->x, result->x, primeNumber);
	
	//result->y
	fmpz_mul(temp1, a->x, b->y);
	fmpz_mul(result->y, a->y, b->x);
	fmpz_add(result->y, result->y, temp1);
	fmpz_mod(result->y, result->y, primeNumber);
	
	fmpz_clear(temp1);
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


void PellCurve_ScalarPower(PellPoint result, PellPoint base,  PellPoint temp0, PellPoint currentBase, fmpz_t D, fmpz_t privateKey, fmpz_t primeNumber)
{
	//Set result to infinity
	fmpz_set_ui(result->x, 1);
	fmpz_set_ui(result->y, 0);
	PellCurve_CopyPoint(base,currentBase);
	
	//Find no. of bits in private key
	size_t binaryLength = fmpz_sizeinbase(privateKey, 2);
	for(ssize_t i = 0; i < binaryLength; i++)
	{
		if(fmpz_tstbit(privateKey, i) != 0)
		{
			PellCurve_Multiply(temp0, result, currentBase, D, primeNumber);
			PellCurve_CopyPoint(temp0,result);
		}
		PellCurve_Multiply(temp0, currentBase, currentBase, D, primeNumber);
		PellCurve_CopyPoint(temp0,currentBase);
		//fmpz_print(base->x);printf(" ");fmpz_print(base->y);printf("\n");
	}
}

bool PellCurve_FindFundamentalSolution(PellPoint fundamentalSolution, fmpz_t groupOrder, fmpz_t n, fmpz_t D, fmpz_t primeNumber, fmpz_factor_t factorization, flint_rand_t randomState)
{
	int trials = 1000;
	bool isGenerator = false;
	PellPoint result = PellCurve_CreateEmptyPoint();
	PellPoint temp0  = PellCurve_CreateEmptyPoint();
	PellPoint temp1  = PellCurve_CreateEmptyPoint();
	fmpz_t temp,rhs, y_sq;
	fmpz_init(temp);
        fmpz_init(rhs); fmpz_init(y_sq);
	while(isGenerator == false && trials > 0)
	{
		//Generate random x
		fmpz_randm(fundamentalSolution->x, randomState, primeNumber);

		//Solve for y: x² - Dy² = 1 ⇒ Dy² = x² - 1
		fmpz_mul(rhs, fundamentalSolution->x, fundamentalSolution->x);
		fmpz_sub_ui(rhs, rhs, 1); 
		fmpz_mod(rhs, rhs, primeNumber);
        
		// Check if (x² - 1)/D is a square
		fmpz_invmod(y_sq, D, primeNumber);
		fmpz_mul(y_sq, y_sq, rhs);      
		fmpz_mod(y_sq, y_sq, primeNumber);
        
		if(fmpz_sqrtmod(fundamentalSolution->y, y_sq, primeNumber) && fmpz_cmp_ui(fundamentalSolution->x,0) != 0)
		{
			isGenerator  =true;
			//Test if it has full order q+1
			for(int i = 0; i < factorization->num; i++)
			{
				fmpz_divexact(temp, groupOrder, factorization->p + i);
				PellCurve_ScalarPower(result, fundamentalSolution, temp0, temp1, temp, D, primeNumber);
				if(fmpz_cmp_ui(result->x, 1) == 0 && fmpz_cmp_ui(result->y, 0) == 0)
				{
					isGenerator = false;
					break;
				}
			}
			if(isGenerator == true){break;}
		}
		trials--;
	}

	PellCurve_ClearPoint(result);
	PellCurve_ClearPoint(temp0);
	PellCurve_ClearPoint(temp1);
	fmpz_clear(temp);
	return isGenerator;
}



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
	flint_rand_init(pellCurve->randomState);
	fmpz_set(pellCurve->D, D);
	fmpz_set(pellCurve->n, n);
	fmpz_set(pellCurve->prime, prime);
	fmpz_sub_ui(pellCurve->primeMinusOne, pellCurve->prime, 1);
	fmpz_add_ui(pellCurve->primePlusOne, pellCurve->prime, 1);
	fmpz_factor(pellCurve->factorizationMinusOne, pellCurve->primeMinusOne);
	fmpz_factor(pellCurve->factorizationPlusOne, pellCurve->primePlusOne);
	pellCurve->legendreSymbolD = fmpz_jacobi(D, prime);
	pellCurve->legendreSymbolN = fmpz_jacobi(n, prime);
	pellCurve->fundamentalSolution = PellCurve_CreateEmptyPoint();
	pellCurve->tempPoint0 = PellCurve_CreateEmptyPoint();
	pellCurve->tempPoint1 = PellCurve_CreateEmptyPoint();
	pellCurve->foundGenerator = false;
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
		pellCurve->foundGenerator = PellCurve_FindFundamentalSolution(pellCurve->fundamentalSolution, pellCurve->groupOrder , pellCurve->n, pellCurve->D, pellCurve->prime, pellCurve->factorizationPlusOne, pellCurve->randomState);
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
		flint_rand_clear(pellCurve->randomState);
		PellCurve_ClearPoint(pellCurve->fundamentalSolution );
		PellCurve_ClearPoint(pellCurve->tempPoint0 );
		PellCurve_ClearPoint(pellCurve->tempPoint1 );
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
	else
	{
		assert(pellCurve->foundGenerator == true);
		//allocate (p+1) points
		allPoints = malloc(fmpz_get_ui(pellCurve->groupOrder) * sizeof(PellPoint));
		for(int i = 0; i < fmpz_get_ui(pellCurve->groupOrder); i++)
		{
			allPoints[i] = PellCurve_CreateEmptyPoint(); 
			fmpz_set_ui(pellCurve->temp0, i);
			//PellCurve_CopyPoint(pellCurve->fundamentalSolution,allPoints[i]);
			PellCurve_ScalarPower(allPoints[i], pellCurve->fundamentalSolution, pellCurve->tempPoint0, pellCurve->tempPoint1, pellCurve->D, pellCurve->temp0, pellCurve->prime);
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

void PellCurve_Matmul(PellPoint *output, PellPoint *input, PellPoint *weight, PellPoint tempPell0,PellPoint tempPell1, int inputRowCount, int inputColCount, int outputColCount, fmpz_t D, fmpz_t prime)
{
	for(int i = 0; i < inputRowCount; i++)
	{
		for(int j = 0; j < outputColCount; j++)
		{
			//Set to 1,0
			PellCurve_ResetPoint(output[PellCurve_MATRIX_INDEX(i,j,outputColCount)]);
			for(int k = 0; k < inputColCount; k++)
			{
				PellCurve_Multiply(tempPell0, input[PellCurve_MATRIX_INDEX(i, k, inputColCount)], weight[PellCurve_MATRIX_INDEX(k, j, outputColCount)], D, prime);
	                	PellCurve_Multiply(tempPell1, output[PellCurve_MATRIX_INDEX(i, j, outputColCount)], tempPell0, D, prime);
				PellCurve_CopyPoint(tempPell1, output[PellCurve_MATRIX_INDEX(i, j, outputColCount)]);
			}	
		}
	}


}

void PellCurve_SetMatrix(PellPoint *source, PellPoint *destination, int sourceRowCount, int sourceColCount, int destinationRowCount, int destinationColCount)
{
	assert(sourceRowCount == destinationRowCount);
	assert(sourceColCount == destinationColCount);
	for(int i = 0; i < sourceRowCount; i++)
	{
		for(int j = 0; j < sourceColCount; j++)
		{
			PellCurve_CopyPoint(source[PellCurve_MATRIX_INDEX(i,j,sourceColCount)], destination[PellCurve_MATRIX_INDEX(i,j,sourceColCount)]);
		}	
	}
}

void PellCurve_MatrixPrettyPrint(PellPoint *matrix, int rows, int cols)
{
	for(int i = 0; i < rows; i++)
	{
		printf("]\n");
		for(int j = 0; j < cols; j++)
		{
			PellCurve_PrintPointTab(matrix[PellCurve_MATRIX_INDEX(i,j,cols)]);
		}
		printf("]\n");
	}
}


