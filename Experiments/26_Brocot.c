#include <stdio.h>
#include <stdlib.h>


void BinaryToSternBrocotFraction(int binaryLength, int *binary)
{
	/*
	Create the 2*2 identity matrix 
	|1 0| is the matrix {1, 0, 0, 1}
	|0 1|
	*/
	int product[] = {1,0,0,1};
	for(int i = 0; i < binaryLength; i++)
	{
		if(binary[i] == 'L')
		{
			/*
			For the 2*2 matrix represented by the array {a,b,c,d}
			|a b| *  |1 1|= |a*1 +b*0 a*1 +b*1|  = |a a+b| 
			|c d|    |0 1|  |c*1 +d*0 c*1 +d*1|    |c c+d|
			*/	
			product[1] = product[0] + product[1];
			product[3] = product[2] + product[3];
		}
		else if(binary[i] == 'R')
		{
			/*
			For the 2*2 matrix represented by the array {a,b,c,d}
			|a b| *  |1 0|= |a*1 +b*1 a*0 +b*1|  = |a+b b|
			|c d|    |1 1|  |c*1 +d*1 c*0 +d*1|    |c+d d|
			*/	
			product[0] = product[0] + product[1];
			product[2] = product[2] + product[3];
		}
	}
	//Find Mediant and Reciprocal
	int numerator   = product[2] + product[3];
	int denominator = product[0] + product[1];
	printf("(%d %d)\n", numerator, denominator);
}
int main()
{
	int binary[] = {'L','R','R', 'L'};
	int binaryLength = sizeof(binary) / sizeof(int);
	
	BinaryToSternBrocotFraction(binaryLength, binary);
	return 0;
}
