#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <gmp.h>
typedef struct blankinship_row_struct *BlankinshipRow;
typedef BlankinshipRow* BlankinshipMatrix;
typedef struct mpz_index_struct *MpzWithIndex;
struct mpz_index_struct
{	
	mpz_t integer;
	int index;
	mpz_t *generalSolution; 
};
struct blankinship_row_struct
{
	mpz_t leader;
	int numberOfColumns;
	mpz_t *columns;
};

int BlankinshipCompareLeader(const void *a, const void *b)
{
	BlankinshipRow rowA = *(BlankinshipRow *)a;
	BlankinshipRow rowB = *(BlankinshipRow *)b;
	return mpz_cmp(rowB->leader, rowA->leader);
}

int MpzArrayCompare(const void *a, const void *b)
{
	mpz_t *ia = (mpz_t *)a;
	mpz_t *ib = (mpz_t *)b;
	return mpz_cmp(*ib, *ia);
}
double Find_Log_MPZ_Double(mpz_t x){signed long int ex;const double di = mpz_get_d_2exp(&ex, x);return ( (log(di) + log(2) * (double) ex) /log(2));}

void PrintMpzWithIndex(int length, MpzWithIndex *mpzArray)
{
	int k0 = 1;
	int k1 = 1;
	for(int i = 0; i < length; i++)
	{
		gmp_printf("%6d: %5Zd * (%5Zd ",mpzArray[i]->index, mpzArray[i]->integer, mpzArray[i]->generalSolution[0]);
		for(int j = 1; j < length-1; j++)
		{
			gmp_printf("+ %5Zd * k%d ", mpzArray[i]->generalSolution[j], j-1);
		}
		gmp_printf("+ %5Zd * k%d)\n",mpzArray[i]->generalSolution[length-1], length-2);
	}
	printf("\n");
}

void GetGeneralSolution(int length, MpzWithIndex *mpzArray)
{
	int k0 = 1;
	int k1 = 1;
	for(int i = 0; i < length; i++)
	{
		printf("x%d: ", i );
		gmp_printf(" %5Zd * k%d ", mpzArray[i]->generalSolution[1], 0);
		for(int j = 2; j < length; j++)
		{
			gmp_printf("+ %5Zd * k%d ", mpzArray[i]->generalSolution[j], j-1);
		}
		printf("\n");
	}
	printf("\n");
}
void PrintMpzWithIndexLog(int length, MpzWithIndex *mpzArray){for(int i = 0; i < length; i++){printf("%6d: %.3f\n",mpzArray[i]->index, Find_Log_MPZ_Double(mpzArray[i]->integer));}printf("\n");}
int MpzWithIndexCompareMpz(const void *a, const void *b){const MpzWithIndex *mpz1 = (const MpzWithIndex *)a;const MpzWithIndex *mpz2 = (const MpzWithIndex *)b;return mpz_cmp((*mpz2)->integer, (*mpz1)->integer);}
int MpzWithIndexCompareIndex(const void *a, const void *b){const MpzWithIndex *mpz1 = (const MpzWithIndex *)a;const MpzWithIndex *mpz2 = (const MpzWithIndex *)b;return (*mpz1)->index - (*mpz2)->index;}
/*
37474 * (-1586 +  4317 * 2 +  -986 * 1)
+
29389 * (    2 +    -5 * 2 +    -1 * 1)
+
9247 * ( 6421 + -17479 * 2 +  3999 * 1)
*/

BlankinshipMatrix CreateBlankinshipMatrix(int variableCount, MpzWithIndex *variables)
{
	qsort(variables, variableCount, sizeof(MpzWithIndex), MpzWithIndexCompareMpz);
	int rows = variableCount;int columns = variableCount;
	BlankinshipMatrix matrix = malloc(rows * sizeof(BlankinshipRow));
	for(int i = 0; i < rows; i++)
	{
		matrix[i] = malloc(sizeof(struct blankinship_row_struct));
		mpz_init(matrix[i]->leader);
		mpz_set(matrix[i]->leader, variables[i]->integer);
		matrix[i]->numberOfColumns = columns;
		matrix[i]->columns = malloc(columns * sizeof(mpz_t));
		for(int j = 0; j < columns; j++)
		{
			mpz_init(matrix[i]->columns[j]);
			mpz_set_ui(matrix[i]->columns[j], 0);
		}
		mpz_set_ui(matrix[i]->columns[i], 1);
		
	}
	return matrix;
}


void SortBlankinshipMatrix(int variableCount, BlankinshipMatrix matrix)
{
	qsort(matrix, variableCount, sizeof(BlankinshipRow), BlankinshipCompareLeader);
}

