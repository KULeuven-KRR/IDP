/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *               CONDENSATION OF CLAUSES                  * */
/* *                                                        * */
/* *  $Module:   CONDENSING                                 * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1997, 2001 MPI fuer Informatik    * */
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


/* $RCSfile: condensing.c,v $ */

#include "subsumption.h"
#include "misc.h"
#include "condensing.h"


LIST cond_CondFast(CLAUSE c)
/**********************************************************
  INPUT:   A clause c.
  RETURNS: A list with indexes with respect to c that can
           be deleted due to condensing.
  CAUTION: None.
***********************************************************/
{
  int    vec, i, j, k;
  LIST   indexlist;

  indexlist = list_Nil();
  vec       = vec_ActMax();
  
  for (i = 0; i < clause_Length(c); i++) {
    vec_Push((POINTER) i);
  }
  
  for (k = clause_Length(c) - 1; k >= 0; k--) {
    for (i = vec; i < vec_ActMax(); i++) {
      if ((int)vec_GetNth(i) != k) {
	cont_StartBinding();
	if (unify_Match(cont_LeftContext(),
			clause_GetLiteralTerm(c,k),
			clause_GetLiteralTerm(c,(int)vec_GetNth(i)))) {
	  cont_BackTrack();
	  for (j = vec; j < vec_ActMax(); j++) {
	    if (k == (int)vec_GetNth(j)) {
	      vec_Swap((vec_ActMax() -1) ,j);
	      j = vec_ActMax();
	    }
	  }
	  
	  if (subs_IdcRes(c,vec,(vec_ActMax() -1))) {
	    indexlist = list_Cons((POINTER)k,indexlist);
	    vec_Pop();
	  }
	  
	  i = vec_ActMax()+1;
	}
	else
	  cont_BackTrack();
      }
    }
  }
 
  vec_SetMax(vec);
  return indexlist;
}
