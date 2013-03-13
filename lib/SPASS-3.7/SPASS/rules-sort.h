/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *           INFERENCE RULES FOR SORTS                    * */
/* *                                                        * */
/* *  $Module:   SORTRULES                                  * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1997, 1999, 2000, 2001            * */
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


/* $RCSfile: rules-sort.h,v $ */

#ifndef _SORTRULES_
#define _SORTRULES_

/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include "sort.h"
#include "unify.h"
#include "clause.h"
#include "flags.h"

/**************************************************************/
/* Functions                                                  */
/**************************************************************/        

LIST inf_ForwardSortResolution(CLAUSE, st_INDEX, SORTTHEORY, BOOL, FLAGSTORE,
			       PRECEDENCE);
LIST inf_BackwardSortResolution(CLAUSE, st_INDEX, SORTTHEORY, BOOL, FLAGSTORE,
				PRECEDENCE);
LIST inf_ForwardEmptySort(CLAUSE, st_INDEX, SORTTHEORY, BOOL, FLAGSTORE,
			  PRECEDENCE);
LIST inf_BackwardEmptySort(CLAUSE, st_INDEX, SORTTHEORY, BOOL, FLAGSTORE,
			   PRECEDENCE);
LIST inf_ForwardWeakening(CLAUSE, st_INDEX, SORTTHEORY, FLAGSTORE, PRECEDENCE);
LIST inf_BackwardWeakening(CLAUSE, st_INDEX, SORTTHEORY, FLAGSTORE, PRECEDENCE);
LIST inf_ForwardEmptySortPlusPlus(CLAUSE, st_INDEX, SORTTHEORY, FLAGSTORE,
				  PRECEDENCE);
LIST inf_BackwardEmptySortPlusPlus(CLAUSE, st_INDEX, SORTTHEORY, FLAGSTORE,
				   PRECEDENCE);

#endif
