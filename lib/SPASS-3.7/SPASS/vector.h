/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *             GLOBAL SYSTEM VECTOR                       * */
/* *                                                        * */
/* *  $Module:   VECTOR                                     * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1998, 1999, 2000, 2001            * */
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
/* $Date: 2010-02-22 14:10:00 $                             * */
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


/* $RCSfile: vector.h,v $ */


#ifndef _VECTOR_
#define _VECTOR_

#include "misc.h"


/**************************************************************/
/* Type and Variable Definitions                              */
/**************************************************************/

#define vec_SIZE 20000

typedef POINTER VECTOR[vec_SIZE];

extern VECTOR vec_VECTOR;
extern int    vec_MAX;

/**************************************************************/
/* Inline Functions                                           */
/**************************************************************/

/* Stack operations */

static __inline__ void vec_Init(void)
{
  vec_MAX = 0;
}

static __inline__ void vec_Push(POINTER Value)
{
#ifdef CHECK
  if (vec_MAX >= vec_SIZE) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In vec_Push: Vector Overflow.");
    misc_FinishErrorReport();
  }
#endif

  vec_VECTOR[vec_MAX++] = Value;
}


static __inline__ POINTER vec_GetNth(NAT Index)
{
#ifdef CHECK
  if (Index >= vec_SIZE || Index >= vec_MAX) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In vec_GetNth: Illegal vector access.");
    misc_FinishErrorReport();
  }
#endif

  return vec_VECTOR[Index];
}


static __inline__ void vec_PutNth(NAT Index, POINTER Value)
{
#ifdef CHECK
  if (Index >= vec_SIZE || Index >= vec_MAX) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In vec_PutNth: Illegal vector access.");
    misc_FinishErrorReport();
  }
#endif

  vec_VECTOR[Index] = Value;
}


static __inline__ void vec_Pop(void)
{
  --vec_MAX;
}

static __inline__ POINTER vec_PopResult(void)
{
  return vec_VECTOR[--vec_MAX];
}

static __inline__ void vec_PopToN(int N)
{
  vec_VECTOR[N] = vec_VECTOR[--vec_MAX];
}

static __inline__ void vec_NPop(int N)
{
  vec_MAX -= N;
}

static __inline__ POINTER vec_Top(void)
{
  return vec_VECTOR[vec_MAX-1];
}

static __inline__ POINTER vec_NthTop(int N)
{
  return vec_VECTOR[vec_MAX-(1+N)];
}


static __inline__ void vec_RplacTop(POINTER Value)
{
  vec_VECTOR[vec_MAX-1] = Value;
}

static __inline__ void vec_RplacNthTop(int N, POINTER Value)
{
  vec_VECTOR[vec_MAX-(1+N)] = Value;
}

static __inline__ int vec_ActMax(void)
{
  return vec_MAX;
}

static __inline__ void vec_SetMax(int Index)
{
  vec_MAX = Index;
}

static __inline__ BOOL vec_IsMax(int Index)
{
  return vec_MAX == Index;
}

static __inline__ BOOL vec_IsEmpty(void)
{
  return vec_MAX == 0;
}

/**************************************************************/
/* Prototypes                                                 */
/**************************************************************/

void vec_Swap(int, int);
void vec_PrintSel(int, int, void(*)(POINTER));
void vec_PrintAll(void(*)(POINTER));

#endif
