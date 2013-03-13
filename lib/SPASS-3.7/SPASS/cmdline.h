/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                COMMAND LINE PARSER                     * */
/* *                                                        * */
/* *  $Module:   CMDLNE                                     * */ 
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


/* $RCSfile: cmdline.h,v $ */

#ifndef _CMDLNE_
#define _CMDLNE_


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "misc.h"
#include "flags.h"
#include "flags.h"
#include "list.h"
#include "strings.h"

/**************************************************************/
/* Data Types and Constants                                   */
/**************************************************************/

#define cmdlne_MAXTOKENLENGTH 100
#define cmdlne_DEFAULTARGVALUE "1"


typedef char* TOKEN;

/**************************************************************/
/* Functions                                                  */
/**************************************************************/


BOOL cmdlne_Read(int, const char **);
BOOL cmdlne_SetFlags(FLAGSTORE); 
const char* cmdlne_GetInputFile();
const char* cmdlne_GetOutputFile();
void cmdlne_Init();
void cmdlne_Free();
void cmdlne_PrintSPASSNames();

#endif
