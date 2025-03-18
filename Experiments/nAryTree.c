#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <gmp.h>
typedef struct modular_tree_struct *ModularTree;
struct modular_tree_struct 
{
	int congruence;          
	int modulo;            
	int treeLevel;  
	mpz_t runningProduct; 
	ModularTree *children;
};

int GCD(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}
ModularTree CreateModularNode(int congruence, int modulo, int treeLevel)
{
	ModularTree newNode = malloc(sizeof(struct modular_tree_struct));
	newNode->congruence = congruence;
	newNode->modulo = modulo;
	newNode->treeLevel = treeLevel;
	mpz_init(newNode->runningProduct);
	newNode->children = NULL;
	return newNode;
}

void FindNodesAtLevel(ModularTree root, int newTreeLevel, ModularTree **result)
{
	if(root == NULL){return;}
	if(newTreeLevel == root->treeLevel + 1)
	{
		arrput(*result, root);
	}
	for(size_t i = 0; i < arrlen(root->children); i++)
	{
		FindNodesAtLevel(root->children[i], newTreeLevel, result);
	}
}


void PrintModularTree(ModularTree root , int level)
{
	printf("x ≡ %2d (mod %2d) [Tree Level: Root]\n", root->congruence, root->modulo);
	for(size_t i = 0; i < arrlen(root->children); i++)
	{
		printf("\tx ≡ %2d (mod %2d)\n", root->congruence, root->modulo);
	}

	
}

int group1[][2] = {
    {3, 10}, // x ≡ 3 (mod 10)
    {1, 2},  // x ≡ 1 (mod 2)
    {3, 5},  // x ≡ 3 (mod 5)
    {4, 5},  // x ≡ 4 (mod 5)
    {9, 10}, // x ≡ 9 (mod 10)
    {-1, 1} // Represents not picking any congruence from this group
};

int group2[][2] = {
    {1, 12}, // x ≡ 1 (mod 12)
    {1, 6},  // x ≡ 1 (mod 6)
    {-1, 1} // Represents not picking any congruence from this group
};

int group3[][2] = {
    {2, 9},  // x ≡ 2 (mod 9)
    {-1, 1} // Represents not picking any congruence from this group
};

int group4[][2] = {
    {3, 28}, // x ≡ 3 (mod 28)
    {5, 28}, // x ≡ 5 (mod 28)
    {1, 4},  // x ≡ 1 (mod 4)
    {2, 7},  // x ≡ 2 (mod 7)
    {9, 28}, // x ≡ 9 (mod 28)
    {3, 7},  // x ≡ 3 (mod 7)
    {-1, 1} // Represents not picking any congruence from this group
};

int group5[][2] = {
    {43, 52}, // x ≡ 43 (mod 52)
    {23, 26}, // x ≡ 23 (mod 26)
    {12, 13}, // x ≡ 12 (mod 13)
    {25, 26}, // x ≡ 25 (mod 26)
    {0, 52},  // x ≡ 0 (mod 52)
    {1, 52},  // x ≡ 1 (mod 52)
    {5, 52},  // x ≡ 5 (mod 52)
    {3, 26},  // x ≡ 3 (mod 26)
    {7, 52},  // x ≡ 7 (mod 52)
    {9, 52},  // x ≡ 9 (mod 52)
    {3, 13},  // x ≡ 3 (mod 13)
    {1, 4},   // x ≡ 1 (mod 4)
    {15, 52}, // x ≡ 15 (mod 52)
    {-1, 1}  // Represents not picking any congruence from this group
};
int IsPairwiseCoprime(int arr[], int size) {
    for (int i = 0; i < size; i++) {
        for (int j = i + 1; j < size; j++) {
            if (GCD(arr[i], arr[j]) != 1) {
                return 0; // Not pairwise coprime
            }
        }
    }
    return 1; // Pairwise coprime
}
int main()
{
	ModularTree root = CreateModularNode(-1,-1,-1);
	int g1 = sizeof(group1) / sizeof(group1[0]);
	int g2 = sizeof(group2) / sizeof(group2[0]);
	int g3 = sizeof(group3) / sizeof(group3[0]);
	int g4 = sizeof(group4) / sizeof(group4[0]);
	int g5 = sizeof(group5) / sizeof(group5[0]);
	int count = 0;
	for(int i = 0; i < g1; i++)
	{
		int a1 = group1[i][0];
		int m1 = group1[i][1];
		for(int j = 0; j < g2; j++)
		{
			int a2 = group2[j][0];
			int m2 = group2[j][1];
			for(int k = 0; k < g3; k++)
			{
				int a3 = group3[k][0];
				int m3 = group3[k][1];
				for(int l = 0; l < g4; l++)
				{
					int a4 = group4[l][0];
					int m4 = group4[l][1];
					for(int m = 0; m < g5; m++)
					{
						int a5 = group5[m][0];
						int m5 = group5[m][1];
						int aarr[5] = {0};
						aarr[0]=m1;aarr[1]=m2;aarr[2]=m3;aarr[3]=m4;aarr[4]=m5;
						int kk = IsPairwiseCoprime(aarr, 5);
						if(kk == 1)
						{
							printf("%3d : %3d, (%d %d),(%d %d),(%d %d),(%d %d),(%d %d)\n",count,kk, a1,m1,a2,m2,a3,m3,a4,m4,a5,m5);
							count += 1;
						}
					}
				}
			}
		}
	}

	PrintModularTree(root, 0);
	return 0;
}
