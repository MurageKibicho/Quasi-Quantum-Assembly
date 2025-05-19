#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define PI 3.14159265358979323846  
#define MIN_BETA 0.0001
#define MAX_BETA 0.9999
float DiffusionInternalSigmoid(float x)
{
	return 1 / (1 + exp(-x));
}

// Function to generate linearly spaced values between start and end
float *linspace(float start, float end, int arrayLength) 
{
	assert(arrayLength > 0);
	float *result = (float*)malloc(arrayLength * sizeof(float));
	assert(result != NULL);
	if(arrayLength == 1){result[0] = start;return result;}
	float step = (end - start) / (arrayLength - 1);
	for(int i = 0; i < arrayLength; i++)
	{
		result[i] = start + i * step;
	}
	return result;
}

float *LinearBetaSchedule(int timeSteps)
{
	float start = 0.0001;
	float end   = 0.02;
	return linspace(start, end, timeSteps);
}

float *QuadraticBetaSchedule(int timeSteps)
{
	float start = 0.0001;
	float end   = 0.02;
	start = sqrt(start);
	end   = sqrt(end);
	float *betas = linspace(start, end, timeSteps);
	for(int i = 0; i < timeSteps; i++)
	{
		betas[i] = betas[i] * betas[i];
	}
	return betas;
}

float *CosineBetaSchedule(int timeSteps)
{
	float s = 0.008;
	int steps = timeSteps + 1;
	float *temporary = linspace(0, timeSteps, steps);
	float *betas = calloc(timeSteps, sizeof(float));
	// Step 2: Compute alphas_cumprod using the cosine schedule formula
	float *alphasCumProd = calloc(steps, sizeof(float));
	float factor = PI * 0.5 / (1 + s);
	for(int i = 0; i < steps; i++)
	{
		float term = ((temporary[i] / timeSteps) + s) * factor;
		alphasCumProd[i] = pow(cos(term), 2);
	}
 
	// Step 3: Normalize alphas_cumprod by dividing by alphas_cumprod[0]
	float firstAlpha = alphasCumProd[0];
	for(int i = 0; i < steps; i++)
	{
		alphasCumProd[i] /= firstAlpha;
	}

	// Step 4: Compute betas
	for(int i = 1; i < steps; i++)
	{
		float beta = 1 - (alphasCumProd[i] / alphasCumProd[i - 1]);
		// Clip beta to the range [MIN_BETA, MAX_BETA]
		if(beta < MIN_BETA) beta = MIN_BETA;
		if(beta > MAX_BETA) beta = MAX_BETA;
		betas[i - 1] = beta;
	}
	free(temporary);free(alphasCumProd);
	return betas; 
}

float *SigmoidBetaSchedule(int timeSteps)
{
	float start = 0.0001;
	float end   = 0.02;
	float *betas = linspace(-6, 6, timeSteps);
	for(int i = 0; i < timeSteps; i++)
	{
		betas[i] = DiffusionInternalSigmoid(betas[i]) * (end - start) + start;
	}
	return betas;
}
void ComputeDiffusionParameters(int timeSteps, float *betas,
float *alphas, float *alphasCumulativeProduct, float *alphasCumulativeProductPrevious,
float *squareRootReciprocalAlphas, float *squareRootAlphasCumulativeProduct,
float *squareRootMinusOneAlphasCumulativeProduct, float *posteriorVariance
)
{
	//Step 1 : Define alphas
	for(int i = 0; i < timeSteps; i++)
	{
		alphas[i] = 1.0f - betas[i];
		//Avoid division by 0 in later steps
		assert(alphas[i] != 0.0f);
	}
	//Step 2: Compute alphas cumulative product
	alphasCumulativeProduct[0] = alphas[0];
	for(int i = 1; i < timeSteps; i++)
	{
		alphasCumulativeProduct[i] = alphasCumulativeProduct[i-1] * alphas[i];
	}
	//Step 3: Compute alphas cumulative product previous
	alphasCumulativeProductPrevious[0] = 1.0f;
	for(int i = 1; i < timeSteps; i++)
	{
		alphasCumulativeProductPrevious[i] = alphasCumulativeProduct[i-1];
	}
	//Step 4 : Compute squareRoot of the Reciprocal of Alphas
	for(int i = 0; i < timeSteps; i++)
	{
		squareRootReciprocalAlphas[i] = sqrtf(1.0f / alphas[i]);
	}
	//Step 5 : Compute squareRoot of alphasCumulativeProduct
	for(int i = 0; i < timeSteps; i++)
	{
		squareRootAlphasCumulativeProduct[i] = sqrtf(alphasCumulativeProduct[i]);
	}
	//Step 6 : Compute squareRoot of MinusOne AlphasCumulativeProduct
	for(int i = 0; i < timeSteps; i++)
	{
		squareRootMinusOneAlphasCumulativeProduct[i] = sqrtf(1.0f - alphasCumulativeProduct[i]);
	}
	//Step 7 : Compute Posterior Variance
	for(int i = 0; i < timeSteps; i++)
	{
		if(1.0f - alphasCumulativeProduct[i] == 0.0f)
		{
			posteriorVariance[i] = 0.0f;
		}
		else
		{
			posteriorVariance[i] = betas[i] * (1.0f - alphasCumulativeProductPrevious[i]) / (1.0f - alphasCumulativeProduct[i]);
		}
	}
}
float random_uniform(float min, float max) {
    return min + (max - min) * ((float)rand() / RAND_MAX);
}

