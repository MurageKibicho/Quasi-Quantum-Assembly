#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <assert.h>
#include <gmp.h>
#include "ff_asm_primes.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#define INDEX(x, y, cols) ((x) * (cols) + (y))
#define TRUE 1
#define FALSE 0
uint32_t rngState[1] = {15546};  
enum layer_type_enum{BIAS, NO_BIAS, LAYER_TYPE_LENGTH};
typedef struct model_struct *Model;
typedef struct layer_struct *Layer;
struct layer_struct
{
	int layerType;
	int rowCount;
	int columnCount;
	double bias;
	int *rowPrimes;
	double *exponents;//Exponents
};

struct model_struct
{
	int layerCount;
	int tempLength;
	int finalRowCount;
	int finalColumnCount;
	double *tempExponents;
	Layer *layers;
};
int ModularExponentiation(int base, int exp, int mod){int result = 1;base = base % mod;while(exp > 0){if(exp % 2 == 1){result = (result * base) % mod;}exp = exp >> 1;base = (base * base) % mod;}return result;}
int FastCheck_2IsAGenerator(int primeNumber){int result = FALSE;if(primeNumber == 2){result = TRUE;}if(primeNumber % 8 == 3 || primeNumber % 8 == 5){int primeNumberMinusOne = primeNumber - 1;int n = primeNumberMinusOne;int q = 0;if(n % 2 == 0){q = 2;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}while(n % 2 == 0){n /= 2;}}for(int i = 3; i * i <= primeNumberMinusOne; i += 2){if(n % i == 0){q = i;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}while(n % i == 0){n /= i;}}}if(n > 1){q = n;int checkvalue = ModularExponentiation(2, primeNumberMinusOne / q, primeNumber);if(checkvalue == 1){result = FALSE;return result;}}result = TRUE;}return result;}
uint32_t XorShift(uint32_t state[]){uint32_t x = state[0];x ^= x >> 17;x ^= x << 5;state[0] = x;return x;}
double Find_Log_MPZ_Double(mpz_t x){if(mpz_cmp_ui(x, 0) == 0){return 0;}signed long int ex;const double di = mpz_get_d_2exp(&ex, x);return ( (log(di) + log(2) * (double) ex) /log(2));}

Model CreateModel()
{
	Model model = malloc(sizeof(struct model_struct));
	model->layerCount = 0;
	model->tempLength = 5*5;
	model->finalRowCount = 0;
	model->finalColumnCount = 0;
	model->tempExponents = calloc(5 * 5, sizeof(double));
	model->layers = NULL;
	return model;
}

Layer CreateNewLayer(int layerType, int rowCount, int columnCount)
{
	assert(layerType >= 0);assert(layerType < LAYER_TYPE_LENGTH);
	assert(rowCount > 0);
	assert(columnCount > 0);
	Layer layer = malloc(sizeof(struct layer_struct));
	layer->layerType = layerType;
	layer->rowCount  = rowCount;
	layer->columnCount = columnCount;
	layer->bias = 0;
	layer->exponents = calloc(layer->rowCount * layer->columnCount, sizeof(double));
	layer->rowPrimes = calloc(layer->rowCount, sizeof(int));
	int currentPrimeIndex = 0;
	int currentRowIndex = 0;
	double logValue = 0.0f;
	int bSmooth = 8;
	while(currentPrimeIndex < 5239)
	{
		if(first5239[currentPrimeIndex] > bSmooth && FastCheck_2IsAGenerator(first5239[currentPrimeIndex]) == TRUE)
		{
			layer->rowPrimes[currentRowIndex] = first5239[currentPrimeIndex];
			currentRowIndex += 1;
			if(currentRowIndex == layer->rowCount){break;}
		}
		currentPrimeIndex += 1;
	}
	for(int i = 0; i < layer->columnCount; i++)
	{
		for(int j = 0; j < layer->rowCount; j++)
		{
			layer->exponents[INDEX(j, i, layer->columnCount)] = (double) (XorShift(rngState) % (layer->rowPrimes[j] - 1));
		}
	}
	assert(currentRowIndex == layer->rowCount);
	return layer;
}

void AddLinearLayer(Model model, int inputFeatures, int outputFeatures, int layerType)
{
	if(model)
	{
		int newLength = outputFeatures * inputFeatures;
		if(newLength > model->tempLength)
		{
			double *temp = realloc(model->tempExponents, outputFeatures * inputFeatures * sizeof(double));
			assert(temp != NULL);
			model->tempExponents = temp;
			model->tempLength = newLength;
		}
		Layer layer = CreateNewLayer(layerType, outputFeatures, inputFeatures);
		arrput(model->layers, layer);
	}
}

