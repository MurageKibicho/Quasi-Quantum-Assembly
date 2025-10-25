#include "Pell.h"
//clear && gcc PellTest.c -lm -lflint -lgmp -lmpfr -o m.o && ./m.o
void TestSmallCurveLegendre0()
{
	fmpz_t prime, D, n;
	fmpz_init(prime);
	fmpz_init(D);
	fmpz_init(n);
	fmpz_set_ui(prime, 20947);
	//D ≡ 0 mod p
	fmpz_mul_ui(D, prime, 2);
	
	//n is QR
	fmpz_set_ui(n, 10);
	printf("\nTest 1: n is QR\n");
	PellCurve curve20947QR = PellCurve_CreateCurveAuto(prime, D,n);
	PellCurve_PrettyPrint(curve20947QR);
	PellPoint *curve20947QRPoints = PellCurve_GenerateAllPoints(curve20947QR);
	printf("\nValid Points curve20947QR\n");
	for(int i = 0; i < fmpz_get_ui(curve20947QR->groupOrder); i++)
	{
		PellCurve_PrintPoint(curve20947QRPoints[i]);
	}
	PellCurve_DestroyAllPoints(curve20947QR, curve20947QRPoints);
	PellCurve_Clear(curve20947QR);
	
	//n is nonQR
	printf("\nTest 2: n is nonQR\n");
	fmpz_set_ui(n, 11);
	PellCurve curve20947NQR = PellCurve_CreateCurveAuto(prime, D,n);
	printf("\nValid Points curve20947NQR\n");
	PellPoint *curve20947NQRPoints = PellCurve_GenerateAllPoints(curve20947NQR);
	for(int i = 0; i < fmpz_get_ui(curve20947NQR->groupOrder); i++)
	{
		PellCurve_PrintPoint(curve20947QRPoints[i]);
	}
	PellCurve_DestroyAllPoints(curve20947NQR, curve20947NQRPoints);
	PellCurve_PrettyPrint(curve20947NQR);
	PellCurve_Clear(curve20947NQR);
	fmpz_clear(prime);
	fmpz_clear(D);
	fmpz_clear(n);
}

void TestSmallCurveLegendre1()
{
	fmpz_t prime, D, n;
	fmpz_init(prime);
	fmpz_init(D);
	fmpz_init(n);
	fmpz_set_ui(prime, 20947);
	
	//D legendre symbol is 1 mod p
	fmpz_set_ui(D, 235);
	
	//n is 0
	fmpz_set_ui(n, 0);
	printf("\nTest 3: n is 0\n");
	PellCurve curve20947QR_0 = PellCurve_CreateCurveAuto(prime, D,n);
	PellCurve_PrettyPrint(curve20947QR_0);
	PellPoint *curve20947QR_0_Points = PellCurve_GenerateAllPoints(curve20947QR_0);
	printf("\nValid Points curve20947QR_0\n");
	for(int i = 0; i < fmpz_get_ui(curve20947QR_0->groupOrder); i++)
	{
		//PellCurve_PrintPoint(curve20947QRPoints[i]);
	}
	PellCurve_DestroyAllPoints(curve20947QR_0, curve20947QR_0_Points);
	PellCurve_Clear(curve20947QR_0);
	
	//n > 0
	fmpz_set_ui(prime, 17);
	fmpz_set_ui(n, 5);
	fmpz_set_ui(D, 2);
	printf("\nTest 4: n > 0\n");
	PellCurve curve20947QR = PellCurve_CreateCurveAuto(prime, D,n);
	PellCurve_PrettyPrint(curve20947QR);
	PellPoint *curve20947QR_Points = PellCurve_GenerateAllPoints(curve20947QR);
	printf("\nValid Points curve20947QR_0\n");
	for(int i = 0; i < fmpz_get_ui(curve20947QR->groupOrder); i++)
	{
		PellCurve_PrintPoint(curve20947QR_Points[i]);
	}
	PellCurve_DestroyAllPoints(curve20947QR, curve20947QR_Points);
	PellCurve_Clear(curve20947QR);
	
	fmpz_clear(prime);
	fmpz_clear(D);
	fmpz_clear(n);
}

void TestSmallCurveLegendreMinusOne()
{
	
}

int main()
{
	//TestSmallCurveLegendre0();
	//TestSmallCurveLegendre1();
	TestSmallCurveLegendreMinusOne();
	flint_cleanup();
	return 0;
}


