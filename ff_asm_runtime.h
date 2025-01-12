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
	uint64_t *data;
	mpz_t integer;
	mpz_t integerBias;
	ff_asmNode next;
};

struct ff_asm_field_struct
{
	int nodeCount;
	int bias;
	int type;
	uint64_t *primes;
};
#endif // FF_ASM_RUNTIME_H