void PrintModel(Model model)
{
	if(model)
	{
		printf("\nModel Summary: %ld layers\n", arrlen(model->layers));
		for(size_t i = 0; i < arrlen(model->layers); i++)
		{
			printf("\tLayer %ld : Rows %d, Columns %d, Prime Range (%d - %d)\n",i, model->layers[i]->rowCount, model->layers[i]->columnCount, model->layers[i]->rowPrimes[0],model->layers[i]->rowPrimes[model->layers[i]->rowCount-1]);
			for(int j = 0; j < model->layers[i]->rowCount; j++)
			{
				for(int k = 0; k < model->layers[i]->columnCount; k++)
				{
					printf("%.3f, ", model->layers[i]->exponents[INDEX(j,k, model->layers[i]->columnCount)]);
				}
				printf("\n");
			}
		}
	}
	
}

void FreeLayer(Layer layer)
{
	if(layer)
	{
		free(layer->rowPrimes);
		free(layer->exponents);
		free(layer);
	}
}

void FreeModel(Model model)
{
	if(model)
	{
		for(size_t i = 0; i < arrlen(model->layers); i++)
		{
			FreeLayer(model->layers[i]);
		}
		arrfree(model->layers);
		free(model->tempExponents);
		free(model);
	}
}

void PrintMatrix(int rowCount, int columnCount, double *matrix)
{
	for(int i = 0; i < rowCount; i++)
	{
		for(int j = 0; j < columnCount; j++)
		{
			printf("%2.0f,", matrix[INDEX(i,j,columnCount)]);
		}
		printf("\n");
	}
}

void ForwardPass(Model model)
{
	if(model)
	{
		//printf("|%d|\n", model->tempLength);
		double *inputExponents = model->tempExponents;
		memcpy(inputExponents, model->layers[0]->exponents, model->layers[0]->rowCount * model->layers[0]->columnCount * sizeof(double)); 
		int inputExponentRowCount = model->layers[0]->rowCount;
		int inputExponentColumnCount = model->layers[0]->columnCount;
		//PrintMatrix(inputExponentRowCount, inputExponentColumnCount, inputExponents);
		for(size_t i = 1; i < arrlen(model->layers); i++)
		{
			Layer weights = model->layers[i];
			if(weights && inputExponents)
			{
				assert(weights->columnCount == inputExponentRowCount);
				double *outputExponents = calloc(weights->rowCount * inputExponentColumnCount, sizeof(double));
				for(int j = 0; j < weights->rowCount; j++)
				{
					for(int k = 0; k < inputExponentColumnCount; k++)
					{
						outputExponents[INDEX(j,k,inputExponentColumnCount)] = 0.0f;
						for(int l = 0; l < weights->columnCount; l++)
						{
							double weightExponent = weights->exponents[INDEX(j,l,weights->columnCount)];
							double inputExponent  = inputExponents[INDEX(l,k,inputExponentColumnCount)];
							double sumExponent    = weightExponent + inputExponent;
							sumExponent = fmod(sumExponent, (double) (weights->rowPrimes[j] - 1.0f) );
							outputExponents[INDEX(j,k,inputExponentColumnCount)]    += pow(2, sumExponent); 
							//printf("(2^(%.0f + %.0f) = %.1f) + ", weightExponent, inputExponent, outputExponents[INDEX(j,k,inputExponentColumnCount)]);
						}
						outputExponents[INDEX(j,k,inputExponentColumnCount)] =fmod(outputExponents[INDEX(j,k,inputExponentColumnCount)], (double) (weights->rowPrimes[j]) );
						//printf("\n");
					}
					//printf("\n");
				}
				inputExponentRowCount = weights->rowCount;
				inputExponentColumnCount = inputExponentColumnCount;
				memcpy(inputExponents, outputExponents, inputExponentRowCount * inputExponentColumnCount * sizeof(double));
				free(outputExponents);
			}
		}
		model->finalRowCount = inputExponentRowCount;
		model->finalColumnCount = inputExponentColumnCount;
	}
}

void BackwardPass(Model model)
{
	if(model)
	{
		assert(model->finalRowCount > 0);
		assert(model->finalColumnCount > 0);
		int inputExponentRowCount = model->finalRowCount;
		int inputExponentColumnCount = model->finalColumnCount;
		double *inputExponents = model->tempExponents;
		PrintMatrix(inputExponentRowCount, inputExponentColumnCount, inputExponents);
		
	}
}

int main()
{
	/*y =z(wx+b)*/  
	Model model = CreateModel();
	int numberOfIntegers = 2;
	int layers[] = {5,3,4};
	int layerCount = sizeof(layers) / sizeof(int);
	for(int i = 0; i < layerCount-1; i++)
	{
		AddLinearLayer(model, layers[i], layers[i+1], NO_BIAS);
	}
	//PrintModel(model);
	printf("\nForward\n");
	ForwardPass(model);
	
	printf("\nBackward\n");
	BackwardPass(model);
	
	FreeModel(model);
	return 0;
}

