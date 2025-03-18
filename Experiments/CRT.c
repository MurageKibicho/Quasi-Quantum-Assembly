#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>

/* 
 * Implements the Chinese Remainder Theorem
 * fieldOrder: number of congruences/moduli
 * finiteField: array of residues (the 'a' values in a ≡ a_i (mod m_i))
 * mods: array of moduli (the 'm' values)
 * result: output parameter to store the result
 * 
 * Assumes all moduli are pairwise coprime
 */
void ChineseRemainderTheorem(int fieldOrder, int *finiteField, int *mods, mpz_t result) 
{
    mpz_t *m = malloc(fieldOrder * sizeof(mpz_t));        // Array to hold moduli as mpz_t
    mpz_t *a = malloc(fieldOrder * sizeof(mpz_t));        // Array to hold residues as mpz_t
    mpz_t M, M_i, M_i_inv, temp;                          // Various mpz_t variables for computation
    
    // Initialize GMP variables
    mpz_init_set_ui(result, 0);
    mpz_init_set_ui(M, 1);
    mpz_init(M_i);
    mpz_init(M_i_inv);
    mpz_init(temp);
    
    // Convert input arrays to mpz_t and calculate product of all moduli (M)
    for (int i = 0; i < fieldOrder; i++) {
        mpz_init_set_ui(m[i], mods[i]);
        mpz_init_set_ui(a[i], finiteField[i]);
        mpz_mul(M, M, m[i]);                              // M = m[0] * m[1] * ... * m[n-1]
    }
    
    // Apply Chinese Remainder Theorem formula
    for (int i = 0; i < fieldOrder; i++) {
        mpz_divexact(M_i, M, m[i]);                       // M_i = M / m[i]
        
        // Calculate modular multiplicative inverse of M_i modulo m[i]
        // M_i * M_i_inv ≡ 1 (mod m[i])
        mpz_invert(M_i_inv, M_i, m[i]);
        
        // temp = a[i] * M_i * M_i_inv
        mpz_mul(temp, a[i], M_i);
        mpz_mul(temp, temp, M_i_inv);
        
        // result = (result + temp) % M
        mpz_add(result, result, temp);
    }
    
    // Ensure result is in the range [0, M-1]
    mpz_mod(result, result, M);
    
    // Free allocated memory
    for (int i = 0; i < fieldOrder; i++) {
        mpz_clear(m[i]);
        mpz_clear(a[i]);
    }
    free(m);
    free(a);
    
    mpz_clear(M);
    mpz_clear(M_i);
    mpz_clear(M_i_inv);
    mpz_clear(temp);
}

// Example usage
int main() {
    mpz_t result;
    mpz_init(result);
    
    // Example: solve the system
    // x ≡ 2 (mod 3)
    // x ≡ 3 (mod 5)
    // x ≡ 2 (mod 7)
    
    int residues[] = {3 ,0 ,1 ,13, 4,37}; 
    int moduli[]   = {11,13,19,29,37,53};  
    
    ChineseRemainderTheorem(3, residues, moduli, result);
    
    gmp_printf("Result: %Zd\n", result);   // Should print 23
    
    mpz_clear(result);
    return 0;
}
