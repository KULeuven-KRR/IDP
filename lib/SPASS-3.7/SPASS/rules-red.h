/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                REDUCTION RULES                         * */
/* *                                                        * */
/* *  $Module:   REDRULES                                   * */ 
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
/* $Revision: 1.4 $                                         * */
/* $State: Exp $                                            * */
/* $Date: 2010-02-22 14:09:59 $                             * */
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


/* $RCSfile: rules-red.h,v $ */

#ifndef _REDRULES_
#define _REDRULES_

#include "sort.h"
#include "subsumption.h"
#include "condensing.h"
#include "search.h"
#include "rules-split.h"
#include "rules-inf.h"
#include "doc-proof.h"
#include "clock.h"
#include "closure.h"

/**************************************************************/
/* Macros                                                     */
/**************************************************************/
#define CRW_CHECK_STACK stack_SIZE - ((stack_SIZE / 100) * 10)

/**************************************************************/
/* Constants                                                  */
/**************************************************************/

extern const NAT red_USABLE;
extern const NAT red_WORKEDOFF;
extern const NAT red_ALL;


/**************************************************************/
/* Functions                                                  */
/**************************************************************/

static __inline__ BOOL red_WorkedOffMode(NAT Mode)
{
  return (Mode == red_WORKEDOFF || Mode == red_ALL);
}

static __inline__ BOOL red_OnlyWorkedOffMode(NAT Mode)
{
  return (Mode == red_WORKEDOFF);
}

static __inline__ BOOL red_UsableMode(NAT Mode)
{
  return (Mode == red_USABLE || Mode == red_ALL);
}

static __inline__ BOOL red_AllMode(NAT Mode)
{
  return (Mode == red_ALL);
}


void   red_Init(void);
void   red_Free(void);


LIST   red_CompleteReductionOnDerivedClauses(PROOFSEARCH, LIST, NAT, int, NAT, int*);
CLAUSE red_ReductionOnDerivedClause(PROOFSEARCH, CLAUSE, NAT);
CLAUSE red_CompleteReductionOnDerivedClause(PROOFSEARCH, CLAUSE, NAT);
LIST   red_BackReduction(PROOFSEARCH, CLAUSE, NAT);
LIST   red_SatUnit(PROOFSEARCH, LIST);
LIST   red_SatInput(PROOFSEARCH);
LIST   red_ReduceInput(PROOFSEARCH, LIST);
BOOL   red_ClauseDeletion(SORTTHEORY, CLAUSE, FLAGSTORE, PRECEDENCE);
BOOL   red_ExplicitTautology(CLAUSE, FLAGSTORE, PRECEDENCE);


void   red_CheckSplitSubsumptionCondition(PROOFSEARCH);


unsigned long red_GetCRwCalls();
unsigned long red_GetCRwSucceeded();



#endif
