#ifndef FF_ASM_RUNTIME_H
#define FF_ASM_RUNTIME_H
#include <stdint.h>
#include <gmp.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "ff_asm_primes.h"

#define FF_ASM_ENABLE_ERRORS 1
#define PRINT_ERROR(msg) \
    do { \
        if (FF_ASM_ENABLE_ERRORS) { \
            fprintf(stderr, "\033[31mERROR: %s\033[0m\n", msg); \
            exit(1); \
        } \
    } while (0)

enum ff_asmDataTypeEnum
{
	Float_32_Field,  Double_64_Field,  
	Int_8_Field,  Int_16_Field,  Int_32_Field, Int_64_Field,
	Int_8_U_Field,Int_16_U_Field,Int_32_U_Field,Int_64_U_Field,
	ff_asmDataType_LENGTH
};
const char *ff_asmDataTypeNames[] = 
{
	"Float_32_Field",  "Double_64_Field",  
	"Int_8_Field",  "Int_16_Field",  "Int_32_Field", "Int_64_Field",
	"Int_8_U_Field","Int_16_U_Field","Int_32_U_Field","Int_64_U_Field",
	"ff_asmDataTypeName_LENGTH"
};

const size_t ff_asmDataTypeSize[] = 
{
	sizeof(float),  sizeof(double),  
	sizeof(int8_t),  sizeof(int16_t),  sizeof(int32_t), sizeof(int64_t),
	sizeof(uint8_t),  sizeof(uint16_t),  sizeof(uint32_t),  sizeof(uint64_t),
	-1
};

typedef struct ff_asm_field_struct *ff_asmField;
typedef struct ff_asm_node_struct *ff_asmNode;

struct ff_asm_node_struct 
{
	uint64_t dataLength;
	uint64_t *data;
	uint8_t dataType;
	mpz_t integer;
	mpz_t integerBias;
	ff_asmNode next;
};

struct ff_asm_field_struct
{
	int nodeCount;
	int bias;
	uint8_t dataType;
	size_t fieldLength;
	uint64_t *fieldOrder;
	ff_asmNode dataHolder;
};

ff_asmNode ff_asmCreateNode(uint64_t dataLength, uint8_t type)
{
	ff_asmNode newNode = malloc(sizeof(struct ff_asm_node_struct));
	if(newNode == NULL){PRINT_ERROR("FFASM CreateNode Error: ff_asmNode is NULL");}
	if(type >= ff_asmDataType_LENGTH)  {PRINT_ERROR("FFASM CreateNode Error: unsupported data type.");}
	newNode->dataLength = dataLength;
	newNode->data       = calloc(dataLength, sizeof(uint64_t));
	newNode->dataType   = type;
	mpz_init(newNode->integer);
	mpz_init(newNode->integerBias);
	mpz_set_ui(newNode->integer, 0);
	mpz_set_ui(newNode->integerBias, 0);
	newNode->next = NULL;
	return newNode;
}

ff_asmField ff_asmMalloc(size_t fieldLength, uint8_t dataType)
{
	if(dataType != Int_8_U_Field){PRINT_ERROR("FFASM AppendData Error: Only Int_8_U_Field supported at the moment.");}
	if(dataType >= ff_asmDataType_LENGTH){PRINT_ERROR("FFASM Malloc Error: unsupported data type.");}
	
	ff_asmField field = malloc(sizeof(struct ff_asm_node_struct));
	if(field == NULL){PRINT_ERROR("FFASM Malloc Error: failed to allocate memory for the field structure.");}

	field->nodeCount    = 0;
	field->bias         = 0;
	field->dataType     = dataType;
	field->fieldLength  = fieldLength;
	field->fieldOrder   = calloc(fieldLength, sizeof(uint64_t));
	field->dataHolder   = NULL;

	if (field->fieldLength > 5238) {PRINT_ERROR("FFASM Malloc Error: Insufficient prime numbers.");}
	for(size_t i = 0; i < field->fieldLength; i++)
	{
		field->fieldOrder[i] = (uint64_t) first5239[i];
	}
	if(field->fieldOrder == NULL) {PRINT_ERROR("FFASM Malloc Error: failed to allocate memory for the field->fieldOrder structure.");}
	return field;
}

void ff_asmSetFieldOrder(ff_asmField field, size_t fieldLength, uint64_t *fieldOrder)
{
	for(size_t i = 0; i < field->fieldLength && i < fieldLength; i++)
	{
		field->fieldOrder[i] = (uint64_t) fieldOrder[i];
	}
}