/*
void BruteForceSolutions(int rowCount, int columnCount, int *board, Square *squares, double compositeNumberBits, mpz_t sumOfQuotients)
{
	mpz_t setSize,base, mod, modResult, exponentResult,dX, dY, temporary0;
	mpz_inits(setSize,base, mod, modResult,exponentResult,dX, dY, temporary0,NULL);
	mpz_set_ui(setSize, 1);mpz_set_ui(base, 2);
	int *currentIndex = calloc(columnCount, sizeof(int));
	int *maxIndex = calloc(columnCount, sizeof(int));
	int *moduli = calloc(columnCount, sizeof(int));
	int *congruences = calloc(columnCount, sizeof(int));
	int *expos = calloc(columnCount, sizeof(int));
	int **holder = malloc(columnCount * sizeof(int*));
	for(int i = 0; i < columnCount; i++){maxIndex[i] = squares[i]->length;moduli[i]=squares[i]->modulo;mpz_mul_ui(setSize, setSize, squares[i]->length);holder[i] = calloc(7, sizeof(int));}
	

	//gmp_printf("Set size : %.3f\nComposite Bits : %.3f\n", Find_Log_MPZ_Double(setSize), compositeNumberBits);
	int stop = 0; int breakCheck = 0;int exponent = 0;
	while(1)
	{	
		//break;
		RandomIndexArray(columnCount, currentIndex, maxIndex);
		mpz_set_ui(dX, 0);mpz_set_ui(dY, 0);
		for(int j = 0; j < columnCount; j++)
		{
			assert(currentIndex[j] < squares[j]->length);
			assert(currentIndex[j] >= 0);
			assert(squares[j]->square[currentIndex[j]]->y + 1 < arrlen(squares[j]->discreteLogInput));
			mpz_set_ui(mod, moduli[j]);
			
			int regularMultiple = squares[j]->discreteLogInput[squares[j]->square[currentIndex[j]]->y + 1]->y;
			int originalMod = squares[j]->square[currentIndex[j]]->x;
			
			holder[j][0] = board[INDEX(0,j,columnCount)];
			holder[j][1] = squares[j]->square[currentIndex[j]]->y;
			holder[j][2] = regularMultiple;
			holder[j][3] = board[INDEX(1,j,columnCount)];
			holder[j][4] = moduli[j];
			holder[j][5] = squares[j]->square[currentIndex[j]]->x;
			congruences[j] = squares[j]->square[currentIndex[j]]->x;
			expos[j] = regularMultiple;
			mpz_mul_ui(temporary0, squares[j]->sumOfQuotientsInverse, holder[j][2]);
			mpz_add(dX, dX, temporary0);
			
			//mpz_mul_ui(temporary0, squares[j]->sumOfQuotientsInverse, holder[j][5]);
			//mpz_add(dY, dY, temporary0);
			
			//mpz_mod(dX, dX, sumOfQuotients);
			//mpz_mod(dY, dY, sumOfQuotients);
			
			gmp_printf("2^(%4d + %4d (%4d)) + %4d (mod %4d) = %4d: %Zd | %Zd = %Zd * %d mod(%Zd)\n", holder[j][0],holder[j][1],holder[j][2],holder[j][3],holder[j][4],holder[j][5], dX, temporary0, squares[j]->sumOfQuotientsInverse, holder[j][2],sumOfQuotients);
			exponent = board[INDEX(0,j,columnCount)] + squares[j]->square[currentIndex[j]]->y;
			mpz_powm_ui(modResult, base,(unsigned long) exponent, mod);
			//mpz_add_ui(modResult, modResult, );
		}
		ChineseRemainderTheorem(columnCount, moduli, congruences, modResult);
		ChineseRemainderTheorem(columnCount, moduli, expos, exponentResult);
		double squareNumberBits = Find_Log_MPZ_Double(modResult);
		double expoNumberBits = Find_Log_MPZ_Double(exponentResult);
		double leftSideBits = expoNumberBits + compositeNumberBits;
		int perfectCheck = mpz_perfect_square_p(modResult);
		int quotientCompare = mpz_cmp(dX, dY);
		//if(perfectCheck > 0)
		//if(perfectCheck > 0 && leftSideBits < squareNumberBits)
		gmp_printf("%d : %.3f (%Zd) %.3f (%Zd)\n\n",perfectCheck, leftSideBits,dX, squareNumberBits, dY);
		//gmp_printf("%Zd %Zd\n", exponentResult, modResult);
		if(perfectCheck > 0 && leftSideBits < squareNumberBits){break;}
		//if(leftSideBits < squareNumberBits){break;}
	}
		
	for(int i = 0; i < columnCount; i++){free(holder[i]);}
	free(holder);free(currentIndex);free(maxIndex);free(moduli);free(congruences);free(expos);
	mpz_clears(setSize,base,mod, modResult,exponentResult,dX, dY, temporary0, NULL);
}
*/


/*
1. If dad dies, does his shareholding go to Rosa or does it go to us.
2. If one of us dies, does our shareholding distributed equally?
3. Is the house demolished?
4. If there's a decision to make are there dad's veto powers, must a quorum be achieved? How many people
5. If any costs arise who's required to handle the expense.
6. If anyone wants to sell their shares 
*/
