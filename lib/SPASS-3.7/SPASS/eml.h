/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *              EXTENDED MODAL LOGICS ROUTINES            * */
/* *                                                        * */
/* *  $Module:   EML                                        * */ 
/* *                                                        * */
/* *                                                        * */
/* $Revision: 1.3 $                                         * */
/* $State: Exp $                                            * */
/* *  This program is free software; you can redistribute   * */
/* *  it and/or modify it under the terms of the FreeBSD    * */
/* *  Licence.                                              * */
/* *                                                        * */
/* *  This program is distributed in the hope that it will  * */
/* *  be useful, but WITHOUT ANY WARRANTY; without even     * */
/* *  the implied warranty of MERCHANTABILITY or FITNESS    * */
/* *  FOR A PARTICULAR PURPOSE.  See the LICENCE file       * */ 
/* *  for more details.                                     * */
/* $Date: 2010-02-22 14:09:58 $                             * */
/* $Author: weidenb $                                       * */
/* *                                                        * */
/* *             Contact:                                   * */
/* *             Renate Schmidt                             * */
/* *             School of Computer Science                 * */
/* *             University of Manchester                   * */
/* *             Oxford Rd, Manchester M13 9PL, UK          * */
/* *             Email: Renate.Schmidt@manchester.ac.uk     * */
/* *                                                        * */
/* ********************************************************** */
/**************************************************************/


/* $RCSfile: eml.h,v $ */

#ifndef _EMLDFG_
#define _EMLDFG_

/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include "term.h"
#include "list.h"
#include "hasharray.h"

/**************************************************************/
/* Global Variables and Constants (Only seen by macros)       */
/**************************************************************/

extern SYMBOL  eml_APPLYF;
extern SYMBOL  eml_BOX;
extern SYMBOL  eml_COMP;
extern SYMBOL  eml_CONV;
extern SYMBOL  eml_DIA;
extern SYMBOL  eml_DIV;
extern SYMBOL  eml_DOMAIN;
extern SYMBOL  eml_DOMRESTR;
extern SYMBOL  eml_ID;
extern SYMBOL  eml_RANGE;
extern SYMBOL  eml_RANRESTR;
extern SYMBOL  eml_SUM;
extern SYMBOL  eml_TEST;

typedef enum { eml_PROP,
               eml_REL,
	       eml_NDE,
               eml_NARYPROP,
	       eml_NARYNDE,
	       eml_SEMIFNDE, 
               eml_VARSORT
} eml_SYMBOLTYPE;

extern LIST   *eml_PROPINDEXTOFOSYMBOL;
extern SYMBOL eml_SEMIFNDESYMBOLS[symbol__MAXSIGNATURE];            
/* Records which nde symbols have been created, for the semi-functional
 * translation */

#define eml__MAXMODALDEPTH 100

extern SYMBOL eml_NARYSYMBOLS[symbol__MAXSIGNATURE][eml__MAXMODALDEPTH];    
/* Records which symbols of the n-ary signature have been created */

/**************************************************************/
/* Access to the first-order symbols.                         */
/**************************************************************/

static __inline__ SYMBOL eml_Box(void)
{
  return eml_BOX;
}

static __inline__ SYMBOL eml_Dia(void)
{
  return eml_DIA;
}

static __inline__ SYMBOL eml_Domain(void)
{
  return eml_DOMAIN;
}

static __inline__ SYMBOL eml_Range(void)
{
  return eml_RANGE;
}

static __inline__ SYMBOL eml_Comp(void)
{
  return eml_COMP;
}

static __inline__ SYMBOL eml_Sum(void)
{
  return eml_SUM;
}

static __inline__ SYMBOL eml_Conv(void)
{
  return eml_CONV;
}

static __inline__ SYMBOL eml_Test(void)
{
  return eml_TEST;
}

static __inline__ SYMBOL eml_Id(void)
{
  return eml_ID;
}

static __inline__ SYMBOL eml_Div(void)
{
  return eml_DIV;
}

static __inline__ SYMBOL eml_DomRestr(void)
{
  return eml_DOMRESTR;
}

static __inline__ SYMBOL eml_RanRestr(void)
{
  return eml_RANRESTR;
}

