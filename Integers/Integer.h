#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <limits.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpz_factor.h>
#include <flint/fmpz_mat.h>
#define BLANKINSHIP_TRIAL_COUNT 100000
//clear && gcc main.c -lm -lgmp -lmpfr -lflint -o m.o && ./m.o
typedef struct blankinship_solver_struct *LinearIntSolver;
struct blankinship_solver_struct 
{
	int rows;
	int cols;
	int solutionRow;
	fmpz_t quotient;
	fmpz_t temp0;
	fmpz_t smallest;
	fmpz_t absval;
	fmpz_t gcd;
	fmpz_mat_t matrix;
};

LinearIntSolver Integer_CreateLinearIntSolver(int variables)
{
	assert(variables > 0);
	LinearIntSolver intSolver = malloc(sizeof(struct blankinship_solver_struct));
	intSolver->rows = variables;
	intSolver->cols = variables + 1;
	fmpz_init(intSolver->quotient);
	fmpz_init(intSolver->temp0);
	fmpz_init(intSolver->smallest);
	fmpz_init(intSolver->absval);
	fmpz_init(intSolver->gcd);
	fmpz_mat_init(intSolver->matrix, intSolver->rows, intSolver->cols);
	return intSolver;
}

void Integer_DestroyLinearIntSolver(LinearIntSolver intSolver)
{
	if(intSolver)
	{
		fmpz_mat_clear(intSolver->matrix);
		fmpz_clear(intSolver->quotient);
		fmpz_clear(intSolver->temp0);
		fmpz_clear(intSolver->smallest);
		fmpz_clear(intSolver->absval);
		fmpz_clear(intSolver->gcd);
		free(intSolver);
	}
}

bool Integer_SolveLinearIntSolver(LinearIntSolver intSolver, int length, fmpz_t *search)
{
	bool solved = false;
	assert(intSolver->rows == length);
	assert(intSolver->rows + 1 == intSolver->cols);
	/*Initialize Blankinship Solver*/
	for(int i = 0; i < intSolver->rows; i++)
	{
		fmpz_set(fmpz_mat_entry(intSolver->matrix, i, 0), search[i]);
		for(int j = 1; j < intSolver->cols; j++)
		{
			fmpz_set_ui(fmpz_mat_entry(intSolver->matrix, i, j), (i + 1 == j) ? 1 : 0);
		}
	}
	
	int trialCount = 10000;
	#ifdef BLANKINSHIP_TRIAL_COUNT
		trialCount = BLANKINSHIP_TRIAL_COUNT;
	#else
		trialCount = 10000;
	#endif
	
	while(trialCount > 0)
	{
		//Find operator row with smallest nonzero
		int operatorIndex = -1;
		fmpz_set(intSolver->smallest, search[0]);
		for(int i = 0; i < intSolver->rows; i++)
		{
			if(fmpz_cmp_ui(fmpz_mat_entry(intSolver->matrix, i, 0), 0) != 0)
			{
				fmpz_abs(intSolver->absval, fmpz_mat_entry(intSolver->matrix, i, 0));
				if(operatorIndex == -1 || fmpz_cmp(intSolver->absval, intSolver->smallest) < 0)
				{
					operatorIndex = i;
					fmpz_set(intSolver->smallest, intSolver->absval);
				}
			}
		}
		if(operatorIndex == -1){solved=true;break;}
		
		//Find operand row to reduce
		int operandIndex = -1;
		for(int i = 0; i < intSolver->rows; i++)
		{
			if(i != operatorIndex && fmpz_cmp_ui(fmpz_mat_entry(intSolver->matrix, i, 0), 0) != 0)
			{
				operandIndex = i;
				break;
			}
		}

		if(operandIndex == -1){solved=true;break;}

		//Stability: swap if necessary 
		fmpz_abs(intSolver->temp0, fmpz_mat_entry(intSolver->matrix, operandIndex, 0));
		if(fmpz_cmp(intSolver->temp0, intSolver->smallest) < 0)
		{
			int tmp = operatorIndex;
			operatorIndex = operandIndex;
			operandIndex = tmp;
		}

		fmpz_fdiv_q(intSolver->quotient,fmpz_mat_entry(intSolver->matrix, operandIndex, 0),fmpz_mat_entry(intSolver->matrix, operatorIndex, 0));

		for(int j = 0; j < intSolver->cols; j++)
		{
			fmpz_mul(intSolver->temp0, intSolver->quotient, fmpz_mat_entry(intSolver->matrix, operatorIndex, j));
			fmpz_sub(fmpz_mat_entry(intSolver->matrix, operandIndex, j),fmpz_mat_entry(intSolver->matrix, operandIndex, j),intSolver->temp0);
		}
		trialCount -= 1;
	}
	//Find GCD 
	fmpz_set_ui(intSolver->gcd, 0);
	intSolver->solutionRow = -1;
	for(int i = 0; i < intSolver->rows; i++)
	{
		if(fmpz_cmp_ui(fmpz_mat_entry(intSolver->matrix, i, 0), 0) != 0)
		{
			fmpz_abs(intSolver->gcd, fmpz_mat_entry(intSolver->matrix, i, 0));
			intSolver->solutionRow = i;
			break;
		}
	}
	
	return solved;
}