void PrintMatrix(int variableCount, BlankinshipMatrix matrix)
{
	int rows = variableCount;int columns = variableCount;
	for(int i = 0; i < rows; i++)
	{
		gmp_printf("%Zd : ", matrix[i]->leader);
		for(int j = 0; j < columns; j++)
		{
			gmp_printf("%Zd ", matrix[i]->columns[j]);
		}
		printf("\n");
	}
}

void FreeBlankinshipMatrix(int variableCount, BlankinshipMatrix matrix)
{
	int rows = variableCount;int columns = variableCount;
	for(int i = 0; i < rows; i++)
	{
		for(int j = 0; j < columns; j++)
		{
			mpz_clear(matrix[i]->columns[j]);
		}
		free(matrix[i]->columns);
		mpz_clear(matrix[i]->leader);
		free(matrix[i]);
	}
	free(matrix);
}

int FindOperatorIndex(int variableCount, BlankinshipMatrix matrix)
{
	int operatorIndex = 0;
	for(int i = variableCount-1; i >= 0; i--)
	{
		if(mpz_cmp_ui(matrix[i]->leader, 0)> 0)
		{
			operatorIndex = i;
			break;
		}
	}
	return operatorIndex;
}

void ReduceRows(int operatorIndex, int variableCount, BlankinshipMatrix matrix)
{
	assert(operatorIndex < variableCount);
	mpz_t quotient; mpz_init(quotient);
	mpz_t multiple; mpz_init(multiple);
	for(int i = operatorIndex-1; i >= 0; i--)
	{
		mpz_tdiv_q(quotient, matrix[i]->leader, matrix[operatorIndex]->leader);
		mpz_mul(multiple, matrix[operatorIndex]->leader, quotient);
		mpz_sub(matrix[i]->leader, matrix[i]->leader, multiple);
		assert(mpz_cmp_ui(matrix[i]->leader, 0) >= 0);
		for(int j = 0; j < matrix[i]->numberOfColumns; j++)
		{
			mpz_mul(multiple, matrix[operatorIndex]->columns[j], quotient);
			mpz_sub(matrix[i]->columns[j], matrix[i]->columns[j], multiple);
		}
	}
	mpz_clear(quotient);mpz_clear(multiple);
}

void StoreSolutions(int variableCount, BlankinshipMatrix matrix, MpzWithIndex *variables)
{
	int rows = variableCount;int columns = variableCount;
	for(int i = 0; i < columns; i++)
	{
		for(int j = 0; j < columns; j++)
		{
			mpz_set(variables[i]->generalSolution[j], matrix[j]->columns[i]);
		}
	}
}


/*int main()
{
	//int variables[] = {98,105, 77,21, 63};
	//int variables[] = {29389,9247,37474};
	//int variables[] = {99,77,63};
	int variables[] = {4, 51};
	
	int variableCount = sizeof(variables) / sizeof(int);
	MpzWithIndex *bigVariables = malloc(variableCount * sizeof(MpzWithIndex));
	for(int i = 0; i < variableCount; i++)
	{
		bigVariables[i] = malloc(sizeof(struct mpz_index_struct));
		bigVariables[i]->index = i;
		mpz_init(bigVariables[i]->integer);
		bigVariables[i]->generalSolution = malloc(variableCount * sizeof(mpz_t));
		for(int j = 0; j < variableCount; j++)
		{
			mpz_init(bigVariables[i]->generalSolution[j]);
		}
		mpz_set_ui(bigVariables[i]->integer, variables[i]);
	}

	BlankinshipMatrix matrix = CreateBlankinshipMatrix(variableCount, bigVariables);
	
	while(1)
	{
		int operatorIndex = FindOperatorIndex(variableCount, matrix);
		PrintMatrix(variableCount, matrix);printf("\n");
		if(operatorIndex == 0)
		{
			StoreSolutions(variableCount, matrix, bigVariables);
			PrintMpzWithIndex(variableCount, bigVariables);
			
			break;
		}
		ReduceRows(operatorIndex, variableCount, matrix);
		SortBlankinshipMatrix(variableCount, matrix);
	}
	GetGeneralSolution(variableCount, bigVariables);
			
	FreeBlankinshipMatrix(variableCount, matrix);
	for(int i = 0; i < variableCount; i++)
	{
		mpz_clear(bigVariables[i]->integer);
		for(int j = 0; j < variableCount; j++)
		{
			mpz_clear(bigVariables[i]->generalSolution[j]);
		}
		free(bigVariables[i]->generalSolution);
		free(bigVariables[i]);
	}
	free(bigVariables);
	return 0;
}*/
