/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *           HASHMAP SUPPORTED BY GROWABLE ARRAY          * */
/* *                                                        * */
/* ********************************************************** */
/**************************************************************/

/**************************************************************/                                  
/* *  This program is free software; you can redistribute   * */
/* *  it and/or modify it under the terms of the FreeBSD    * */
/* *  Licence.                                              * */
/* *                                                        * */
/* *  This program is distributed in the hope that it will  * */
/* *  be useful, but WITHOUT ANY WARRANTY; without even     * */
/* *  the implied warranty of MERCHANTABILITY or FITNESS    * */
/* *  FOR A PARTICULAR PURPOSE.  See the LICENCE file       * */
/* *  for more details.                                     * */
/**************************************************************/

#include "hashmap.h"

/**************************************************************/
/* Functions                                                  */
/**************************************************************/

void hm_Init(void)
/**************************************************************
  INPUT:   None.
  RETURNS: void.
  SUMMARY: Placebo satisfying the general module layout.
***************************************************************/
{
  return;
}

void hm_Free(void)
/**************************************************************
  INPUT:   None.
  RETURNS: void.
  SUMMARY: Placebo satisfying the general module layout.
***************************************************************/
{
  return;
}

HASHMAP_HASH hm_StringHash(const char* key) 
/**************************************************************
  INPUT:   A string.
  RETURNS: A hash of a given string.
  COMMENT: The formula is made to ensure both
           admissable speed and hashing robustness.
***************************************************************/
{
  int i;
  HASHMAP_HASH result;
  
  i      = 0;
  result = 0;
  while (key[i] != '\0') {
    result = result*31 + key[i];
    i++;
  }
  return result;
}

HASHMAP hm_Create(int init_size_bits)
/**************************************************************
  INPUT:   
     An int denoting the logarithm of the initial size
     (the new size will be (1<<init_size_bits)).
  RETURNS: A newly allocated hashmap.
  MEMORY:  Allocates a new hashmap struct
            and the internal array.
***************************************************************/
{
  int i;
  HASHMAP hm;

  hm             = (HASHMAP)memory_Malloc(sizeof(HASHMAP_NODE));
  hm->size       = (1<<init_size_bits);
  hm->table      = (LIST*)memory_Malloc(sizeof(LIST)*hm->size);
  hm->num_of_els = 0;
    
  for(i = 0; i < hm->size; i++)
    hm->table[i] = list_Nil();
  
  return hm;
}

void hm_Delete(HASHMAP hm)
/**************************************************************
  INPUT:   Pointer to the hashmap to be deleted.
  MEMORY:  Frees the memory associated with the given hashmap.
***************************************************************/
{
  int i;
  for(i = 0; i < hm->size; i++)
    list_DeleteWithElement(hm->table[i],(void (*)(POINTER))list_PairFree);  
  
  memory_Free(hm->table,sizeof(LIST)*hm->size);
  memory_Free(hm,sizeof(HASHMAP_NODE));
}

static __inline__ unsigned hm_IndexFor(HASHMAP_HASH h, int length) 
/**************************************************************
  Transformes a given hash to an address in an array of a given length.
  The length must be a power of 2.
***************************************************************/
{
  return h & (length-1);
}

void hm_Insert(HASHMAP hm, POINTER value, HASHMAP_HASH hash)
/**************************************************************
  INPUT:   A hashmap, an element to be inserted and its hash.
  EFFECT:  Inserts a given value in the hashmap accoring to its hash.           

  The function doesn't check if the key was already stored
  (which should not be the case).

  If the number of values stored in the hashmap
  exceeds a double of the current table size
  the table is doubled, new array allocated,
  and the records are rehashed. (This is completely
  heuristical and could be modified to exchange
  speed for memory and vice versa.)
***************************************************************/
{
  int i;

  i = hm_IndexFor(hash,hm->size);  
  hm->table[i] = list_Cons(list_PairCreate((POINTER)hash,value),hm->table[i]);
  
  if (++hm->num_of_els > (hm->size << 1)) { /*grow and rehash*/
    int   new_size;
    LIST* new_table;

    new_size  = hm->size << 1;
    new_table = (LIST*)memory_Malloc(sizeof(LIST)*new_size);
    
    for(i = 0; i < new_size; i++)
      new_table[i] = list_Nil();

    for(i = 0; i < hm->size; i++) {
      LIST Scan;
      Scan = hm->table[i];
      
      while (!list_Empty(Scan)) {
        int j;
        LIST Elem, Pair;
        
        Elem = Scan;
        Pair = list_Car(Elem);
        hash = (HASHMAP_HASH)list_PairFirst(Pair);
        j = hm_IndexFor(hash,new_size);

        Scan = list_Cdr(Scan);
        list_Rplacd(Elem,new_table[j]);
        new_table[j] = Elem;
      }
    }
    
    memory_Free(hm->table,sizeof(LIST)*hm->size);
    hm->size = new_size;
    hm->table = new_table;
  }
}

static __inline__ BOOL hm_ValueTest(POINTER value, POINTER pr)
/**************************************************************
  Test function used to find an element in a hashmap's list.
  The first parameter is in fact just a value
  to be matched with the second part of pair thats stored in the list.

  (Used only in hm_Remove)
***************************************************************/
{  
  return list_PairSecond((LIST)pr) == value;
}

void hm_Remove(HASHMAP hm, POINTER value, HASHMAP_HASH hash)
/**************************************************************
  INPUT:   A hashmap, an element to be inserted and its hash.
  EFFECT:  Removes the given value from appropriate list in the hashmap's table.
***************************************************************/
{
  int i;
  LIST List;

  i            = hm_IndexFor(hash,hm->size);
  List         = hm->table[i];
  hm->table[i] = list_DeleteElementFree(List, value, hm_ValueTest, (void (*)(POINTER))list_PairFree);
  hm->num_of_els--;
}

POINTER hm_Retrieve(HASHMAP hm, HASHMAP_HASH hash, BOOL(*test)(POINTER))
/**************************************************************
  INPUT:   A hashmap, element's hash, and a test function that
           should recognize if a given a value stored in the hashmap
           is the one looked for (and for such return TRUE)
  RETURNS: The value found or HASHMAP_NO_RETRIEVE if the search fails.
***************************************************************/
{
  int i;
  LIST Scan;
  
  i    = hm_IndexFor(hash,hm->size);
  Scan = hm->table[i];

  while (!list_Empty(Scan)) {
    LIST Pair;
    HASHMAP_HASH h;
    
    Pair = list_Car(Scan);
    h    = (HASHMAP_HASH)list_PairFirst(Pair);
    
    if (h == hash) { /*could be him*/
      POINTER value;
      
      value = list_PairSecond(Pair);
      if (test(value))
        return value;      
    }
    
    Scan = list_Cdr(Scan);
  }

  return HASHMAP_NO_RETRIEVE;
}
