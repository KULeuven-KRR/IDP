/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                RESOLUTION                              * */
/* *                                                        * */
/* *  $Module:   RESOLUTION                                 * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1998, 2001 MPI fuer Informatik    * */
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
/* $Date: 2010-02-22 14:09:59 $                             * */
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

/* $RCSfile: resolution.h,v $ */

#ifndef _RESOLUTION_
#define _RESOLUTION_

/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include "misc.h"
#include "unify.h"
#include "symbol.h"
#include "foldfg.h"
#include "st.h"
#include "subsumption.h"
#include "condensing.h"


/**************************************************************/
/* Functions                                                  */
/**************************************************************/


void    res_InsertClauseIndex(CLAUSE, st_INDEX); /* used by cnf */
void    res_DeleteClauseIndex(CLAUSE, st_INDEX); /* used by cnf */
CLAUSE  res_SelectLightestClause(LIST);          /* used by cnf */
BOOL    res_BackSubWithLength(CLAUSE, st_INDEX); /* used by cnf */
BOOL    res_HasTautology(CLAUSE);                /* used by cnf */

#endif
