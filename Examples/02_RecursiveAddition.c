#include <stdio.h>
#include "ff_asm_runtime.h" 

//Run : gcc 02_RecursiveAddition.c -lgmp -lm -o m.o && ./m.o
int main()
{
	//Unsigned 8-bit arrays we want to add
	uint8_t data0[] = {5, 3, 6};
	uint8_t data1[] = {2, 4, 1};
	uint64_t fieldOrder[] = {8, 9, 11};
	int dataLength = sizeof(data0) / sizeof(uint8_t);
	
	//Allocate memory for one FF-asm fields of type Int_8_U_Field
	ff_asmField field = ff_asmMalloc(dataLength, Int_8_U_Field);
	
	//Set our finite field order
	ff_asmSetFieldOrder(field, dataLength, fieldOrder);
	
	//Place data0 in the FF-asm fieldNode at index 0
	ff_asmAppendData(field, dataLength, data0, Int_8_U_Field);
	
	//Add data1 in the FF-asm field at index 1
	ff_asmAppendData(field, dataLength, data1, Int_8_U_Field);
	
	//Add data at index 0 and data at index 1 : store at index 2.
	ff_asmAdd(field, 0, 1, 2);
	
	//Prepare the data for printing.
	ff_asmDataDebug(field);
	
	//Print the FF-asm field
	ff_asmPrintField(field);
	
	//Free the FF-asm field
	ff_asmFreeField(field);
	return 0;
}
