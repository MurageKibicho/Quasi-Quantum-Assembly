#define SECP_HAS_EXPONENT
#include "../Curves/EllipticCurve/Elliptic_Secp.h"
#include "../Curves/PellCurve/Pell.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#include <math.h>
#include <gmp.h>

typedef struct hash_table_struct *HashTable;
typedef struct hash_table_entry_struct *HashTableEntry;

struct hash_table_entry_struct
{
	fmpz_t key;
	fmpz_t value;
};

struct hash_table_struct
{
	HashTableEntry **entry;//Handle collisions
	fmpz_t modulo;
	fmpz_t inverseY2;
	size_t capacity;
};

HashTableEntry CreateEntry()
{
	HashTableEntry entry = malloc(sizeof(struct hash_table_entry_struct));
	fmpz_init(entry->key);
	fmpz_init(entry->value);
	return entry;
}

void FreeHashTableEntry(HashTableEntry entry)
{
	if(entry)
	{
		fmpz_clear(entry->key);
		fmpz_clear(entry->value);
		free(entry);
	}
}

HashTable CreateHashTable(size_t capacity)
{
	HashTable table = malloc(sizeof(struct hash_table_struct));
	fmpz_init(table->modulo);
	fmpz_init(table->inverseY2);
	assert(capacity > 0);
	fmpz_set_ui(table->modulo, capacity);
	table->capacity = capacity;
	table->entry = malloc(capacity * sizeof(HashTableEntry*));
	for(size_t i = 0; i < capacity; i++)
	{
		table->entry[i] = NULL;
	}
	return table;
}

void DestroyHashTable(HashTable table)
{
	for(size_t i = 0; i < table->capacity; i++)
	{
		for(size_t j = 0; j < arrlen(table->entry[i]); j++)
		{
			FreeHashTableEntry(table->entry[i][j]);
		}
		arrfree(table->entry[i]);
	}
	fmpz_clear(table->inverseY2);
	fmpz_clear(table->modulo);
	free(table->entry);
	free(table);
}

void InitializeHashTable(HashTable table, fmpz_t x, fmpz_t y, fmpz_t temp0, fmpz_t prime)
{
	//Set y inverse square
	fmpz_mul_ui(table->inverseY2, y, 2);
	fmpz_invmod(table->inverseY2, table->inverseY2, prime);
	//n=0
	HashTableEntry entry0 = CreateEntry();
	fmpz_set_ui(entry0->key, 0);
	fmpz_set_ui(entry0->value, 0);
	arrput(table->entry[0],entry0);
	
	//n=1
	HashTableEntry entry1 = CreateEntry();
	fmpz_set_ui(entry1->key, 1);
	fmpz_set_ui(entry1->value, 1);
	arrput(table->entry[1],entry1);
	
	//n=2
	HashTableEntry entry2 = CreateEntry();
	fmpz_set_ui(entry2->key, 2);
	fmpz_mul_ui(entry2->value, y, 2);
	fmpz_mod(entry2->value, entry2->value, prime);
	arrput(table->entry[2],entry2);
	
	//n=3, ψ3 = 3x^4 + 84x
	HashTableEntry entry3 = CreateEntry();
	fmpz_set_ui(entry3->key, 3);
	fmpz_powm_ui(entry3->value, x, 4, prime);
	fmpz_mul_ui(entry3->value, entry3->value, 3);
	fmpz_mul_ui(temp0, x, 84);
	fmpz_add(entry3->value, entry3->value, temp0);
	fmpz_mod(entry3->value, entry3->value, prime);
	arrput(table->entry[3],entry3);
	
	//n=4, ψ₄ = 4y(x⁶ + 140x³ − 392)
	HashTableEntry entry4 = CreateEntry();
	fmpz_set_ui(entry4->key, 4);
	fmpz_powm_ui(temp0, x, 3, prime);
	fmpz_mul(entry4->value, temp0, temp0);
	
	fmpz_mul_ui(temp0, temp0, 140);
	fmpz_add(entry4->value, entry4->value, temp0);
	
	fmpz_sub_ui(entry4->value, entry4->value, 392);
	
	fmpz_mul(entry4->value, entry4->value, y);
	fmpz_mul_ui(entry4->value, entry4->value, 4);

	fmpz_mod(entry4->value, entry4->value, prime);
	arrput(table->entry[4],entry4);
	
}

