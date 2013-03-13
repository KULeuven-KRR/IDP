/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                 SCANNER FOR DFG SYNTAX                 * */
/* *                                                        * */
/* *  $Module:   DFG                                        * */ 
/* *                                                        * */
/* *  Copyright (C) 1997, 1998, 1999, 2000, 2001            * */
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
/* $Revision: 1.6 $                                         * */
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


/* $RCSfile: dfgscanner.l,v $ */

%{

#include <ctype.h>    /* for isprint */
#include <errno.h>
#include "misc.h"
#include "memory.h"
#include "symbol.h"
#include "term.h"
#include "dfg.h"
#include "dfgparser.h"

/* defined in dfgparser.y */
extern NAT  dfg_LINENUMBER;
extern BOOL dfg_IGNORETEXT;

static NAT dfg_CountNewlines(char*);

static __inline__ char* dfg_StringCopy(void)
{
  char *copy;
  copy = (char*) memory_Malloc(yyleng+1);
  strcpy(copy, yytext);
  return copy;
}

%}

%option noyywrap
/* Force the scanner to read the input character by character */
%option always-interactive
/* Omit unused function yyunput */
%option nounput

/* For reading filenames for includes */
single_quoted         [']{sq_char}{sq_char}*[']
sq_char               ([\40-\46\50-\133\135-\176]|[\\]['\\])

/* Start conditions */
%x TXT

%%

"||"              return DFG_END_OF_CONSTRAINT_LITERALS;
"->"           return DFG_END_OF_ANTECEDENT_LITERALS;
"+"             return DFG_LITERAL_IS_SELECTED;
  /*BEGIN RESERVED*/
and				return DFG_AND;
author				return DFG_AUTHOR;
axioms				return DFG_AXIOMS;
begin_problem			return DFG_BEGPROB;
by				return DFG_BY;
box|all				return DFG_BOX;
clause				return DFG_CLAUSE;
cnf				return DFG_CNF;
comp				return DFG_COMP;
conjectures			return DFG_CONJECS;
conv				return DFG_CONV;
date				return DFG_DATE;
description			return DFG_DESC;
dia|some			return DFG_DIA;
div  				return DFG_DIVERSITY;
dnf				return DFG_DNF;
domain				return DFG_DOMAIN;
domrestr			return DFG_DOMRESTR;
eml|EML|DL				return DFG_EML;
end_of_list			return DFG_ENDLIST;
end_problem			return DFG_ENDPROB;
equal				return DFG_EQUAL;
equiv				return DFG_EQUIV;
exists				return DFG_EXISTS;
false				return DFG_FALSE;
forall				return DFG_FORALL;
formula				return DFG_FORMULA;
freely				return DFG_FREELY;
functions			return DFG_FUNC;
generated			return DFG_GENERATED;
hypothesis			return DFG_HYPOTH;
id  				return DFG_IDENTITY;
implied				return DFG_IMPLIED;
implies				return DFG_IMPLIES;
list_of_clauses			return DFG_CLSLIST;
list_of_declarations		return DFG_DECLLIST;
list_of_descriptions		return DFG_DESCLIST;
list_of_formulae		return DFG_FORMLIST;
list_of_general_settings	return DFG_GENSET;
list_of_proof			return DFG_PRFLIST;
list_of_settings		return DFG_SETTINGS;
list_of_special_formulae	return DFG_SPECIALFORMLIST;
list_of_symbols			return DFG_SYMLIST;
list_of_terms                   return DFG_TERMLIST;
list_of_includes		return DFG_INCLUDELIST;
include			        return DFG_INCLUDE;
logic				return DFG_LOGIC;
name				return DFG_NAME;
not				return DFG_NOT;
operators			return DFG_OPERAT;
or				return DFG_OR;
prop_formula|concept_formula	return DFG_PROP_FORMULA;
predicate			return DFG_PRED;
predicates			return DFG_PRDICAT;
quantifiers			return DFG_QUANTIF;
ranrestr			return DFG_RANRESTR;
range				return DFG_RANGE;
rel_formula|role_formula	return DFG_REL_FORMULA;
satisfiable			return DFG_SATIS;
set_DomPred			return DFG_DOMPRED;
set_flag			return DFG_SETFLAG;
set_precedence			return DFG_PREC;
set_ClauseFormulaRelation       return DFG_CLFORE;
set_selection			return DFG_SELECT;                   
sort				return DFG_SORT;
sorts				return DFG_SORTS;
status				return DFG_STATUS;
step                            return DFG_STEP;
subsort				return DFG_SUBSORT;
sum				return DFG_SUM;
test 				return DFG_TEST;
translpairs			return DFG_TRANSLPAIRS;
true				return DFG_TRUE;
unknown				return DFG_UNKNOWN;
unsatisfiable			return DFG_UNSATIS;
version				return DFG_VERSION;
xor				return DFG_XOR;
nor				return DFG_NOR;
nand				return DFG_NAND;
  /*END RESERVED*/
"-1"				return DFG_MINUS1;

"%"[^\n]*			/* one-line comment */
"{*"				{ /* Start of multiline comment */
				  if (dfg_IGNORETEXT) {
				    BEGIN(TXT);
				    yymore();
				  } else
				    return DFG_OPENBRACE;
				}
"*}"				return DFG_CLOSEBRACE;
<TXT>[^*]+			yymore();
<TXT>"*"+[^*}]*			yymore();
<TXT>"*"+"}"			{ BEGIN(INITIAL);
				  dfg_lval.string = dfg_StringCopy();
				  dfg_LINENUMBER += dfg_CountNewlines(yytext);
				  return DFG_TEXT;
				}

{single_quoted} {
    dfg_lval.string = dfg_StringCopy();
    return single_quoted;
}

[0-9]+				{ unsigned long n;
                                  errno = 0;
                                  n = strtoul(yytext, NULL, 10);
				  if (errno != 0 || n > INT_MAX) {
                                    misc_StartUserErrorReport();
                                    misc_UserErrorReport("\n Number too big in line %d.\n",
							 dfg_LINENUMBER);
                                    misc_FinishUserErrorReport();
                                  }
                                  dfg_lval.number = (int) n;
                                  return DFG_NUM;
                                }
[a-zA-Z0-9_]+			{ dfg_lval.string = dfg_StringCopy();
                                  return DFG_ID;
                                }
[\xd \t]+			/* ignore */
"\n"				dfg_LINENUMBER++;
[-()\[\],\.:]			return yytext[0];
.				{ misc_StartUserErrorReport();
                                  misc_UserErrorReport("\n Illegal character '");
				  if (isprint((int)yytext[0]))
				    misc_UserErrorReport("%c",yytext[0]);
				  else
				    misc_UserErrorReport("\\x%x", (unsigned int) yytext[0]);
				  misc_UserErrorReport("' in line %d.\n", dfg_LINENUMBER);
                                  misc_FinishUserErrorReport();
                                }

%%

static NAT dfg_CountNewlines(char* Text)
{
  NAT result = 0;
  
  while (*Text != 0) {
    if (*Text++ == '\n')
      result++;
  }
  return result;
}
