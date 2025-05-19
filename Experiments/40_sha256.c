#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


uint32_t bigToLittleEndian(uint32_t val)
{
    return ((val >> 24) & 0xff)      |   // byte 3 to byte 0
           ((val << 8)  & 0xff0000)  |   // byte 1 to byte 2
           ((val >> 8)  & 0xff00)    |   // byte 2 to byte 1
           ((val << 24) & 0xff000000);   // byte 0 to byte 3
}


uint32_t RightRotate(uint32_t x, int n)
{
	return (x >> n) | (x << (32 - n));
}


uint32_t Choose(uint32_t x, uint32_t y, uint32_t z) 
{
	return (x & y) ^ ((~x) & z);
}

uint32_t Majority(uint32_t x, uint32_t y, uint32_t z)
{
	return (x & y) ^ (x & z) ^ (y & z);
}

uint32_t UppercaseSigmaZero(uint32_t x)
{
	return RightRotate(x, 2) ^ RightRotate(x, 13) ^ RightRotate(x, 22);
}

uint32_t UppercaseSigmaOne(uint32_t x)
{
	return RightRotate(x, 6) ^ RightRotate(x, 11) ^ RightRotate(x, 25);
}

uint32_t LittleToBig(uint32_t val)
{
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF );
    return (val << 16) | (val >> 16);
}

int *DecimalToBinaryInternal(uint32_t decimal)
{
	int *binary = calloc(32,sizeof(int));
	int index = 0;
	int *result = calloc(32,sizeof(int));
	
	while(decimal > 0)
	{
		if(decimal % 2 == 0)
		{
			binary[index] = 0;
		}
		else
		{
			binary[index] = 1;
		}
		decimal /= 2;
		index++; 
	}
	int j = 0;
	for(int i = 31; i>=0; i--)
	{
		result[j] = binary[i];
		j++;
	}
	return result;
}


void AddTo256(int *to256, uint32_t num, int *index)
{
	int *num1 = DecimalToBinaryInternal(num);
	int j = 0;
	for(int i = *index; i < *index+32; i++)
	{
		to256[i] = num1[j];
		j++;
	}
	*index = *index + 32;
	
}

void PrintArray(int *array, int arrayLength)
{
	for(int i = 0 ; i < arrayLength; i++)
	{
		printf("%d ",array[i]);
	}
	putchar('\n');
}

int* Bits256(uint32_t a, uint32_t b,uint32_t c,uint32_t d,uint32_t e,uint32_t f,uint32_t g,uint32_t h)
{
	int index = 0;
	int *to256 = calloc(256, sizeof(int));
	AddTo256(to256,a,&index);
	AddTo256(to256,b,&index);
	AddTo256(to256,c,&index);
	AddTo256(to256,d,&index);
	AddTo256(to256,e,&index);
	AddTo256(to256,f,&index);
	AddTo256(to256,g,&index);
	AddTo256(to256,h,&index);
	return to256;
}



void sha256_hash(const uint8_t input[], const uint64_t inputBytes, uint32_t H[8],int zero)
{
	H[0] = 0x6a09e667; H[1] = 0xbb67ae85; H[2] = 0x3c6ef372; H[3] = 0xa54ff53a;
	H[4] = 0x510e527f; H[5] = 0x9b05688c; H[6] = 0x1f83d9ab, H[7] = 0x5be0cd19;
	const uint32_t k[64] =
	{
       	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };
	
	uint64_t inputBits = inputBytes * 8;
	uint32_t numberOfBlocks = 1 + ((inputBits + 16 + 64) / 512);
	uint32_t* paddedInput = calloc(16 * numberOfBlocks, 32);
	memcpy(paddedInput, input, inputBytes);
	((uint8_t*)paddedInput)[inputBytes] = 0x80; // append 1 bit in big-endian
	
	for(uint8_t i = 0; i < (numberOfBlocks * 16) - 1; i++)
	{
		paddedInput[i] = LittleToBig(paddedInput[i]);
	}
	
	paddedInput[((numberOfBlocks * 512 - 64) / 32) + 1] = inputBits;
	
	for(uint8_t i = 0; i < numberOfBlocks; i++)
	{
		uint32_t* w = calloc(64, 32);
		memcpy(w, &paddedInput[i * 16], 512);
		
		for(uint8_t j = 16; j < 64; j++) //(64)
		{
			uint32_t s0 = RightRotate(w[j-15], 7) ^ RightRotate(w[j-15], 18) ^ (w[j-15] >> 3);
			uint32_t s1 = RightRotate(w[j-2], 17) ^ RightRotate(w[j-2], 19) ^ (w[j-2] >> 10);
			w[j] = w[j-16] + s0 + w[j-7] + s1;
		}
		
		uint32_t a = H[0]; uint32_t b = H[1]; uint32_t c = H[2]; uint32_t d = H[3];
		uint32_t e = H[4]; uint32_t f = H[5]; uint32_t g = H[6]; uint32_t h = H[7];
		for(uint8_t j = 0; j < 64; j++)
		{
			
			uint32_t temp1 = h + UppercaseSigmaOne(e) + Choose(e, f, g) + k[j] + w[j];
			uint32_t temp2 = UppercaseSigmaZero(a) + Majority(a, b, c);
			h = g;
			g = f;
			f = e;
			e = d + temp1;
			d = c;
			c = b;
			b = a;
			a = temp1 + temp2;
		}
		
		H[0] += a; H[1] += b; H[2] += c; H[3] += d;
		H[4] += e; H[5] += f; H[6] += g; H[7] += h;
		
		
		free(w);
		}
		free(paddedInput);
}


void PrintHash(uint32_t* hash)
{
    for (int i = 0; i < 8; i++){printf("%.2x ", hash[i]);}
    printf("\n");
}

void PrintByteArray(int length,uint8_t* byteArray)
{
    for (int i = 0; i < length; i++){printf("%.2x", byteArray[i]);}
    printf("\n");
}

void HashToByteArray(uint32_t* hash, uint8_t* byteArray)
{
	int j = 0;
	for(int i = 0; i < 8; i++,j+=4)
	{
		byteArray[j] = (hash[i] >> 24) & 0xFF;
		byteArray[j+1] = (hash[i] >> 16) & 0xFF;
		byteArray[j+2] = (hash[i] >> 8) & 0xFF;
		byteArray[j+3] = (hash[i] >> 0) & 0xFF;
	}
}


void SingleSha256()
{
	uint32_t hash[8]; uint8_t byteArray[32];
	uint8_t msg[] = {"kjghfgdsadrsteysadasdasdasdadasdasdasdasdaugowfsdofdsofgusdf"};
   	sha256_hash(msg, (uint64_t)strlen((char*)msg), hash,0);
   	HashToByteArray(hash,byteArray);
	PrintHash(hash);
	//PrintByteArray(32,byteArray);	
}

int main()
{
	SingleSha256();
	return 0;
}






