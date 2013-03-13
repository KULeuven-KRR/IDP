/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *              INTERFACE FOR THE TPTP PARSER             * */
/* *                                                        * */
/* *                                                        * */
/* ********************************************************** */
/**************************************************************/

#ifndef _TPTP_
/* *  This program is free software; you can redistribute   * */
/* *  it and/or modify it under the terms of the FreeBSD    * */
/* *  Licence.                                              * */
/* *                                                        * */
/* *  This program is distributed in the hope that it will  * */
/* *  be useful, but WITHOUT ANY WARRANTY; without even     * */
/* *  the implied warranty of MERCHANTABILITY or FITNESS    * */
/* *  FOR A PARTICULAR PURPOSE.  See the LICENCE file       * */
/* *  for more details.                                     * */
#define _TPTP_

/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include "list.h"
#include "flags.h"
#include "symbol.h"
#include "description.h"
#include "dfg.h"

/**************************************************************/
/* Functions                                                  */
/**************************************************************/

void   tptp_Init(void);
void   tptp_Free(void);

void   tptp_SetParserOutput(FILE*);     /*to be called prior to calling the parser*/
void   tptp_SetDoTranslateIdents(BOOL); /*to be called prior to calling the parser*/

LIST   tptp_TPTPParserIncludesExplicit(FILE*, FLAGSTORE, PRECEDENCE, DFGDESCRIPTION, LIST*, LIST*, LIST*, LIST*, LIST*, LIST*, LIST*, BOOL*);
LIST   tptp_TPTPParser(FILE*, FLAGSTORE, PRECEDENCE, DFGDESCRIPTION, LIST*, LIST*, LIST*, LIST*, LIST*, LIST*, BOOL*);

BOOL   tptp_WasParsingCNF(void); /*to be read after the parser finishes*/

FILE*  tptp_OpenFile(const char *, char ** const);

void   tptp_DeleteLabelTermPair(POINTER);

#endif
