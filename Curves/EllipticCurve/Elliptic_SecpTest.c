#include "Elliptic_Secp.h"
//clear && gcc Elliptic_SecpTest.c -lm -lgmp -lmpfr -lflint -o m.o && ./m.o


void TestSmallCurve()
{
	int fieldCharacteristic = 20959;
	int groupOrder = 20947;
	EllipticSecpCurve curve = EllipticSecpCurve_CreateSmallCurve(fieldCharacteristic, groupOrder, 32);
	EllipticSecp_PrintCurve(curve);
	EllipticSecp *allPoints = EllipticSecp_GenerateAllPoints(curve);
	for(int i = 0; i < fmpz_get_ui(curve->pointOrder); i++)
	{
		//EllipticSecp_PrintPoint(allPoints[i]);
	}
	
	for(int i = 0; i < fmpz_get_ui(curve->pointOrder); i++)
	{
		EllipticSecp_DestroyPoint(allPoints[i]);
	}
	free(allPoints);
	EllipticSecp_DestroyCurve(curve);
}
int main()
{
	//EllipticSecp_BenchMarkLSB();
	TestSmallCurve();
	flint_cleanup();
	return 0;
}

