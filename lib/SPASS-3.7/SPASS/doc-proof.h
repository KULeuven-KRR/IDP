/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                 PROOF DOCUMENTATION                    * */
/* *                                                        * */
/* *  $Module:   DOCPROOF                                   * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1997, 1998, 2000, 2001            * */
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
/* $Date: 2010-02-22 14:09:58 $                             * */
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


/* $RCSfile: doc-proof.h,v $ */

#ifndef _DOC_PROOF_
#define _DOC_PROOF_

/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include "clause.h"
#include "st.h"
#include "sharing.h"
#include "search.h"
#include "doc-proof.h"
#include "proofcheck.h"

/**************************************************************/
/* Data Structures and Constants                              */
/**************************************************************/

extern int dp_DEPTH;


/**************************************************************/
/* Macros                                                     */
/**************************************************************/ 

static __inline__ int dp_ProofDepth(void)
{
  return dp_DEPTH;
}

static __inline__ void dp_SetProofDepth(int Depth)
{
  dp_DEPTH = Depth;
}


/**************************************************************/
/* Functions                                                  */
/**************************************************************/        

void dp_Init(void);
LIST dp_PrintProof(PROOFSEARCH, LIST, const char*);

#endif
