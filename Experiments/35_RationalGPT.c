#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <gmp.h>
#include <sys/mman.h>
#include <stdbool.h>
#include "Safetensor.h"
#define VOCABULARY_SIZE 50257
#define tf_d_vocab 50257
#define tf_d_seq 1024
#define tf_d_model 768
#define tf_d_k 64
#define tf_n_heads 12
#define tf_n_layers 12
#define tf_rsqrt_d_k 0.125f
#define m32_ENABLE_ERRORS 1
#define m32_TRUE 1
#define m32_FALSE 0
#define PRINT_ERROR(msg) \
do { \
if (m32_ENABLE_ERRORS) { \
    fprintf(stderr, "\033[31mERROR: %s\033[0m\numerator", msg); \
    exit(1); \
} \
} while (0)
typedef struct token_struct *Token;
typedef struct decoder_struct *Decoder;
typedef struct model_parameter_struct *ModelParameters;
typedef struct model_activation_struct *ModelActivations;
struct token_struct
{
	uint32_t offset;
	uint32_t size;
};
struct decoder_struct
{
	Token *tokens;
	char *rawData;
	size_t rawDataLength;
};
struct model_parameter_struct
{
	ModelActivations activations;
	struct{float *weight;size_t length;}wte;
	struct{float *weight;size_t length;}wpe;
	struct
	{
		struct{float *bias; float *weight;size_t biasLength;size_t weightLength;}ln_1;
		struct
		{	
			struct{float *bias; float *weight;size_t biasLength;size_t weightLength;}c_attn;
			struct{float *bias; float *weight;size_t biasLength;size_t weightLength;}c_proj;
		}attn;
		struct{float *bias; float *weight;size_t biasLength;size_t weightLength;}ln_2;
		struct
		{
			struct{float *bias; float *weight;size_t biasLength;size_t weightLength;}c_fc;
			struct{float *bias; float *weight;size_t biasLength;size_t weightLength;}c_proj;
		}mlp;
	}h[12];
	struct{float *bias; float *weight;size_t biasLength;size_t weightLength;}ln_f;
};

struct model_activation_struct
{
	struct{float out[tf_d_seq][tf_d_model];}embedding;
	struct
	{
		struct{float r_std[tf_d_seq];float mean[tf_d_seq];float out[tf_d_seq][tf_d_model];}ln_1;
		struct
		{
			struct{float out[tf_d_seq][3 * tf_d_model];}c_attn;
			struct{float out[tf_n_heads][tf_d_seq][tf_d_seq];}softmax;
			struct{float out[tf_d_seq][tf_d_model];}z;
			struct{float out[12][tf_d_seq][tf_d_seq];}attn;
			struct{float out[tf_d_seq][tf_d_model];}c_proj;
		}attn;
		struct{float out[tf_d_seq][tf_d_model];}res_1;
		struct{float r_std[tf_d_seq];float mean[tf_d_seq];float out[tf_d_seq][tf_d_model];}ln_2;
		struct
		{
			struct{float out[tf_d_seq][4 * tf_d_model];} c_fc;
			struct{float out[tf_d_seq][4 * tf_d_model];}gelu;
			struct{float out[tf_d_seq][tf_d_model];}c_proj;
		}mlp;
		struct{float out[tf_d_seq][tf_d_model];}res_2;
	}h[12];
	struct{float r_std[tf_d_seq];float mean[tf_d_seq];float out[tf_d_seq][tf_d_model];}ln_f;
	struct{float out[tf_d_seq][tf_d_vocab];} unembedding;
};