void PrintTableContents(HashTable table)
{
	for(size_t i = 0; i < table->capacity; i++)
	{
		if(arrlen(table->entry[i]) > 0)
		{
			printf("Mod %3ld\n", i);
			for(size_t j = 0; j < arrlen(table->entry[i]); j++)
			{
				printf("\t");
				fmpz_print(table->entry[i][j]->key);
				printf(" : ");
				fmpz_print(table->entry[i][j]->value);
				printf("\n");
			}
		}
	}
}

bool FindElementHashTable(HashTable table, fmpz_t search, fmpz_t temp0, fmpz_t result)
{
	bool found = false;
	//Find modulo
	fmpz_mod(temp0, search, table->modulo);
	int searchIndex = fmpz_get_ui(temp0);
	for(size_t i = 0; i < arrlen(table->entry[searchIndex]); i++)
	{
		if(fmpz_cmp(search, table->entry[searchIndex][i]->key) == 0)
		{
			fmpz_set(result, table->entry[searchIndex][i]->value);
			found = true;
			break;
		}
	}
	return found;
}

void SetElementHashTable(HashTable table, fmpz_t n, fmpz_t temp0, fmpz_t result)
{
	//Find modulo
	fmpz_mod(temp0, n, table->modulo);
	int searchIndex = fmpz_get_ui(temp0);
	HashTableEntry entry = CreateEntry();
	fmpz_set(entry->key, n);
	fmpz_set(entry->value, result);
	arrput(table->entry[searchIndex],entry);
}

void DivisionPolynomial_Recursive(HashTable table, fmpz_t result, fmpz_t n, fmpz_t x, fmpz_t y, fmpz_t prime, fmpz_t temp0)
{
	bool found = FindElementHashTable(table, n, temp0, result);
	if(found == true)
	{
		return;
	}
	else
	{
		//Recurse
		if(fmpz_is_odd(n))
		{
			fmpz_t k, oddTemp, psikMinus1, psik, psikPlus1, psikPlus2;
			fmpz_init(k);fmpz_init(oddTemp);fmpz_init(psikMinus1);fmpz_init(psik);fmpz_init(psikPlus1);fmpz_init(psikPlus2);
			fmpz_sub_ui(k, n, 1);
			fmpz_divexact_ui(k,n,2);
			
			//ψk-1
			fmpz_sub_ui(k, k , 1);
			DivisionPolynomial_Recursive(table, psikMinus1, k, x, y, prime, oddTemp);	
		
			//ψk
			fmpz_add_ui(k, k , 1);
			DivisionPolynomial_Recursive(table, psik, k, x, y, prime, oddTemp);	
		
			//ψk
			fmpz_add_ui(k, k , 1);
			DivisionPolynomial_Recursive(table, psikPlus1, k, x, y, prime, oddTemp);	
		
			//ψk+2
			fmpz_add_ui(k,k , 1);
			DivisionPolynomial_Recursive(table, psikPlus2, k, x, y, prime, oddTemp);	
	
			//term = (compute_psi(k+2)*pow(compute_psi(k),3,p) - compute_psi(k-1)*pow(compute_psi(k+1),3,p)) % p
			fmpz_powm_ui(psik, psik, 3, prime);
			fmpz_powm_ui(psikPlus1, psikPlus1, 3, prime);
			
			fmpz_mul(result, psikPlus2, psik);
			fmpz_mul(oddTemp,psikPlus1, psikMinus1);
			fmpz_sub(result, result, oddTemp);
			fmpz_mod(result, result, prime);
			
			fmpz_clear(k);fmpz_clear(oddTemp);fmpz_clear(psikMinus1);fmpz_clear(psik);fmpz_clear(psikPlus1);fmpz_clear(psikPlus2);
		}
		else
		{
			fmpz_t k, oddTemp, psikMinus2, psikMinus1, psik, psikPlus1, psikPlus2;
			fmpz_init(k);fmpz_init(oddTemp);fmpz_init(psikMinus2);fmpz_init(psikMinus1);fmpz_init(psik);fmpz_init(psikPlus1);fmpz_init(psikPlus2);
			
			fmpz_divexact_ui(k,n,2);
			
			//ψk-2
			fmpz_sub_ui(k, k , 2);
			DivisionPolynomial_Recursive(table, psikMinus2, k, x, y, prime, oddTemp);	
		
			//ψk-1
			fmpz_add_ui(k, k , 1);
			DivisionPolynomial_Recursive(table, psikMinus1, k, x, y, prime, oddTemp);	
		
			//ψk
			fmpz_add_ui(k, k , 1);
			DivisionPolynomial_Recursive(table, psik, k, x, y, prime, oddTemp);	
		
			//ψk
			fmpz_add_ui(k, k , 1);
			DivisionPolynomial_Recursive(table, psikPlus1, k, x, y, prime, oddTemp);	
		
			//ψk+2
			fmpz_add_ui(k,k , 1);
			DivisionPolynomial_Recursive(table, psikPlus2, k, x, y, prime, oddTemp);	
	
			//numerator = (compute_psi(k+2)*pow(compute_psi(k-1),2,p) - compute_psi(k-2)*pow(compute_psi(k+1),2,p)) % p
           		//denom_inv = pow(2*y,p-2,p)
           		//term = (compute_psi(k)*numerator*denom_inv) % p
			fmpz_powm_ui(psikMinus1, psikMinus1, 2, prime);
			fmpz_powm_ui(psikPlus1, psikPlus1, 2, prime);
			
			fmpz_mul(psikMinus2, psikMinus2, psikPlus1);
			fmpz_mul(psikPlus2, psikPlus2, psikMinus1);
			
			fmpz_sub(psikPlus2, psikPlus2, psikMinus2);
			
			fmpz_mul(result, psik, psikPlus2);
			fmpz_mul(result, result, table->inverseY2);
			fmpz_mod(result, result, prime);
			
			fmpz_clear(k);fmpz_clear(oddTemp);fmpz_clear(psikMinus2);fmpz_clear(psikMinus1);fmpz_clear(psik);fmpz_clear(psikPlus1);fmpz_clear(psikPlus2);
		}
		//Store result 
		SetElementHashTable(table, n, temp0, result);
	}
}

