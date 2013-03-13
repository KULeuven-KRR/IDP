/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                  CLAUSE APPROXIMATION                  * */
/* *                                                        * */
/* *  $Module:   APPROX                                     * */ 
/* *                                                        * */
/* *  Copyright (C) 1998, 1999, 2000, 2001                  * */
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

/* $RCSfile: approx.h,v $ */

#ifndef _APPROX_
#define _APPROX_

#include "list.h"
#include "clause.h"

void approx_Init(void);

LIST approx_MonadicByProjection(LIST, FLAGSTORE, PRECEDENCE);
LIST approx_MonadicByTermEncoding(LIST, FLAGSTORE, PRECEDENCE);
LIST approx_MonadicMakeLinear(LIST, FLAGSTORE, PRECEDENCE);
LIST approx_HornMonadicFlattenHeads(CLAUSE, BOOL, BOOL, FLAGSTORE, PRECEDENCE);
LIST approx_MakeHorn(CLAUSE, FLAGSTORE, PRECEDENCE);

#endif
