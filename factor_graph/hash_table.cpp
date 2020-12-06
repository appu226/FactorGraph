/*

Copyright 2019 Parakram Majumdar

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/



#include "hash_table.h"

#define TABLE_SIZE 100

int hash_index[TABLE_SIZE];
bdd_ptr *G[TABLE_SIZE];
int Gs[TABLE_SIZE];
bdd_ptr V[TABLE_SIZE];
bdd_ptr *R[TABLE_SIZE];
int Rs[TABLE_SIZE];
int TIME;
int last_used[TABLE_SIZE];
DdManager *HTM;

char BS[] = "0110001111111110011110101101101110010101110001010001011100110001000100010110011111000010001000110100101110000111111111011110101011011001001000011101000000010110001101000110111101010010011101101011101110111000001000011101011110000000011001001101100010101010001101100000100010100000010011001111000100111010001101111110010011111101000101100010001110100001110111011101011001101011101101000011110000110110000111001110010111010111110101100111001010001010111101000010010001111011011110101110000001000001110111010101101010010011011010101111010001111000101110011010010100011110011111100001000101100101100111110000010010111101010110000010101010011011100011110101100001000101100101011010111000100101101111011001101000001100110010100011110010100001101011010011001000011010100110000100011110000000001010000111101010001110110111101000000010010111111000110101011011010111110111000010001011000101100110001110001100100111011101010101110001101011010001100110000010001001110111010010111100000011011010101111100100001111";
#define BSs 1000

/* LOCAL FUNCTION DECLARATIONS */
int hash_value(bdd_ptr *G, int Gs, bdd_ptr V);
void hash_table_delete_index(int i);

/* LOCAL FUNCTION DEFINITIONS*/

/* compute the hash value from the key 
 * INPUTS : G - array of bdd pointers to the functions
 *          Gs - size of array G
 *          V - set of variables to be projected upon
 * OUTPUT : the +ve hashed value, if successful
 *          -1, if unsuccessful
 */
int hash_value(bdd_ptr *G, int Gs, bdd_ptr V)
{
  bdd_ptr ss = bdd_vector_support(HTM, G, Gs);
  bdd_ptr ssp, v, ssv, temp, result;
  int bsp = 0;
  int output = 0;
  int loop, i, Gp;
  
  bdd_and_accumulate(HTM, &ss, V);
  
  for(loop = 0; loop < 32; loop++)
  {
    ssp = bdd_dup(ss);
    ssv = bdd_one(HTM);
    while(!bdd_is_one(HTM, ssp))
    {
      v = bdd_new_var_with_index(HTM, bdd_get_lowest_index(HTM, ssp));
      if(BS[bsp] == '0')
      	bdd_and_accumulate(HTM, &ssv, bdd_not(v));
      else
        bdd_and_accumulate(HTM, &ssv, bdd_dup(v));

			bsp = (bsp+1)%BSs;
      temp = bdd_cube_diff(HTM, ssp, v);
      bdd_free(HTM, ssp);
      bdd_free(HTM, v);
      ssp = temp;
      temp = NULL;
    }
    
    result = bdd_one(HTM);
    for(Gp = 0; Gp < Gs && bdd_is_one(HTM, result); Gp++)
    {
      bdd_free(HTM, result);
      result = bdd_cofactor(HTM, G[Gp], ssv);
    }
    
    if(bdd_is_one(HTM, result))
      output = (output << 1) + 1;
    else
      output = (output << 1);
    bdd_free(HTM, ssp);
    bdd_free(HTM, result);
    bdd_free(HTM, ssv);
  }
  
  bdd_free(HTM, ss);
  
  return (output < 0 ? -output : output);
}

/** Clears the location at a given index in the hashtable
  */
void hash_table_delete_index(int i)
{
  int ctr;
  if(hash_index[i] == -1)
   return;
  hash_index[i] = -1;
  for(ctr = 0; ctr < Gs[i]; ctr++)
    bdd_free(HTM, G[i][ctr]);
  free(G[i]);
  G[i] = NULL;
  Gs[i] = -1;
  
  for(ctr = 0; ctr < Rs[i]; ctr++)
    bdd_free(HTM, R[i][ctr]);
  free(R[i]);
  R[i] = NULL;
  Rs[i] = -1;
  
  bdd_free(HTM, V[i]);
  V[i] = NULL;
  last_used[i] = -1;
}

/* EXTERNAL FUNCTION DEFINITIONS */
void hash_table_add(bdd_ptr * g, int gs, bdd_ptr v, bdd_ptr * r, int rs)
{
  int hash;
  int i, mintime;
  
  hash = hash_value(g, gs, v);
  TIME++;
  mintime = 0;
  for(i = 0; i < TABLE_SIZE; i++)
  {
    if(hash_index[i] == -1)
      break;
    if(last_used[i] < last_used[mintime])
      mintime = i;
  }
  if(i == TABLE_SIZE)
    i = mintime;
  
	hash_table_delete_index(i);
	
	G[i] = (bdd_ptr *)malloc(sizeof(bdd_ptr) * gs);
	R[i] = (bdd_ptr *)malloc(sizeof(bdd_ptr) * rs);
	if(G[i] == NULL || R[i] == NULL)
	{
	  if(G[i] != NULL)
	    free(G[i]);
	  if(R[i] != NULL)
	    free(R[i]);
	  G[i] = R[i] = NULL;
	  return;
	}
	
	hash_index[i] = hash;
	Gs[i] = gs;
	Rs[i] = rs;
	last_used[i] = TIME;
	
	for(mintime = 0; mintime < gs; mintime++)
	  G[i][mintime] = bdd_dup(g[mintime]);
 	for(mintime = 0; mintime < rs; mintime++)
	  R[i][mintime] = bdd_dup(r[mintime]);
	  
	V[i] = bdd_dup(v);
	
  return;
}


/** looks up a value from the hashtable.
  * IMPORTANT NOTE : the result is directly from the hashtable and should not be tampered with, nor freed
  * for any modifications, create a copy and then use
  */
bdd_ptr * hash_table_lookup(bdd_ptr * G, int Gs, bdd_ptr V, int *rs)
{
  int hash = hash_value(G, Gs, V);
  int i;
  
  for(i = 0; i < TABLE_SIZE; i++)
    if(hash_index[i] == hash)
      break;
  if(i == TABLE_SIZE)
    return NULL;
  *rs = Rs[i];
  return R[i];
}

void hash_table_init(DdManager *m)
{
  int i;
  HTM = m;
  for(i = 0; i < TABLE_SIZE; i++)
  {
    hash_index[i] = Gs[i] = Rs[i] = last_used[i] = -1;
    G[i] = R[i] = NULL;   
    V[i] = NULL;
  }
  TIME = 0;
}

void hash_table_clean()
{
  int i;
  for( i = 0; i < TABLE_SIZE; i++)
    hash_table_delete_index(i);
}


bdd_ptr * hash_table_lookup_fg(factor_graph * G, bdd_ptr V, int *rs)
{
return NULL;
}

void hash_table_add_fg(factor_graph * G, bdd_ptr V, bdd_ptr * r, int rs)
{return;}













