/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *           HASHMAP SUPPORTED BY GROWABLE ARRAY          * */
/* *                                                        * */
/* ********************************************************** */
/**************************************************************/

#ifndef _HASHMAP_
#define _HASHMAP_
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
/* Includes                                                   */
/**************************************************************/

#include "memory.h"
#include "misc.h"
#include "list.h"

/**************************************************************/
/* Structures                                                 */
/**************************************************************/

#define HASHMAP_NO_RETRIEVE ((POINTER)-1)

typedef unsigned HASHMAP_HASH;

typedef struct HASHMAP_HELP {
  LIST* table;            /* the actual array, it is an array of lists of pairs (hash, value) */
  int   size;             /* size of the table, has to be a power of two */
  int   num_of_els;       /* number of elements currently stored */
} HASHMAP_NODE;

typedef HASHMAP_NODE *HASHMAP;

/**************************************************************/
/* Functions                                                  */
/**************************************************************/

void         hm_Init(void);
void         hm_Free(void);

HASHMAP      hm_Create(int);
void         hm_Delete(HASHMAP);

HASHMAP_HASH hm_StringHash(const char*);

void         hm_Insert(HASHMAP,POINTER,HASHMAP_HASH);
void         hm_Remove(HASHMAP,POINTER,HASHMAP_HASH);

POINTER      hm_Retrieve(HASHMAP,HASHMAP_HASH,BOOL(*)(POINTER));

#endif
