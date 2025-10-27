#include "Elliptic_Secp.h"
//clear && gcc Elliptic_SimpleTest.c -lm -lgmp -lmpfr -lflint -o m.o && ./m.o
int main()
{
	EllipticSecp_BenchMarkLSB();
	return 0;
}
