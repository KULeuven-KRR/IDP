/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *               SPLITTING OF CLAUSES                     * */
/* *                                                        * */
/* *  $Module:   SPLIT                                      * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1997, 1998, 2000                  * */
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


/* $RCSfile: rules-split.h,v $ */

#ifndef _RULES_SPLIT_
#define _RULES_SPLIT_

/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include "clause.h"
#include "search.h"


/**************************************************************/
/* Function Prototypes                                        */
/**************************************************************/        

LIST   split_Backtrack(PROOFSEARCH, CLAUSE, CLAUSE*);
void   split_KeepClauseAtLevel(PROOFSEARCH, CLAUSE, int);
void   split_DeleteClauseAtLevel(PROOFSEARCH, CLAUSE, int);
LIST   split_ExtractEmptyClauses(LIST, LIST*);
CLAUSE split_SmallestSplitLevelClause(LIST);


#endif
