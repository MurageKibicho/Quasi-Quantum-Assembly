#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <gmp.h>
#define MATRIX_INDEX(x, y, cols) ((x) * (cols) + (y))
int *GenerateRowSums(int height, int width, int *matrix)
{
	int *result = calloc(height, sizeof(int));
	for(int i = 0; i < height; i++)
	{
		int sum = 0;
		for(int j = 0; j < width; j++)
		{
			sum += matrix[(i*width) + j];
		}
		result[i]=sum;
	}
	return result;
}

int *GenerateColumnSums(int height, int width, int *matrix)
{
	int *result = calloc(width, sizeof(int));
	for(int j = 0; j < width; j++)
	{
		int sum = 0;
		for(int i = 0; i < height; i++)
		{
			sum += matrix[(i*width) + j];
		}
		result[j] = sum;
	}
	return result;
}
/*
n+k-1 choose k-1
893577484800
4350399746568876
239382173
*/
mpz_t *SumBounds(int blocks, int arrayLength, int *array, mpz_t totalBound)
{
	mpz_t *sumBounds = malloc(arrayLength * sizeof(mpz_t));
	int n = 0;
	int k = blocks;
	mpz_set_ui(totalBound, 1);
	for(int i = 0; i < arrayLength; i++)
	{
		mpz_init(sumBounds[i]);
		n = array[i];
		assert(k - 1 >= 0);
		mpz_bin_uiui(sumBounds[i], n+k-1, k-1);
		mpz_mul(totalBound, totalBound, sumBounds[i]);
		gmp_printf("(%2d %2d) : |%2d %2d|%Zd\n",n, k, n+k-1, k-1,sumBounds[i]);
	}
	//gmp_printf("\t%Zd\n",totalBound);
	return sumBounds;
}

void PrintIntArray(int length, int*array){for(int i = 0; i < length; i++){printf("%4d, ", array[i]);}printf("\n");}
void ConditionSums(int secondaryLength, int *secondary, int primaryLength, int *primary)
{
	for(int i = 0; i < primaryLength; i++)
	{
		printf("|%d| ", primary[i]);
		for(int j = 0; j < secondaryLength; j++)
		{
			if(primary[i] > secondary[j])
			{
				printf("%3d(%d,%d)", secondary[j], i, j);
			}
		}	
		printf("\n");
	}
}

int main()
{
	int numberOfRows = 5;
	int numberOfColumns = 3;
	mpz_t maxRowBound;mpz_init(maxRowBound);
	mpz_t maxColumnBound;mpz_init(maxColumnBound);
	int *table = calloc(numberOfRows * numberOfColumns, sizeof(int)); 
	table[MATRIX_INDEX(0,0,numberOfColumns)] = 5; table[MATRIX_INDEX(0,1,numberOfColumns)] = 2;table[MATRIX_INDEX(0,2,numberOfColumns)] = 3;
	table[MATRIX_INDEX(1,0,numberOfColumns)] = 50;table[MATRIX_INDEX(1,1,numberOfColumns)] = 7;table[MATRIX_INDEX(1,2,numberOfColumns)] = 5;
	table[MATRIX_INDEX(2,0,numberOfColumns)] = 3; table[MATRIX_INDEX(2,1,numberOfColumns)] = 6;table[MATRIX_INDEX(2,2,numberOfColumns)] = 4;
	table[MATRIX_INDEX(3,0,numberOfColumns)] = 5; table[MATRIX_INDEX(3,1,numberOfColumns)] = 3;table[MATRIX_INDEX(3,2,numberOfColumns)] = 3;
	table[MATRIX_INDEX(4,0,numberOfColumns)] = 2; table[MATRIX_INDEX(4,1,numberOfColumns)] = 7;table[MATRIX_INDEX(4,2,numberOfColumns)] = 30;
	int *rowSums = GenerateRowSums(numberOfRows, numberOfColumns, table);
	int *columnSums = GenerateColumnSums(numberOfRows, numberOfColumns, table);
	mpz_t *rowSumBounds = SumBounds(numberOfColumns, numberOfRows, rowSums, maxRowBound);
	mpz_t *columnSumBounds = SumBounds(numberOfRows, numberOfColumns, columnSums, maxColumnBound);

	if(mpz_cmp(maxRowBound, maxColumnBound) >= 0)
	{
		gmp_printf("0 : %Zd %Zd\n", maxRowBound, maxColumnBound);	
		ConditionSums(numberOfRows, rowSums, numberOfColumns, columnSums);
	}
	else
	{
		gmp_printf("1 : %Zd %Zd\n", maxRowBound, maxColumnBound);	
		ConditionSums(numberOfColumns, columnSums, numberOfRows, rowSums);
	}
	
	mpz_clear(maxRowBound);mpz_clear(maxColumnBound);
	for(int i = 0; i < numberOfRows; i++){mpz_clear(rowSumBounds[i]);}
	for(int i = 0; i < numberOfColumns; i++){mpz_clear(columnSumBounds[i]);}
	free(rowSumBounds);free(columnSumBounds);free(rowSums);free(columnSums);free(table);
	return 0;
}
