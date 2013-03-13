/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                STANDARD UNIFICATION                    * */
/* *                                                        * */
/* *  $Module:   UNIFY                                      * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1997, 1998, 2001                  * */
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


/* $RCSfile: unify.h,v $ */

#ifndef _UNIFY_
#define _UNIFY_

/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include "term.h"
#include "symbol.h"
#include "list.h"
#include "context.h"
#include "subst.h"

/**************************************************************/
/* Functions for Initialization and Controlling               */
/**************************************************************/
           
void unify_Init(void);
void unify_Free(void);

/**************************************************************/
/* Functions for Misc                                         */
/**************************************************************/

/**************************************************************/
/* Functions for Occur Check                                  */
/**************************************************************/

BOOL unify_OccurCheckCom(SYMBOL, CONTEXT, TERM);
BOOL unify_OccurCheck(CONTEXT, SYMBOL, CONTEXT, TERM);

/**************************************************************/
/* Functions for Unification                                  */
/**************************************************************/

BOOL unify_Unify(CONTEXT, TERM, CONTEXT, TERM);
BOOL unify_UnifyCom(CONTEXT, TERM, TERM);
BOOL unify_UnifyNoOC(CONTEXT, TERM, CONTEXT, TERM);
BOOL unify_UnifyAllOC(CONTEXT, CONTEXT, TERM, CONTEXT, TERM);

/**************************************************************/
/* Functions for Generalization Test                          */
/**************************************************************/

BOOL unify_Match(CONTEXT, TERM, TERM);
BOOL unify_MatchFlexible(CONTEXT, TERM, TERM);
void unify_EstablishMatcher(CONTEXT, SUBST);
BOOL unify_MatchBindings(const CONTEXT, TERM, TERM);

/**************************************************************/
/* Functions for Instance Test                                */
/**************************************************************/

BOOL unify_MatchReverse(const CONTEXT, TERM, CONTEXT, TERM);

/**************************************************************/
/* Functions for Variation Test                               */
/**************************************************************/

BOOL unify_Variation(const CONTEXT, TERM, TERM);

/**************************************************************/
/* Functions for Computing MSCGs                              */
/**************************************************************/

TERM unify_ComGenLinear(const CONTEXT, SUBST*, TERM, SUBST*, TERM);

/**************************************************************/
/* Functions for Debugging                                    */
/**************************************************************/

#endif
