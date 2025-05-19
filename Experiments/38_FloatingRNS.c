#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "ff_asm_primes.h"

/*Run : clear && gcc 38_FloatingRNS.c -lm -o m.o && ./m.o*/
void PrintRNS(int moduliLength, int *moduli, int *residues)
{
	for(int i = 0; i < moduliLength; i++)
	{
		printf("(%3d mod %3d)\n", residues[i], moduli[i]);
	}
}

int FindRNSRange(int moduliLength, int *moduli)
{
	int range = 1;
	for(int i = 0; i < moduliLength; i++)
	{
		//We avoid all even numbers or negative numbers
		assert(moduli[i] % 2 == 1 && moduli[i] > 2);
		
		range *= moduli[i];
	}
	return range;
}


void IntegerToSignedRNS(int moduliLength, int *moduli, int *residues, int integer, int range)
{
	//Get the sign
	int sign = 1;
	if(integer < 0){sign = -1;}
	
	//Find absolute value of integer
	integer = abs(integer);
	
	//Ensure integer is less than half of signed RNS
	assert(integer < range / 2);
	
	//Fill the residue array
	for(int i = 0; i < moduliLength; i++)
	{
		//We avoid all even numbers
		assert(moduli[i] % 2 == 1);
		residues[i] = integer % moduli[i];
		
		//Handle the case our integer is negative
		//This may be unnecessary in LibGMP or your preferred programming language
		if(sign == -1)
		{
			residues[i] = (moduli[i] + (residues[i] * sign)) % moduli[i];
		}
	}
}

int ModularInverse(int number, int modulo)
{
	int m0 = modulo;
	int t  = 0; int q = 0; int x0 = 0; int x1 = 1;
	if(modulo == 1){return 0;}
	while(number > 1)
	{
		q = number / modulo;
		t = modulo;
		modulo = number % modulo; 
		number = t;
		t = x0;
		x0 = x1 - q * x0;
		x1 = t;
	} 
	if(x1 < 0){x1 += m0;}
	return x1;
}

int ChineseRemainderTheorem(int moduliLength, int *moduli, int *residues)
{
	int M = 1;
	for(int i = 0; i < moduliLength; i++){M *= moduli[i];}
	int result = 0;
	for(int i = 0; i < moduliLength; i++)
	{
		int Mi = M / moduli[i];
		int modularInverse =  ModularInverse(Mi, moduli[i]);
		result += residues[i] * Mi * modularInverse;
	}
	return result % M; 
}

int RNSToInteger(int moduliLength, int *moduli, int *residues, int range)
{
	//We assume all moduli are odd. ie. no multiples of 2
	int integer = ChineseRemainderTheorem(moduliLength, moduli, residues);
	//Check if Integer is greater than (range / 2) 
	if(integer > (range / 2))
	{
		integer = integer - range;
	}
	return integer;
}

void TestIntegerConversions()
{
	int moduli[] = {3,5,7,11};
	int moduliLength = sizeof(moduli) / sizeof(int);
	int residues[moduliLength];
	int range = FindRNSRange(moduliLength, moduli);
	
	int integer = 475;
	IntegerToSignedRNS(moduliLength, moduli, residues, integer, range);
	PrintRNS(moduliLength, moduli, residues);
	
	int integerReverse = RNSToInteger(moduliLength, moduli, residues, range);
	assert(integer == integerReverse);
	printf("%d\n", integerReverse);
}


void AddResidues(int moduliLength, int *moduli, int *aResidues, int *bResidues, int *cResidues)
{
	for(int i = 0; i < moduliLength; i++)
	{
		cResidues[i] = (aResidues[i] + bResidues[i]) % moduli[i];
	}
}

void MultiplyResidues(int moduliLength, int *moduli, int *aResidues, int *bResidues, int *cResidues)
{
	for(int i = 0; i < moduliLength; i++)
	{
		cResidues[i] = (aResidues[i] * bResidues[i]) % moduli[i];
	}
}

void TestFloatConversions()
{
	int base = 2;
	float a = 3.3434;
	float b = -2.7689;
	
	int aSign = (a < 0) ? -1 : 1;
	int bSign = (b < 0) ? -1 : 1;
	
	a = fabs(a);
	b = fabs(b);
	
	assert(a < 16.0f);
	assert(b < 16.0f);
	int aInteger = (int) roundf(pow(base, a));
	int bInteger = (int) roundf(pow(base, b));
	int cInteger = 0;
	
	aInteger *= aSign;
	bInteger *= bSign;
	
	int moduli[] = {3,5,7,11};
	int moduliLength = sizeof(moduli) / sizeof(int);
	int range = FindRNSRange(moduliLength, moduli);
	
	int aResidues[moduliLength];
	int bResidues[moduliLength];
	int cResidues[moduliLength];
	IntegerToSignedRNS(moduliLength, moduli, aResidues, aInteger, range);
	IntegerToSignedRNS(moduliLength, moduli, bResidues, bInteger, range);

	MultiplyResidues(moduliLength, moduli, aResidues, bResidues, cResidues);
	MultiplyResidues(moduliLength, moduli, cResidues, bResidues, cResidues);
	cInteger = RNSToInteger(moduliLength, moduli, cResidues, range);
	printf("%d :\n", aInteger);PrintRNS(moduliLength, moduli, aResidues);
	printf("%d :\n", bInteger);PrintRNS(moduliLength, moduli, bResidues);
	printf("%d :\n", cInteger);PrintRNS(moduliLength, moduli, cResidues);
}

int main()
{
	//TestIntegerConversions();
	TestFloatConversions();
	return 0;
}