float random_gaussian(float mean, float std_dev) {
    float u1 = ((float)rand() + 1.0f) / ((float)RAND_MAX + 1.0f);
    float u2 = ((float)rand() + 1.0f) / ((float)RAND_MAX + 1.0f);
    return mean + std_dev * sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);
}

void AddDiffusionParameters(int diffusionStep, int totalTimeSteps, int swissRollLength, float *actualNoise, float *original, float *result, float *squareRootAlphasCumulativeProduct, float *squareRootMinusOneAlphasCumulativeProduct)
{	
	assert(diffusionStep > -1);
	assert(diffusionStep < totalTimeSteps);
	float sqrt_alphas_cumprod_t = squareRootAlphasCumulativeProduct[diffusionStep];
	float sqrt_one_minus_alphas_cumprod_t = squareRootMinusOneAlphasCumulativeProduct[diffusionStep];
	//printf("%.3f %.3f\n", sqrt_alphas_cumprod_t, sqrt_one_minus_alphas_cumprod_t);
	for(int i = 0; i < swissRollLength; i++)
	{
		float noise = random_gaussian(0.0f, 1.0f);
		actualNoise[i] = noise;
		result[i] = sqrt_alphas_cumprod_t * original[i] + sqrt_one_minus_alphas_cumprod_t * noise;
	}
}

void RGBtoYCBCR(unsigned char *r, unsigned char *g, unsigned char *b)
{
	unsigned char y  =0;unsigned char cb =0;unsigned char cr =0;	
	y = 0.299* (*r) + 0.587 * (*g)    + 0.114* (*b);
	cb= 128 - 0.168736* (*r) - 0.331364 * (*g)    + 0.5* (*b);
	cr= 128 + 0.5* (*r)  - 0.418688 * (*g)    - 0.081312* (*b);
	*r = y;*g = cb;*b = cr;
}

void ConvertPixelsToYCBCR(unsigned char *image, int arrayLength){unsigned char r =0;unsigned char g =0;unsigned char b =0;for(int i = 0; i < arrayLength-3; i+=3){r = image[i];g = image[i+1];b = image[i+2];RGBtoYCBCR(&r, &g, &b);image[i]   = r;image[i+1] = g;image[i+2] = b;}}
void YCBCRToRGB(unsigned char *y, unsigned char *cb, unsigned char *cr)
{
	unsigned char r  =0;unsigned char g =0;unsigned char b =0;
	
	float rFloat = 0;float gFloat = 0;float bFloat = 0;	
	rFloat =  (float)(*y) + 1.402 * ((float)*cr - 128);	
	if(rFloat < 0){rFloat = 0;}if(rFloat > 255){rFloat = 255;}
	r = (unsigned char)rFloat;
	
	gFloat = (float)(*y) - 0.34414 * ((float)*cb - 128) - 0.71414 * ((float)*cr - 128);
	if(gFloat < 0){gFloat = 0;}if(gFloat > 255){gFloat = 255;}
	g = (unsigned char)gFloat;
	
	//Handle color conversion problems b
	bFloat = (float)(*y) + 1.772 * ((float)*cb - 128);
	if(bFloat < 0){bFloat = 0;}if(bFloat > 255){bFloat = 255;}
	b = (unsigned char)bFloat;
	*y = r;*cb =g;*cr =b;
}
void ConvertPixelsToRGB(unsigned char *image, int arrayLength){unsigned char y =0;unsigned char cb =0;unsigned char cr =0;for(int i = 0; i < arrayLength-3; i+=3){y = image[i];cb = image[i+1];cr = image[i+2];YCBCRToRGB(&y, &cb, &cr);image[i] = y;image[i+1] = cb;image[i+2] = cr;}}
void GenerateRowSums(int height, int width, unsigned char *matrix, int *result)
{
	for(int i = 0; i < height; i++)
	{
		int sum = 0;
		for(int j = 0; j < width; j++)
		{
			sum += matrix[(i*width) + j];
		}
		result[i]=sum;
	}
}

