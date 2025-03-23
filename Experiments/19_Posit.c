#ifndef _FFASM_H_
#define _FFASM_H_

#ifndef ff_BITS
#define ff_BITS 32
#endif

#include <stdarg.h> 
#include <stdint.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <assert.h>
#include <math.h>
#include <gmp.h>
#include "ff_asm_primes.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define FF_PRIME_ARRAY_LENGTH 5239
#define ff_INDEX(x, y, cols) ((x) * (cols) + (y))
#define ff_TRUE 1
#define ff_FALSE 0
#define ff_ENABLE_ERRORS 1
#define PRINT_ERROR(msg) \
do { \
if (ff_ENABLE_ERRORS) { \
    fprintf(stderr, "\033[31mERROR: %s\033[0m\n", msg); \
    exit(1); \
} \
} while (0)
typedef struct ff_node_struct *ffNode;
typedef struct ff_finite_fields_struct *ffField;
typedef struct ff_system_struct *ffSystem;
typedef struct ff_crt_struct *ffCRT;
struct ff_finite_fields_struct
{
	int *finiteField;
	int *generator;
	int floatingPointBits;
	int bitCountGoal;
	int smoothnessFactor;
	int onlyGenerator2;
	mpz_t numberRange;
	ffCRT *cachedCRTConstants;
};
struct ff_node_struct 
{
	int finiteFieldIndex;
	mpz_t integer;
	int *residues; 
};

struct ff_system_struct
{
	ffField *fields;
	ffNode *nodes;
};
struct ff_crt_struct
{	
	mpz_t temporary1;
	mpz_t fixedPointInverse;
};

int ModularExponentiation(int base, int exp, int mod){int result = 1;base = base % mod;while(exp > 0){if(exp % 2 == 1){result = (result * base) % mod;}exp = exp >> 1;base = (base * base) % mod;}return result;}
int FastCheck_2IsAGenerator(int primeNumber){int result = ff_FALSE;if(primeNumber == 2){result = ff_TRUE;}if(primeNumber % 8 == 3 || primeNumber % 8 == 5){int primeNumberMinusOne = primeNumber - 1;int n = primeNumberMinusOne;int q = 0;if(n % 2 == 0){q = 2;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = ff_FALSE;return result;}while(n % 2 == 0){n /= 2;}}for(int i = 3; i * i <= primeNumberMinusOne; i += 2){if(n % i == 0){q = i;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = ff_FALSE;return result;}while(n % i == 0){n /= i;}}}if(n > 1){q = n;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = ff_FALSE;return result;}}result = ff_TRUE;}return result;}
uint32_t XorShift(uint32_t state[]){uint32_t x = state[0];x ^= x >> 17;x ^= x << 5;state[0] = x;return x;}

ffField ffField_CreateDefaultField()
{
	ffField field = malloc(sizeof(struct ff_finite_fields_struct));
	if(field == NULL){PRINT_ERROR("ffField_CreateDefaultField Error: field is NULL");}
	field->finiteField = NULL;
	field->generator   = NULL;
	field->bitCountGoal = 1;
	field->smoothnessFactor = 2;
	field->floatingPointBits = 8;
	field->onlyGenerator2= ff_TRUE;
	mpz_init(field->numberRange);
	mpz_set_ui(field->numberRange, 2);
	
	field->cachedCRTConstants = NULL;
	ffCRT tempConstant = malloc(sizeof(struct ff_crt_struct));
	mpz_init(tempConstant->temporary1);
	arrput(field->cachedCRTConstants,tempConstant);
	arrput(field->finiteField, 2);
	arrput(field->generator, 2);
	return field;
}


void ffSystem_CreateNewField(ffSystem system)
{
	ffField field  = ffField_CreateDefaultField();
	arrput(system->fields, field);
}

