#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

unsigned char *ReadMnistImages(char *fileName, int *imageCount)
{
	FILE *fp = fopen(fileName, "rb");assert(fp != NULL);
	int numberOfImages = 0;
	int temporaryVariable = 0;
	fread(&temporaryVariable, sizeof(int), 1, fp);
	fread(&numberOfImages, sizeof(int), 1, fp);
	fread(&temporaryVariable, sizeof(int), 1, fp);
	fread(&temporaryVariable, sizeof(int), 1, fp);
	
	numberOfImages = __builtin_bswap32(numberOfImages);
	unsigned char *images = malloc(numberOfImages * 784);
	fread(images, sizeof(unsigned char), numberOfImages * 784, fp);
	fclose(fp);
	*imageCount = numberOfImages;
	return images;
}
unsigned char *ReadMnistLabels(char *fileName, int *labelCount)
{
	FILE *fp = fopen(fileName, "rb");assert(fp != NULL);
	int numberOfLabels = 0;
	fread(&numberOfLabels, sizeof(int), 1, fp);
	fread(&numberOfLabels, sizeof(int), 1, fp);
	numberOfLabels = __builtin_bswap32(numberOfLabels);
	//printf("%3d\n", numberOfLabels);
	
	
	unsigned char *labels = malloc(numberOfLabels * sizeof(unsigned char));
	fread(labels, sizeof(unsigned char), numberOfLabels, fp);
	*labelCount = numberOfLabels ;
	fclose(fp);
	return labels;
}

void ShuffleData(unsigned char *images, unsigned char *labels, int imagesCount)
{
	unsigned char temporaryVariable = 0;
	for(int i = imagesCount - 1; i > 0; i--)
	{
		int j = rand() % (i + 1);
		for(int k = 0; k < 784; k++)
		{
			temporaryVariable = images[i * 784 + k];
			images[i * 784 + k] = images[j * 784 + k];
			images[j * 784 + k] = temporaryVariable;
		}
		temporaryVariable = labels[i];
		labels[i] = labels[j];
		labels[j] = temporaryVariable;
	}
}

/*
int main()
{
	srand(657849);
	char *trainImagesPath = "data/train-images.idx3-ubyte";
	char *trainLabelsPath = "data/train-labels.idx1-ubyte";
	int trainImagesCount = 0;
	int trainLabelsCount = 0;
	unsigned char *trainImages = ReadMnistImages(trainImagesPath, &trainImagesCount);
	unsigned char *trainLabels = ReadMnistLabels(trainLabelsPath, &trainLabelsCount);
	
	assert(trainLabels != NULL);
	assert(trainImagesCount == trainLabelsCount);
	ShuffleData(trainImages, trainLabels, trainImagesCount);
	
	
	int hiddenLayers[] = {256};
	int batchSize = 64;
	int hiddenLayerCount = sizeof(hiddenLayers) / sizeof(int);
	
	
	free(trainLabels);
	free(trainImages);
	return 0;
}*/
