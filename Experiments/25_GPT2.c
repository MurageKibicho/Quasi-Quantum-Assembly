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
enum SAFETENSORS_DTYPES
{
	SAFETENSORS_F64 = 0,
	SAFETENSORS_F32,
	SAFETENSORS_F16,
	SAFETENSORS_BF16,
	SAFETENSORS_I64,
	SAFETENSORS_I32,
	SAFETENSORS_I16,
	SAFETENSORS_I8,
	SAFETENSORS_U8,
	SAFETENSORS_BOOL,
	SAFETENSORS_NUM_DTYPES
};
char dataTypes_String_Safetensors[SAFETENSORS_NUM_DTYPES][20] ={"F64","F32","F16","BF16","I64","I32","I16","I8","U8","BOOL"};
int GetSafetensorSize(int dtype)
{
	switch(dtype)
	{
		case SAFETENSORS_F64:  return 8;
		case SAFETENSORS_F32:  return 4;
		case SAFETENSORS_F16:  return 2;
		case SAFETENSORS_BF16: return 2;
		case SAFETENSORS_I64:  return 8;
		case SAFETENSORS_I32:  return 4;
		case SAFETENSORS_I16:  return 2;
		case SAFETENSORS_I8:   return 1;
		case SAFETENSORS_U8:   return 1;
		case SAFETENSORS_BOOL: return 1; 
	}
	return 0;
}
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

struct ln
{
	float *in;
	float *weight;
	float *bias;
	float *out;
	size_t in_count;
	size_t sample_count;
};
void ln(struct ln *ln)
{
	for(size_t i = 0; i < ln->sample_count; i++)
	{
		float *input = ln->in + i * ln->in_count;
		float *inputEnd = input + ln->in_count;
		float *inputReset = input;
		float mean = 0.0f;
		for(; input != inputEnd; input++)
		{
			mean += *input;
		}
		mean /= ln->in_count;
		float total_diff_sq = 0.0f;
		for(input = inputReset; input != inputEnd; input++)
		{
			float diff = *input - mean;
			total_diff_sq += diff * diff;
		}
		float r_stddev = 1.0f / sqrtf(total_diff_sq / ln->in_count);
		//printf("%.3f %.3f\n", r_stddev, mean);
		float *output = ln->out + i * ln->in_count;
		float *weight = ln->weight;
		float *bias   = ln->bias;	
		for(input = inputReset; input != inputEnd; input++, weight++, bias++, output++)
		{
			float in_norm = (*input - mean) * r_stddev;
			*output = in_norm * *weight + *bias;
		}
	}
}

struct fc
{
	float *in;
	float *weight;
	float *bias;
	float *out;
	size_t in_count;
	size_t out_count;
	size_t sample_count;
};

