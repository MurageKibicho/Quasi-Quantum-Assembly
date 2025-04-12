#include "Mediant32.h"
/*
Run : clear && gcc 34_Mediant.c cJSON.c -lm -lgmp -o m.o && ./m.o
*/

void TestRoot()
{
	m32 a = m32_Init();
	a->numerator = 10;
	a->denominator = 3;
	//m32_IntegerExponent(5, a);
	m32_IntegerNthRoot(2, a);
	m32_Print(a);
	m32_Clear(a);
}
int main()
{
	//TestFloatInit();
	TestSafeTensorLoad();
	TestRoot();
	return 0;
}