static __inline__ SYMBOL eml_ApplyFunct(void)
{
  return eml_APPLYF;
}

/**************************************************************/
/* Symbol identification and access                           */
/**************************************************************/

static __inline__ BOOL eml_IsId(TERM S)
{
  return symbol_Equal(eml_ID,term_TopSymbol(S));
}

static __inline__ BOOL eml_IsDiv(TERM S)
{
  return symbol_Equal(eml_DIV,term_TopSymbol(S));
}

static __inline__ BOOL eml_IsNotSymbol(SYMBOL S)
{
  return symbol_Equal(fol_Not(), S);
}

static __inline__ BOOL eml_IsImpliesSymbol(SYMBOL S)
{
  return symbol_Equal(fol_Implies(), S);
}

static __inline__ BOOL eml_IsImpliedSymbol(SYMBOL S)
{
  return symbol_Equal(fol_Implied(), S);
}

static __inline__ BOOL eml_IsEquivSymbol(SYMBOL S)
{
  return symbol_Equal(fol_Equiv(), S);
}

static __inline__ BOOL eml_IsDiaSymbol(SYMBOL S)
{
  return (symbol_Equal(eml_DIA,S));
}

static __inline__ BOOL eml_IsBoxSymbol(SYMBOL S)
{
  return (symbol_Equal(eml_BOX,S));
}

static __inline__ BOOL eml_IsModalOpSymbol(SYMBOL S)
{
  return (symbol_Equal(eml_BOX,S) || symbol_Equal(eml_DIA,S));
}

static __inline__ BOOL eml_IsRelPropSymbol(SYMBOL S)
{
  return eml_IsModalOpSymbol(S);
}

static __inline__ BOOL eml_IsModalLiteral(TERM T)
/* unused */
{
  return fol_IsLiteral(T) || eml_IsModalOpSymbol(term_TopSymbol(T)) ||
      (symbol_Equal(term_TopSymbol(T),fol_Not()) && 
       eml_IsModalOpSymbol(term_TopSymbol(term_FirstArgument(T))));
}


static __inline__ BOOL eml_IsPropJunctor(SYMBOL S)
{
  return symbol_Equal(S, fol_AND) || 
      symbol_Equal(S, fol_OR) || 
      symbol_Equal(S, fol_NOT) || 
      symbol_Equal(S, fol_IMPLIES) ||
      symbol_Equal(S, fol_IMPLIED) ||
      symbol_Equal(S, fol_EQUIV);
}

static __inline__ BOOL eml_IsDesignatedPropSymbol(SYMBOL S)
{
  return symbol_Equal(S, fol_TRUE) || 
      symbol_Equal(S, fol_FALSE) || 
      symbol_Equal(S, eml_ID) || 
      symbol_Equal(S, eml_DIV);
}

static __inline__ BOOL eml_IsNullaryPredicate(SYMBOL S)
{
  return symbol_IsPredicate(S) && !eml_IsDesignatedPropSymbol(S) &&
	  (symbol_Arity(S) == 0);
}

static __inline__ BOOL eml_IsNaryJunctor(SYMBOL S)
{
  return symbol_Equal(S, fol_AND) || 
      symbol_Equal(S, fol_OR) || 
      symbol_Equal(S, eml_COMP) || 
      symbol_Equal(S, eml_SUM);
}

static __inline__ BOOL eml_IsRelationalJunctor(SYMBOL S)
{
  return eml_IsPropJunctor(S) ||
      symbol_Equal(S, eml_COMP) || 
      symbol_Equal(S, eml_CONV) ||
      symbol_Equal(S, eml_SUM) ||
      symbol_Equal(S, eml_DOMRESTR) ||
      symbol_Equal(S, eml_RANRESTR) ||
      symbol_Equal(S, eml_TEST);
}

static __inline__ BOOL eml_IsModalJunctor(SYMBOL S)
{
  return eml_IsModalOpSymbol(S) || eml_IsPropJunctor(S);
}

static __inline__ BOOL eml_IsPredefinedPred(SYMBOL S)
{
  return symbol_Equal(S, eml_DIV) || symbol_Equal(S, eml_ID);
}

