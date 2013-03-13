/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                UR-RESOLUTION                           * */
/* *                                                        * */
/* *  $Module:   INFERENCE RULES                            * */
/* *                                                        * */
/* *  Copyright (C) 1999, 2000, 2001 MPI fuer Informatik    * */
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

/* $RCSfile: rules-ur.h,v $ */

#ifndef _UR_RESOLUTION_
#define _UR_RESOLUTION_

#include "clause.h"
#include "sharing.h"
#include "flags.h"

LIST inf_URResolution(CLAUSE, SHARED_INDEX, FLAGSTORE, PRECEDENCE);

#endif