void ffSystem_CreateNewNode(ffSystem system, double number, int finiteFieldIndex)
{
	ffNode node = malloc(sizeof(struct ff_node_struct));
	if(node == NULL){PRINT_ERROR("ffNode_CreateDefaultNode Error: node is NULL");}
	if(finiteFieldIndex < 0 || finiteFieldIndex >= arrlen(system->fields)){PRINT_ERROR("ffSystem_CreateNewNode Error: finiteFieldIndex out of range");}
	
	node->residues = NULL;
	mpz_init(node->integer);
	node->finiteFieldIndex = finiteFieldIndex;
	system->fields[finiteFieldIndex]->floatingPointBits = 8;
	mpf_t numberHolder, half, fixedPointOne;
	mpz_t temp, temporary0;mpz_inits(temp, temporary0, NULL);
	mpf_inits(numberHolder, half, fixedPointOne, NULL);
	mpf_set_d(numberHolder, number);
	mpf_set_d(half, 0.5);
	mpf_set_ui(fixedPointOne, 1);
	mpf_mul_2exp(fixedPointOne,fixedPointOne, system->fields[finiteFieldIndex]->floatingPointBits);
	mpf_mul(numberHolder, numberHolder, fixedPointOne);
	//printf("%.3f\n", number);
	if(number >= 0)
	{
		mpf_add(numberHolder, numberHolder, half);
	} 
	else
	{
		mpf_sub(numberHolder, numberHolder, half);
	}
	mpf_trunc(numberHolder, numberHolder);
	
	for(size_t i = 0; i < arrlen(system->fields[finiteFieldIndex]->finiteField); i++)
	{
		mpz_set_f(temp, numberHolder);
		int field = system->fields[finiteFieldIndex]->finiteField[i];
		int residue = mpz_mod_ui(temp,temp,field);
		mpz_mul_ui(temporary0, system->fields[finiteFieldIndex]->cachedCRTConstants[i]->temporary1, residue);
		mpz_add(node->integer, node->integer, temporary0);
		//gmp_printf("%.0Ff %2d %2d %Zd\n", numberHolder, residue, field,system->fields[finiteFieldIndex]->cachedCRTConstants[i]->temporary1);
		arrput(node->residues, residue);
	}
	mpz_mod(node->integer,node->integer, system->fields[finiteFieldIndex]->numberRange);
	arrput(system->nodes, node);
	mpf_clears(numberHolder, half, fixedPointOne, NULL);
	mpz_clears(temp,temporary0, NULL);
}

void ffNode_AddNodes(ffSystem system, ffNode result, ffNode a, ffNode b)
{
	if(result == NULL){PRINT_ERROR("ffNode_AddNodes Error: result node is NULL");}	
	if(a == NULL){PRINT_ERROR("ffNode_AddNodes Error: a node is NULL");}	
	if(b == NULL){PRINT_ERROR("ffNode_AddNodes Error: b node is NULL");}	
	if(a->finiteFieldIndex != b->finiteFieldIndex){PRINT_ERROR("ffNode_AddNodes Error: a, b different finite fields");}	
	if(result->finiteFieldIndex != b->finiteFieldIndex){PRINT_ERROR("ffNode_AddNodes Error: result different finite fields from a and b");}	
	if(system == NULL){PRINT_ERROR("ffNode_AddNodes Error: system is NULL");}
	if(a->finiteFieldIndex < 0 || a->finiteFieldIndex >= arrlen(system->fields)){PRINT_ERROR("ffNode_AddNodes Error: finiteFieldIndex out of range");}
	int finiteFieldIndex = a->finiteFieldIndex;
	int *finiteField = system->fields[finiteFieldIndex]->finiteField;
	
	mpz_t temporary0;mpz_init(temporary0);	
	mpz_set_ui(result->integer, 0);
	for(size_t i = 0; i < arrlen(finiteField); i++)
	{
		int field = finiteField[i];
		int residue = (a->residues[i] + b->residues[i]) % field;
		result->residues[i] = residue;
		mpz_mul_ui(temporary0, system->fields[finiteFieldIndex]->cachedCRTConstants[i]->temporary1, residue);
		mpz_add(result->integer,result->integer, temporary0);
	}
	mpz_mod(result->integer,result->integer, system->fields[finiteFieldIndex]->numberRange);
	mpz_clear(temporary0);
}