void GenerateColumnSums(int height, int width, unsigned char *matrix, int *result)
{
	for(int j = 0; j < width; j++)
	{
		int sum = 0;
		for(int i = 0; i < height; i++)
		{
			sum += matrix[(i*width) + j];
		}
		result[j] = sum;
	}
}

float *LoadImage(char *inputFileName, int *heightHolder, int *widthHolder,int *channelHolder)
{
	int imageHeight = 0;int imageWidth = 0;int colorChannels = 0;int imageLength = 0; 
	unsigned char *initialImage = stbi_load(inputFileName,&imageWidth,&imageHeight,&colorChannels,0);
	assert(imageWidth > 0);assert(imageHeight > 0);assert(colorChannels > 0);
	*heightHolder = imageHeight;
	*widthHolder  = imageWidth;
	*channelHolder= colorChannels;
	float *result = calloc(imageHeight * imageWidth * colorChannels, sizeof(float));
	for(int i = 0; i < imageHeight * imageWidth * colorChannels; i++)
	{
		result[i] = initialImage[i];
		result[i] /= 255.0f;
	}
	free(initialImage);
	return result;
}

void SaveImage(char *outputFileName, int imageHeight, int imageWidth, int colorChannels, float *imageFloat)
{
	unsigned char *image = calloc(imageHeight * imageWidth * colorChannels, sizeof(unsigned char));
	for(int i = 0; i < imageHeight * imageWidth * colorChannels; i++)
	{
		//Clamp results to prevent overflow
		imageFloat[i] *= 255.0f;
		if(imageFloat[i] > 255.0f){imageFloat[i] = 255.0f;}
		if(imageFloat[i] < 0.0f){imageFloat[i] = 0.0f;}
		image[i] = (unsigned char) imageFloat[i];
	}
	stbi_write_jpg(outputFileName,imageWidth,imageHeight,colorChannels,image,80);
	free(image);
}


int GetTileElement(int arrayLength, unsigned char *array, int rowCount, int columnCount, int partitionRowCount, int partitionColumnCount, int externalRowIndex, int externalColumnIndex, int internalRowIndex, int internalColumnIndex)
{
	assert(rowCount * columnCount == arrayLength);
	assert(rowCount % partitionRowCount == 0);
	assert(columnCount % partitionColumnCount == 0);
	assert(partitionRowCount <= rowCount);
	assert(partitionColumnCount <= columnCount);
	
	assert(externalRowIndex < rowCount / partitionRowCount);
	assert(externalColumnIndex < columnCount / partitionColumnCount);
	
	assert(internalRowIndex < partitionRowCount);
	assert(internalColumnIndex < partitionColumnCount);
	
	// Calculate the starting row and column of the tile
	int startRow = externalRowIndex * partitionRowCount;
	int startCol = externalColumnIndex * partitionColumnCount;

	// Calculate the absolute row and column in the matrix
	int globalRow = startRow + internalRowIndex;
	int globalCol = startCol + internalColumnIndex;
	
	int index = globalRow * columnCount + globalCol;
	assert(index > -1);
	assert(index < arrayLength);
	return array[index];
}

void TestIndexing()
{
	unsigned char array[] = 
	{
	1,   2,  3,  4,  5,  6,  7,  8,  9,
	10, 11, 12, 13, 14, 15, 16, 17, 18,
	19, 20, 21, 22, 23, 24, 25, 26, 27,
	28, 29, 30, 31, 32, 33, 34, 35, 36,
	37, 38, 39, 40, 41, 42, 43, 44, 45,
	46, 47, 48, 49, 50, 51, 52, 53, 54
	};
	int arrayLength = sizeof(array) / sizeof(unsigned char);
	int rowCount = 6;
	int columnCount = 9;
	int partitionRowCount = 3;
	int partitionColumnCount = 3;
	
	int externalRowIndex = 1;
	int externalColumnIndex = 1;
	int internalRowIndex = 0;
	int internalColumnIndex =0;

	GetTileElement(arrayLength, array, rowCount, columnCount, partitionRowCount, partitionColumnCount, externalRowIndex,externalColumnIndex, internalRowIndex, internalColumnIndex);
	
}