static __inline__ TERM eml_Atom(TERM Lit)
{
  if (term_TopSymbol(Lit) == fol_NOT)
    return term_FirstArgument(Lit);
  else
    return Lit;
}

static __inline__ SYMBOL eml_GetDualSymbol(SYMBOL symbol)
{
  SYMBOL dual;

  dual = symbol;

  if (symbol_Equal(symbol,fol_True()))
    dual = fol_False();
  else if (symbol_Equal(symbol,fol_False()))
    dual = fol_True();
  else if (symbol_Equal(symbol,fol_Or()))
    dual = fol_And();
  else if (symbol_Equal(symbol,fol_And()))
    dual =  fol_Or();
  else if (symbol_Equal(symbol,fol_Exist()))
    dual = fol_All();
  else if (symbol_Equal(symbol,fol_All()))
    dual = fol_Exist();
  else if (symbol_Equal(symbol,eml_Dia()))
    dual = eml_Box();
  else if (symbol_Equal(symbol,eml_Box()))
    dual = eml_Dia();
  else if (symbol_Equal(symbol,eml_Id()))
    dual = eml_Div();
  else if (symbol_Equal(symbol,eml_Div()))
    dual = eml_Id();
  else if (symbol_Equal(symbol,eml_Comp()))
    dual = eml_Sum();
  else if (symbol_Equal(symbol,eml_Sum()))
    dual = eml_Comp();

  return dual;
}

/**************************************************************/
/* Propositional/first order symbol associations              */
/**************************************************************/

static __inline__ BOOL eml_VarSortType(eml_SYMBOLTYPE SymbolType)
{
  return SymbolType == eml_VARSORT;
}

static __inline__ BOOL eml_NaryType(eml_SYMBOLTYPE SymbolType)
{
  return SymbolType == eml_NARYPROP || SymbolType == eml_NARYNDE;
}

static __inline__ BOOL eml_SemiFuncNdeType(eml_SYMBOLTYPE SymbolType)
{
  return SymbolType == eml_SEMIFNDE;
}

static __inline__ BOOL eml_RelType(eml_SYMBOLTYPE SymbolType)
{
  return SymbolType == eml_REL;
}

static __inline__ void eml_SetPropNarySymbolAssocList(SYMBOL PropSymbol, 
		int Arity, SYMBOL FoSymbol)
{
  eml_NARYSYMBOLS[symbol_Index(PropSymbol)][Arity-1] = FoSymbol;
}

static __inline__ void eml_SetPropSemiFNdeSymbolAssocList(SYMBOL PropSymbol, 
		int Arity, SYMBOL FoSymbol)
{
  eml_SEMIFNDESYMBOLS[symbol_Index(PropSymbol)] = FoSymbol;
}

static __inline__ void eml_SetPropFoSymbolAssocList(SYMBOL PropSymbol, LIST FoSymbols)
{
  eml_PROPINDEXTOFOSYMBOL[symbol_Index(PropSymbol)] =  FoSymbols;
}

static __inline__ SYMBOL eml_FoQuantAssocWithPropSymbol(SYMBOL PropSymbol)
{
  LIST symbolList;
  symbolList = eml_PROPINDEXTOFOSYMBOL[symbol_Index(PropSymbol)];
  if (list_Empty(symbolList))
    return symbol__NULL;
  else
    return (SYMBOL) list_First(symbolList);
}

static __inline__ SYMBOL eml_FoJunctorAssocWithPropSymbol(SYMBOL PropSymbol)
{
  LIST symbolList;
  symbolList = eml_PROPINDEXTOFOSYMBOL[symbol_Index(PropSymbol)];
  if (list_Empty(symbolList) || list_Empty(list_Cdr(symbolList)))
    return symbol__NULL;
  else
  return (SYMBOL) list_Second(symbolList);
}

/**************************************************************/
/* Functions                                                  */
/**************************************************************/

void eml_Init(PRECEDENCE);
void eml_Free(void);
void eml_TranslateToFolMain(LIST*, LIST*, BOOL, FLAGSTORE, PRECEDENCE);
BOOL eml_FormulaContainsEMLTheorySymbol(TERM);

#endif