void fc(struct fc *fc)
{
	for(size_t i = 0; i < fc->sample_count; i++)
	{
		float *in = fc->in + i * fc->in_count;
		float *weight = fc->weight;
		float *weightEnd = weight + fc->in_count * fc->out_count;
		float *bias   = fc->bias;
		float *out = fc->out + i * fc->out_count;
		float *outEnd = out + fc->out_count;
		float *outReset = out;
		memcpy(out, bias, fc->out_count * sizeof(float));
		while(true)
		{
			*out += *weight * *in;
			weight++;
			out++;
			if(out == outEnd)
			{
				out = outReset;
				in++;
				if(weight == weightEnd)
				{
					break;
				}
			}
		}
	}
}



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
		float *wteRow  = parameters->wte.weight + token * tf_d_model;
		float *wpeRow  = parameters->wpe.weight + i * tf_d_model;
		float *output  = (float *)parameters->activations->embedding.out + i * tf_d_model;
		float *outputEnd = output + tf_d_model;
		for(; output!=outputEnd; wteRow++,wpeRow++,output++)
		{
			*output = *wteRow + *wpeRow;
		}
	}
	
	double total = 0.0f;
	for(size_t i = 0; i < inputSize * tf_d_model; i++)
	{
		total += (double) ((float *) parameters->activations->embedding.out)[i];
	}
	printf("Test embedding : %f\n",total);
	
	/*Layer norm*/
	for(int layer_i = 0; layer_i < 12; layer_i++)
	{
		float *layer_in = (layer_i == 0) ? (float *) parameters->activations->embedding.out :  (float *) parameters->activations->h[layer_i - 1].res_2.out;
		
		ln(&(struct ln)
		{
		.in = layer_in,
		.in_count = tf_d_model,
		.weight = (float *) parameters->h[layer_i].ln_1.weight,
		.bias = (float *) parameters->h[layer_i].ln_1.bias,
		.out = (float *) parameters->activations->h[layer_i].ln_1.out,
		.sample_count = inputSize
		});
		total = 0.0f;
		for(size_t i = 0; i < inputSize * tf_d_model; i++)
		{
			total += (double) ((float *) parameters->activations->h[layer_i].ln_1.out)[i];
		}
		printf("Test layer norm : %f\n",total);
		
		fc(&(struct fc)
		{
		.in = (float *) parameters->activations->h[layer_i].ln_1.out,
		.weight = parameters->h[layer_i].attn.c_attn.weight,
		.bias = parameters->h[layer_i].attn.c_attn.bias,
		.out = (float *) parameters->activations->h[layer_i].attn.c_attn.out,
		.in_count = tf_d_model,
		.out_count = 3 * tf_d_model,
		.sample_count = inputSize
		});
		total = 0.0f;
		for(size_t i = 0; i < inputSize * 3 * tf_d_model; i++)
		{
			total += (double) ((float *) parameters->activations->h[layer_i].attn.c_attn.out)[i];
		}
		printf("Test Attention : %f %ld\n",total, parameters->h[layer_i].attn.c_attn.weightLength);
		//exit(1);
		/*Heads*/
		memset(parameters->activations->h[layer_i].attn.z.out, 0, sizeof(parameters->activations->h[layer_i].attn.z.out));
		for(size_t head_i = 0; head_i < 12; head_i++)
		{
			for(size_t q_i = 0; q_i < inputSize; q_i++)
			{
				float softmax_max = -INFINITY;
				for(size_t k_i = 0; k_i <= q_i; k_i++)
				{
					float *q = (float *)parameters->activations->h[layer_i].attn.c_attn.out + q_i * 3 * tf_d_model + head_i * tf_d_k;
					float *q_end = q + tf_d_k;
					float *k = (float *)parameters->activations->h[layer_i].attn.c_attn.out + k_i * 3 * tf_d_model + tf_d_model + head_i * tf_d_k;
					float dot = 0.0f;
					for(; q!= q_end; q++, k++)
					{
						dot += *q * *k;
					}
					dot *= tf_rsqrt_d_k;
					parameters->activations->h[layer_i].attn.attn.out[head_i][q_i][k_i] = dot;
					if(dot > softmax_max)
					{
						softmax_max = dot;
					}
					
				}
				float softmax_sum = 0.0f;
				for(size_t k_i = 0; k_i <= q_i; k_i++)
				{
					float e = parameters->activations->h[layer_i].attn.attn.out[head_i][q_i][k_i];
					float softmax_exp_i = expf(e - softmax_max);
					parameters->activations->h[layer_i].attn.softmax.out[head_i][q_i][k_i] = softmax_exp_i;
					softmax_sum += softmax_exp_i;
				}
				float r_softmax_sum = 1.0f / softmax_sum;
				for(size_t k_i = 0; k_i <= q_i; k_i++)
				{
					parameters->activations->h[layer_i].attn.softmax.out[head_i][q_i][k_i] *= r_softmax_sum;
				}
				
				for(size_t v_i = 0; v_i <= q_i; v_i++)
				{
					float *v = (float *)parameters->activations->h[layer_i].attn.c_attn.out + v_i * 3 * tf_d_model + 2 * tf_d_model + head_i * tf_d_k;
					float *v_end = v + tf_d_k;
					float *z = (float *)parameters->activations->h[layer_i].attn.z.out + q_i * tf_d_model + head_i * tf_d_k;
					float factor = parameters->activations->h[layer_i].attn.softmax.out[head_i][q_i][v_i];
					for(; v != v_end; v++, z++)
					{
						*z += *v * factor;
					}
				}	
			}
		}
		total = 0.0f;
		for(size_t i = 0; i < inputSize * tf_d_model; i++)
		{
			total += (double) ((float *) parameters->activations->h[layer_i].attn.z.out)[i];
		}
		printf("Test Head : %f \n",total);
		
		fc(&(struct fc)
		{
		.in = (float *) parameters->activations->h[layer_i].attn.z.out,
		.weight = parameters->h[layer_i].attn.c_proj.weight,
		.bias = parameters->h[layer_i].attn.c_proj.bias,
		.out = (float *) parameters->activations->h[layer_i].attn.c_proj.out,
		.in_count = tf_d_model,
		.out_count = tf_d_model,
		.sample_count = inputSize
		});
		total = 0.0f;
		for(size_t i = 0; i < inputSize * tf_d_model; i++)
		{
			total += (double) ((float *) parameters->activations->h[layer_i].attn.c_proj.out)[i];
		}
		printf("Test FC2 : %f \n",total);
		
		/*Residual layer*/
		float *in_1 = layer_in;
		float *in_2 = (float *)parameters->activations->h[layer_i].attn.c_proj.out ;
		float *out  = (float *)parameters->activations->h[layer_i].res_1.out ;
		float *out_end = out + inputSize *tf_d_model;
		for(; out != out_end; in_1++, in_2++, out++)
		{	
			*out = *in_1 + *in_2;
		}
		total = 0.0f;
		for(size_t i = 0; i < inputSize * tf_d_model; i++)
		{
			total += (double) ((float *) parameters->activations->h[layer_i].res_1.out)[i];
		}
		printf("Test Residual : %f \n",total);
		
		ln(&(struct ln)
		{
		.in = (float *) parameters->activations->h[layer_i].res_1.out,
		.in_count = tf_d_model,
		.weight = (float *) parameters->h[layer_i].ln_2.weight,
		.bias = (float *) parameters->h[layer_i].ln_2.bias,
		.out = (float *) parameters->activations->h[layer_i].ln_2.out,
		.sample_count = inputSize
		});
		total = 0.0f;
		for(size_t i = 0; i < inputSize * tf_d_model; i++)
		{
			total += (double) ((float *) parameters->activations->h[layer_i].ln_2.out)[i];
		}
		printf("Test layer norm : %f\n",total);
		
		fc(&(struct fc)
		{
		.in = (float *) parameters->activations->h[layer_i].ln_2.out,
		.weight = parameters->h[layer_i].mlp.c_fc.weight,
		.bias = parameters->h[layer_i].mlp.c_fc.bias,
		.out = (float *) parameters->activations->h[layer_i].mlp.c_fc.out,
		.in_count = tf_d_model,
		.out_count = 4*tf_d_model,
		.sample_count = inputSize
		});
		total = 0.0f;
		for(size_t i = 0; i < inputSize * 4 * tf_d_model; i++)
		{
			total += (double) ((float *) parameters->activations->h[layer_i].mlp.c_fc.out)[i];
		}
		printf("Test mlp : %f\n",total);
		
		float *in = (float *)parameters->activations->h[layer_i].mlp.c_fc.out;
		out  = (float *)parameters->activations->h[layer_i].mlp.gelu.out ;
		out_end = out + inputSize*4 *tf_d_model;
		
		for(; out != out_end; in++, out++)
		{
			float cdf = 0.5f * (1.0f + erff(*in * (float) M_SQRT1_2));
			*out = *in * cdf;
		}
		total = 0.0f;
		for(size_t i = 0; i < inputSize * 4 * tf_d_model; i++)
		{
			total += (double) ((float *) parameters->activations->h[layer_i].mlp.gelu.out)[i];
		}
		printf("Test gelu : %f\n",total);
		
		fc(&(struct fc)
		{
		.in = (float *) parameters->activations->h[layer_i].mlp.gelu.out,
		.weight = parameters->h[layer_i].mlp.c_proj.weight,
		.bias = parameters->h[layer_i].mlp.c_proj.bias,
		.out = (float *) parameters->activations->h[layer_i].mlp.c_proj.out,
		.in_count = 4*tf_d_model,
		.out_count = tf_d_model,
		.sample_count = inputSize
		});
		total = 0.0f;
		for(size_t i = 0; i < inputSize * 4 * tf_d_model; i++)
		{
			total += (double) ((float *) parameters->activations->h[layer_i].mlp.c_proj.out)[i];
		}
		printf("Test proj : %f\n",total);
		
		in_1 = (float *)parameters->activations->h[layer_i].res_1.out;
		in_2 = (float *)parameters->activations->h[layer_i].mlp.c_proj.out ;
		out  = (float *)parameters->activations->h[layer_i].res_2.out ;
		out_end = out + inputSize *tf_d_model;
		for(; out != out_end; in_1++, in_2++, out++)
		{	
			*out = *in_1 + *in_2;
		}
		total = 0.0f;
		for(size_t i = 0; i < inputSize * tf_d_model; i++)
		{
			total += (double) ((float *) parameters->activations->h[layer_i].res_2.out)[i];
		}
		printf("Test Residual : %f \n",total);
	} 
	
	ln(&(struct ln)
	{
	.in = (float *) parameters->activations->h[11].res_2.out,
	.in_count = tf_d_model,
	.weight = (float *) parameters->ln_f.weight,
	.bias = (float *) parameters->ln_f.bias,
	.out = (float *) parameters->activations->ln_f.out,
	.sample_count = inputSize
	});
	total = 0.0f;
	for(size_t i = 0; i < inputSize * tf_d_model; i++)
	{
		total += (double) ((float *) parameters->activations->ln_f.out)[i];
	}
	printf("Test layer norm : %f\n",total);
	
	for(size_t i = 0; i < inputSize; i++)
	{
		float *in = (float *) parameters->activations->ln_f.out + i * tf_d_model;
		float *in_end = in + tf_d_model;
		float *in_reset = in;
		
		float *weight = parameters->wte.weight;
		float *weight_end = weight + tf_d_vocab * tf_d_model;
		float *out = (float *) parameters->activations->unembedding.out + i * tf_d_vocab;
		float dot = 0.0f;
		while(true)
		{
			dot += *in * *weight;
			in++;
			weight++;
			if(in == in_end)
			{
				in = in_reset;
				*out = dot;
				dot = 0.0f;
				out++;
				if(weight == weight_end)
				{
					break;
				}
			}
		}
	}
	for(size_t i = 0; i < inputSize * tf_d_vocab; i++)
	{
		total += (double) ((float *) parameters->activations->unembedding.out)[i];
	}
	printf("Test output norm : %f\n",total);
	exit(1);
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
