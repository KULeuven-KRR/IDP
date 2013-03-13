/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *         RECURSIVE PATH ORDERING WITH STATUS            * */
/* *                                                        * */
/* *  $Module:   RPOS                                       * */
/* *                                                        * */
/* *  Copyright (C) 1997, 1998 MPI fuer Informatik          * */
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


/* $RCSfile: rpos.h,v $ */

#ifndef _RPOS_
#define _RPOS_

#include "misc.h"
#include "term.h"
#include "order.h"
#include "context.h"

/**************************************************************/
/*  Function Prototypes                                       */
/**************************************************************/

BOOL       rpos_Equal(TERM, TERM);
ord_RESULT rpos_GreaterEqual(TERM, TERM, BOOL);
ord_RESULT rpos_Compare(TERM, TERM);
ord_RESULT rpos_CompareAux(TERM, TERM, BOOL);
ord_RESULT rpos_CompareSkolem(TERM, TERM);

ord_RESULT rpos_ContCompare(CONTEXT, TERM, CONTEXT, TERM);

BOOL rpos_Greater(TERM, TERM, BOOL);
BOOL rpos_ContGreater(CONTEXT, CONTEXT, TERM, CONTEXT, CONTEXT, TERM);
BOOL rpos_ContGreaterSkolemSubst(CONTEXT, CONTEXT, TERM, CONTEXT, CONTEXT, TERM);

#endif
