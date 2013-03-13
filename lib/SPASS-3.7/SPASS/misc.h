/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                MISCELLANEOUS                           * */
/* *                                                        * */
/* *  $Module:   MISC                                       * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001      * */
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
/* $Revision: 1.14 $                                        * */
/* $State: Exp $                                            * */
/* $Date: 2010-02-22 14:09:58 $                             * */
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


/* $RCSfile: misc.h,v $ */

#ifndef _MISC_
#define _MISC_

/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#define __USE_FIXED_PROTOTYPES__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <stdarg.h>

#include "version.h"

/**************************************************************/
/* More basic types and macros                                */
/**************************************************************/

#if defined(TRUE)
#undef TRUE
#endif

#if defined(FALSE)
#undef FALSE
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif


#define misc_ERROROUT     stderr
#define misc_USERERROROUT stderr

typedef enum { FALSE=0, TRUE=1 } BOOL;

typedef void*           POINTER;
typedef unsigned int    NAT;

/* Limits for EARL data types */
#define NAT_MAX UINT_MAX


/**************************************************************/
/* Inline Functions                                           */
/**************************************************************/

static __inline__ void misc_Error(void)
{
  fflush(misc_USERERROROUT);
  fflush(stdout);
  fflush(stderr);
#ifdef CHECK
  abort();
#else
  exit(EXIT_FAILURE);
#endif
}

static __inline__ BOOL misc_SmallerThan(int i, int j)
{
  return (BOOL)(i < j);
}

#define misc_StartErrorReport()        misc_StartErrorReporting(__FILE__,__LINE__)
#define misc_StartUserErrorReport()    fflush(stdout)
#define misc_FinishUserErrorReport()   misc_Error()

/**************************************************************/
/* Functions                                                  */
/**************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void  misc_StartErrorReporting(const char*, int);
void  misc_FinishErrorReport(void);
void  misc_ErrorReport(const char * , ...);
void  misc_UserWarning(const char * , ...);
void  misc_UserErrorReport(const char * , ...);

void  misc_PrintChar(NAT Number, char Character);
void  misc_DumpCore(void);
void  misc_DumpCoreOut(const char*);
int   misc_ReturnValue(void);
int   misc_Max(int, int);

FILE* misc_OpenFile(const char*, const char*);
FILE* misc_OpenFileExt(const char*, const char*, char*,char ** const);
FILE* misc_OpenFileEnv(const char*, const char*, const char*,char ** const);
void  misc_CloseFile(FILE*, const char*);

#ifdef __cplusplus
}
#endif

#endif
