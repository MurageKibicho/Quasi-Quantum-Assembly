#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <stdbool.h>
#include "cJSON.h" 
#define VOCABULARY_SIZE 50257
#define tf_d_vocab 50257
#define tf_d_seq 1024
#define tf_d_model 768
#define tf_d_k 64
#define tf_n_heads 12
#define tf_n_layers 12
#define tf_rsqrt_d_k 0.125f
//clear && gcc 25_GPT2.c cJSON.c -lm -o m.o && ./m.o
enum SAFETENSORS_DTYPES{SAFETENSORS_F64 = 0,SAFETENSORS_F32,SAFETENSORS_F16,SAFETENSORS_BF16,SAFETENSORS_I64,SAFETENSORS_I32,SAFETENSORS_I16,SAFETENSORS_I8,SAFETENSORS_U8,SAFETENSORS_BOOL,SAFETENSORS_NUM_DTYPES};
char dataTypes_String_Safetensors[SAFETENSORS_NUM_DTYPES][20] ={"F64","F32","F16","BF16","I64","I32","I16","I8","U8","BOOL"};
int GetSafetensorSize(int dtype){switch(dtype){case SAFETENSORS_F64:  return 8;case SAFETENSORS_F32:  return 4;case SAFETENSORS_F16:  return 2;case SAFETENSORS_BF16: return 2;case SAFETENSORS_I64:  return 8;case SAFETENSORS_I32:  return 4;case SAFETENSORS_I16:  return 2;case SAFETENSORS_I8:   return 1;case SAFETENSORS_U8:   return 1;case SAFETENSORS_BOOL: return 1; }return 0;}
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

int GetTensorOffset(cJSON *tensorData, char *tensorName, size_t *tensorStart, size_t *tensorEnd)
{
	int foundTensor = -1;
	cJSON *item = NULL;
	cJSON *offset = NULL;
	cJSON *dtype = NULL;
	cJSON *data_offsets = NULL;
	cJSON *shape = NULL;
	cJSON *eachShape = NULL;
	cJSON_ArrayForEach(item, tensorData)
	{
		dtype = cJSON_GetObjectItem(item, "dtype");data_offsets = cJSON_GetObjectItem(item, "data_offsets");
		shape = cJSON_GetObjectItem(item, "shape");
		if(dtype && data_offsets && shape)
		{
			if(strcmp(tensorName, item->string) == 0)
			{
				//printf("Key: %s\n", item->string);printf("  dtype: %s\n", dtype->valuestring);printf("  data_offsets: ");	
				cJSON_ArrayForEach(offset, data_offsets)
				{
					foundTensor += 1;
					if(foundTensor == 0)
					{
						*tensorStart = (size_t) offset->valuedouble;
					}
					else if(foundTensor == 1)
					{
						*tensorEnd = (size_t) offset->valuedouble;
					}
				}
				break;
			}
		}
	}
	return foundTensor;
}

