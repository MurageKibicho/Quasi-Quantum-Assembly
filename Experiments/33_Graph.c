#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gmp.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define kb_ENABLE_ERRORS 1
#define kb_INDEX(x, y, cols) ((x) * (cols) + (y))
#define PRINT_ERROR(msg) \
do { \
if (kb_ENABLE_ERRORS) { \
    fprintf(stderr, "\033[31mERROR: %s\033[0m\n", msg); \
    exit(1); \
} \
} while (0)
typedef struct kb_base_struct *kb;
typedef struct kb_node_struct *kbRational;
typedef struct kb_matrix_struct *kbMatrix;
struct kb_matrix_struct
{
	kbRational *nodes;
	size_t rowCount;
	size_t columnCount;
};
struct kb_base_struct
{
	int *numbers;
	size_t label;
};

struct kb_node_struct
{
	kb numerator;
	kb denominator;
};


kb kb_init(size_t label)
{
	kb result = malloc(sizeof(struct kb_base_struct));
	result->numbers = NULL;
	result->label = label;
	return result;
}

kbRational kbRational_init(size_t labelStart)
{
	kbRational result = malloc(sizeof(struct kb_node_struct));
	result->numerator = kb_init(labelStart);
	result->denominator = kb_init(labelStart+1);
	return result;
}

kbMatrix kbMatrix_init(size_t rowCount, size_t columnCount, size_t *labelStart)
{
	kbMatrix result = malloc(sizeof(struct kb_matrix_struct));
	result->nodes = NULL;
	result->rowCount =rowCount;
	result->columnCount =columnCount;
	for (size_t i = 0; i < rowCount * columnCount; i++)
	{
		kbRational rational = kbRational_init(*labelStart);
		*labelStart += 2;
		arrput(result->nodes, rational);
	}
	return result;
}


void kbRational_Print(kbRational a)
{
	printf("(%2ld/%2ld) ", a->numerator->label,a->denominator->label);
}

void kbMatrix_Print(kbMatrix a)
{
	for(size_t i = 0; i < a->rowCount; i++)
	{
		for(size_t j = 0; j < a->columnCount; j++)
		{
			kbRational_Print(a->nodes[kb_INDEX(i,j,a->columnCount)]);
		}
		printf("\n");
	}
	printf("\n");
}


void kb_clear(kb a)
{
	if(a)
	{
		if(a->numbers){arrfree(a->numbers);}
		free(a);
	}
}

void kbRational_clear(kbRational a)
{
	if(a)
	{
		if(a->numerator){kb_clear(a->numerator);}
		if(a->denominator){kb_clear(a->denominator);}
		free(a);
	}
}

void kbMatrix_clear(kbMatrix a)
{
	if(a)
	{
		for(size_t i = 0; i < arrlen(a->nodes); i++)
		{
			kbRational_clear(a->nodes[i]);
		}
		arrfree(a->nodes);
		free(a);
	}
}

void kbMatrix_Multiply(kbMatrix result, kbMatrix a, kbMatrix b)
{
	if(a->columnCount != b->rowCount){PRINT_ERROR("kbMatrix_Multiply Error: a, b dimensions error");}
	if(result->rowCount != a->rowCount){PRINT_ERROR("kbMatrix_Multiply Error: result row dimensions error");}
	if(result->columnCount != b->columnCount){PRINT_ERROR("kbMatrix_Multiply Error: result column dimensions error");}
	for(size_t i = 0; i < a->rowCount; i++)
	{
		for(size_t j = 0; j < b->columnCount; j++)
		{
			for(size_t k = 0; k < a->columnCount; k++)
			{
				kbRational_Print(a->nodes[kb_INDEX(i,k , a->columnCount)]);
				printf("•");
				kbRational_Print(b->nodes[kb_INDEX(k,j, b->columnCount)]);
				if(k < a->columnCount-1){printf("+");}
			}
			printf("\n");
		}
		printf("\n");
	}
	
}

void RationalTest()
{
	kbRational *nodes = NULL;
	for(size_t i = 0; i < 20; i+=2)
	{
		kbRational rational = kbRational_init(i);
		kbRational_Print(rational);
		arrput(nodes, rational);
	}
	
	for(size_t i = 0; i < arrlen(nodes); i++)
	{
		kbRational_clear(nodes[i]);
	}
	if(nodes){arrfree(nodes);}
}

void MatrixTest()
{
	size_t labelStart = 0;
	size_t labelReset = 0;
	size_t aRows = 3;
	size_t aCols = 4;
	size_t bCols = 5;
	kbMatrix a = kbMatrix_init(aRows, aCols, &labelStart);
	kbMatrix b = kbMatrix_init(aCols, bCols, &labelStart);
	kbMatrix L = kbMatrix_init(aRows, bCols, &labelReset);

	kbMatrix_Multiply(L, a, b);
	
	printf("\n");
	kbMatrix_Print(a);
	kbMatrix_Print(b);
	kbMatrix_Print(L);
	kbMatrix_clear(a);
	kbMatrix_clear(b);
	kbMatrix_clear(L);
}


int main()
{
	//RationalTest();
	MatrixTest();
	return 0;
}
