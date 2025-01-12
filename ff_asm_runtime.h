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
	mpz_set_ui(array->integer, 0);
	mpz_set_ui(array->integerBias, 0);
	newNode->next = NULL;
	return newNode;
}

ff_asmField ff_asmMalloc(size_t fieldLength, uint8_t dataType)
{
	if(type >= ff_asmDataType_LENGTH){PRINT_ERROR("FFASM Malloc Error: unsupported data type.");}
	
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
	if (field->fieldOrder == NULL) {PRINT_ERROR("FFASM Malloc Error: failed to allocate memory for the field->fieldOrder structure.");}
	return field;
}


#endif // FF_ASM_RUNTIME_H
