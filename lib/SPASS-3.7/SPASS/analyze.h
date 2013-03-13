/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                ANALYSIS OF CLAUSE SETS                 * */
/* *                                                        * */
/* *  $Module:   ANALYZE                                    * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001      * */
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
/* $Date: 2010-02-22 14:09:57 $                             * */
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


/* $RCSfile: analyze.h,v $ */
 
#ifndef _ANALYZE_
#define _ANALYZE_

#include "search.h"
#include "clause.h"
#include "sort.h"
#include "list.h"
#include "graph.h"

/**************************************************************/
/* Global Variables                                           */
/**************************************************************/

extern BOOL ana_EQUATIONS;
extern BOOL ana_SORTRES;
extern BOOL ana_USORTRES;
extern BOOL ana_SORTDECEQUATIONS;
extern BOOL ana_PUREPROPOSITIONAL;

extern LIST ana_FINITEMONADICPREDICATES;

/**************************************************************/
/* Inline Functions                                           */
/**************************************************************/

static __inline__ BOOL ana_Equations(void)
{
  return ana_EQUATIONS;
}

static __inline__ BOOL ana_SortRestrictions(void)
{
  return ana_SORTRES;
}

static __inline__ BOOL ana_PurePropositional(void)
{
  return ana_PUREPROPOSITIONAL;
}

static __inline__ BOOL ana_UnsolvedSortRestrictions(void)
{
  return ana_USORTRES;
}

static __inline__ BOOL ana_SortDecreasing(void)
{
  return ana_SORTDECEQUATIONS;
}

static __inline__ void ana_Init(void)
{
  ana_FINITEMONADICPREDICATES = list_Nil();
}

static __inline__ void ana_Free(void)
{
  list_Delete(ana_FINITEMONADICPREDICATES);
}

static __inline__ LIST ana_FinMonPredList(void)
{
  return ana_FINITEMONADICPREDICATES;
}





/**************************************************************/
/* Inline Functions                                           */
/**************************************************************/

void ana_AnalyzeProblem(PROOFSEARCH, LIST);
void ana_AnalyzeSortStructure(LIST, FLAGSTORE, PRECEDENCE);
void ana_AutoConfiguration(LIST, FLAGSTORE, PRECEDENCE);
void ana_ExploitSortAnalysis(FLAGSTORE);
void ana_Print(FLAGSTORE, PRECEDENCE);

#endif
