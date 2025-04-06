#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <gmp.h>
#include "29_2_Iris.h"
#include "ff_asm_primes.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define kb_PRIME_ARRAY_LENGTH 5239
#define kb_ENABLE_ERRORS 1
#define kb_TRUE 1
#define kb_FALSE 0
#define kb_INDEX(x, y, cols) ((x) * (cols) + (y))
#define PRINT_ERROR(msg) \
do { \
if (kb_ENABLE_ERRORS) { \
    fprintf(stderr, "\033[31mERROR: %s\033[0m\n", msg); \
    exit(1); \
} \
} while (0)
typedef int8_t type;
typedef struct kibicho_brocot_struct *kb;
struct kibicho_brocot_struct
{
	int8_t sign;
	float gradient;
	type numerator;
	type denominator;
	kb *children;
	void (*backward)(kb);
};
void kb_initBack(kb output){}

float Random_Uniform_Rational(type maxDenominator, type *numerator, type *denominator, int8_t *sign) 
{
	*denominator = 1 + (rand() % maxDenominator);
	assert(*denominator > 0);
	*numerator   = (rand() % *denominator);
	*sign        = (rand() % 2 == 0) ? 1 : -1;
}


kb kb_init()
{
	kb result = malloc(sizeof(struct kibicho_brocot_struct));
	result->children = NULL;
	result->gradient = 0.0f;
	result->backward = kb_initBack;
	Random_Uniform_Rational(4, &(result->numerator),&(result->denominator),&(result->sign));
	return result;
}

void kb_print(kb a)
{
	if(a == NULL){PRINT_ERROR("kb_print Error: kb is NULL");}
	printf("(%c %3d %3d)", a->sign < 0 ? '-': '+', a->numerator, a->denominator);
	//printf("%.3f : %.3f (%c %3d %3d)\n", a->number, a->gradient, a->sign < 0 ? '-': '+', a->numerator, a->denominator);
}

void kb_clear(kb a)
{
	if(a)
	{
		if(a->children)
		{
			arrfree(a->children);
		}
		free(a);
	}
}
void kb_FreeModel(kb *model)
{
	if(model)
	{
		for(size_t i = 0; i < arrlen(model); i++)
		{
			kb_clear(model[i]);	
		}
		arrfree(model);
	}
}

void kb_addBack(kb output)
{
	if(arrlen(output->children) < 2){PRINT_ERROR("kb_addBack Error: children < 2");}
	output->children[0]->gradient += output->gradient;
	output->children[1]->gradient += output->gradient;
}

kb kb_add(kb a, kb b)
{
	kb c = kb_init();
	c->denominator = a->denominator * b->denominator;
	c->numerator   = a->sign * (a->numerator * b->denominator) +  b->sign * (b->numerator * a->denominator);
	c->sign = 1;
	if(c->numerator < 0){c->sign = -1; c->numerator *= -1;}
	//kb_print(a);kb_print(b);kb_print(c);printf("\n");
	c->backward = kb_addBack;
	arrput(c->children, a);
	arrput(c->children, b);
	return c;
}


void AppendEmbeddingWeights(kb **model, size_t maxTokenValue, size_t embedDimension)
{
	//Weights embedding, each row is a token, each column is a weight
	for(size_t i = 0; i < maxTokenValue * embedDimension; i++)
	{
		kb a = kb_init();
		arrput(*model, a);
	}
}

void AppendEmbeddingPosition(kb **model, size_t maxTokenCount, size_t embedDimension)
{
	//Weights embedding, each row is a token, each column is a weight
	for(size_t i = 0; i < maxTokenCount * embedDimension; i++)
	{
		kb a = kb_init();
		arrput(*model, a);
	}
}

void AppendEmbeddingSums(kb **model, size_t inputLength, int16_t *tokenizedData, size_t maxTokenValue, size_t maxTokenCount, size_t embedDimension)
{
	for(size_t i = 0; i < inputLength; i++)
	{
		uint16_t token = tokenizedData[i];
		assert(token < maxTokenValue);
		for(size_t j = 0; j < embedDimension; j++)
		{
			size_t weightIndex = token * embedDimension + j;assert(weightIndex < maxTokenValue * embedDimension);
			size_t positionIndex= (i * embedDimension + j) + maxTokenValue * embedDimension;
			
			kb weight = (*model)[weightIndex] ;
			kb positionEmbedding = (*model)[positionIndex];
			
			kb sum = kb_add(weight, positionEmbedding);
			arrput(*model, sum);
		}
	}
}


int main()
{
	srand(546);
	kb *model = NULL;
	size_t maxTokenValue = 5257;//50257;
	size_t maxTokenCount = 1024;//Maximum length of input sequence
	size_t embedDimension= 768;//768
	int inputLength = 64;
	int16_t *tokenizedData = calloc(inputLength,sizeof(int));for(int i = 0; i < inputLength; i++){tokenizedData[i] = rand() % maxTokenValue;}
	AppendEmbeddingWeights(&model,maxTokenValue, embedDimension);
	AppendEmbeddingPosition(&model,maxTokenCount, embedDimension);
	AppendEmbeddingSums(&model, inputLength, tokenizedData, maxTokenValue, maxTokenCount, embedDimension);
	printf("%ld\n", arrlen(model));
	kb_FreeModel(model);
	free(tokenizedData);
	return 0;
}
