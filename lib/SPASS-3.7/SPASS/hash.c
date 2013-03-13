/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                 SIMPLE HASHING                         * */
/* *                                                        * */
/* *  $Module:   HASH                                       * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1999, 2000, 2001                  * */
/* *  MPI fuer Informatik                                   * */
/* *                                                        * */
/* *  This program is free software; you can redistribute   * */
/* *  it and/or modify it under the terms of the FreeBSD    * */
/* *  Licence.                                              * */
/* *                                                        * */
/* *  This program is distributed in the hope that it will  * */
/* *  be useful, but WITHOUT ANY WARRANTY; without even     * */
/* *  the implied warranty of MERCHANTABILITY or FITNESS    * */
/* *  FOR A PARTICULAR PURPOSE.  See the LICENCE file       * */
/* *  for more details.                                     * */
/* *                                                        * */
/* *                                                        * */
/* $Revision: 1.3 $                                         * */
/* $State: Exp $                                            * */
/* $Date: 2010-02-22 14:09:58 $                             * */
/* $Author: weidenb $                                       * */
/* *                                                        * */
/* *             Contact:                                   * */
/* *             Christoph Weidenbach                       * */
/* *             MPI fuer Informatik                        * */
/* *             Stuhlsatzenhausweg 85                      * */
/* *             66123 Saarbruecken                         * */
/* *             Email: spass@mpi-inf.mpg.de                * */
/* *             Germany                                    * */
/* *                                                        * */
/* ********************************************************** */
/**************************************************************/


/* $RCSfile: hash.c,v $ */

#include "hash.h"

/**************************************************************/
/* Global Variables                                           */
/**************************************************************/

LIST hash_TABLE[hash__SIZE];


/**************************************************************/
/* Functions                                                  */
/**************************************************************/

void hash_Init(void)
{
  int i;
  
  for (i = 0; i < hash__SIZE; i++)
    hash_PutList(i, list_Nil());
}

void hash_Reset(void)
{
  int  i;
  LIST Scan;
  
  for (i = 0; i < hash__SIZE; i++) {
    Scan = hash_List(i);
    while (!list_Empty(Scan)) {
      list_Free(list_Car(Scan));
      Scan = list_Cdr(Scan);
    }
    list_Delete(hash_List(i));
    hash_PutList(i, list_Nil());
  }
}

void hash_ResetWithValue(void (*ValueDelete)(POINTER))
{
  int  i;
  LIST Scan;
  
  for (i = 0; i < hash__SIZE; i++) {
    Scan = hash_List(i);
    while (!list_Empty(Scan)) {
      ValueDelete(list_PairSecond(list_Car(Scan)));
      list_Free(list_Car(Scan));
      Scan = list_Cdr(Scan);
    }
    list_Delete(hash_List(i));
    hash_PutList(i, list_Nil());
  }
}

void hash_Print(void (*ValuePrint)(POINTER))
{
  int  i;
  LIST Scan;
  
  for (i = 0; i < hash__SIZE; i++) {
    Scan = hash_List(i);
    printf("\n %d:",i);
    while (!list_Empty(Scan)) {
      ValuePrint(list_PairSecond(list_Car(Scan)));
      printf("\n   ");
      Scan = list_Cdr(Scan);
    }
  }
}

POINTER hash_Get(POINTER key)
{
  LIST Scan;

  Scan = hash_List(hash_Index(key));
  
  while (!list_Empty(Scan)) {
    if (list_PairFirst(list_Car(Scan)) == key)
      return list_PairSecond(list_Car(Scan));
    Scan = list_Cdr(Scan);
  }

  return NULL;
}
