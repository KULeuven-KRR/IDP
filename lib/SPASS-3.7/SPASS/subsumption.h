/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                     SUBSUMPTION                        * */
/* *                                                        * */
/* *  $Module:   SUBSUMPTION                                * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1997, 1999, 2000                  * */
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


/* $RCSfile: subsumption.h,v $ */


#ifndef _SUBSUMPTION_
#define _SUBSUMPTION_

/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include "misc.h"
#include "unify.h"
#include "component.h"
#include "vector.h"
#include "clause.h"

/**************************************************************/
/* Functions                                                 */
/**************************************************************/

static __inline__ int subs_NoException(void)
{
  return -1;
}

void subs_Init(void);
void subs_Free(void);
BOOL subs_Subsumes(CLAUSE, CLAUSE, int, int);
BOOL subs_SubsumesBasic(CLAUSE, CLAUSE, int, int);
BOOL subs_SubsumesWithSignature(CLAUSE, CLAUSE, BOOL, LIST*);
BOOL subs_Idc(CLAUSE, CLAUSE);
BOOL subs_IdcRes(CLAUSE, int, int);
BOOL subs_IdcEq(CLAUSE, CLAUSE);
BOOL subs_IdcEqMatch(CLAUSE, CLAUSE, SUBST);
BOOL subs_IdcEqMatchExcept(CLAUSE, int, CLAUSE, int, SUBST);

#endif

