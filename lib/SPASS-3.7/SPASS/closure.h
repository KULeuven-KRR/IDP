/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *             CONGRUENCE CLOSURE ALGORITHM               * */
/* *                                                        * */
/* *  $Module:   CLOSURE                                    * */ 
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
/* $Date: 2010-02-22 14:09:57 $                             * */
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

#ifndef _CLOSURE_
#define _CLOSURE_


/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include "clause.h"
#include "table.h"
#include "ras.h"


/**************************************************************/
/* Prototype                                                  */
/**************************************************************/

void cc_Init(void);
void cc_Free(void);
BOOL cc_Tautology(CLAUSE);


#endif