void TestDivisionPolynomial()
{
	fmpz_t x, y, prime, n, result, temp0;
	fmpz_init(x);fmpz_init(y);fmpz_init(n);fmpz_init(prime);fmpz_init(result);fmpz_init(temp0);
	
	fmpz_set_ui(prime, 20959);
	fmpz_set_ui(x, 3248);
	fmpz_set_ui(y, 3059);
	fmpz_set_ui(n, 200);
	
	size_t tableCapacity = 10;
	HashTable table = CreateHashTable(tableCapacity);
	InitializeHashTable(table, x, y, temp0, prime);
	
	fmpz_set_ui(n, 0);
	bool find0Test = FindElementHashTable(table, n, temp0, result);
	assert(find0Test == true);
	
	fmpz_set_ui(n, 1);
	bool find1Test = FindElementHashTable(table, n, temp0, result);
	assert(find1Test == true);
	
	fmpz_set_ui(n, 2);
	bool find2Test = FindElementHashTable(table, n, temp0, result);
	assert(find2Test == true);
	
	fmpz_set_ui(n, 3);
	bool find3Test = FindElementHashTable(table, n, temp0, result);
	assert(find3Test == true);
	
	fmpz_set_ui(n, 4);
	bool find4Test = FindElementHashTable(table, n, temp0, result);
	assert(find4Test == true);
	
	fmpz_set_ui(n, 5);
	bool find5Test = FindElementHashTable(table, n, temp0, result);
	assert(find5Test == false);
	DivisionPolynomial_Recursive(table, result, n, x, y, prime, temp0);
	
	fmpz_set_ui(n, 6);
	DivisionPolynomial_Recursive(table, result, n, x, y, prime, temp0);
	
	fmpz_set_ui(n, 100000);
	DivisionPolynomial_Recursive(table, result, n, x, y, prime, temp0);
	printf("100000: ");fmpz_print(result);printf("\n");
	
	PrintTableContents(table);
	DestroyHashTable(table);
	fmpz_clear(x);fmpz_clear(y);fmpz_clear(n);fmpz_clear(prime);fmpz_clear(result);fmpz_clear(temp0);
}




int main()
{
	TestDivisionPolynomial();
	return 0;
}