void Integer_PrettyPrintLinearIntSolver(LinearIntSolver intSolver)
{
	printf("GCD = ");fmpz_print(intSolver->gcd);printf("\n");
	printf("Particular solution (x₁, x₂, ..., xₙ): ");
	
	for(int j = 1; j < intSolver->cols; j++)
	{
		fmpz_print(fmpz_mat_entry(intSolver->matrix, intSolver->solutionRow, j));
		if(j < intSolver->cols - 1){printf(", ");}
	}
	printf("\n");
	
	printf("\nGeneral integer solution:\n");
	printf("(x₁, x₂, ..., xₙ) = (");
	for(int j = 1; j < intSolver->cols; j++)
	{
		fmpz_print(fmpz_mat_entry(intSolver->matrix, intSolver->solutionRow, j));
		if (j < intSolver->cols - 1) printf(", ");
	}
	printf(")");

	for(int i = 0; i < intSolver->rows; i++)
	{
		printf(" + t%d*v%d", i + 1, i + 1);
	}
	printf(", where t₁, t₂, ... ∈ ℤ\n");
	
	printf("\n(nullspace) basis vectors satisfying a·v = 0:\n");
	int nullCount = 0;
	for(int i = 0; i < intSolver->rows; i++)
	{
		if(i == intSolver->solutionRow) continue;
		if(fmpz_cmp_ui(fmpz_mat_entry(intSolver->matrix, i, 0), 0) == 0)
		{
			nullCount++;
			printf("v%d = (", nullCount);
			for(int j = 1; j < intSolver->cols; j++)
			{
				fmpz_print(fmpz_mat_entry(intSolver->matrix, i, j));
				if (j < intSolver->cols - 1) printf(", ");
			}
			printf(")\n");
		}
	}

}

void Integer_TestLinearIntSolver()
{
	int search[] = {99, 77, 63};
	int rows = sizeof(search) / sizeof(int);
	fmpz_t *test = malloc(rows * sizeof(fmpz_t));
	for(int i = 0; i < rows; i++)
	{
		fmpz_init(test[i]);
		fmpz_set_ui(test[i], search[i]);
	}
	LinearIntSolver intSolver = Integer_CreateLinearIntSolver(rows);
	bool solved = Integer_SolveLinearIntSolver(intSolver,rows, test);
	assert(solved == true);
	
	Integer_PrettyPrintLinearIntSolver(intSolver);
	
	for(int i = 0; i < rows; i++)
	{
		fmpz_clear(test[i]);
	}
	free(test);
	Integer_DestroyLinearIntSolver(intSolver);
}