int main()
{
	char *fileName0 = "../Datasets/Article/0.jpg";
	char *fileName1 = "../Datasets/Article/0.jpg";
	char *fileName2 = "../Datasets/Article/0.jpg";
	
	char *fileName0Out = "../Outputs/1.jpg";
	char *fileName1Out = "../Outputs/2.jpg";
	char *fileName2Out = "../Outputs/3.jpg";
	
	int h0,h1,h2=0;int w0,w1,w2=0;int c0,c1,c2=0;
	
	float *image0 = LoadImage(fileName0, &h0, &w0, &c0);
	float *image1 = LoadImage(fileName1, &h1, &w1, &c1);
	float *image2 = LoadImage(fileName2, &h2, &w2, &c2);
	
	float *noise0 = calloc(h0 * w0 * c0, sizeof(float));
	float *noise1 = calloc(h1 * w1 * c1, sizeof(float));
	float *noise2 = calloc(h2 * w2 * c2, sizeof(float));
	
	float *diffusionResult0 = calloc(h0 * w0 * c0, sizeof(float));
	float *diffusionResult1 = calloc(h1 * w1 * c1, sizeof(float));
	float *diffusionResult2 = calloc(h2 * w2 * c2, sizeof(float));
	
	
	int timeSteps = 2000;
	float *betas = LinearBetaSchedule(timeSteps);
	float *alphas = calloc(timeSteps, sizeof(float));
	float *alphasCumulativeProduct = calloc(timeSteps, sizeof(float));
	float *alphasCumulativeProductPrevious = calloc(timeSteps, sizeof(float));
	float *squareRootReciprocalAlphas = calloc(timeSteps, sizeof(float));
	float *squareRootAlphasCumulativeProduct = calloc(timeSteps, sizeof(float));
	float *squareRootMinusOneAlphasCumulativeProduct = calloc(timeSteps, sizeof(float));
	float *posteriorVariance = calloc(timeSteps, sizeof(float));
	ComputeDiffusionParameters(timeSteps, betas,alphas, alphasCumulativeProduct, alphasCumulativeProductPrevious,squareRootReciprocalAlphas,squareRootAlphasCumulativeProduct,squareRootMinusOneAlphasCumulativeProduct, posteriorVariance);
	
	int diffusionStep = 0;
	AddDiffusionParameters(diffusionStep+10, timeSteps, h0 * w0 * c0, noise0, image0, diffusionResult0, squareRootAlphasCumulativeProduct, squareRootMinusOneAlphasCumulativeProduct);
	AddDiffusionParameters(diffusionStep+20, timeSteps, h1 * w1 * c1, noise1, image1, diffusionResult1, squareRootAlphasCumulativeProduct, squareRootMinusOneAlphasCumulativeProduct);
	AddDiffusionParameters(diffusionStep+40, timeSteps, h2 * w2 * c2, noise2, image2, diffusionResult2, squareRootAlphasCumulativeProduct, squareRootMinusOneAlphasCumulativeProduct);
	printf("(%d %d %d)\n(%d %d %d)\n(%d %d %d)\n", h0,w0,c0,h1,w1,c1,h2,w2,c1);
	TestIndexing();

	SaveImage(fileName0Out, h0,w0,c0, diffusionResult0);
	SaveImage(fileName1Out, h1,w1,c1, diffusionResult1);
	SaveImage(fileName2Out, h2,w2,c2, diffusionResult2);
	free(image0);
	free(image1);
	free(image2);
	free(betas);
	free(alphas);
	free(alphasCumulativeProduct);
	free(alphasCumulativeProductPrevious);
	free(squareRootReciprocalAlphas);
	free(squareRootAlphasCumulativeProduct);
	free(squareRootMinusOneAlphasCumulativeProduct);
	free(posteriorVariance);
	free(noise0);
	free(noise1);
	free(noise2);
	free(diffusionResult0);
	free(diffusionResult1);
	free(diffusionResult2);
}