void ffNode_SubNodes(ffSystem system, ffNode result, ffNode a, ffNode b)
{
	if(result == NULL){PRINT_ERROR("ffNode_AddNodes Error: result node is NULL");}	
	if(a == NULL){PRINT_ERROR("ffNode_AddNodes Error: a node is NULL");}	
	if(b == NULL){PRINT_ERROR("ffNode_AddNodes Error: b node is NULL");}	
	if(a->finiteFieldIndex != b->finiteFieldIndex){PRINT_ERROR("ffNode_AddNodes Error: a, b different finite fields");}	
	if(result->finiteFieldIndex != b->finiteFieldIndex){PRINT_ERROR("ffNode_AddNodes Error: result different finite fields from a and b");}	
	if(system == NULL){PRINT_ERROR("ffNode_AddNodes Error: system is NULL");}
	if(a->finiteFieldIndex < 0 || a->finiteFieldIndex >= arrlen(system->fields)){PRINT_ERROR("ffNode_AddNodes Error: finiteFieldIndex out of range");}
	int finiteFieldIndex = a->finiteFieldIndex;
	int *finiteField = system->fields[finiteFieldIndex]->finiteField;	
	
	mpz_t temporary0;mpz_init(temporary0);	
	mpz_set_ui(result->integer, 0);
	for(size_t i = 0; i < arrlen(finiteField); i++)
	{
		int field = finiteField[i];
		int residue = (a->residues[i] + (field - b->residues[i])) % field;
		result->residues[i] = residue;
		mpz_mul_ui(temporary0, system->fields[finiteFieldIndex]->cachedCRTConstants[i]->temporary1, residue);

		mpz_add(result->integer,result->integer, temporary0);
	}
	mpz_mod(result->integer,result->integer, system->fields[finiteFieldIndex]->numberRange);
}

void ffNode_MulNodes(ffSystem system, ffNode result, ffNode a, ffNode b)
{
	if(result == NULL){PRINT_ERROR("ffNode_AddNodes Error: result node is NULL");}	
	if(a == NULL){PRINT_ERROR("ffNode_AddNodes Error: a node is NULL");}	
	if(b == NULL){PRINT_ERROR("ffNode_AddNodes Error: b node is NULL");}	
	if(a->finiteFieldIndex != b->finiteFieldIndex){PRINT_ERROR("ffNode_AddNodes Error: a, b different finite fields");}	
	if(result->finiteFieldIndex != b->finiteFieldIndex){PRINT_ERROR("ffNode_AddNodes Error: result different finite fields from a and b");}	
	if(system == NULL){PRINT_ERROR("ffNode_AddNodes Error: system is NULL");}
	if(a->finiteFieldIndex < 0 || a->finiteFieldIndex >= arrlen(system->fields)){PRINT_ERROR("ffNode_AddNodes Error: finiteFieldIndex out of range");}
	int finiteFieldIndex = a->finiteFieldIndex;
	int *finiteField = system->fields[finiteFieldIndex]->finiteField;	
	
	mpz_t temporary0;mpz_init(temporary0);	
	mpz_set_ui(result->integer, 0);
	for(size_t i = 0; i < arrlen(finiteField); i++)
	{
		int field = finiteField[i];
		int residue = (a->residues[i] * b->residues[i]) % field;
		//gmp_printf("%d %Zd\n",residue, );
		result->residues[i] = residue;
		mpz_mul_ui(temporary0, system->fields[finiteFieldIndex]->cachedCRTConstants[i]->temporary1, residue);
		
		mpz_add(result->integer,result->integer, temporary0);
	}
	//,3294492900247);
	mpz_mod(result->integer,result->integer, system->fields[finiteFieldIndex]->numberRange);
	
}

ffSystem ffSystem_init()
{
	ffSystem system = malloc(sizeof(struct ff_system_struct));
	if(system == NULL){PRINT_ERROR("ffSystem_init Error: system is NULL");}
	system->fields = NULL;
	system->nodes  = NULL;
	ffSystem_CreateNewField(system);
	return system;
}

