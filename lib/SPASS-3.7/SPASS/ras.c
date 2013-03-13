/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *             RANDOM ACCESS STACK                        * */
/* *                                                        * */
/* *  $Module:   RAS                                        * */ 
/* *                                                        * */
/* *  Copyright (C) 2005 MPI fuer Informatik                * */
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

/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include "ras.h"

/**************************************************************/
/* Functions                                                  */
/**************************************************************/

RAS ras_InitWithSize(RAS ras, int size)
/****************************************************************
  INPUT:   A random access stack the maximal expected size of the
           stack to init.
  RETURNS: The initialized and potentially new stack.
  CAUTION: Because it potentially frees the old stack this
           function must be called inside an assignment like:
              stack = ras_InitWithSize(stack, ...)
*****************************************************************/
{

#ifdef CHECK
  if (size <= 0) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In ras_InitWithSize: size not positive.");
    misc_FinishErrorReport();
  }
#endif

  if (size > (int) ras[ras_alloc]) {
    ras_Free(ras);
    ras = ras_CreateWithSize(size);
  }
  else
    ras[ras_top] = (POINTER) 0;
  return ras;
}

