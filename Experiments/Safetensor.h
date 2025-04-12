#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <stdbool.h>
#include "cJSON.h" 
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
char dataTypes_String_Safetensors[SAFETENSORS_NUM_DTYPES][20] =
{
	"F64",
	"F32",
	"F16",
	"BF16",
	"I64",
	"I32",
	"I16",
	"I8",
	"U8",
	"BOOL"
};
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

size_t GetFileSize_SafeTensor(char *fileName)
{
	FILE *fp = fopen(fileName, "rb");assert(fp != NULL);
	fseek(fp, 0L, SEEK_END);
	size_t currentFileSize = ftell(fp);rewind(fp);
	fclose(fp);
	return currentFileSize;
}

unsigned char *LoadSafeTensorData(char *fileName, size_t *fileSizeHolder)
{
	size_t fileSize = GetFileSize_SafeTensor(fileName);
	printf("%ld\n", fileSize);
	FILE *fp = fopen(fileName, "rb");assert(fp != NULL);
	int fileNumber = fileno(fp);
	unsigned char *fileData = mmap(NULL,fileSize, PROT_READ, MAP_PRIVATE, fileNumber, 0);assert(fileData != NULL);
	assert(fileData != MAP_FAILED);
	fclose(fp);
	*fileSizeHolder = fileSize;
	return fileData;
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

void TestSafeTensorLoad()
{
	//char *safeTensorFileName = "../Datasets/GPT2Tensors/model.safetensors";
	char *safeTensorFileName = "../Datasets/Wan/wan.safetensors";
	size_t safeTensorDataSize = 0;
	unsigned char *safeTensorData = LoadSafeTensorData(safeTensorFileName, &safeTensorDataSize);
	assert(safeTensorData != NULL);
	assert(safeTensorData[8] == '{');
	size_t headerLength = 0;for(int i = 7; i >= 0; i--){headerLength <<= 8;headerLength += safeTensorData[i];}assert(headerLength >= 0);

	cJSON *tensorData = cJSON_ParseWithLength(safeTensorData+8, headerLength);
	assert(tensorData != NULL);
	
	//printf("%ld %d\n", headerLength, tensorParameterSize);
	char *formatted_json = cJSON_Print(tensorData);if(formatted_json != NULL){printf("%s\n", formatted_json);free(formatted_json);}
	unsigned char  *weightData = (safeTensorData+8+headerLength);
	
	size_t tensorOffsetStart = 0;
	size_t tensorOffsetEnd   = 0;
	int foundTensor = 0;
	
	foundTensor = GetTensorOffset(tensorData, "blocks.36.ffn.2.bias", &tensorOffsetStart, &tensorOffsetEnd);
	assert(foundTensor == 1);assert(tensorOffsetEnd > tensorOffsetStart);assert((tensorOffsetEnd - tensorOffsetStart) % 4 == 0);
	float *sampleWeights =  (float *) (weightData + tensorOffsetStart);
	cJSON_Delete(tensorData);
	assert(munmap(safeTensorData, safeTensorDataSize) != -1);
}
