/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *             PARTITION                                  * */
/* *                                                        * */
/* *  $Module:   PARTITION                                  * */ 
/* *                                                        * */
/* *  Copyright (C) 1999, 2000, 2001 MPI fuer Informatik    * */
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
/* $Revision: 1.2 $                                     * */
/* $State: Exp $                                            * */
/* $Date: 2010-02-22 14:09:58 $                             * */
/* $Author: weidenb $                                         * */
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

/* module manages partitions (i. e. sets of equivalence classes) of the set */
/* {0, 1, 2, ..., size - 1}                                                 */

#ifndef _PARTITION_
#define _PARTITION_


/**************************************************************/
/* Include                                                    */
/**************************************************************/

#include "memory.h"


/**************************************************************/
/* Constants and types                                        */
/**************************************************************/

#define part_CARD         -1  /* index of cardinality of partitioned set */
#define part_ALLOC        -2  /* index of size of allocated space */
#define part_STAMPCOUNTER -3  /* index of stampcounter */
#define part_HEAD          3  /* length of head for management purposes */


typedef int ELEMENT;
typedef ELEMENT ECLASS, *PARTITION;

/* an equivalence class is represented by one of its elements, the           */
/* _representative_; a partition is an array to hold the size of the         */
/* partitioned set and of the allocated space, the equivalence classes,      */
/* their sizes and stamps and the stampcounter:                              */
/*           STAMPCOUNTER ALLOC CARD                                         */
/*                      |   |   |                                            */
/* -size - 3, ..., -4, -3, -2, -1, 0, ..., size - 1, size, ..., 2 * size - 1 */
/* <--- stamp[] ----|              |--- class[] -->  |---- classsize[] ----> */


/**************************************************************/
/* Prototypes                                                 */
/**************************************************************/

PARTITION part_Create(int);
PARTITION part_Init(PARTITION, int);
ECLASS part_Find(PARTITION, ELEMENT);
  /* gets (the representative of) the class of the second argument */
PARTITION part_Union(PARTITION, ECLASS, ECLASS);
  /* unions the classes, the representative of the first class is the */
  /* representative of the union                                      */


/**************************************************************/
/* Inline functions                                           */
/**************************************************************/

static __inline__ void part_Free(PARTITION p)
{
  if (p != NULL)
    memory_Free(
      p - (p[part_ALLOC] - part_HEAD) / 3 - part_HEAD,
      p[part_ALLOC] * sizeof(int)
    );
}


static __inline__ int part_Size(PARTITION p)
{
  return p[part_CARD];
}


static __inline__ BOOL part_Element(PARTITION p, ELEMENT e)
{
  return 0 <= e && e < part_Size(p);
}


static __inline__ BOOL part_Equivalent(PARTITION p, ELEMENT e1, ELEMENT e2)
{
  return part_Find(p, e1) == part_Find(p, e2);
}


#endif

