/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                STRING HANDLING                         * */
/* *                                                        * */
/* *  $Module:   STRINGS                                    * */ 
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


/* $RCSfile: strings.h,v $ */

#ifndef _STRINGS_
#define _STRINGS_

/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include <math.h>
#include "memory.h"
#include "list.h"

/**************************************************************/
/* Functions                                                  */
/**************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

BOOL   string_Equal(const char*, const char*);
BOOL   string_StringIsNumber(const char*);
BOOL   string_StringIsInteger(const char*);
char*  string_StringCopy(const char*);
void   string_StringFree(char*);
char*  string_IntToString(int);
BOOL   string_StringToInt(const char*, BOOL, int*);
char*  string_Conc(const char*, const char*);
char*  string_Nconc(char*, char*);
char*  string_EmptyString(void);
char*  string_Prefix(const char*, int);
char*  string_Suffix(const char*, int);
LIST   string_Split(char*, const char*);
char** string_Tokens(char*, int*);

#ifdef __cplusplus
}
#endif

#endif