size_t GetFileSize(char *fileName)
{
	FILE *fp = fopen(fileName, "rb");assert(fp != NULL);
	fseek(fp, 0L, SEEK_END);
	size_t currentFileSize = ftell(fp);rewind(fp);
	fclose(fp);
	return currentFileSize;
}
Decoder LoadTokenDecoder(char *vocabularyFileName)
{
	/*Open the vocabulary file*/
	size_t inputLength = GetFileSize(vocabularyFileName);
	FILE *fp = fopen(vocabularyFileName, "rb");assert(fp != NULL);
	int fileNumber = fileno(fp);
	unsigned char *input = mmap(NULL,inputLength, PROT_READ|PROT_WRITE, MAP_PRIVATE, fileNumber, 0);assert(input != NULL);
	
	Decoder tokenDecoder = malloc(sizeof(struct decoder_struct));
	tokenDecoder->tokens = malloc(VOCABULARY_SIZE * sizeof(Token));
	tokenDecoder->rawDataLength = inputLength - VOCABULARY_SIZE*8;
	tokenDecoder->rawData= malloc(tokenDecoder->rawDataLength * sizeof(unsigned char));
	
	memcpy(tokenDecoder->rawData, input + (VOCABULARY_SIZE * 8), tokenDecoder->rawDataLength * sizeof(unsigned char));
	uint32_t tokenValue = 0;
	uint32_t tokenOffset = 0;
	uint32_t tokenSize   = 0;
	for(size_t i = 0, k = 0; i < VOCABULARY_SIZE*8; i += 8, k+=1)
	{
		tokenDecoder->tokens[k] = malloc(sizeof(struct token_struct));
		tokenDecoder->tokens[k]->offset = 0;
		tokenDecoder->tokens[k]->size = 0;
		for(size_t j = i + 3; j-- > i;)
		{
			tokenDecoder->tokens[k]->offset <<= 8;
			tokenDecoder->tokens[k]->offset += input[j];
			//printf("%u\n", input[j]);
		}
		for(int j = i+7; j >= i + 4; j--)
		{
			tokenSize <<= 8;
			tokenDecoder->tokens[k]->size += input[j];
			//printf("%u\n", input[j]);
		}
		//printf("(%ld %u %u : %.*s)\n", k, tokenDecoder->tokens[k]->offset, tokenDecoder->tokens[k]->size,tokenDecoder->tokens[k]->size, tokenDecoder->rawData + tokenDecoder->tokens[k]->offset);
	}
	assert(munmap(input, inputLength) != -1);
	fclose(fp);
	return tokenDecoder;
}

uint16_t *GetTokenizedData(size_t *tokenCount, char *fileName, Decoder tokenDecoder)
{
	size_t inputLength = GetFileSize(fileName);
	FILE *fp = fopen(fileName, "rb");assert(fp != NULL);
	int fileNumber = fileno(fp);
	unsigned char *input = mmap(NULL,inputLength, PROT_READ|PROT_WRITE, MAP_PRIVATE, fileNumber, 0);assert(input != NULL);
	//printf("%ld\n", inputLength);
	uint16_t *tokenizedData = calloc(inputLength / 2, sizeof(uint16_t));
	for(size_t i = 0, k = 0; i < inputLength; i += 2, k++)
	{
		tokenizedData[k] = 0;
		tokenizedData[k] += input[i+1];
		tokenizedData[k] <<= 8;
		tokenizedData[k] += input[i];
		uint16_t token = tokenizedData[k];
		assert(token < VOCABULARY_SIZE);
		uint32_t offset = tokenDecoder->tokens[token]->offset;
		uint32_t size  = tokenDecoder->tokens[token]->size;
		//printf("(%ld %u %u : %.*s)", k, offset, size,size, tokenDecoder->rawData + offset);
	}
	
	assert(munmap(input, inputLength) != -1);
	fclose(fp);
	return tokenizedData;
}