void ffCRT_FillCRTConstants(int *finiteField, mpz_t setSize, ffCRT **cachedCRTConstants, mpz_t fixedPointHolder)
{
	if(finiteField == NULL){PRINT_ERROR("ffCRT_FillCRTConstants Error: field is uninitialized");}
	if(setSize == NULL){PRINT_ERROR("ffCRT_FillCRTConstants Error: setSize is uninitialized");}
	if(*cachedCRTConstants == NULL){PRINT_ERROR("ffCRT_FillCRTConstants Error: cachedCRTConstants is uninitialized");}
	if(mpz_cmp_ui(setSize, 0) <= 0){PRINT_ERROR("ffCRT_FillCRTConstants Error: setSize is 0");}
	if(arrlen(*cachedCRTConstants) != 0){PRINT_ERROR("ffCRT_FillCRTConstants Error: cachedCRTConstants is more than 0");}
	if(arrlen(finiteField) < 1){PRINT_ERROR("ffCRT_FillCRTConstants Error: arrlen(finiteField) is less than 1");}
	mpz_t temporary0,temporary1, temporary2, productHolder,modularInverseHolder;
	mpz_inits(temporary0, temporary1, temporary2,productHolder,modularInverseHolder,NULL);
	mpz_set(productHolder, setSize);
	

	for(size_t i = 0; i < arrlen(finiteField); i++)
	{
		ffCRT tempConstant = malloc(sizeof(struct ff_crt_struct));
		mpz_init(tempConstant->temporary1);
		mpz_set_ui(temporary0, finiteField[i]);
		mpz_tdiv_q(temporary1, productHolder, temporary0);
		mpz_invert(modularInverseHolder, temporary1, temporary0);
		mpz_mul(temporary1, temporary1,modularInverseHolder);
		mpz_set(tempConstant->temporary1, temporary1);			
		mpz_invert(tempConstant->fixedPointInverse, fixedPointHolder, temporary0);
		//gmp_printf("||%Zd %d||\n", tempConstant->fixedPointInverse,finiteField[i]);
		arrput(*cachedCRTConstants,tempConstant);
	}
	mpz_clears(temporary0, temporary1, temporary2, productHolder,modularInverseHolder, NULL);
}

void ffField_SetField(ffField field, int smoothnessFactor, int generator2, int bitCount)
{
	if(smoothnessFactor < 2){PRINT_ERROR("ffField_SetField Error: smoothnessFactor must be >= 0");}
	if(smoothnessFactor > 45000){PRINT_ERROR("ffField_SetField Error: smoothnessFactor must be < 45000");}
	if(generator2 != ff_TRUE){PRINT_ERROR("ffField_SetField Error: generator2 MUST be TRUE. Only 2 supported right now");}
	if(field == NULL){PRINT_ERROR("ffField_SetField Error: field is uninitialized");}
	if(bitCount <= 0 || bitCount > 1000){PRINT_ERROR("ffField_SetField Error: bitCount MUST be between 0 and 1000 bits");}
	arrsetlen(field->finiteField, 0);
	arrsetlen(field->generator, 0);
	for(size_t i = 0; i < arrlen(field->cachedCRTConstants); i++){mpz_clear(field->cachedCRTConstants[i]->temporary1);if(field->cachedCRTConstants[i]){free(field->cachedCRTConstants[i]);}}
	arrsetlen(field->cachedCRTConstants, 0);
	
	mpz_t fixedPointHolder;mpz_init(fixedPointHolder);mpz_set_ui(fixedPointHolder,1);
	mpz_mul_2exp(fixedPointHolder,fixedPointHolder, field->floatingPointBits);
	mpz_set_ui(field->numberRange, 1);
	field->smoothnessFactor = smoothnessFactor;
	field->bitCountGoal = bitCount;
	int currentPrimeIndex = 0;
	double logValue = 0.0f;
	while(currentPrimeIndex < FF_PRIME_ARRAY_LENGTH)
	{
		if(first5239[currentPrimeIndex] >= smoothnessFactor && FastCheck_2IsAGenerator(first5239[currentPrimeIndex]) == ff_TRUE)
		{	
			arrput(field->finiteField, first5239[currentPrimeIndex]);
			arrput(field->generator, 2);
			logValue += log2(first5239[currentPrimeIndex]);
			mpz_mul_ui(field->numberRange, field->numberRange, first5239[currentPrimeIndex]);
			if(logValue > (double) bitCount){break;}
		}
		currentPrimeIndex += 1;
	}
	if(arrlen(field->generator) != arrlen(field->finiteField)){PRINT_ERROR("ffField_SetField Error: arrlen(field->generator) != arrlen(field->finiteField)");}
	ffCRT_FillCRTConstants(field->finiteField, field->numberRange, &(field->cachedCRTConstants), fixedPointHolder);
	mpz_clear(fixedPointHolder);
}