unsigned char *LoadSafeTensorData(char *fileName, size_t *fileSizeHolder)
{
	size_t fileSize = GetFileSize(fileName);
	FILE *fp = fopen(fileName, "rb");assert(fp != NULL);
	int fileNumber = fileno(fp);
	unsigned char *fileData = mmap(NULL,fileSize, PROT_READ|PROT_WRITE, MAP_PRIVATE, fileNumber, 0);assert(fileData != NULL);
	fclose(fp);
	*fileSizeHolder = fileSize;
	return fileData;
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

struct LayerNorm
{
	float *input;
	float *weight;
	float *bias;
	float *out;
	size_t modelDimension;
	size_t inputLength;
};

void LayerNorm(struct LayerNorm *layer)
{
	size_t modelDimension = layer->modelDimension;
	size_t inputLength = layer->inputLength;
	for(size_t i = 0; i < inputLength; i++)
	{
		//Find mean
		float mean = 0.0f;for(size_t j = 0; j < modelDimension; j++){mean += layer->input[i * modelDimension + j];}mean /= modelDimension;
		//Find variance
		float variance = 0.0f;for(size_t j = 0; j < modelDimension; j++){float difference = layer->input[i * modelDimension + j] - mean;variance += difference * difference;}
		variance /= modelDimension;
		float reciprocalStandardDeviation = 1.0f / sqrtf(variance);
		for(size_t j = 0; j < modelDimension; j++)
		{
			float normalized =(layer->input[i * modelDimension + j] - mean) * reciprocalStandardDeviation;
			layer->out[i * modelDimension + j] = normalized * layer->weight[j] + layer->bias[j];
		}
	}
}

struct fullconvolution
{
	float *input;
	float *weight;
	float *bias;
	float *out;
	size_t inputDimension;
	size_t outputDimension;
	size_t inputLength;
};

void FullConvolution(struct fullconvolution *layer)
{
	size_t batchSize = layer->inputLength;
	size_t inputDimension = layer->inputDimension;
	size_t outputDimension = layer->outputDimension;
	for(size_t b = 0; b < batchSize; b++)
	{
		//Add Bias to output
		for(size_t i = 0; i < outputDimension; i++)
		{
			layer->out[b * outputDimension + i] = layer->bias[i];
		}
		//Matmul
		for(size_t i = 0; i < inputDimension; i++)
		{
			float currentInput = layer->input[b * inputDimension + i];
			for(size_t j = 0; j < outputDimension; j++)
			{
				layer->out[b * outputDimension + j] += currentInput * layer->weight[i * outputDimension + j]; 	
			}
		}
	}
}

struct residual
{
	float *input0;
	float *input1;
	float *output;
	size_t inputDimension;
	size_t inputLength;
};

void Residual(struct residual *layer)
{
	for(size_t i = 0; i < layer->inputLength; i++)
	{
		for(size_t j = 0; j < layer->inputDimension; j++)
		{
			size_t index = i * layer->inputDimension + j;
			layer->output[index] = layer->input0[index] + layer->input1[index];
		}	
	}
}

struct gelu
{
	float *input;
	float *output;
	size_t geluLength;
};

void Gelu(struct gelu *layer)
{
	for(size_t i = 0; i < layer->geluLength; i++)
	{
		float cdf = 0.5f * (1.0f + erff(layer->input[i] * (float) M_SQRT1_2));
		layer->output[i] = layer->input[i] * cdf;	
	}
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
	/*Layer norm*/
	for(int layerIndex = 0; layerIndex < 12; layerIndex++)
	{
		float *currentLayer = (layerIndex == 0) ? (float *) parameters->activations->embedding.out :  (float *) parameters->activations->h[layerIndex - 1].res_2.out;
		LayerNorm(&(struct LayerNorm){.modelDimension = tf_d_model,.inputLength = inputSize,.input = currentLayer,.weight = parameters->h[layerIndex].ln_1.weight,.bias = parameters->h[layerIndex].ln_1.bias,.out = (float *)parameters->activations->h[layerIndex].ln_1.out,});
		TestOutput(inputSize * tf_d_model, (float *) parameters->activations->h[layerIndex].ln_1.out, "Layer Norm 0");
		FullConvolution(&(struct fullconvolution){.inputDimension = tf_d_model,.outputDimension = 3 * tf_d_model,.inputLength = inputSize,.input = (float *) parameters->activations->h[layerIndex].ln_1.out,.weight = parameters->h[layerIndex].attn.c_attn.weight,.bias = parameters->h[layerIndex].attn.c_attn.bias,.out = (float *) parameters->activations->h[layerIndex].attn.c_attn.out,});
		TestOutput(inputSize * 3 * tf_d_model, (float *) parameters->activations->h[layerIndex].attn.c_attn.out, "FC0");

		/*Heads*/
		memset(parameters->activations->h[layerIndex].attn.z.out, 0, sizeof(parameters->activations->h[layerIndex].attn.z.out));
		for(size_t headIndex = 0; headIndex < 12; headIndex++)
		{
			for(size_t queryIndex = 0; queryIndex < inputSize; queryIndex++)
			{
				float softmaxMax = -INFINITY;
				for(size_t keyIndex = 0; keyIndex <= queryIndex; keyIndex++)
				{
					float queryKeyDotProduct = 0.0f;
					for(size_t i = 0; i < tf_d_k; i++)
					{
						float queryValue = ((float*)parameters->activations->h[layerIndex].attn.c_attn.out)[queryIndex * 3 * tf_d_model + headIndex * tf_d_k + i];
						float keyValue   = ((float*)parameters->activations->h[layerIndex].attn.c_attn.out)[keyIndex * 3 * tf_d_model + tf_d_model + headIndex * tf_d_k + i];
						queryKeyDotProduct += queryValue * keyValue;
					}
					queryKeyDotProduct *= tf_rsqrt_d_k;
					parameters->activations->h[layerIndex].attn.attn.out[headIndex][queryIndex][keyIndex] = queryKeyDotProduct;
					softmaxMax = fmaxf(softmaxMax, queryKeyDotProduct);
				}
				float softmaxSum = 0.0f;
				for(size_t keyIndex = 0; keyIndex <= queryIndex; keyIndex++)
				{
					float e = parameters->activations->h[layerIndex].attn.attn.out[headIndex][queryIndex][keyIndex];
					float softmax_exp_i = expf(e - softmaxMax);
					parameters->activations->h[layerIndex].attn.softmax.out[headIndex][queryIndex][keyIndex] = softmax_exp_i;
					softmaxSum += softmax_exp_i;
				}
				float reciprocalSoftmaxSum = 1.0f / softmaxSum;
				for(size_t keyIndex = 0; keyIndex <= queryIndex; keyIndex++){parameters->activations->h[layerIndex].attn.softmax.out[headIndex][queryIndex][keyIndex] *= reciprocalSoftmaxSum;}
				for(size_t valueIndex = 0; valueIndex <= queryIndex; valueIndex++)
				{
					float factor = parameters->activations->h[layerIndex].attn.softmax.out[headIndex][queryIndex][valueIndex];
					for(size_t i = 0; i < tf_d_k; i++)
					{
						size_t valueOffset = valueIndex * 3 * tf_d_model + 2 * tf_d_model + headIndex * tf_d_k + i;
						size_t zOffset = queryIndex * tf_d_model + headIndex * tf_d_k + i;
						float vValue = ((float*)parameters->activations->h[layerIndex].attn.c_attn.out)[valueOffset];
						((float*)parameters->activations->h[layerIndex].attn.z.out)[zOffset] += vValue * factor;
					}
				}	
				
			}
		}
		TestOutput(inputSize * tf_d_model, (float *) parameters->activations->h[layerIndex].attn.z.out, "Heads");
		FullConvolution(&(struct fullconvolution){.inputDimension = tf_d_model,.outputDimension = tf_d_model,.inputLength = inputSize,.input = (float *) parameters->activations->h[layerIndex].attn.z.out,.weight = parameters->h[layerIndex].attn.c_proj.weight,.bias = parameters->h[layerIndex].attn.c_proj.bias,.out = (float *) parameters->activations->h[layerIndex].attn.c_proj.out});
		TestOutput(inputSize * tf_d_model, (float *) parameters->activations->h[layerIndex].attn.c_proj.out, "FC1");
		
		/*Residual layer*/
		Residual(&(struct residual){.inputLength =inputSize,.inputDimension = tf_d_model, .input0=currentLayer, .input1=(float *)parameters->activations->h[layerIndex].attn.c_proj.out,.output=(float *)parameters->activations->h[layerIndex].res_1.out});
		TestOutput(inputSize * tf_d_model, (float *) parameters->activations->h[layerIndex].res_1.out, "Residual0");
		LayerNorm(&(struct LayerNorm){.inputLength = inputSize,.modelDimension = tf_d_model,.input = (float *) parameters->activations->h[layerIndex].res_1.out,.weight = (float *) parameters->h[layerIndex].ln_2.weight,.bias = (float *) parameters->h[layerIndex].ln_2.bias,.out = (float *) parameters->activations->h[layerIndex].ln_2.out,});
		TestOutput(inputSize * tf_d_model, (float *) parameters->activations->h[layerIndex].ln_2.out, "Layer Norm 1");
		FullConvolution(&(struct fullconvolution){.inputDimension = tf_d_model,.outputDimension = 4*tf_d_model,.inputLength = inputSize,.input = (float *) parameters->activations->h[layerIndex].ln_2.out,.weight = parameters->h[layerIndex].mlp.c_fc.weight,.bias = parameters->h[layerIndex].mlp.c_fc.bias,.out = (float *) parameters->activations->h[layerIndex].mlp.c_fc.out,});
		TestOutput(inputSize * 4 * tf_d_model, (float *) parameters->activations->h[layerIndex].mlp.c_fc.out, "FC2"); 
		
		Gelu(&(struct gelu){.geluLength = inputSize*4 *tf_d_model,.input = (float *)parameters->activations->h[layerIndex].mlp.c_fc.out,.output= (float *)parameters->activations->h[layerIndex].mlp.gelu.out});
		TestOutput(inputSize * 4 * tf_d_model, (float *) parameters->activations->h[layerIndex].mlp.gelu.out, "Gelu"); 
		FullConvolution(&(struct fullconvolution){.inputDimension = 4*tf_d_model,.outputDimension = tf_d_model,.inputLength = inputSize,.input = (float *) parameters->activations->h[layerIndex].mlp.gelu.out,.weight = parameters->h[layerIndex].mlp.c_proj.weight,.bias = parameters->h[layerIndex].mlp.c_proj.bias,.out = (float *) parameters->activations->h[layerIndex].mlp.c_proj.out});
		TestOutput(inputSize * 4 * tf_d_model, (float *) parameters->activations->h[layerIndex].mlp.c_proj.out, "FC3"); 

		Residual(&(struct residual){.inputLength =inputSize,.inputDimension = tf_d_model, .input0=(float *)parameters->activations->h[layerIndex].res_1.out, .input1=(float *)parameters->activations->h[layerIndex].mlp.c_proj.out,.output=(float *)parameters->activations->h[layerIndex].res_2.out});
		TestOutput(inputSize * tf_d_model, (float *) parameters->activations->h[layerIndex].res_2.out, "Residual 1");
		
	} 
	LayerNorm(&(struct LayerNorm){.inputLength = inputSize,.modelDimension = tf_d_model,.input = (float *) parameters->activations->h[11].res_2.out,.weight = (float *) parameters->ln_f.weight,.bias = (float *) parameters->ln_f.bias,.out = (float *) parameters->activations->ln_f.out});
	TestOutput(inputSize * tf_d_model, (float *) parameters->activations->ln_f.out, "Layer Norm 3");
	for(size_t i = 0; i < inputSize; i++)
	{
		for(size_t j = 0; j < tf_d_vocab; j++)
		{
			float dot = 0.0f;
			for(size_t k = 0; k < tf_d_model; k++)
			{
				dot += parameters->activations->ln_f.out[i][k] * parameters->wte.weight[j * tf_d_model + k];
			}
			parameters->activations->unembedding.out[i][j] = dot;	
		}
	}
	for(size_t i = 0; i < inputSize; i++)
	{
		for(size_t j = 0; j < tf_d_vocab; j++)
		{
			float dot = 0.0f;
			for(size_t k = 0; k < tf_d_model; k++){dot += ((float*)parameters->activations->ln_f.out)[i * tf_d_model + k] * parameters->wte.weight[j * tf_d_model + k];}
			((float*)parameters->activations->unembedding.out)[i * tf_d_vocab + j] = dot;
		}
	}
	TestOutput(inputSize * tf_d_vocab, (float *) parameters->activations->unembedding.out, "Unembedding");
	float maximumValue = -INFINITY;
	size_t maxIndex = 0;
	float *row = (float *) parameters->activations->unembedding.out + (inputSize - 1) *tf_d_vocab;
	for(size_t s = 0; s < tf_d_vocab; s++)
	{
		if(row[s] > maximumValue)
		{
			maximumValue = row[s];
			maxIndex = s;
		}
	}
	printf("%lu\n",maxIndex);

	printf("%.*s\n",tokenDecoder->tokens[maxIndex]->size,tokenDecoder->rawData + tokenDecoder->tokens[maxIndex]->offset);
	
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