ModelParameters GetModelParameters(size_t fileSize, unsigned char *fileData)
{
	ModelParameters parameters = malloc(sizeof(struct model_parameter_struct));
	parameters->activations = malloc(sizeof(struct model_activation_struct));
	/*Read HeaderLength(1st 8 bytes in reverse)*/
	size_t headerLength = 0;for(int i = 7; i >= 0; i--){headerLength <<= 8;headerLength += fileData[i];}assert(headerLength >= 0);
	assert(fileData[8] == '{');
	
	cJSON *tensorData = cJSON_ParseWithLength(fileData+8, headerLength);
	assert(tensorData != NULL);
	//Subtract 1 to remove metadata key
	int tensorParameterSize = cJSON_GetArraySize(tensorData)-1;
	assert(tensorParameterSize > 0);
	
	//printf("%ld %d\n", headerLength, tensorParameterSize);
	//char *formatted_json = cJSON_Print(tensorData);if(formatted_json != NULL){printf("%s\n", formatted_json);free(formatted_json);}
	unsigned char  *weightData = (fileData+8+headerLength);
	size_t tensorOffsetStart = 0;
	size_t tensorOffsetEnd   = 0;
	int foundTensor = 0;
	
	foundTensor = GetTensorOffset(tensorData, "wte.weight", &tensorOffsetStart, &tensorOffsetEnd);assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
	parameters->wte.length = (tensorOffsetEnd - tensorOffsetStart) / 4;
	parameters->wte.weight = (float *) (weightData + tensorOffsetStart);
	//printf("%ld %ld : %ld\n", tensorOffsetStart, tensorOffsetEnd, parameters->wte.length);
	foundTensor = GetTensorOffset(tensorData, "wpe.weight", &tensorOffsetStart, &tensorOffsetEnd);assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
	parameters->wpe.length = (tensorOffsetEnd - tensorOffsetStart) / 4;
	parameters->wpe.weight = (float *) (weightData + tensorOffsetStart);
	
	foundTensor = GetTensorOffset(tensorData, "ln_f.bias", &tensorOffsetStart, &tensorOffsetEnd);assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
	parameters->ln_f.biasLength = (tensorOffsetEnd - tensorOffsetStart) / 4;
	parameters->ln_f.bias = (float *) (weightData + tensorOffsetStart);
	
	foundTensor = GetTensorOffset(tensorData, "ln_f.weight", &tensorOffsetStart, &tensorOffsetEnd);assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
	parameters->ln_f.weightLength = (tensorOffsetEnd - tensorOffsetStart) / 4;
	parameters->ln_f.weight = (float *) (weightData + tensorOffsetStart);
	for(int i = 0; i < 12; i++)
	{
		char layerWeightName_ln1[64] = {0};
		char layerBiasName_ln1[64] = {0};
		char layerWeightName_ln2[64] = {0};
		char layerBiasName_ln2[64] = {0};
		
		char layerWeightName_attn_cattn[64] = {0};
		char layerBiasName_attn_cattn[64] = {0};
		char layerWeightName_attn_c_proj[64] = {0};
		char layerBiasName_attn_c_proj[64] = {0};
		
		char layerWeightName_mlp_c_fc[64] = {0};
		char layerBiasName_mlp_c_fc[64] = {0};
		char layerWeightName_mlp_c_proj[64] = {0};
		char layerBiasName_mlp_c_proj[64] = {0};
		
		snprintf(layerWeightName_ln1, sizeof(layerWeightName_ln1), "h.%d.ln_1.weight", i);
		snprintf(layerBiasName_ln1, sizeof(layerBiasName_ln1), "h.%d.ln_1.bias", i);
		snprintf(layerWeightName_ln2, sizeof(layerWeightName_ln2), "h.%d.ln_2.weight", i);
		snprintf(layerBiasName_ln2, sizeof(layerBiasName_ln2), "h.%d.ln_2.bias", i);
		
		snprintf(layerWeightName_attn_cattn, sizeof(layerWeightName_attn_cattn), "h.%d.attn.c_attn.weight", i);
		snprintf(layerBiasName_attn_cattn, sizeof(layerBiasName_attn_cattn), "h.%d.attn.c_attn.bias", i);
		snprintf(layerWeightName_attn_c_proj, sizeof(layerWeightName_attn_c_proj), "h.%d.attn.c_proj.weight", i);
		snprintf(layerBiasName_attn_c_proj, sizeof(layerBiasName_attn_c_proj), "h.%d.attn.c_proj.bias", i);
		
		snprintf(layerWeightName_mlp_c_fc, sizeof(layerWeightName_mlp_c_fc), "h.%d.mlp.c_fc.weight", i);
		snprintf(layerBiasName_mlp_c_fc, sizeof(layerBiasName_mlp_c_fc), "h.%d.mlp.c_fc.bias", i);
		snprintf(layerWeightName_mlp_c_proj, sizeof(layerWeightName_mlp_c_proj), "h.%d.mlp.c_proj.weight", i);
		snprintf(layerBiasName_mlp_c_proj, sizeof(layerBiasName_mlp_c_proj), "h.%d.mlp.c_proj.bias", i);
		
		//printf("%s\n",layerName);
		foundTensor = GetTensorOffset(tensorData, layerWeightName_ln1, &tensorOffsetStart, &tensorOffsetEnd);assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
		parameters->h[i].ln_1.weightLength = (tensorOffsetEnd - tensorOffsetStart) / 4;
		parameters->h[i].ln_1.weight = (float *) (weightData + tensorOffsetStart);
	
		foundTensor = GetTensorOffset(tensorData, layerBiasName_ln1, &tensorOffsetStart, &tensorOffsetEnd);assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
		parameters->h[i].ln_1.biasLength = (tensorOffsetEnd - tensorOffsetStart) / 4;
		parameters->h[i].ln_1.bias = (float *) (weightData + tensorOffsetStart);
		
		foundTensor = GetTensorOffset(tensorData, layerWeightName_ln2, &tensorOffsetStart, &tensorOffsetEnd);assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
		parameters->h[i].ln_2.weightLength = (tensorOffsetEnd - tensorOffsetStart) / 4;
		parameters->h[i].ln_2.weight = (float *) (weightData + tensorOffsetStart);
	
		foundTensor = GetTensorOffset(tensorData, layerBiasName_ln2, &tensorOffsetStart, &tensorOffsetEnd);assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
		parameters->h[i].ln_2.biasLength = (tensorOffsetEnd - tensorOffsetStart) / 4;
		parameters->h[i].ln_2.bias = (float *) (weightData + tensorOffsetStart);
		
		foundTensor = GetTensorOffset(tensorData, layerWeightName_attn_cattn, &tensorOffsetStart, &tensorOffsetEnd);assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
		parameters->h[i].attn.c_attn.weightLength = (tensorOffsetEnd - tensorOffsetStart) / 4;
		parameters->h[i].attn.c_attn.weight = (float *) (weightData + tensorOffsetStart);
		
		foundTensor = GetTensorOffset(tensorData, layerBiasName_attn_cattn, &tensorOffsetStart, &tensorOffsetEnd);assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
		parameters->h[i].attn.c_attn.biasLength = (tensorOffsetEnd - tensorOffsetStart) / 4;
		parameters->h[i].attn.c_attn.bias = (float *) (weightData + tensorOffsetStart);
		
		foundTensor = GetTensorOffset(tensorData, layerWeightName_attn_c_proj, &tensorOffsetStart, &tensorOffsetEnd);assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
		parameters->h[i].attn.c_proj.weightLength = (tensorOffsetEnd - tensorOffsetStart) / 4;
		parameters->h[i].attn.c_proj.weight = (float *) (weightData + tensorOffsetStart);
		
		foundTensor = GetTensorOffset(tensorData, layerBiasName_attn_c_proj, &tensorOffsetStart, &tensorOffsetEnd);assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
		parameters->h[i].attn.c_proj.biasLength = (tensorOffsetEnd - tensorOffsetStart) / 4;
		parameters->h[i].attn.c_proj.bias = (float *) (weightData + tensorOffsetStart);
		
		
		foundTensor = GetTensorOffset(tensorData, layerWeightName_mlp_c_fc, &tensorOffsetStart, &tensorOffsetEnd);assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
		parameters->h[i].mlp.c_fc.weightLength = (tensorOffsetEnd - tensorOffsetStart) / 4;
		parameters->h[i].mlp.c_fc.weight = (float *) (weightData + tensorOffsetStart);
		
		foundTensor = GetTensorOffset(tensorData, layerBiasName_mlp_c_fc, &tensorOffsetStart, &tensorOffsetEnd);assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
		parameters->h[i].mlp.c_fc.biasLength = (tensorOffsetEnd - tensorOffsetStart) / 4;
		parameters->h[i].mlp.c_fc.bias = (float *) (weightData + tensorOffsetStart);
		
		foundTensor = GetTensorOffset(tensorData, layerWeightName_mlp_c_proj, &tensorOffsetStart, &tensorOffsetEnd);assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
		parameters->h[i].mlp.c_proj.weightLength = (tensorOffsetEnd - tensorOffsetStart) / 4;
		parameters->h[i].mlp.c_proj.weight = (float *) (weightData + tensorOffsetStart);
		
		foundTensor = GetTensorOffset(tensorData, layerBiasName_mlp_c_proj, &tensorOffsetStart, &tensorOffsetEnd);assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
		parameters->h[i].mlp.c_proj.biasLength = (tensorOffsetEnd - tensorOffsetStart) / 4;
		parameters->h[i].mlp.c_proj.bias = (float *) (weightData + tensorOffsetStart);
	}
	cJSON_Delete(tensorData);
	return parameters;
}
void FloatToRational(double floatValue, int64_t maxDenominator, int64_t *numeratorHolder, int64_t *denominatorHolder)
{
	int64_t a, h[3] = { 0, 1, 0 }, k[3] = { 1, 0, 0 };
	int64_t x, denominator, numerator = 1;int sign = 0;
	if(maxDenominator <= 1){ *denominatorHolder = 1; *numeratorHolder = (int64_t) floatValue; return; }
	if(floatValue < 0){ sign = 1; floatValue = -floatValue;}
	while (floatValue != floor(floatValue)){numerator <<= 1; floatValue *= 2;}
	denominator = floatValue;
	for(int i = 0; i < 64; i++) 
	{
		a = numerator ? denominator / numerator : 0;if(i && !a){break;}
		x = denominator; denominator = numerator; numerator = x % numerator;x = a;
		if(k[1] * a + k[0] >= maxDenominator){x = (maxDenominator - k[0]) / k[1];if(x * 2 >= a || k[1] >= maxDenominator){i = 65;}else{break;}}
		h[2] = x * h[1] + h[0]; h[0] = h[1]; h[1] = h[2];
		k[2] = x * k[1] + k[0]; k[0] = k[1]; k[1] = k[2];
	}
	*denominatorHolder = k[1];
	*numeratorHolder = sign ? -h[1] : h[1];
}