void ffField_Free(ffField field)
{
	if(field)
	{
		for(size_t i = 0; i < arrlen(field->cachedCRTConstants); i++)
		{
			mpz_clear(field->cachedCRTConstants[i]->temporary1);
			if(field->cachedCRTConstants[i])
			{
				free(field->cachedCRTConstants[i]);
			}
		}
		arrfree(field->finiteField);
		arrfree(field->generator);
		mpz_clear(field->numberRange);
		arrfree(field->cachedCRTConstants);
		free(field);
	}
}
void ffNode_Free(ffNode node)
{
	if(node)
	{
		mpz_clear(node->integer);
		arrfree(node->residues);
		free(node);
	}
}
void ffSystem_clear(ffSystem system)
{
	if(system)
	{
		for(size_t i = 0; i < arrlen(system->fields); i++)
		{
			ffField_Free(system->fields[i]);	
		}
		for(size_t i = 0; i < arrlen(system->nodes); i++)
		{
			ffNode_Free(system->nodes[i]);	
		}
		arrfree(system->fields);
		arrfree(system->nodes);
		free(system);
	}
}

void ffField_PrintField(ffField field)
{
	if(field == NULL){PRINT_ERROR("ffField_PrintField Error: field is NULL");}
	if(arrlen(field->finiteField) < 0){PRINT_ERROR("ffField_PrintField Error: field->finiteField is NULL");}
	printf("Smoothness : %d\n", field->smoothnessFactor);
	printf("Bit count goal : %d , Actual range(bits) %ld\n", field->bitCountGoal, mpz_sizeinbase(field->numberRange, 2));
	printf("Prime Number Range : %d to %d\n", field->finiteField[0], field->finiteField[arrlen(field->finiteField)-1]);
	gmp_printf("Range : %Zd\n", field->numberRange);
	printf("2 is a Primitive Root : %s\n", field->onlyGenerator2 == ff_TRUE ? "TRUE" : "FALSE");
}

void ffNode_PrintNode(ffNode node)
{
	if(node == NULL){PRINT_ERROR("ffNode_PrintNode Error: node is NULL");}
	if(node->finiteFieldIndex < 0){PRINT_ERROR("ffNode_PrintNode Error: finiteFieldIndex out of range");}
	if(node->residues)
	{
		for(size_t i = 0; i < arrlen(node->residues); i++)
		{
			//printf("%d, ", node->residues[i]);
			gmp_printf("%d ",node->residues[i]);
		}
		gmp_printf("%Zd\n", node->integer);
	}
	
}

void ffSystem_PrintSystem(ffSystem system)
{
	if(system == NULL){PRINT_ERROR("ffSystem_PrintSystem Error: system is NULL");}
	printf("Field count %ld, Node count %ld\n", arrlen(system->fields),arrlen(system->nodes));
	for(size_t i = 0; i < arrlen(system->fields); i++)
	{
		printf("Fields Index %ld\n", i);
		ffField_PrintField(system->fields[i]);	
		printf("\n");
	}
	for(size_t i = 0; i < arrlen(system->nodes); i++)
	{
		printf("Node Index %ld\n", i);
		ffNode_PrintNode(system->nodes[i]);	
		printf("\n");
	}
}

void TestSystem()
{
	ffSystem system0 = ffSystem_init();
	if(system0 == NULL){PRINT_ERROR("TestSystem Error : system0 is NULL");}

	int smoothnessFactor = 257;
	int bitCount = 40;
	double number0 = 4546.8;
	double number1 = 4546.8;
	ffField_SetField(system0->fields[0], smoothnessFactor, ff_TRUE, bitCount);
	//ffSystem_CreateNewField(system0);
	//ffField_SetField(system0->fields[1], smoothnessFactor+300, ff_TRUE, bitCount + 200);
	
	ffSystem_CreateNewNode(system0, number0, 0);
	ffSystem_CreateNewNode(system0, number1, 0);
	ffSystem_CreateNewNode(system0, 0.0, 0);
	ffSystem_CreateNewNode(system0, 1.0/1.0f, 0);
	
	ffNode_AddNodes(system0, system0->nodes[2], system0->nodes[0],system0->nodes[1]);
	ffNode_SubNodes(system0, system0->nodes[2], system0->nodes[0],system0->nodes[1]);
	ffNode_MulNodes(system0, system0->nodes[2], system0->nodes[0],system0->nodes[1]);
	ffSystem_PrintSystem(system0);
	ffSystem_clear(system0);
}


int main()
{
	TestSystem();
	return 0;
}

#endif

