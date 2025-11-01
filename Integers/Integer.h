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

//clear && gcc main.c -lm -lgmp -lmpfr -lflint -o m.o && ./m.o

void TestBlankinship()
{
	int search[] = {99, 77, 63};
	int rows = sizeof(search) / sizeof(int);
	int cols = rows + 1;

	fmpz_mat_t matrix;
	fmpz_mat_init(matrix, rows, cols);
	fmpz_t quotient, temp, smallest, absval, gcd;
	fmpz_init(quotient);
	fmpz_init(temp);
	fmpz_init(smallest);
	fmpz_init(absval);
	fmpz_init(gcd);

	for(int i = 0; i < rows; i++)
	{
		fmpz_set_ui(fmpz_mat_entry(matrix, i, 0), search[i]);
		for(int j = 1; j < cols; j++)
		{
			fmpz_set_ui(fmpz_mat_entry(matrix, i, j), (i + 1 == j) ? 1 : 0);
		}
	}

	while (1)
	{
		//Find operator row with smallest nonzero
		int operatorIndex = -1;
		fmpz_set_ui(smallest, UINT_MAX);

		for(int i = 0; i < rows; i++)
		{
			if(fmpz_cmp_ui(fmpz_mat_entry(matrix, i, 0), 0) != 0)
			{
				fmpz_abs(absval, fmpz_mat_entry(matrix, i, 0));
				if(operatorIndex == -1 || fmpz_cmp(absval, smallest) < 0)
				{
					operatorIndex = i;
					fmpz_set(smallest, absval);
				}
			}
		}

		if(operatorIndex == -1){break;}

		//Find operand row to reduce
		int operandIndex = -1;
		for(int i = 0; i < rows; i++)
		{
			if(i != operatorIndex && fmpz_cmp_ui(fmpz_mat_entry(matrix, i, 0), 0) != 0)
			{
				operandIndex = i;
				break;
			}
		}

		if(operandIndex == -1){break;}

		//Stability: swap if necessary 
		fmpz_t absOperand;
		fmpz_init(absOperand);
		fmpz_abs(absOperand, fmpz_mat_entry(matrix, operandIndex, 0));
		if(fmpz_cmp(absOperand, smallest) < 0)
		{
			int tmp = operatorIndex;
			operatorIndex = operandIndex;
			operandIndex = tmp;
		}
		fmpz_clear(absOperand);


		fmpz_fdiv_q(quotient,fmpz_mat_entry(matrix, operandIndex, 0),fmpz_mat_entry(matrix, operatorIndex, 0));

		for(int j = 0; j < cols; j++)
		{
			fmpz_mul(temp, quotient, fmpz_mat_entry(matrix, operatorIndex, j));
			fmpz_sub(fmpz_mat_entry(matrix, operandIndex, j),fmpz_mat_entry(matrix, operandIndex, j),temp);
		}
	}

	printf("Final matrix:\n");
	fmpz_mat_print_pretty(matrix);
	printf("\n");

	// Find GCD 
	fmpz_set_ui(gcd, 0);
	int baseRow = -1;
	for(int i = 0; i < rows; i++)
	{
		if(fmpz_cmp_ui(fmpz_mat_entry(matrix, i, 0), 0) != 0)
		{
			fmpz_abs(gcd, fmpz_mat_entry(matrix, i, 0));
			baseRow = i;
			break;
		}
	}

	printf("GCD = ");
	fmpz_print(gcd);
	printf("\n");

	printf("Particular solution (x₁, x₂, ..., xₙ): ");
	for(int j = 1; j < cols; j++)
	{
		fmpz_print(fmpz_mat_entry(matrix, baseRow, j));
		if(j < cols - 1) printf(", ");
	}
	printf("\n");

	printf("\nHomogeneous (nullspace) basis vectors (satisfying a·v = 0):\n");
	int nullCount = 0;
	for(int i = 0; i < rows; i++)
	{
		if(i == baseRow) continue;
		if(fmpz_cmp_ui(fmpz_mat_entry(matrix, i, 0), 0) == 0)
		{
			nullCount++;
			printf("v%d = (", nullCount);
			for(int j = 1; j < cols; j++)
			{
				fmpz_print(fmpz_mat_entry(matrix, i, j));
				if (j < cols - 1) printf(", ");
			}
			printf(")\n");
		}
	}

	printf("\nGeneral integer solution:\n");
	printf("(x₁, x₂, ..., xₙ) = (");
	for(int j = 1; j < cols; j++)
	{
		fmpz_print(fmpz_mat_entry(matrix, baseRow, j));
		if (j < cols - 1) printf(", ");
	}
	printf(")");

	for(int i = 0; i < nullCount; i++)
	{
		printf(" + t%d*v%d", i + 1, i + 1);
	}


	printf(", where t₁, t₂, ... ∈ ℤ\n");

	fmpz_mat_clear(matrix);
	fmpz_clear(quotient);
	fmpz_clear(temp);
	fmpz_clear(smallest);
	fmpz_clear(absval);
	fmpz_clear(gcd);
}