void ff_asmChineseRemainderTheorem(size_t fieldOrder, uint64_t *finiteField, uint64_t *mods, mpz_t result)
{
	/*Chinese Remainder Theorem*/
	mpz_set_ui(result,0);
	mpz_t productHolder; mpz_init(productHolder);mpz_set_ui(productHolder, 1);
	mpz_t modularInverseHolder; mpz_init(modularInverseHolder);
	mpz_t temporary0; mpz_init(temporary0);
	mpz_t temporary1; mpz_init(temporary1);
	
	for(size_t i = 0; i < fieldOrder; i++)
	{
		assert(finiteField[i] > 0);
		mpz_mul_ui(productHolder, productHolder, finiteField[i]);
	}
	
	for(size_t i = 0; i < fieldOrder; i++)
	{
		mpz_set_ui(temporary0, finiteField[i]);
		mpz_divexact(temporary1, productHolder, temporary0);
		mpz_invert(modularInverseHolder, temporary1, temporary0);
		mpz_mul(temporary1, temporary1,modularInverseHolder);
		mpz_mul_ui(temporary0, temporary1, mods[i]);
		mpz_add(result,result, temporary0);
	}
	mpz_mod(result, result,productHolder);
	mpz_clear(temporary0);
	mpz_clear(temporary1);
	mpz_clear(productHolder);
	mpz_clear(modularInverseHolder);
}


void ff_asmAppendData(ff_asmField field, size_t dataLength, void *data, uint8_t dataType)
{
	if(field == NULL){PRINT_ERROR("FFASM AppendData Error: field is NULL.");}
	if(data == NULL){PRINT_ERROR("FFASM AppendData Error: data is NULL.");}
	if(dataType >= ff_asmDataType_LENGTH){PRINT_ERROR("FFASM AppendData Error: unsupported data type.");}
	
	
	ff_asmNode newNode = ff_asmCreateNode(dataLength, dataType);
	if(newNode == NULL){PRINT_ERROR("FFASM AppendData Error: failed to create a new node.");}
	switch(dataType)
	{
		case Int_8_U_Field:
		{
			uint8_t *inputArray = (uint8_t *)data;
			for(size_t i = 0; i < dataLength; i++)
			{
				newNode->data[i] = (uint8_t)(inputArray[i]);
			}
			ff_asmChineseRemainderTheorem(dataLength, field->fieldOrder, newNode->data, newNode->integer);
			break;
		}
		default:
		{
			PRINT_ERROR("FFASM Append Error: unsupported data type.");
			return;
		}
	}
	if(field->dataHolder == NULL)
	{
		field->dataHolder = newNode;
	}
	else
	{
		ff_asmNode current = field->dataHolder;
		while(current->next != NULL) 
		{
			current = current->next;
		}
		current->next = newNode;
	}
	field->nodeCount += 1;
}

void ff_asmPrintNode(ff_asmNode node)
{
	if(node == NULL){printf("Node is NULL.\n");return;}

	printf("\tNode Data Type: %s\n", ff_asmDataTypeNames[node->dataType]);
	printf("\tNode Data Length: %lu\n\tNode Data : ", node->dataLength);

	// Print the data based on its type
	size_t dataSize = ff_asmDataTypeSize[node->dataType];
	switch (node->dataType)
	{
		case Int_8_U_Field:
		{
			for(size_t i = 0; i < node->dataLength; i++)
			{
				printf("%3lu, ", node->data[i]);
			}
			break;
		}
	}
	printf("\n\t");
	gmp_printf("Node integer : %Zd\n\n", node->integer);
}

void ff_asmPrintField(ff_asmField field)
{
	if(field == NULL){printf("Field is NULL.\n");return;}

	printf("Field Data Type: %s\n", ff_asmDataTypeNames[field->dataType]);
	printf("Field Length: %lu\n", field->fieldLength);
	printf("Field Node Count: %d\n", field->nodeCount);

	printf("Field Order : ");
	for(size_t i = 0; i < field->fieldLength; i++) {printf("%lu ", field->fieldOrder[i]);}
	printf("\n");

	ff_asmNode current = field->dataHolder;
	while(current != NULL)
	{
		ff_asmPrintNode(current);
		current = current->next;
	}
}

void ff_asmFreeNode(ff_asmNode node)
{
	if(node == NULL){return;}
	if(node->data != NULL){free(node->data);node->data = NULL;}

	mpz_clear(node->integer);
	mpz_clear(node->integerBias);

	free(node);
}

void ff_asmFreeField(ff_asmField field)
{
	if(field == NULL){return;}

	ff_asmNode current = field->dataHolder;
	ff_asmNode next;
	while (current != NULL)
	{
		next = current->next;
		ff_asmFreeNode(current);
		current = next;
	}

	if(field->fieldOrder != NULL)
	{
		free(field->fieldOrder);
		field->fieldOrder = NULL;
	}

	free(field);
}

#endif // FF_ASM_RUNTIME_H
