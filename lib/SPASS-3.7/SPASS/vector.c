/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *             GLOBAL SYSTEM VECTOR                       * */
/* *                                                        * */
/* *  $Module:   VECTOR                                     * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 2000, 2001 MPI fuer Informatik    * */
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
/* $Date: 2010-02-22 14:10:00 $                             * */
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


/* $RCSfile: vector.c,v $ */

#include "vector.h"

/**************************************************************/
/* Global Variables                                           */
/**************************************************************/

VECTOR vec_VECTOR;
int    vec_MAX;


/**************************************************************/
/* Functions                                                  */
/**************************************************************/

void vec_Swap(int i, int j)
/**********************************************************
  INPUT:   Two integers i and j which designate the i-th and
           the j-th cell of the vector.
  RETURNS: None.
  CAUTION: The values in the i-th and the j-th cell in the
            vector are interchanged.
********************************************************/
{
  POINTER help;

  help  = vec_GetNth(i);
  vec_PutNth(i, vec_GetNth(j));
  vec_PutNth(j, help);
 
}


void vec_PrintSel(int beg, int end, void (*ElementPrint)(POINTER))
/**********************************************************
  INPUT:   An int for the start position, an int for
           the end position and a print function for
	   elements.
  RETURNS: None.
  EFFECT:  Writes the vector from beg to end to stdout.
  CAUTION: None.
********************************************************/
{
  int i;

  if (vec_ActMax() > 0) {
    for (i = beg; i < end; i++){
      printf("Entry %d:\t",i);
      ElementPrint(vec_GetNth(i));
      putchar('\n');
    }
  } else
    puts("Vector is empty");
}


void vec_PrintAll(void (*ElementPrint)(POINTER))
/**********************************************************
  INPUT:   A print function for the elements of the vector.
  RETURNS: None.
  EFFECT:  Writes the vector to stdout.
  CAUTION: None.
********************************************************/
{
  int i;

  if (vec_ActMax() > 0) {
    for (i = 0; i < vec_ActMax(); i++) {
      printf("Entry %d:\t", i);
      ElementPrint(vec_GetNth(i));
      putchar('\n');
    }
  } else
    puts("Vector is empty");
}
