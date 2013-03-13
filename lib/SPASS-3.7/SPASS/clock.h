/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                     CLOCK                              * */
/* *                                                        * */
/* *  $Module:   CLOCK                                      * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1997, 1999, 2001                  * */
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
/* $Revision: 1.5 $                                         * */
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

/* $RCSfile: clock.h,v $ */

#ifndef _CLOCK_
#define _CLOCK_

#include "gettimeofday.h"
#include "misc.h"
#include <sys/types.h>
#include <sys/time.h>

typedef enum {
  clock_BACKTRACK,
  clock_OVERALL,
  clock_INPUT,
  clock_CNF,
  clock_CNFREDUCTION,
  clock_REDUCTION,
  clock_INFERENCE,
  clock_TRANSL,
  clock_TYPESIZE
} CLOCK_CLOCKS;


typedef struct timeval CLOCK_TMS;

void   clock_Init(void);
void   clock_InitCounter(CLOCK_CLOCKS);
void   clock_StartCounter(CLOCK_CLOCKS);
void   clock_StopPassedTime(CLOCK_CLOCKS);
void   clock_StopAddPassedTime(CLOCK_CLOCKS);
float  clock_GetSeconds(CLOCK_CLOCKS);
float  clock_GetAkku(CLOCK_CLOCKS);
void   clock_PrintTime(CLOCK_CLOCKS);

#endif
