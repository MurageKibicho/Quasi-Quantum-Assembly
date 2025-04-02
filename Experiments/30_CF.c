#include "29_CF.h"
#include "29_2_Iris.h"
typedef struct kibicho_brocot_node_struct *kbNode;
typedef struct layer_linear_struct Linear;
typedef struct layer_relu_struct ReLU;
typedef struct layer_dropout_struct Dropout;
struct kibicho_brocot_node_struct
{
	kb kbValue;
	float floatValue;
	long numerator;
	long denominator;
	
};
struct layer_linear_struct
{
	kbNode *bias; 
	kbNode *weight;
	size_t biasLength;
	size_t weightLength;
};
struct layer_relu_struct
{
	kbNode *weight;
	size_t weightLength;
};
struct layer_dropout_struct
{
	kbNode *weight;
	size_t weightLength;
};
struct model_parameter_struct
{
	Linear linearLayer0;
	ReLU reluLayer0;
	Dropout dropout0;
	ReLU reluLayer1;
	Linear linearLayer1;
};

void ShuffleIrisDataset(iris_dataset_holder *array, size_t n)
{
	srand(4567870);
	for(size_t i = n - 1; i > 0; i--)
	{
		size_t j = rand() % (i + 1);
		iris_dataset_holder temp = array[i];
		array[i] = array[j];
		array[j] = temp;
	}
}

int main()
{
	size_t irisDataSetSize = 150;
	ShuffleIrisDataset(iris_dataset_holder_array, irisDataSetSize);
	srand(3456);
	size_t trainSize = (150*8)/10;
	size_t testSize  = irisDataSetSize - trainSize;
	size_t batchSize = 15;
	
	TestScale();
	return 0;
}
