#include "ff_asm_runtime.h"

//Run : gcc 01_HelloWorld.c -lgmp -lm -o m.o && ./m.o
int main()
{
	//Unsigned 8-bit array
	uint8_t data[] = {50, 100, 20};
	uint64_t fieldOrder[] = {8, 9, 11};
	size_t dataLength = sizeof(data) / sizeof(uint8_t);
	
	//Allocate memory for a FF-asm field of type Int_8_U_Field
	ff_asmField field = ff_asmMalloc(dataLength, Int_8_U_Field);
	
	//Set our finite field order
	ff_asmSetFieldOrder(field, dataLength, fieldOrder);
	
	//Place data in the FF-asm fieldNode at index 0
	ff_asmAppendData(field, dataLength, data, Int_8_U_Field);
	
	//Print the FF-asm field
	ff_asmPrintField(field);
	
	//Free the FF-asm field
	ff_asmFreeField(field);
	return 0;
}
