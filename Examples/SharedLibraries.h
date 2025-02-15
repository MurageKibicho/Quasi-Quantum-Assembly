#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "tensorflow/c/c_api.h"

extern int superMax;

long BinaryToLong(int *binary, int binaryLength);
int BinaryToInt(int *binary, int binaryLength);
long NchooseK(int n, int k);
void PrintArray(int array[], int length);

/*fileio.c*/
int *OpenFile(char *fileName, int *fileLength);
void PrintFileBinary(int *file, int fileLength);

int FindLargestN(double searchLog, int base);
int FindLargestNLogarithm(double searchLog, int base);

/*postings.c*/
int **GenerateLookupTable(int maxValue,int arrayLength);
void DestroyPostings(int **postingsTable, long dataCount);

/*tensor.c*/
typedef struct train *Model;
struct train 
{
 	const char *modelDirectory;
 	const char *tags;
 	
 	TF_Graph *graph;
 	TF_Status *status;
 	TF_SessionOptions *sessionOptions; 
 	TF_Buffer *runOptions;
 	TF_Session *session; 
 	
 	TF_Output *input;
 	TF_Output *output;
 	
 	int numberOfInputs;
	int numberOfOutputs;
	
 	TF_Tensor **inputValues;
 	TF_Tensor **outputValues;
 	
 	float *data;
 	int dataLength;
};

Model CreateModel(const char *modelDirectory, const char *tags, char *inputLayerName, char *outputLayerName, int dataLength);
void Infer(Model model, int *data, int64_t *tensorDimension, int numberOfDimensions);
void FindBest(Model model, int *data, int64_t *tensorDimension, int numberOfDimensions);
void DestroyModel(Model model);
void ModifyArray2(int *array, int arrayLength, int stepSize);
