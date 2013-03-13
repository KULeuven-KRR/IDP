/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *               NAIVE CNF TRANSLATOR                     * */
/* *                                                        * */
/* *  $Module:   CNF                                        * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1997, 1998, 1999, 2000            * */
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


/* $RCSfile: cnf.h,v $ */

/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#ifndef _CNF_
#define _CNF_

#include "hasharray.h"
#include "renaming.h"
#include "resolution.h"
#include "search.h"
#include "flags.h"

#include <string.h>

/**************************************************************/
/* Functions                                                 */
/**************************************************************/

static __inline__ BOOL cnf_LabelEqual(const char* l1, const char* l2)
/********************************************************
  INPUT:   Two labels.
  RETURNS: TRUE, if the labels are equal, FALSE otherwise.
*********************************************************/
{
  return string_Equal(l1, l2);
}


static __inline__ LIST cnf_DeleteDuplicateLabelsFromList(LIST Labels)
/********************************************************
  INPUT:   A list of labels.
  RETURNS: The list where duplicate labels are removed.
  EFFECTS: The duplicate labels are not freed.
*********************************************************/
{
  return list_DeleteDuplicates(Labels, (BOOL (*)(POINTER,POINTER))cnf_LabelEqual);
}


TERM        cnf_ApplyDefinitionOnce(TERM, TERM, TERM, TERM, FLAGSTORE);
LIST        cnf_ApplyDefinitionToClause(CLAUSE, TERM, TERM,FLAGSTORE,PRECEDENCE);

BOOL        cnf_ContainsDefinition(TERM, TERM*);
BOOL        cnf_ContainsPredicate(TERM, SYMBOL, TERM*, TERM*, LIST*, LIST*);

TERM        cnf_DeSkolemFormula(LIST);
TERM        cnf_DefConvert(TERM, TERM, TERM*);
void        cnf_FilePrint(TERM, FILE*);
TERM        cnf_DefTargetConvert(TERM, TERM, TERM, LIST, LIST, LIST, LIST,
				 FLAGSTORE, PRECEDENCE, BOOL*);

void        cnf_FilePrintPrefix(TERM, FILE*);
void        cnf_FPrint(TERM, FILE*);
TERM        cnf_Flatten(TERM, SYMBOL);
PROOFSEARCH cnf_Flotter(LIST, LIST, LIST*, LIST*, HASH, HASH, FLAGSTORE,
			PRECEDENCE, LIST*);
void        cnf_Free(FLAGSTORE);

LIST        cnf_HandleDefinition(PROOFSEARCH, LIST, LIST, LIST, LIST);
void        cnf_Init(FLAGSTORE);
TERM        cnf_NegationNormalFormula(TERM);
TERM        cnf_ObviousSimplifications(TERM);

LIST        cnf_QueryFlotter(PROOFSEARCH, TERM, LIST*);
void        cnf_StdoutPrint(TERM);

BOOL        cnf_PropagateSubstEquations(TERM);

BOOL        cnf_HaveProof(LIST, TERM, FLAGSTORE, PRECEDENCE);


#endif