void TestOutput(size_t length, float *output, char *layerName){double total = 0.0f;for(size_t i = 0; i < length; i++){total += (double)output[i];}printf("Test %s : %f\n",layerName, total);}

int main()
{
	char *vocabularyFileName = "../Datasets/GPT2Tensors/enc";
	char *textFileName = "../Datasets/GPT2Tensors/data";
	char *safeTensorFileName = "../Datasets/GPT2Tensors/model.safetensors";
	
	size_t tokenCount = 0;
	Decoder tokenDecoder = LoadTokenDecoder(vocabularyFileName);
	uint16_t *tokenizedData = GetTokenizedData(&tokenCount, textFileName, tokenDecoder);
	size_t safeTensorDataSize = 0;
	unsigned char *safeTensorData = LoadSafeTensorData(safeTensorFileName, &safeTensorDataSize);
	ModelParameters parameters = GetModelParameters(safeTensorDataSize, safeTensorData);
	
	/*Forward Pass*/
	
	/*Embedding*/
	size_t inputSize = 64;
	for(size_t i = 0; i < inputSize; i++)
	{
		uint16_t token = tokenizedData[i];
		for(size_t j = 0; j < tf_d_model; j++)
		{
			parameters->activations->embedding.out[i][j] = parameters->wte.weight[token * tf_d_model + j] + parameters->wpe.weight[i * tf_d_model + j];
		}
	}
	TestOutput(inputSize * tf_d_model, (float *) parameters->activations->embedding.out, "Embedding");
	
	free(parameters->activations);
	free(parameters);
	assert(munmap(safeTensorData, safeTensorDataSize) != -1);
	free(tokenizedData);
	free(tokenDecoder->rawData);
	for(size_t i = 0; i < VOCABULARY_SIZE; i++){free(tokenDecoder->tokens[i]);}
	free(tokenDecoder->tokens);
	free(tokenDecoder);
	return 0;
}
