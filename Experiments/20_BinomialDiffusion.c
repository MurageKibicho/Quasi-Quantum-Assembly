#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <gmp.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define PI 3.14159265358979323846  
#define MIN_BETA 0.0001
#define MAX_BETA 0.9999
typedef struct mpz_index_struct *MpzWithIndex;
struct mpz_index_struct
{
	mpz_t binomialCoefficient;
	int n;
	int k;
	int splitIndex;
};
int CompareBinomialCoefficient(const void *a, const void *b)
{
	const MpzWithIndex *mpz_a = (const MpzWithIndex *)a;
	const MpzWithIndex *mpz_b = (const MpzWithIndex *)b;
	return mpz_cmp((*mpz_a)->binomialCoefficient, (*mpz_b)->binomialCoefficient);
}
int CompareSplitIndex(const void *a, const void *b)
{
	const MpzWithIndex *mpz_a = (const MpzWithIndex *)a;
	const MpzWithIndex *mpz_b = (const MpzWithIndex *)b;
	return (*mpz_a)->splitIndex - (*mpz_b)->splitIndex;
}
void FisherYatesShuffle(int arrayLength, int *array){if(arrayLength > 1){for(int i = arrayLength - 1; i > 0; i--){int j = rand() % (i + 1);int temp = array[i];array[i] = array[j];array[j] = temp;}}}
int largestPossible = 1 << 30;
double Find_Log_MPZ_Double(mpz_t x){if(mpz_cmp_ui(x, 0) == 0){return 0;}signed long int ex;const double di = mpz_get_d_2exp(&ex, x);return ( (log(di) + log(2) * (double) ex) /log(2));}
double FileForma_GammaApproximation(double n, double k){double result = 0;result = (lgamma(n+1) -lgamma(n-k+1) -lgamma(k+1)) / log(2);return result;}
int FindLargestNLogarithm(double searchLog, int base){int max = largestPossible;int min = 0;int mid = 0;double currentLog = 0;double k = (double)base;while(min <= max){mid = min + (max - min) / 2;currentLog = FileForma_GammaApproximation((double)mid, k);if(currentLog < searchLog){min = mid + 1;}else{max = mid - 1;}}return max;}
int FindLargestNoLog(int number, int base, mpz_t binomialCoefficient){int currentValue = 0;int k = base;int n = 0;while(currentValue <= number){mpz_bin_uiui(binomialCoefficient, n, k);currentValue = mpz_get_ui(binomialCoefficient);n++;}return n-2;}
void PrintIntArray(int length, int*array){for(int i = 0; i < length; i++){printf("%4d ", array[i]);}printf("\n");}
void PrintFloatArray(int length, float*array){for(int i = 0; i < length; i++){printf("(%3d: %.3f)  ",i, array[i]);}printf("\n");}
void DimensionToInteger(mpz_t binomialCoefficient, mpz_t integer, int oneCount, int *array){assert(oneCount > 0);mpz_set_ui(integer, 0);for(int i = 0; i < oneCount; i++){mpz_bin_uiui(binomialCoefficient, array[i], i+1);mpz_add(integer, integer, binomialCoefficient);}}
void ChangeDimension(mpz_t binomialCoefficient, mpz_t integer, int oneCount, int *array){assert(oneCount > 0);double currentLogValue = 0.0f; int n = 0; int k = oneCount;while(mpz_cmp_ui(integer, 0) >= 0){if(k <= 1){break;}currentLogValue = Find_Log_MPZ_Double(integer);if(currentLogValue > 25.0){n = FindLargestNLogarithm(currentLogValue, k);}else{int number = mpz_get_ui(integer); n = FindLargestNoLog(number, k,binomialCoefficient);}if(n < 0){break;}mpz_bin_uiui(binomialCoefficient, n, k);mpz_sub(integer, integer, binomialCoefficient);array[k-1] = n;k -=1;}if(k == 1){array[k-1] = mpz_get_ui(integer);}}
void PrintMpzWithIndex(int length, MpzWithIndex *array)
{
	for(int i = 0; i < length; i++)
	{
		printf("(%d : %d %d)", array[i]->splitIndex,array[i]->n,array[i]->k);
	}
	printf("\n");
}
int SumMpzWithIndex(int length, MpzWithIndex *array)
{
	int sum = 0;
	for(int i = 0; i < length; i++)
	{
		sum += array[i]->n;
	}
	return sum;
}
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
void TestLinearSpace()
{
	int arrayLength = 40;
	float start = 25.7f;
	float end = 27.9f;
	float* result = linspace(start, end, arrayLength);
	assert(result != NULL);
	for(int i = 0; i < arrayLength; i++)
	{
		printf("%f ", result[i]);
	}
	free(result);
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

void GenerateSwissRoll2D(int arrayLength, float noise, float *timeStep, float *xCoordinates, float *yCoordinates)
{
	for(int i = 0; i < arrayLength; i++)
	{
		timeStep[i] = (3 * M_PI / 2) * (1 + 2 * random_uniform(0, 1));
		xCoordinates[i] = timeStep[i] * cosf(timeStep[i]) + random_gaussian(0, noise);
		yCoordinates[i] = timeStep[i] * sinf(timeStep[i]) + random_gaussian(0, noise);
	}
}

double FindLoss(mpz_t compositeNumber,mpz_t temporary0,mpz_t temporary1,mpz_t temporary2, mpz_t temporary3, int stateLength, MpzWithIndex *diffusionState)
{
	double loss = 0.0f;
	mpz_set_ui(temporary0, 0);mpz_set_ui(temporary1, 0);mpz_set_ui(temporary2, 0);mpz_set_ui(temporary3, 0);
	for(int i = 0; i < stateLength; i++)
	{
		assert(diffusionState[i]->k > 0);
		mpz_bin_uiui(diffusionState[i]->binomialCoefficient, diffusionState[i]->n, diffusionState[i]->k);
		if(diffusionState[i]->splitIndex == 0)
		{
			mpz_add(temporary0, temporary0, diffusionState[i]->binomialCoefficient);
		}
		else if(diffusionState[i]->splitIndex == 1)
		{
			mpz_add(temporary1, temporary1, diffusionState[i]->binomialCoefficient);
		}
		else
		{
			mpz_add(temporary2, temporary2, diffusionState[i]->binomialCoefficient);
		}
	}
	if(mpz_cmp(temporary0, temporary1) < 0)
	{
		mpz_set(temporary3, temporary0);
		mpz_set(temporary0, temporary1);
		mpz_set(temporary1, temporary3);
	}
	if(mpz_cmp(temporary0, temporary2) < 0)
	{
		mpz_set(temporary3, temporary0);
		mpz_set(temporary0, temporary2);
		mpz_set(temporary2, temporary3);
	}
	if(mpz_cmp(temporary1, temporary2) < 0)
	{
		mpz_set(temporary3, temporary1);
		mpz_set(temporary1, temporary2);
		mpz_set(temporary2, temporary3);
	}
	assert(mpz_cmp(temporary0, temporary1) >= 0);
	assert(mpz_cmp(temporary1, temporary2) >= 0);
	mpz_mul(temporary0, temporary0,temporary0);
	mpz_mul(temporary1, temporary1,temporary1);
	mpz_mul(temporary2, temporary2,compositeNumber);
	mpz_sub(temporary0, temporary0, temporary1);
	mpz_sub(temporary0, temporary0, temporary2);
	int zeroCompare = mpz_cmp_ui(temporary0,0);
	if(zeroCompare != 0)
	{
		if(zeroCompare < 0)
		{
			mpz_mul_si(temporary0,temporary0,-1);
			loss = Find_Log_MPZ_Double(temporary0);
			loss *= -1.0f;
		}
		else
		{
			loss = Find_Log_MPZ_Double(temporary0);
		}
	}
	return loss;
}

void CopyCurrentState(int stateLength, MpzWithIndex *diffusionTemp, MpzWithIndex *diffusionState)
{
	for(int i = 0; i < stateLength; i++)
	{
		diffusionTemp[i]->n = diffusionState[i]->n;
		diffusionTemp[i]->k = diffusionState[i]->k;
		diffusionTemp[i]->splitIndex = diffusionState[i]->splitIndex;
	}
}

void AddDiffusionParameters(int diffusionStep, int totalTimeSteps, int swissRollLength, float *actualNoise, float *original, float *result, float *squareRootAlphasCumulativeProduct, float *squareRootMinusOneAlphasCumulativeProduct)
{	
	assert(diffusionStep > -1);
	assert(diffusionStep < totalTimeSteps);
	float sqrt_alphas_cumprod_t = squareRootAlphasCumulativeProduct[diffusionStep];
	float sqrt_one_minus_alphas_cumprod_t = squareRootMinusOneAlphasCumulativeProduct[diffusionStep];
	for(int i = 0; i < swissRollLength; i++)
	{
		float noise = random_uniform(0.0f, 1.0f);
		actualNoise[i] = noise;
		result[i] = sqrt_alphas_cumprod_t * original[i] + sqrt_one_minus_alphas_cumprod_t * noise;
	}
}

void TestMeanVariance(int diffusionStep, int totalTimeSteps, int stateLength, float m0, float v0,float m1, float v1, MpzWithIndex *diffusionTemp, float *squareRootAlphasCumulativeProduct, float *squareRootMinusOneAlphasCumulativeProduct)
{	
	assert(diffusionStep > -1);
	assert(diffusionStep < totalTimeSteps);
	float sqrt_alphas_cumprod_t = squareRootAlphasCumulativeProduct[diffusionStep];
	float sqrt_one_minus_alphas_cumprod_t = squareRootMinusOneAlphasCumulativeProduct[diffusionStep];
	for(int i = 0; i < stateLength; i++)
	{
		float noise0 = random_uniform(m0, v0);
		float noise1 = random_uniform(m1, v1);
		diffusionTemp[i]->n = (int) roundf(sqrt_alphas_cumprod_t * diffusionTemp[i]->n + sqrt_one_minus_alphas_cumprod_t * noise0);
		diffusionTemp[i]->k = (int) roundf(sqrt_alphas_cumprod_t * diffusionTemp[i]->k + sqrt_one_minus_alphas_cumprod_t * noise1);
		//printf("(%d %d)", diffusionTemp[i]->n, diffusionTemp[i]->k);
	}
}
//(7619 2)(8360 5)(13662 7)(21378 11)(25594 12)(37622 20)(47577 23)(49179 25)(51516 27)(8161 3)(9565 6)(16183 8)(16270 9)(21232 10)(30064 15)(32118 18)(37443 19)(49750 26)(6310 1)(8178 4)(29252 13)(29492 14)(30999 16)(31459 17)(43318 21)(46753 22)(49040 24)658.954 658.954
//(4590 1)(5037 3)(8231 4)(12880 7)(15419 7)(22666 12)(28663 14)(29629 16)(31037 17)(4917 2)(5763 4)(9750 5)(9802 6)(12792 7)(18112 9)(19350 11)(22558 12)(29972 16)(3802 1)(4927 3)(17623 8)(17768 9)(18676 10)(18953 11)(26098 13)(28167 13)(29545 15)658.954 -nan

void TestLinearBetas()
{
	int timeSteps = 40;
	float *betas = LinearBetaSchedule(timeSteps);
	assert(betas != NULL);
	for(int i = 0; i < timeSteps; i++)
	{
		printf("%f ", betas[i]);
	}
	free(betas);
}

void TestQuadraticBetas()
{
	int timeSteps = 4;
	float *betas = QuadraticBetaSchedule(timeSteps);
	assert(betas != NULL);
	for(int i = 0; i < timeSteps; i++)
	{
		printf("%f ", betas[i]);
	}
	free(betas);
}

void TestCosineBetas()
{
	int timeSteps = 40;
	float *betas = CosineBetaSchedule(timeSteps);
	assert(betas != NULL);
	for(int i = 0; i < timeSteps; i++)
	{
		printf("%f ", betas[i]);
	}
	free(betas);
}

void TestSigmoidBetas()
{
	int timeSteps = 4;
	float *betas = SigmoidBetaSchedule(timeSteps);
	assert(betas != NULL);
	for(int i = 0; i < timeSteps; i++)
	{
		printf("%f ", betas[i]);
	}
	free(betas);
}

void Test2DSwissRoll()
{
	float noise = 0.0f;
	int arrayLength = 10000;
	float *timeStep = calloc(arrayLength, sizeof(float));
	float *xCoordinates = calloc(arrayLength, sizeof(float));
	float *yCoordinates = calloc(arrayLength, sizeof(float));
	
	GenerateSwissRoll2D(arrayLength, noise, timeStep, xCoordinates, yCoordinates);
	
	FILE *fp = fopen("swissroll_2d.txt", "w");assert(fp != NULL);
	for(int i = 0; i < arrayLength; i++)
	{
		fprintf(fp,"%f %f\n", xCoordinates[i], yCoordinates[i]);
	}
	fclose(fp);
	free(timeStep);
	free(xCoordinates);
	free(yCoordinates);
}

void TestComputeDiffusionParameters()
{
	int timeSteps = 10000;
	float *betas = SigmoidBetaSchedule(timeSteps);
	float *alphas = calloc(timeSteps, sizeof(float));
	float *alphasCumulativeProduct = calloc(timeSteps, sizeof(float));
	float *alphasCumulativeProductPrevious = calloc(timeSteps, sizeof(float));
	float *squareRootReciprocalAlphas = calloc(timeSteps, sizeof(float));
	float *squareRootAlphasCumulativeProduct = calloc(timeSteps, sizeof(float));
	float *squareRootMinusOneAlphasCumulativeProduct = calloc(timeSteps, sizeof(float));
	float *posteriorVariance = calloc(timeSteps, sizeof(float));
	ComputeDiffusionParameters(timeSteps, betas,alphas, alphasCumulativeProduct, alphasCumulativeProductPrevious,squareRootReciprocalAlphas,squareRootAlphasCumulativeProduct,squareRootMinusOneAlphasCumulativeProduct, posteriorVariance);
	
	
	float  swissRollNoise = 0.0f;
	int    swissRollLength = 1000;
	float *actualNoise = calloc(swissRollLength, sizeof(float));
	float *swissRollTimeStep = calloc(swissRollLength, sizeof(float));
	float *swissRollXCoordinates = calloc(swissRollLength, sizeof(float));
	float *swissRollYCoordinates = calloc(swissRollLength, sizeof(float));
	float *diffusionXCoordinates = calloc(swissRollLength, sizeof(float));
	float *diffusionYCoordinates = calloc(swissRollLength, sizeof(float));
	
	GenerateSwissRoll2D(swissRollLength, swissRollNoise, swissRollTimeStep, swissRollXCoordinates, swissRollYCoordinates);
	//Add to X coordinates
	int diffusionStep = 100;
	AddDiffusionParameters(diffusionStep, timeSteps, swissRollLength, actualNoise, swissRollXCoordinates, diffusionXCoordinates, squareRootAlphasCumulativeProduct, squareRootMinusOneAlphasCumulativeProduct);
	//Add to Y coordinates
	AddDiffusionParameters(diffusionStep, timeSteps, swissRollLength, actualNoise, swissRollYCoordinates, diffusionYCoordinates, squareRootAlphasCumulativeProduct, squareRootMinusOneAlphasCumulativeProduct);
	
	char diffusionFileName[50];
	snprintf(diffusionFileName, sizeof(diffusionFileName), "swissroll_2d%d.txt", diffusionStep);
	FILE *fp = fopen(diffusionFileName, "w");assert(fp != NULL);
	for(int i = 0; i < swissRollLength; i++)
	{
		fprintf(fp,"%f %f\n", diffusionXCoordinates[i], diffusionYCoordinates[i]);
	}
	fclose(fp);
	free(actualNoise);
	free(diffusionXCoordinates);
	free(diffusionYCoordinates);
	free(swissRollTimeStep);
	free(swissRollXCoordinates);
	free(swissRollYCoordinates);
	free(alphas);
	free(alphasCumulativeProduct);
	free(alphasCumulativeProductPrevious);
	free(squareRootReciprocalAlphas);
	free(squareRootAlphasCumulativeProduct);
	free(squareRootMinusOneAlphasCumulativeProduct);
	free(posteriorVariance);
	free(betas);
}

void TestBinomialDiffusion()
{
	srand(3456);
	int timeSteps = 100;
	float *betas = SigmoidBetaSchedule(timeSteps);
	float *alphas = calloc(timeSteps, sizeof(float));
	float *alphasCumulativeProduct = calloc(timeSteps, sizeof(float));
	float *alphasCumulativeProductPrevious = calloc(timeSteps, sizeof(float));
	float *squareRootReciprocalAlphas = calloc(timeSteps, sizeof(float));
	float *squareRootAlphasCumulativeProduct = calloc(timeSteps, sizeof(float));
	float *squareRootMinusOneAlphasCumulativeProduct = calloc(timeSteps, sizeof(float));
	float *posteriorVariance = calloc(timeSteps, sizeof(float));
	ComputeDiffusionParameters(timeSteps, betas,alphas, alphasCumulativeProduct, alphasCumulativeProductPrevious,squareRootReciprocalAlphas,squareRootAlphasCumulativeProduct,squareRootMinusOneAlphasCumulativeProduct, posteriorVariance);
	
	MpzWithIndex *diffusionState = NULL;
	MpzWithIndex *diffusionTemp = NULL;
	mpz_t compositeNumber,temporary0, temporary1,temporary2,temporary3;//x^2 - y ^ 2 = compositeNumber * k;
	mpz_inits(compositeNumber, temporary0,temporary1,temporary2, temporary3, NULL);
	mpz_set_str(compositeNumber, "1522605027922533360535618378132637429718068114961380688657908494580122963258952897654000350692006139", 10);
	mpz_set(temporary0, compositeNumber);
	size_t blockSizeBytes = (mpz_sizeinbase(compositeNumber, 2) / 8) + 1;
	int oneCount = 27;
	assert(oneCount % 3 == 0);
	int split3 = oneCount / 3;
	int *splitIndices = calloc(oneCount, sizeof(int));
	int *binomialHolder = calloc(oneCount, sizeof(int));
	for(int i = 0, k = 0, splitIndex = 0; i < oneCount; i++){splitIndices[i] = splitIndex;k += 1;if(k == split3){k = 0;splitIndex+=1;}}
	FisherYatesShuffle(oneCount, splitIndices);
	largestPossible = FindLargestNLogarithm((double)blockSizeBytes*8,oneCount)+1; assert(largestPossible > 0);
	
	ChangeDimension(temporary1, temporary0, oneCount, binomialHolder);
	DimensionToInteger(temporary1, temporary0,oneCount, binomialHolder);
	assert(mpz_cmp(temporary0, compositeNumber) == 0);
	//gmp_printf("%ld : %d)\n", blockSizeBytes, largestPossible);
	for(int i = 0; i < oneCount; i++)
	{
		MpzWithIndex element = malloc(sizeof(struct mpz_index_struct));assert(element != NULL);
		MpzWithIndex tempElement = malloc(sizeof(struct mpz_index_struct));assert(element != NULL);
		mpz_init(element->binomialCoefficient);
		mpz_init(tempElement->binomialCoefficient);
		element->splitIndex = splitIndices[i];
		mpz_bin_uiui(element->binomialCoefficient, binomialHolder[i], i+1);
		element->n = binomialHolder[i];
		element->k = i+1;
		//gmp_printf("%d (%d %d) : %Zd\n",element->splitIndex, element->n, element->k, element->binomialCoefficient);
		arrput(diffusionState, element);
		arrput(diffusionTemp, tempElement);
	}
	qsort(diffusionState, oneCount, sizeof(MpzWithIndex), CompareSplitIndex);

	
	double loss = FindLoss(compositeNumber,temporary0,temporary1,temporary2,temporary3, oneCount, diffusionState);
	
	int diffusionStep = timeSteps-1;
	float mean0_start = 1.0f, variance0_start = 0.5f;
	float mean1_start = 0.0f, variance1_start = 0.0f;
	float step = 0.1f;
	int iterations = 10;  

	for(float mean0 = mean0_start; mean0 < mean0_start + iterations * step; mean0 += step){
	for(float variance0 = variance0_start; variance0 < variance0_start + iterations * step; variance0 += step){
	for(float mean1 = mean1_start; mean1 < mean1_start + iterations * step; mean1 += step){
	for(float variance1 = variance1_start; variance1 < variance1_start + iterations * step; variance1 += step)
	{
	
	CopyCurrentState(oneCount, diffusionTemp, diffusionState);
	TestMeanVariance(diffusionStep, timeSteps, oneCount, mean0, variance0, mean1, variance1, diffusionTemp, squareRootAlphasCumulativeProduct, squareRootMinusOneAlphasCumulativeProduct);
	double newLoss = FindLoss(compositeNumber,temporary0,temporary1,temporary2,temporary3, oneCount, diffusionTemp);
	if(fabs(newLoss) < fabs(loss) && SumMpzWithIndex(oneCount, diffusionTemp) > SumMpzWithIndex(oneCount, diffusionState))
	{
		loss = newLoss;
		CopyCurrentState(oneCount, diffusionState, diffusionTemp);
		printf("mean0: %.1f, variance0: %.1f, mean1: %.1f, variance1: %.1f : ",mean0, variance0, mean1, variance1);
		printf("%.3f %.3f\n", loss,newLoss);
		PrintMpzWithIndex(oneCount, diffusionState);
	}
	
	}}}}
	
	
	
	for(int i = 0; i < oneCount; i++)
	{
		mpz_clear(diffusionTemp[i]->binomialCoefficient);
		mpz_clear(diffusionState[i]->binomialCoefficient);
		free(diffusionState[i]);
		free(diffusionTemp[i]);
	}
	arrfree(diffusionTemp);
	arrfree(diffusionState);
	mpz_clears(compositeNumber,temporary0,temporary1,temporary2, temporary3, NULL);
	free(binomialHolder);free(splitIndices);
	free(alphas);
	free(alphasCumulativeProduct);
	free(alphasCumulativeProductPrevious);
	free(squareRootReciprocalAlphas);
	free(squareRootAlphasCumulativeProduct);
	free(squareRootMinusOneAlphasCumulativeProduct);
	free(posteriorVariance);
	free(betas);
}
int main()
{
	//TestLinearSpace();
	//TestLinearBetas();
	//TestQuadraticBetas();
	//TestCosineBetas();
	//TestSigmoidBetas();
	//Test2DSwissRoll();
	//TestComputeDiffusionParameters();
	TestBinomialDiffusion();
	return 0;
}
 
