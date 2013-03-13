/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *              EXTENDED MODAL LOGICS ROUTINES            * */
/* *                                                        * */
/* *  $Module:   EML                                        * */ 
/* *                                                        * */
/* *                                                        * */
/* $Revision: 1.7 $                                         * */
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


/* $RCSfile: eml.c,v $ */

#include "cnf.h"
#include "eml.h"
#include "foldfg.h"
#include "flags.h"

SYMBOL  eml_APPLYF;
SYMBOL  eml_BOX;
SYMBOL  eml_COMP;
SYMBOL  eml_CONV;
SYMBOL  eml_DIA;
SYMBOL  eml_DIV;
SYMBOL  eml_DOMAIN;
SYMBOL  eml_DOMRESTR;
SYMBOL  eml_ID;
SYMBOL  eml_RANGE;
SYMBOL  eml_RANRESTR;
SYMBOL  eml_SUM;
SYMBOL  eml_TEST;

LIST    eml_SYMBOLS;                    /* eml junctor symbols */

LIST   *eml_PROPINDEXTOFOSYMBOL;       /* list of FO symbols associated with 
                                           eml prop/junctor symbols */
SYMBOL  eml_SEMIFNDESYMBOLS[symbol__MAXSIGNATURE];            
                                        /* nde symbols associated with diamonds */
SYMBOL  eml_NARYSYMBOLS[symbol__MAXSIGNATURE][eml__MAXMODALDEPTH];    
                                        /* FO symbols with particular
                                         * arity associated with p's */

static const char* eml_PREDNAMEPREFIX[6] = { "SkQ", "SkR", "SkNde" , "SkQ", "SkNde", "SkS" };

static const char* eml_RELPROPERTYLABELS[6] = { "", "serial",
       "reflexive" , "symmetric", "transitive", "euclidean" };

static TERM  eml_RemoveTrivialAtomsAndOps(TERM);
static TERM  eml_RemoveTrivialOperator(TERM Term);

static TERM  eml_EmbedInRelCalculusProp(TERM);
static TERM  eml_EmbedInRelCalculusRel(TERM);

static TERM  eml_RelationalTranslationProp(TERM, SYMBOL, FLAGSTORE, PRECEDENCE);
static TERM  eml_RelationalTranslationRel(TERM, SYMBOL, SYMBOL, FLAGSTORE, PRECEDENCE);

static TERM  eml_FunctTranslProp(TERM, TERM, FLAGSTORE, PRECEDENCE);
static TERM  eml_FunctTranslRel(TERM, TERM, FLAGSTORE, PRECEDENCE);
static TERM  eml_FunctTranslItoNaryPredProp(TERM, LIST, char*, int, FLAGSTORE, PRECEDENCE);
static TERM  eml_FunctTranslItoNaryPredRel(TERM, LIST, char*, int, FLAGSTORE, PRECEDENCE);

static TERM  eml_SemiFunctTranslProp(TERM, TERM, int, FLAGSTORE, PRECEDENCE);
static TERM  eml_SemiFunctTranslRelBox(TERM, TERM, TERM, FLAGSTORE, PRECEDENCE);
static TERM  eml_SemiFunctTranslRelDia(TERM, TERM, FLAGSTORE, PRECEDENCE);

static TERM eml_RelFunctTranslProp(TERM, SYMBOL, char*, FLAGSTORE, PRECEDENCE);
static TERM eml_RelFunctTranslRel(TERM, SYMBOL, SYMBOL, char*, FLAGSTORE, PRECEDENCE);

/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *  FUNCTIONS                                             * */
/* *                                                        * */
/* ********************************************************** */
/**************************************************************/

void eml_Init(PRECEDENCE Precedence)
/**************************************************************
  INPUT:   None.
  RETURNS: None.
  USAGE:   extern public.
  SUMMARY: Initializes the Eml Module.
  EFFECTS: All additional extended modal logic symbols are created.  
           (The first-order symbols are created in fol_Init.)
	   And, creates an array for the list of
	   propositional/first-order symbol pairs as well as Eml logical
	   symbols/first-order logical symbol lists.
  CAUTION: MUST BE CALLED BEFORE ANY OTHER eml-FUNCTION.
***************************************************************/
{ 
  int i;

  eml_BOX	= symbol_CreateJunctor("box", 2, symbol_STATLEX, Precedence);
  eml_COMP	= symbol_CreateJunctor("comp", 2, symbol_STATLEX, Precedence);
  eml_CONV	= symbol_CreateJunctor("conv", 1, symbol_STATLEX, Precedence);
  eml_DIA	= symbol_CreateJunctor("dia", 2, symbol_STATLEX, Precedence);
  eml_DIV	= symbol_CreatePredicate("div", 0, symbol_STATLEX, Precedence);
  eml_DOMAIN	= symbol_CreateJunctor("domain", 1, symbol_STATLEX, Precedence);
  eml_DOMRESTR	= symbol_CreateJunctor("domrestr", 2, symbol_STATLEX, Precedence);
  eml_ID	= symbol_CreatePredicate("id", 0, symbol_STATLEX, Precedence);
  eml_RANGE	= symbol_CreateJunctor("range", 1, symbol_STATLEX, Precedence);
  eml_RANRESTR	= symbol_CreateJunctor("ranrestr", 2, symbol_STATLEX, Precedence);
  eml_SUM	= symbol_CreateJunctor("sum", 2, symbol_STATLEX, Precedence);
  eml_TEST	= symbol_CreateJunctor("test", 1, symbol_STATLEX, Precedence);
  symbol_AddProperty(eml_DIV, PREDEFINED);
  symbol_AddProperty(eml_ID, PREDEFINED);

  eml_SYMBOLS =
  	list_Cons((POINTER)eml_BOX, list_Cons((POINTER)eml_COMP,
  	  list_Cons((POINTER)eml_CONV, list_Cons((POINTER)eml_DIA,
  	    list_Cons((POINTER)eml_DIV, list_Cons((POINTER)eml_DOMAIN, 
              list_Cons((POINTER)eml_DOMRESTR, list_Cons((POINTER)eml_ID, 
		list_Cons((POINTER)eml_RANGE, list_Cons((POINTER)eml_RANRESTR,
  	          list_Cons((POINTER)eml_SUM, list_List((POINTER)eml_TEST))))))))))));

  eml_PROPINDEXTOFOSYMBOL = (LIST*)memory_Malloc(sizeof(LIST) * symbol__MAXSIGNATURE);
  for (i=0;i< symbol__MAXSIGNATURE;i++)
    eml_PROPINDEXTOFOSYMBOL[i] = list_Nil();

  eml_SetPropFoSymbolAssocList(eml_Dia(),
		list_Cons((POINTER)fol_Exist(), list_List((POINTER)fol_And())));
  eml_SetPropFoSymbolAssocList(eml_Box(), 
		list_Cons((POINTER)fol_All(), list_List((POINTER)fol_Implies())));
  eml_SetPropFoSymbolAssocList(eml_Comp(), 
		list_Cons((POINTER)fol_Exist(), list_List((POINTER)fol_And())));
  eml_SetPropFoSymbolAssocList(eml_Sum(), 
		list_Cons((POINTER)fol_All(), list_List((POINTER)fol_Or())));
  eml_SetPropFoSymbolAssocList(eml_Domain(), 
		list_List((POINTER)fol_Exist()));
  eml_SetPropFoSymbolAssocList(eml_Range(), 
		list_List((POINTER)fol_Exist()));
  eml_SetPropFoSymbolAssocList(eml_Id(), 
		list_List((POINTER)fol_Equality()));
}

BOOL eml_FormulaContainsEMLTheorySymbol(TERM Formula)
/**************************************************************
  INPUT:   A term or a formula
  RETURNS: TRUE iff <Formula> contains an EML theory symbol.
  USAGE:   extern public.
***************************************************************/
{ 
  SYMBOL Top;
  LIST   Scan;
  
  Top = term_TopSymbol(Formula);

  if (symbol_IsPredicate(Top)) {
    if (symbol_Equal(eml_DIV,Top) || symbol_Equal(eml_ID,Top))
      return TRUE;
    else
      return FALSE;
  }
  if (symbol_IsFunction(Top) || symbol_IsVariable(Top))
    return FALSE;

  if (symbol_Equal(Top, eml_BOX) || symbol_Equal(Top, eml_COMP) || symbol_Equal(Top, eml_CONV) || 
      symbol_Equal(Top, eml_SUM) || symbol_Equal(Top, eml_DIV) || symbol_Equal(Top, eml_DOMAIN) ||
      symbol_Equal(Top, eml_DOMRESTR) || symbol_Equal(Top, eml_RANGE) || symbol_Equal(Top, eml_RANRESTR) ||
      symbol_Equal(Top, eml_TEST))
    return TRUE;
  
  if (fol_IsQuantifier(Top))
    Scan = list_Cdr(term_ArgumentList(Formula));
  else
    Scan = term_ArgumentList(Formula);

  while (!list_Empty(Scan)) {
    if (eml_FormulaContainsEMLTheorySymbol((TERM)list_First(Scan)))
      return TRUE;
    Scan = list_Cdr(Scan);
  }

  return FALSE;
  
}

void eml_InitFunctTransl(FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   None.
  RETURNS: None.
  USAGE:   static intern.
  SUMMARY: Initializes the routines for the functional translation of
           basic (serial) modal logics ito n-ary predicates.
  EFFECTS: Additonal flags are set.
  CAUTION: Prerequisite for the functional translation ito n-ary predicates
           routines.
***************************************************************/
{ 
	/* printf("\nDoing eml_InitFunctTransl()\n"); */

  /* Dynamic memory allocation of eml_NARYSYMBOLS??
   * This does not work:
  eml_NARYSYMBOLS = (SYMBOL **)memory_Malloc(sizeof(SYMBOL) * symbol__MAXSIGNATURE *
		  symbol__MAXSIGNATURE);
		  */
  if ( (flag_GetFlagIntValue(Flags, flag_EMLTR) == flag_EMLTRSEMIFUNC) || 
       !flag_GetFlagIntValue(Flags, flag_EMLFNARY)) {
    eml_APPLYF	= symbol_CreateFunction("app", 2, symbol_STATLEX, Precedence);
    /*     fol_SYMBOLS = list_Cons((POINTER)eml_APPLYF, fol_SYMBOLS);*/
  }

}


void eml_Free(void)
/**************************************************************
  INPUT:   None.
  RETURNS: void.
  EFFECT:  The eml related memory used by the eml module is freed.
***************************************************************/
{
  int i;

  list_Delete(eml_SYMBOLS);

  for (i=0;i< symbol__MAXSIGNATURE;i++)
    list_Delete(eml_PROPINDEXTOFOSYMBOL[i]);

  memory_Free(eml_PROPINDEXTOFOSYMBOL, sizeof(LIST) * symbol__MAXSIGNATURE);

  /* list_Delete(eml_NARYSYMBOLS); ?? */
}

LIST eml_NullaryPredList()
/**************************************************************
  INPUT:   None.
  RETURNS: The list of nullary predicate symbols in the signature
  USAGE:   not used.
  EFFECTS: Creates a table for the list of
           propositional/first-order symbol pairs.
***************************************************************/
{
  LIST   nullaryPredList;
  int 	 i;

  if (!symbol_SignatureExists()) {
    fprintf(stderr,"\n\t eml_NullaryPredList(): no signature!?\n");
    misc_Error();
  }

  nullaryPredList = list_Nil();
  for(i = 1; i < symbol_ACTINDEX; i++)
    if (!symbol_IsFreed(i) &&
	eml_IsNullaryPredicate(symbol_GetSigSymbol(i)))
      nullaryPredList =
	      list_Cons((POINTER)(symbol_GetSigSymbol(i)),
			      nullaryPredList);

#ifdef DEBUG
  printf("\n nullaryPredList: ");
    for (Scan=nullaryPredList; !list_Empty(Scan); Scan=list_Cdr(Scan)) {
      symbol_Print((SYMBOL)list_Car(Scan));
      printf(" ");
    }
  printf("\n");
#endif
  
  return nullaryPredList;
}

static SYMBOL eml_GetFoSymbolAssocWithPropSymbol(SYMBOL
		PropSymbol, NAT Arity, eml_SYMBOLTYPE SymbolType)
/**************************************************************
  INPUT:   a (propositional) symbol, the arity of the corresponding
           predicate symbol, the symbol type.
  RETURNS: The corresponding first-order symbol.
  EFFECTS: None.
***************************************************************/
{
  /* printf("\n  PropSymbol = "); */
  /* symbol_Print(PropSymbol); */
  if (eml_NaryType(SymbolType)) {
    /* Retrieve symbol with given arity */
    SYMBOL symbol;
    /* printf("\n  eml_NaryType"); */
    symbol = eml_NARYSYMBOLS[symbol_Index(PropSymbol)][Arity-1];
    if (symbol_IsSymbol(symbol))
      return symbol;
  }
  else if (eml_SemiFuncNdeType(SymbolType)) {
    /* Retrieve nde symbol for semi-functional translation */
    SYMBOL symbol;
    /* printf("\n  eml_SemiFuncNdeType"); */
    symbol = eml_SEMIFNDESYMBOLS[symbol_Index(PropSymbol)];
    if (symbol_IsSymbol(symbol))
      return symbol;
  }
  else if (eml_VarSortType(SymbolType)) {
    /* Reuse propositional symbol as predicate symbol to indicate the
     * sort */
    /* printf("\n  eml_VarSortType"); */
    return PropSymbol;  /* Struture sharing!
			 * Alternatively, set up another array called
			 * eml_PROPINDEXTOSORTSYMBOL, say */
  } 
  else {
    /* Retrieve predicate symbol: P_p for p, and R_r for r */
    LIST symbolList;
    /* printf("\n  not eml_NaryType and not eml_VarSortType"); */
    symbolList = eml_PROPINDEXTOFOSYMBOL[symbol_Index(PropSymbol)];
    if (!list_Empty(symbolList))
      return (SYMBOL) list_First(symbolList);
  }

    /* printf("\n  not eml_NaryType and not eml_VarSortType and symbolList is empty"); */
  return symbol__NULL;
}


SYMBOL eml_CreateFoSymbol(SYMBOL PropSymbol, eml_SYMBOLTYPE SymbolType,
		int Arity, int Status, PRECEDENCE Precedence)
/*******************************************************
  INPUT:   A symbol (which MUST be a propositonal symbol/nullary
	   predicate symbol), a symbol type (of the fo symbol to be
	   created), the arity of the new symbol, The status of the new
	   symbol.
  RETURNS: A first-order symbol corresponding to PropSymbol.
********************************************************/
{
  SYMBOL foSymbol;
  int size;
  char*  newName;
    
  if (eml_NaryType(SymbolType)) {
    char* arityStr = string_IntToString(Arity);
    size = strlen(eml_PREDNAMEPREFIX[SymbolType])+strlen(arityStr)+strlen(symbol_Name(PropSymbol))+1;
    newName = memory_Malloc(size*sizeof(char));

    /* Note: if SymbolType is n-ary then any propositional symbol can
     * be associated with more than one n-ary predicate symbol, for
     * that reason no user defined FoSymbols will be considered */
    sprintf(newName,"%s%s%s", eml_PREDNAMEPREFIX[SymbolType], arityStr,
	    symbol_Name(PropSymbol));
    foSymbol = symbol_CreatePredicate(newName, Arity, Status, Precedence);
    eml_SetPropNarySymbolAssocList(PropSymbol, Arity, foSymbol);

    memory_Free(newName,size*sizeof(char));
    string_StringFree(arityStr);
  }
  else if (eml_SemiFuncNdeType(SymbolType)) {
    size = strlen(eml_PREDNAMEPREFIX[eml_NDE])+strlen(symbol_Name(PropSymbol))+1;
    newName = memory_Malloc(size*sizeof(char));

    /* NOTE: For the moment no user defined FoSymbols will be
     * considered.  This restriction is provisional */
    sprintf(newName,"%s%s", eml_PREDNAMEPREFIX[eml_NDE], 
	    symbol_Name(PropSymbol));
    foSymbol = symbol_CreatePredicate(newName, Arity, Status, Precedence);
    eml_SetPropSemiFNdeSymbolAssocList(PropSymbol, Arity, foSymbol);

    memory_Free(newName,size*sizeof(char));
  }
  else {
    size = strlen(eml_PREDNAMEPREFIX[SymbolType])+strlen(symbol_Name(PropSymbol))+1;
    newName = memory_Malloc(size*sizeof(char));

    sprintf(newName,"%s%s", eml_PREDNAMEPREFIX[SymbolType],
	    symbol_Name(PropSymbol));
    foSymbol = symbol_CreatePredicate(newName, Arity, Status, Precedence);
    eml_SetPropFoSymbolAssocList(PropSymbol,list_List((POINTER)foSymbol));

    memory_Free(newName,size*sizeof(char));
  }
  
  return foSymbol;
}

TERM eml_CreateQuantifier(TERM Term, SYMBOL Quantifier, LIST VarList,
		LIST Arguments)
/*******************************************************
  INPUT:   A term (dummy, not used), symbol (which MUST be a fol
	   quantifier), a list of variables that will be bound, and
           a list of arguments.
  RETURNS: A new quantified term.
********************************************************/
{                                                                  
  return term_Create(Quantifier,                                             
		     list_Cons(term_Create(fol_Varlist(), VarList),   
			       Arguments));  
}

static TERM  eml_RplacWithQuantifier(TERM Term, SYMBOL Quantifier, 
		LIST VarList, LIST Arguments)
/**************************************************************
  INPUT:   A formula, a symbol (quantifier), two list of terms.
  RETURNS: A quantifier replacement.
  EFFECT:  Note, a term node is created for the variable list.
  CAUTION: The term in destructively changed.
***************************************************************/
{
  term_RplacTop(Term,Quantifier);
  term_RplacArgumentList(Term, 
	list_Cons(term_Create(fol_Varlist(), VarList), Arguments));

  return Term;
}

static TERM eml_BuildSortQuantifierFor(TERM Term, SYMBOL ModalTop,
		SYMBOL RelSubTop, SYMBOL Var, BOOL WithSort,
		LIST Arguments, POINTER (*BuildFunction)(), 
		PRECEDENCE Precedence)
/*******************************************************
  INPUT:   A term (which MUST be a modal term, i.e. dia(..) or box (...)),
	   a single variable, a Boolean value indicating whether sorts or
	   required, and a list of arguments.
  RETURNS: A quantified term, possibly restricted by the appropriate sort.
  CAUTION: The term is potentially destructively changed (depends on the
           BuildFunction).
********************************************************/
{                                                                  
#ifdef DEBUG                                                          
  if (!eml_IsModalOpSymbol(ModalTop)) {
    printf("\n\nIllegal input in eml_BuildSortQuantifierFor\n");
    misc_Error();
  }
#endif                                                             
                                                                   
  if (WithSort) {
    eml_SYMBOLTYPE symbolType;
    SYMBOL foSymbol;
#ifdef DEBUG                                                          
    if (!eml_IsNullaryPredicate(RelSubTop)) {
      printf("\n\nIllegal relational argument of modal operator, in eml_BuildSortQuantifierFor\n");
      misc_Error();
    }
#endif                                                             

    symbolType = eml_VARSORT;
    foSymbol   = eml_GetFoSymbolAssocWithPropSymbol(RelSubTop, 1, symbolType);
    if (foSymbol == symbol__NULL) {
      foSymbol = eml_CreateFoSymbol(RelSubTop, symbolType, 1, symbol_STATLEX, Precedence);
      /* Inheritance of precedence? */
      /* symbol_SetOrdering(Precedence, foSymbol,symbol_Ordering(Precedence, PropSymbol)); */
    }
    return BuildFunction(Term, eml_FoQuantAssocWithPropSymbol(ModalTop), 
		list_List(term_Create(Var,list_Nil())),   
		list_List(term_Create(eml_FoJunctorAssocWithPropSymbol(ModalTop),
		  list_Cons(term_Create(foSymbol,list_List(term_Create(Var,list_Nil()))), Arguments))));
    /* exists x S(x) /\ ... 
     * forall x S(x) -> ... */
  }
  else {
    return BuildFunction(Term, eml_FoQuantAssocWithPropSymbol(ModalTop),
		list_List(term_Create(Var,list_Nil())), Arguments);
    /* exists x ... 
     * forall x ... */
  }
}

static TERM  eml_RplacWithDesignatedPropAtom(TERM Term, SYMBOL RplacSymbol)
/**************************************************************
  INPUT:   A formula and a symbol.
  RETURNS: A replacement without arguments, e.g. true, false, id, etc.
  CAUTION: The term in destructively changed.
***************************************************************/
{
  term_RplacTop(Term,RplacSymbol);
  list_DeleteWithElement(term_ArgumentList(Term),(void (*)())term_Delete);
  term_RplacArgumentList(Term,list_Nil());

  return Term;
}

static TERM  eml_RplacWithOpAndArgs(TERM Term, SYMBOL RplacSymbol, 
		LIST Arguments)
/**************************************************************
  INPUT:   A formula, a symbol and a list of terms.
  RETURNS: A replacement with argument list.
  CAUTION: The term in destructively changed.
***************************************************************/
{
  term_RplacTop(Term,RplacSymbol);
  term_RplacArgumentList(Term,Arguments);

  return Term;
}

static TERM  eml_RemoveTrivialOperator(TERM Term)
/**************************************************************
  INPUT:   A formula.
  RETURNS: The formula after
           removal of "or", "and", "comp" and "sum" with only one argument
  CAUTION: The term in destrucively changed.
***************************************************************/
{
  if (eml_IsNaryJunctor(term_TopSymbol(Term)) &&
      list_Empty(list_Cdr(term_ArgumentList(Term)))) {
    TERM helpTerm = term_FirstArgument(Term);
    list_Delete(term_ArgumentList(Term));
    Term = eml_RplacWithOpAndArgs(Term, term_TopSymbol(helpTerm), term_ArgumentList(helpTerm));
    term_Free(helpTerm);
  }

  return Term;
}

static TERM  eml_RemoveTrivialAtomsAndOps(TERM Term)
/**************************************************************
  INPUT:   A formula.
  RETURNS: The formula after applying
           or(true,...) -> true,      and(true,phi) -> phi
           or(false,phi) -> phi,      and(false,...) -> false
                 etc plus
           dia(alpha,false) -> false, box(alpha,true) -> true
           dia(false,phi) -> false,   box(false,phi) -> true
  CAUTION: The term in destructively changed.
***************************************************************/
{
  SYMBOL Top, Subtop, RelSubtop;
  LIST   Scan;
  TERM   Subterm, RelSubterm;
  BOOL   Update;

  if (!term_IsComplex(Term))
    return Term;

  Top    = term_TopSymbol(Term);
  Update = FALSE;

  /* Term = and(...) */
  if (symbol_Equal(Top,fol_And())) {
    for (Scan=term_ArgumentList(Term); !list_Empty(Scan); Scan=list_Cdr(Scan)) {
      Subterm = eml_RemoveTrivialAtomsAndOps(list_Car(Scan));
      Subtop = term_TopSymbol(Subterm);
      if (symbol_Equal(Subtop,fol_True()))
        Update = TRUE;
      else if (symbol_Equal(Subtop,fol_False())) {
        /* and(..., false, ...) -> false */ 
        return eml_RplacWithDesignatedPropAtom(Term,fol_False());
      }
    }
    if (Update) {
      /* and(..., true, ..., true, ...) -> and(..., ..., ...) */ 
      term_RplacArgumentList(Term,list_DeleteElementIfFree(term_ArgumentList(Term),(BOOL (*)())fol_IsTrue,(void (*)())term_Delete));
      if (list_Empty(term_ArgumentList(Term))) {
        term_RplacTop(Term,fol_True());
        return Term;
      }
    }
    return eml_RemoveTrivialOperator(Term); /* and(phi) -> phi */
  }
  /* Term = or(...) */
  else if (symbol_Equal(Top,fol_Or())) {
    for (Scan=term_ArgumentList(Term); !list_Empty(Scan); Scan=list_Cdr(Scan)) {
      Subterm = eml_RemoveTrivialAtomsAndOps(list_Car(Scan));
      Subtop = term_TopSymbol(Subterm);
      if (symbol_Equal(Subtop,fol_False()))
        Update = TRUE;
      else if (symbol_Equal(Subtop,fol_True())) {
        /* or(..., true, ...) -> true */ 
        return eml_RplacWithDesignatedPropAtom(Term,fol_True());
      }
    }
    if (Update) {
      term_RplacArgumentList(Term,list_DeleteElementIfFree(term_ArgumentList(Term),(BOOL (*)())fol_IsFalse,(void (*)())term_Delete));
      if (list_Empty(term_ArgumentList(Term))) {
        term_RplacTop(Term,fol_False());
        return Term;
      }
    }
    return eml_RemoveTrivialOperator(Term); /* or(phi) -> phi */
  }
  /* Term = not(...) */
  else if (symbol_Equal(Top,fol_Not())) {
    Subterm = eml_RemoveTrivialAtomsAndOps(term_FirstArgument(Term));
    Subtop = term_TopSymbol(Subterm);
    if (eml_IsDesignatedPropSymbol(Subtop)) {
      /* not(false) -> true,  not(true) -> false,  not(div) -> id,  not(id) -> div */
      return eml_RplacWithDesignatedPropAtom(Term,eml_GetDualSymbol(Subtop));
    }
  }
  /* Term = forall(...)  or  Term = exists(...) */
  else if (fol_IsQuantifier(Top)) {
    Subterm = eml_RemoveTrivialAtomsAndOps(term_SecondArgument(Term));
    Subtop = term_TopSymbol(Subterm);
    if (symbol_Equal(Subtop,fol_True())) {
      /* forall ... true -> true,  exists ... true -> true */
      return eml_RplacWithDesignatedPropAtom(Term,fol_True());
    }
    else if (symbol_Equal(Subtop,fol_False())) {
      /* forall ... false -> false,  exists ... false -> false */
      return eml_RplacWithDesignatedPropAtom(Term,fol_False());
    }
  }
  /* Term = dia(...) or Term = box(...) */
  else if (eml_IsModalOpSymbol(Top)) {
    RelSubterm = eml_RemoveTrivialAtomsAndOps(term_FirstArgument(Term));
    RelSubtop = term_TopSymbol(RelSubterm);
    Subterm    = eml_RemoveTrivialAtomsAndOps(term_SecondArgument(Term));
    Subtop    = term_TopSymbol(Subterm);
    if (symbol_Equal(Top,eml_Dia()) && 
         (symbol_Equal(Subtop,fol_False()) || symbol_Equal(RelSubtop,fol_False()))) {
      /* dia(alpha,false) -> false,  dia(false,phi) -> false */
      return eml_RplacWithDesignatedPropAtom(Term,fol_False());
    }
    else if (symbol_Equal(Top,eml_Box()) && 
       (symbol_Equal(Subtop,fol_True()) || symbol_Equal(RelSubtop,fol_False()))) {
      /* box(alpha,true) -> true,  box(true,phi) -> true */
      return eml_RplacWithDesignatedPropAtom(Term,fol_True());
    }
    else if (symbol_Equal(RelSubtop, eml_Id())) {
      /* dia(id,phi) -> phi,  box(id,phi) -> phi */
      term_Delete(RelSubterm);
      list_Delete(term_ArgumentList(Term));
      Term = eml_RplacWithOpAndArgs(Term, Subtop, term_ArgumentList(Subterm));
      term_Free(Subterm);
      return Term;
    }
  }
  /* Term = domain(...) or Term = range(...) */
  else if (symbol_Equal(Top,eml_Domain()) || symbol_Equal(Top,eml_Range())) {
    RelSubterm = eml_RemoveTrivialAtomsAndOps(term_FirstArgument(Term));
    RelSubtop = term_TopSymbol(RelSubterm);
    if (symbol_Equal(RelSubtop,fol_False())) {
      /* domain(false) -> false,  range(false) -> false */
      return eml_RplacWithDesignatedPropAtom(Term,fol_False());
    }
    else if (symbol_Equal(RelSubtop, eml_Id())) {
      /* domain(id) -> true,  range(id) -> true */
      return eml_RplacWithDesignatedPropAtom(Term,fol_True());
    }
  }
  /* Term = implies(...) */
  else if (symbol_Equal(Top,fol_Implies())) {
    Subterm = eml_RemoveTrivialAtomsAndOps(term_FirstArgument(Term));
    Subtop = term_TopSymbol(Subterm);
    if (symbol_Equal(Subtop,fol_False())) {
      /* implies(false,phi) -> true */
      return eml_RplacWithDesignatedPropAtom(Term,fol_True());
    }
    else if (symbol_Equal(Subtop,fol_True())) {
      /* implies(true,phi) -> phi */
      term_Delete(Subterm);
      Subterm = term_SecondArgument(Term);
      list_Delete(term_ArgumentList(Term));
      Term = eml_RplacWithOpAndArgs(Term,term_TopSymbol(Subterm),term_ArgumentList(Subterm));
      term_Free(Subterm);
      return eml_RemoveTrivialAtomsAndOps(Term);
    }
    Subterm = eml_RemoveTrivialAtomsAndOps(term_SecondArgument(Term));
    Subtop = term_TopSymbol(Subterm);
    if (symbol_Equal(Subtop,fol_True())) {
      /* implies(phi,true) -> true */
      return eml_RplacWithDesignatedPropAtom(Term,fol_True());
    }
    else if (symbol_Equal(Subtop,fol_False()))  {
      /* implies(phi,false) -> not(phi) */
      Term = eml_RplacWithOpAndArgs(Term, fol_Not(),
		      list_DeleteElementIfFree(term_ArgumentList(Term),
			  (BOOL (*)())fol_IsFalse, (void (*)())term_Delete));
      return eml_RemoveTrivialAtomsAndOps(Term);
    }
  }
  /* Term = implied(...) */
  else if (symbol_Equal(Top,fol_Implied())) {
    Subterm = eml_RemoveTrivialAtomsAndOps(term_SecondArgument(Term));
    Subtop = term_TopSymbol(Subterm);
    if (symbol_Equal(Subtop,fol_False())) {
      /* implied(phi,false) -> true */
      return eml_RplacWithDesignatedPropAtom(Term,fol_True());
    }
    else if (symbol_Equal(Subtop,fol_True())) {
      /* implied(phi,true) -> phi */
      term_Delete(Subterm);
      Subterm = term_FirstArgument(Term);
      list_Delete(term_ArgumentList(Term));
      Term = eml_RplacWithOpAndArgs(Term,term_TopSymbol(Subterm),term_ArgumentList(Subterm));
      term_Free(Subterm);
      return eml_RemoveTrivialAtomsAndOps(Term);
    }
    Subterm = eml_RemoveTrivialAtomsAndOps(term_FirstArgument(Term));
    Subtop = term_TopSymbol(Subterm);
    if (symbol_Equal(Subtop,fol_True()))
      /* implied(true,phi) -> true */
      return eml_RplacWithDesignatedPropAtom(Term,fol_True());
    else if (symbol_Equal(Subtop,fol_False())) {
      /* implied(false,phi) -> not(phi) */
      Term = eml_RplacWithOpAndArgs(Term, fol_Not(),
		      list_DeleteElementIfFree(term_ArgumentList(Term),
			  (BOOL (*)())fol_IsFalse, (void (*)())term_Delete));
      return eml_RemoveTrivialAtomsAndOps(Term);
    }
  }
  /* Term = equiv(...) */
  else if (symbol_Equal(Top,fol_Equiv())) {
    Subterm = eml_RemoveTrivialAtomsAndOps(term_FirstArgument(Term));
    Subtop = term_TopSymbol(Subterm);
    if (symbol_Equal(Subtop,fol_False())) {
      /* equiv(false,phi) -> not(phi) */
      Term = eml_RplacWithOpAndArgs(Term, fol_Not(),
		      list_DeleteElementIfFree(term_ArgumentList(Term),
			  (BOOL (*)())fol_IsFalse, (void (*)())term_Delete));
      return eml_RemoveTrivialAtomsAndOps(Term);
    }
    else if (symbol_Equal(Subtop,fol_True())) {
      /* equiv(true,phi) -> phi */
      term_Delete(Subterm);
      Subterm = term_SecondArgument(Term);
      list_Delete(term_ArgumentList(Term));
      Term = eml_RplacWithOpAndArgs(Term, term_TopSymbol(Subterm),
		      term_ArgumentList(Subterm));
      term_Free(Subterm);
      return eml_RemoveTrivialAtomsAndOps(Term);
    }
    Subterm = eml_RemoveTrivialAtomsAndOps(term_SecondArgument(Term));
    Subtop = term_TopSymbol(Subterm);
    if (symbol_Equal(Subtop,fol_False())) {
      /* equiv(phi,false) -> not(phi) */
      Term = eml_RplacWithOpAndArgs(Term, fol_Not(),
		list_DeleteElementIfFree(term_ArgumentList(Term),
		    (BOOL (*)())fol_IsFalse, (void (*)())term_Delete));
      return eml_RemoveTrivialAtomsAndOps(Term);
    }
    else if (symbol_Equal(Subtop,fol_True())) {
      /* equiv(phi,true) -> phi */
      term_Delete(Subterm);
      Subterm = term_FirstArgument(Term);
      list_Delete(term_ArgumentList(Term));
      Term = eml_RplacWithOpAndArgs(Term, term_TopSymbol(Subterm),
		term_ArgumentList(Subterm));
      term_Free(Subterm);
    }
  }
  /* Term = comp(...) */
  else if (symbol_Equal(Top,eml_Comp())) {
    for (Scan=term_ArgumentList(Term); !list_Empty(Scan); Scan=list_Cdr(Scan)) {
      Subterm = eml_RemoveTrivialAtomsAndOps(list_Car(Scan));
      Subtop = term_TopSymbol(Subterm);
      if (symbol_Equal(Subtop,eml_Id()))
        Update = TRUE;
      else if (symbol_Equal(Subtop,fol_False())) {
	/* comp(..., false, ...) -> false */
        return eml_RplacWithDesignatedPropAtom(Term,fol_False());
      }
    }
    if (Update) {
      /* comp(..., id, ..., id, ...) -> comp(..., ..., ...) */
      term_RplacArgumentList(Term,
	      list_DeleteElementIfFree(term_ArgumentList(Term),
		  (BOOL (*)())eml_IsId, (void (*)())term_Delete));
      if (list_Empty(term_ArgumentList(Term))) {
        term_RplacTop(Term,eml_Id());
        return Term;
      }
    }
    return eml_RemoveTrivialOperator(Term); /* comp(alpha) -> alpha */
  }
  /* Term = sum(...) */
  else if (symbol_Equal(Top,eml_Sum())) {
    for (Scan=term_ArgumentList(Term); !list_Empty(Scan); Scan=list_Cdr(Scan)) {
      Subterm = eml_RemoveTrivialAtomsAndOps(list_Car(Scan));
      Subtop = term_TopSymbol(Subterm);
      if (symbol_Equal(Subtop,eml_Div()))
        Update = TRUE;
      else if (symbol_Equal(Subtop,fol_True())) {
	/* sum(..., true, ...) -> true */
        return eml_RplacWithDesignatedPropAtom(Term,fol_True());
      }
    }
    if (Update) {
      /* sum(..., div, ..., div, ...) -> sum(..., ..., ...) */
      term_RplacArgumentList(Term,
	      list_DeleteElementIfFree(term_ArgumentList(Term),
		  (BOOL (*)())eml_IsDiv, (void (*)())term_Delete));
      if (list_Empty(term_ArgumentList(Term))) {
        term_RplacTop(Term,eml_Div());
        return Term;
      }
    }
    return eml_RemoveTrivialOperator(Term); /* sum(alpha) -> alpha */
  } 
  /* Term = test(...) */
  else if (symbol_Equal(Top,eml_Test())) {
    Subterm = eml_RemoveTrivialAtomsAndOps(term_FirstArgument(Term));
    Subtop = term_TopSymbol(Subterm);
    if (symbol_Equal(Subtop,fol_False())) {
      /* test(false) -> false */
      return eml_RplacWithDesignatedPropAtom(Term,fol_False());
    }
    else if (symbol_Equal(Subtop, fol_True())) {
      /* test(true) -> id  */
      return eml_RplacWithDesignatedPropAtom(Term,eml_Id());
    }
  }
  /* Term = conv(...) */
  else if (symbol_Equal(Top,eml_Conv())) {
    RelSubterm = eml_RemoveTrivialAtomsAndOps(term_FirstArgument(Term));
    RelSubtop = term_TopSymbol(RelSubterm);
    if (symbol_Equal(RelSubtop,fol_True()) ||
		symbol_Equal(RelSubtop,fol_False()) ||
		symbol_Equal(RelSubtop,eml_Id()) ||
		symbol_Equal(RelSubtop,eml_Div())) {
      /* conv(false) -> false,  conv(true) -> true,  conv(id) -> id,  conv(div) -> div */
      return eml_RplacWithDesignatedPropAtom(Term,RelSubtop);
    }
    else if (symbol_Equal(RelSubtop,eml_Test())) {
      /* conv(test(...)) -> test(...)  */
      list_Delete(term_ArgumentList(Term));
      Term = eml_RplacWithOpAndArgs(Term, RelSubtop, term_ArgumentList(RelSubterm));
      term_Free(RelSubterm);
      return Term;
    }
  }
  /* Term = domrestr(...) or Term = ranrestr(...) */
  else if (symbol_Equal(Top,eml_DomRestr()) || symbol_Equal(Top,eml_RanRestr())) {
    RelSubterm = eml_RemoveTrivialAtomsAndOps(term_FirstArgument(Term));
    RelSubtop  = term_TopSymbol(RelSubterm);
    Subterm    = eml_RemoveTrivialAtomsAndOps(term_SecondArgument(Term));
    Subtop     = term_TopSymbol(Subterm);
    if (symbol_Equal(RelSubtop,fol_False()) || symbol_Equal(Subtop,fol_False())) {
      /* domrestr(false,...), domrestr(...,false) -> false,  
       * ranrestr(false,...), ranrestr(...,false) -> false */
      return eml_RplacWithDesignatedPropAtom(Term,fol_False());
    }
    else if (symbol_Equal(Subtop, fol_True())) {
      /* domrestr(alpha,true) -> alpha,  ranrestr(alpha,true) -> alpha */
      term_Delete(Subterm);
      list_Delete(term_ArgumentList(Term));
      Term = eml_RplacWithOpAndArgs(Term, RelSubtop, term_ArgumentList(RelSubterm));
      term_Free(RelSubterm);
      return Term;
    }
  }

  return Term;
}

static TERM eml_CreateModalTerm(SYMBOL ModOp, LIST RelTermList, TERM PropTerm)
/**************************************************************
  INPUT:   A symbol (either box or dia), a list of relational terms, 
           a list with one propositional term.
  RETURNS: A nested box(..., box(..., box(..., ....)))
           or dia(..., dia(..., dia(..., ....))) term
***************************************************************/
{
  if (list_Empty(RelTermList)) {
    return PropTerm;
  }
  else {
    return term_Create(ModOp,
             list_Cons(list_Car(RelTermList),
	       list_List(eml_CreateModalTerm(ModOp, list_Cdr(RelTermList), PropTerm))));
  }
}

static TERM eml_EliminateComp(TERM Term)
/**************************************************************
  INPUT:   A Term.
  RETURNS: eliminates comp if possible by replacing, e.g. 
           box(comp(..., ...., ....), ....) with 
	   box(..., box(..., box(..., ....))).
  CAUTION: The Term is destructively changed.
***************************************************************/
{
  SYMBOL Top;
  TERM   RelSubterm;

  Top    = term_TopSymbol(Term);

  /* box(comp(..., ...., ....), ....) -> box(..., box(..., box(..., ....)))
     dia(comp(..., ...., ....), ....) -> dia(..., dia(..., dia(..., ....))) */ 
  if (eml_IsModalOpSymbol(Top)) {
    RelSubterm = term_FirstArgument(Term);
    if (symbol_Equal(term_TopSymbol(RelSubterm),eml_Comp())) {
      LIST ArgsList;
      ArgsList = list_Cons(list_Car(term_ArgumentList(RelSubterm)),
	         list_List(eml_CreateModalTerm(Top, 
	           list_Cdr(term_ArgumentList(RelSubterm)), 
		     eml_EliminateComp(term_SecondArgument(Term)))));
      list_Delete(term_ArgumentList(RelSubterm));
      term_Free(RelSubterm);
      list_Delete(term_ArgumentList(Term));
      Term = eml_RplacWithOpAndArgs(Term, Top, ArgsList);
    }
  } else if (!symbol_IsPredicate(Top)) {
    LIST   Scan;
    for (Scan=term_ArgumentList(Term);!list_Empty(Scan);Scan=list_Cdr(Scan))
      eml_EliminateComp(list_Car(Scan));
  }

    return Term;
}

static TERM eml_MakeOneAndOrCompSum(TERM Term)
/**************************************************************
  INPUT:   A Term.
  RETURNS: Takes all arguments of an and together.
  CAUTION: The Term is destructively changed.
***************************************************************/
{
  SYMBOL Top;
  LIST   Scan;
  TERM   Subterm;

  Top    = term_TopSymbol(Term);

  /* and(..., and( ..., ...)) -> and(..., ..., ...) */ 
  if (symbol_Equal(Top,fol_And())) {
    Scan = term_ArgumentList(Term);
    while (!list_Empty(Scan)) {
      Subterm = (TERM)list_Car(Scan);
      eml_MakeOneAndOrCompSum(Subterm);
      if (symbol_Equal(term_TopSymbol(Subterm),fol_And())) {
	Scan = list_Cdr(Scan);
	term_RplacArgumentList(Term,
	  list_Nconc(term_ArgumentList(Subterm),list_PointerDeleteElement(term_ArgumentList(Term),Subterm)));
	term_Free(Subterm);
      }
      else
	Scan = list_Cdr(Scan);
    }

  /* or(..., or( ..., ...)) -> or(..., ..., ...) */ 
  } else if (symbol_Equal(Top,fol_Or())) {
    Scan = term_ArgumentList(Term);
    while (!list_Empty(Scan)) {
      Subterm = (TERM)list_Car(Scan);
      eml_MakeOneAndOrCompSum(Subterm);
      if (symbol_Equal(term_TopSymbol(Subterm),fol_Or())) {
	Scan = list_Cdr(Scan);
	term_RplacArgumentList(Term,
	  list_Nconc(term_ArgumentList(Subterm),list_PointerDeleteElement(term_ArgumentList(Term),Subterm)));
	term_Free(Subterm);
      }
      else
	Scan = list_Cdr(Scan);
    }

  /* comp(..., comp( ..., ...)) -> comp(..., ..., ...) */ 
  } else if (symbol_Equal(Top,eml_Comp())) {
    Scan = term_ArgumentList(Term);
    while (!list_Empty(Scan)) {
      Subterm = (TERM)list_Car(Scan);
      eml_MakeOneAndOrCompSum(Subterm);
      if (symbol_Equal(term_TopSymbol(Subterm),eml_Comp())) {
	Scan = list_Cdr(Scan);
	term_RplacArgumentList(Term,
	  list_Nconc(term_ArgumentList(Subterm),list_PointerDeleteElement(term_ArgumentList(Term),Subterm)));
	term_Free(Subterm);
      }
      else
	Scan = list_Cdr(Scan);
    }

  /* sum(..., sum( ..., ...)) -> sum(..., ..., ...) */ 
  } else if (symbol_Equal(Top,eml_Sum())) {
    Scan = term_ArgumentList(Term);
    while (!list_Empty(Scan)) {
      Subterm = (TERM)list_Car(Scan);
      eml_MakeOneAndOrCompSum(Subterm);
      if (symbol_Equal(term_TopSymbol(Subterm),eml_Sum())) {
	Scan = list_Cdr(Scan);
	term_RplacArgumentList(Term,
	  list_Nconc(term_ArgumentList(Subterm),list_PointerDeleteElement(term_ArgumentList(Term),Subterm)));
	term_Free(Subterm);
      }
      else
	Scan = list_Cdr(Scan);
    }

  } else if (!symbol_IsPredicate(Top))
    for (Scan=term_ArgumentList(Term);!list_Empty(Scan);Scan=list_Cdr(Scan))
      eml_MakeOneAndOrCompSum(list_Car(Scan));

  return Term;
}

TERM  eml_SimplifyQuantors(TERM Term)
/**************************************************************
  INPUT:   A formula.
  RETURNS: The formula after
	   removal of bindings of variables that don't occur in the
	   respective subformula and possible mergings of quantors
  CAUTION: The term is destrucively changed.
  NOTE:    This procedure conincides with cnf_SimplifyQuantors()
***************************************************************/
{
   SYMBOL Top;
   LIST   Scan;

  Top = term_TopSymbol(Term);

  if (symbol_IsPredicate(Top) || symbol_Equal(Top,fol_Varlist()))
    return Term;

  if (fol_IsQuantifier(Top)) {
    LIST Vars;
    TERM Var,Subterm,Aux;
    Vars    = list_Nil();
    Subterm = term_SecondArgument(Term);

    while (symbol_Equal(term_TopSymbol(Subterm),Top)) {
      term_RplacArgumentList(term_FirstArgument(Term),
			     list_Nconc(fol_QuantifierVariables(Term),fol_QuantifierVariables(Subterm)));
      term_Free(term_FirstArgument(Subterm));
      Aux = term_SecondArgument(Subterm);
      list_Delete(term_ArgumentList(Subterm));
      term_Free(Subterm);
      list_Rplaca(list_Cdr(term_ArgumentList(Term)),Aux);
      Subterm = Aux;
    }
    
    for(Scan=fol_QuantifierVariables(Term);!list_Empty(Scan);Scan=list_Cdr(Scan)) {
      Var = (TERM)list_Car(Scan);
      if (!fol_VarOccursFreely(Var,Subterm))
	Vars = list_Cons(Var,Vars);
    }
    if (!list_Empty(Vars)) {
      Subterm = term_FirstArgument(Term);
      term_RplacArgumentList(Subterm,list_NPointerDifference(term_ArgumentList(Subterm),Vars));
      list_DeleteWithElement(Vars, (void (*)())term_Delete);
      if (list_Empty(term_ArgumentList(Subterm))) {
	Subterm = term_SecondArgument(Term);
	term_Delete(term_FirstArgument(Term));
	list_Delete(term_ArgumentList(Term));
	term_Free(Term);
	return eml_SimplifyQuantors(Subterm);
      }
    }
  }
  
  for (Scan=term_ArgumentList(Term);!list_Empty(Scan);Scan=list_Cdr(Scan))
    list_Rplaca(Scan,eml_SimplifyQuantors(list_Car(Scan)));
  
  return Term; 
}

TERM  eml_ObviousSimplifications(TERM Term, FLAGSTORE Flags)
/**************************************************************
  INPUT:   An eml formula.
  RETURNS: The formula after performing the following simplifications:
             - eliminate true, false, id, etc 
             - remove "or" and "and" with only one argument
             - merge quantors
  CAUTION: The term is destructively changed.
  NOTE:    This does not obliviate simplification of the first-order
	   formula.  E.g. dia(alpha,true) cannot be reduced in the modal
	   context.
	   Nested expression of n-ary associative operations (and, comp,
	   ...) are reduced later.
***************************************************************/
{
  Term = eml_RemoveTrivialAtomsAndOps(Term);
  Term = eml_SimplifyQuantors(Term);   /* strictly only needed for non-propositional formulae */

  /* Attempt to eliminate composition by applying */
  if (flag_GetFlagIntValue(Flags, flag_EMLELIMCOMP)) {
    Term = eml_MakeOneAndOrCompSum(Term);
    Term = eml_EliminateComp(Term);
  }

  return Term;
}

BOOL eml_IsPropositional(TERM Term)
/**************************************************************
  INPUT:   A formula.
  RETURNS: True, if formula does not contain an n-ary predicate symbol
           or a first order quantifier.
***************************************************************/
{
  LIST Scan;
  BOOL result;

  result = TRUE;
  if (term_IsComplex(Term)) {
    if (symbol_IsPredicate(term_TopSymbol(Term)) || 
	fol_IsQuantifier(term_TopSymbol(Term)))
      return FALSE;
    else 
      for(Scan=term_ArgumentList(Term); result && !list_Empty(Scan); Scan=list_Cdr(Scan))
        result = eml_IsPropositional(list_Car(Scan));
  }

  return result;
}

static TERM eml_RplacPropByFoAtom(TERM Atom, eml_SYMBOLTYPE SymbolType,
		int Arity, LIST Args, FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A propositional atom, a symbol type, a natural number, and a
           list of variables.
  RETURNS: A first-order atom with the specified arity formed with the
	   symbol of the specified type and the specified arguments.
	   The predicate name is built from the propositional name.
  EFFECT:  If the predicate symbol does not exist in the signature it is
           created and inherits its precedence setting.
  CAUTION: The atom is destructively changed.
***************************************************************/
{
  SYMBOL propSymbol, foSymbol;

#ifdef DEBUG
  if (list_Empty(term_ArgumentList(Atom)) {
	misc_StartErrorReport();
	misc_ErrorReport("\n In  eml_RplacPropByFoAtom: Propositional atom must not have argument list.");
	misc_FinishErrorReport();
      } 
#endif

  propSymbol = term_TopSymbol(Atom);

  if (symbol_Equal(propSymbol, fol_True()) || 
      symbol_Equal(propSymbol, fol_False())) {
    list_DeleteWithElement(Args, (void (*)())term_Delete);
    return Atom;  /* Attempt to simplify ?? */
  }

  if (symbol_Equal(propSymbol, eml_Id())) {  /* Arguments should be nil, propositional */
    symbol_SetOrdering(Precedence, fol_Equality(), symbol_Ordering(Precedence, eml_Id()));
    return eml_RplacWithOpAndArgs(Atom, fol_Equality(), Args);
  }
  else if (symbol_Equal(propSymbol, eml_Div())) {
    return eml_RplacWithOpAndArgs(Atom, fol_Not(),
		    list_List(term_Create(fol_Equality(), Args)));
  }

  foSymbol   = eml_GetFoSymbolAssocWithPropSymbol(propSymbol, Arity, SymbolType);

#ifdef DEBUG
  if (!eml_NaryType(SymbolType) && (foSymbol != symbol__NULL) &&
	    (symbol_Arity(foSymbol) != Arity)) {
    fprintf(stderr, "Symbol %s declared with arity %d, but arity %d required\n",
    symbol_Name(foSymbol), symbol_Arity(foSymbol),Arity);
    misc_Error();
    return NULL;
  } 
#endif

  if (foSymbol == symbol__NULL) {
    foSymbol = eml_CreateFoSymbol(propSymbol, SymbolType, Arity, symbol_STATLEX, Precedence);
    /* Precedence among the first-order symbols is inherited from that among
     * the propositional symbols */
    symbol_SetOrdering(Precedence, foSymbol,symbol_Ordering(Precedence, propSymbol));
  }

  return eml_RplacWithOpAndArgs(Atom, foSymbol, Args);
}

SYMBOL eml_CreateNarySymbol(SYMBOL PropSymbol, eml_SYMBOLTYPE SymbolType,
		const char* NameSuffix, int Arity, int Status, PRECEDENCE Precedence)
/*******************************************************
  INPUT:   A symbol (which MUST be a propositonal symbol/nullary
	   predicate symbol), a symbol type (of the fo symbol to be
	   created), the predefined name suffix, the arity of the new
	   symbol, the status of the new symbol.
  RETURNS: A first-order symbol corresponding to PropSymbol (special for
	   the functial translation in terms of n-ary predicates with the sorts
	   encoded in the predicate names).
  REMARK:  Similar to dfg_SymbolDecl.  Prototypical.
********************************************************/
{
  int size;
  char*  newName;
  char* arityStr;
  SYMBOL symbol;

  arityStr = string_IntToString(Arity);

  size = strlen(eml_PREDNAMEPREFIX[SymbolType])+strlen(symbol_Name(PropSymbol))+1+strlen(arityStr)+strlen(NameSuffix)+1;
  newName = (char*)memory_Malloc(sizeof(char)*size);
  sprintf(newName,"%s%s_%s%s", eml_PREDNAMEPREFIX[SymbolType],symbol_Name(PropSymbol), arityStr, NameSuffix);
  
  /* Check if this symbol was declared earlier */
  symbol = symbol_Lookup(newName);
  if (symbol != 0) {
    /* Symbol was declared before 
     * Check if the old and new symbol type are equal */
    /* Eventually we would want more checking to be done here, i.e.
     * we need new designated symbol types */
    if ( !symbol_IsPredicate(symbol)) {
      fprintf(stderr, "In eml_CreateNarySymbol: Symbol %s was already declared as ",
	      newName);
      switch (symbol_Type(symbol)) {
      case symbol_CONSTANT:
      case symbol_FUNCTION:
	fputs("function\n", stderr); break;
      case symbol_JUNCTOR:
	fputs("junctor", stderr); break;
      default:
	fputs("unknown non-predicate type\n", stderr);
      }
      misc_Error();
    }
    /* Now check the old and new arity if specified */
    if (Arity != -2 && Arity != symbol_Arity(symbol)) {
      fprintf(stderr, "In eml_CreateNarySymbol: Symbol %s was already declared with arity %d\n",
	      newName, symbol_Arity(symbol));
      misc_Error();
    }
  } else
    /* Symbol was not declared before */
    symbol = symbol_CreatePredicate(newName, Arity, Status, Precedence);

  string_StringFree(arityStr);
  memory_Free(newName,sizeof(char)*size);

  return  symbol;
}

static TERM eml_RplacPropByNaryAtom(TERM Atom, eml_SYMBOLTYPE
		SymbolType, char* NameSuffix, int Arity, LIST Args, 
		FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A propositional atom, a symbol type, a string, a natural
           number, and a list of variables.
  RETURNS: A first-order atom with the specified arity formed with the
	   symbol of the specified type and the specified arguments.
	   The predicate name is built from the propositional name and
	   (different from eml_RplacPropByFoAtom) the given name suffix.
  EFFECT:  A new predicate symbol is created and inherits its precedence
           setting.
  CAUTION: The atom is destructively changed.
***************************************************************/
{
  SYMBOL propSymbol, foSymbol;

  propSymbol = term_TopSymbol(Atom);

  if (symbol_Equal(propSymbol,fol_True()) || symbol_Equal(propSymbol,fol_False())) {
    list_DeleteWithElement(Args, (void (*)())term_Delete);
    return Atom;
  }

  if (flag_GetFlagIntValue(Flags, flag_EMLFFSORTS))  /* with explicit sorts */
    foSymbol = eml_CreateNarySymbol(propSymbol, SymbolType,
		    symbol_Name(propSymbol), Arity, symbol_STATLEX, Precedence);
  else                    /* sort info encoded in predicate names */
    foSymbol = eml_CreateNarySymbol(propSymbol, SymbolType, NameSuffix, Arity,
		    symbol_STATLEX, Precedence);
  symbol_SetOrdering(Precedence, foSymbol,symbol_Ordering(Precedence, propSymbol));  /* ?? */

  return eml_RplacWithOpAndArgs(Atom, foSymbol, Args);
}

static TERM eml_EmbedInRelCalculusProp(TERM Term)
/**************************************************************
  INPUT:   An Eml (propositional) formula.
  RETURNS: Its translation into a relational formula as a right
           injective formula.
  CAUTION: The term is destructively changed.
***************************************************************/
{
  SYMBOL Top;

  Top    = term_TopSymbol(Term);

  /* Term = p */
  if (symbol_IsPredicate(Top)) {
    list_Delete(term_ArgumentList(Term)); /* shouldn't be needed */
    return eml_RplacWithOpAndArgs(Term, eml_Comp(),
	     list_Cons(term_Copy(Term), 
		list_List(term_Create(fol_True(), list_Nil()))));
    /* comp(p,true) */
  }
  /* Term = not(phi), or(...), and(...), comp(...), sum(...), 
   *        test(phi), domrestr(...), ranrestr(...) */
  else  if (eml_IsRelationalJunctor(Top)) { /* Seems strange, but makes sense */
    LIST Scan;
    for (Scan=term_ArgumentList(Term); !list_Empty(Scan); Scan=list_Cdr(Scan))
      eml_EmbedInRelCalculusProp(list_Car(Scan));
    return Term;
  }
  /* Term = dia(alpha,phi) */
  else  if (symbol_Equal(Top, eml_Dia())) {
    LIST args;          
    args = list_Cons(eml_EmbedInRelCalculusRel(term_FirstArgument(Term)), 
	     list_List(eml_EmbedInRelCalculusProp(term_SecondArgument(Term))));
    list_Delete(term_ArgumentList(Term)); 
    return eml_RplacWithOpAndArgs(Term, eml_Comp(), args);
    /* comp(alpha,tr(phi)) */
  }
  /* Term = box(alpha,phi) */
  else  if (symbol_Equal(Top, eml_Box())) {
    TERM helpTerm;          
    helpTerm = term_Create(eml_Comp(), 
		 list_Cons(eml_EmbedInRelCalculusRel(term_FirstArgument(Term)),
		   list_List(term_Create(fol_Not(), 
		     list_List(eml_EmbedInRelCalculusProp(term_SecondArgument(Term)))))));
    list_Delete(term_ArgumentList(Term)); 
    return eml_RplacWithOpAndArgs(Term, fol_Not(), list_List(helpTerm));
    /* not(comp(alpha,not(tr(phi)))) */
  }
  /* Term = domain(alpha)  or  Term = range(alpha) */
  else if (symbol_Equal(Top,eml_Domain()) || symbol_Equal(Top,eml_Range())) {
    LIST args;          
    term_RplacTop(Term, eml_Comp());
    if (symbol_Equal(Top,eml_Domain())) 
      args = list_Cons(eml_EmbedInRelCalculusRel(term_FirstArgument(Term)), 
		   list_List(term_Create(fol_True(), list_Nil())));
    else
      args = list_Cons(term_Create(fol_True(), list_Nil()), 
		   list_List(eml_EmbedInRelCalculusRel(term_FirstArgument(Term))));
    list_Delete(term_ArgumentList(Term)); 
    term_RplacArgumentList(Term,args);

    return Term;
    /* comp(alpha,true)  or  comp(true,alpha) */
  }
  else {
    fprintf(stderr, "\nNo case for %s in eml_EmbedInRelCalculusProp()\n",
             symbol_Name(Top));
    misc_Error();
    return NULL;
  }

}

static TERM eml_EmbedInRelCalculusRel(TERM Term)
/**************************************************************
  INPUT:   An Eml (propositional) formula.
  RETURNS: Its translation into a relational formula as a right
           injective formula.
  CAUTION: The term is destructively changed.
***************************************************************/
{
  SYMBOL Top;
  LIST args;          

  Top    = term_TopSymbol(Term);

  /* Term = p */
  if (symbol_IsPredicate(Top)) {
    return Term;
  }
  /* Term = test(phi) */
  else  if (symbol_Equal(Top, eml_Test())) {
    args = list_Cons(eml_EmbedInRelCalculusProp(term_FirstArgument(Term)),
	       list_List(term_Create(eml_Id(), list_Nil())));
    list_Delete(term_ArgumentList(Term));
    return eml_RplacWithOpAndArgs(Term, fol_And(), args);
    /* tr(phi) /\ id */
  }
  /* Term = domrestr(alpha,phi) */
  else  if (symbol_Equal(Top, eml_DomRestr())) {
    args = list_Cons(term_FirstArgument(Term),
	       list_List(eml_EmbedInRelCalculusProp(term_SecondArgument(Term))));
    list_Delete(term_ArgumentList(Term)); 
    return eml_RplacWithOpAndArgs(Term, fol_And(), args);
    /* alpha /\ tr(phi) */
  }
  /* Term = ranrestr(alpha,phi) */
  else  if (symbol_Equal(Top, eml_RanRestr())) {
    args = list_Cons(term_FirstArgument(Term),
	       list_List(term_Create(eml_Conv(),
		 list_List(eml_EmbedInRelCalculusProp(term_SecondArgument(Term))))));
    list_Delete(term_ArgumentList(Term)); 
    return eml_RplacWithOpAndArgs(Term, fol_And(), args);
    /* alpha /\ conv(tr(phi)) */
  }
  /* Term = not(phi), or(...), and(...), comp(...), sum(...) */
  /* It is important that this test is done last */
  else  if (eml_IsRelationalJunctor(Top)) {
    LIST Scan;
    for (Scan=term_ArgumentList(Term); !list_Empty(Scan); Scan=list_Cdr(Scan))
      eml_EmbedInRelCalculusRel(list_Car(Scan));
    return Term;
  }
  else {
    fprintf(stderr, "\nNo case for %s in eml_EmbedInRelCalculusRel()\n",
             symbol_Name(Top));
    misc_Error();
    return NULL;
  }

}


static TERM eml_RelationalTranslationProp(TERM Term, SYMBOL Var, 
	FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   An eml (propositional) formula and a variable (the free variable).
  RETURNS: Its standard (relational) translation.
  CAUTION: The term is destructively changed.
***************************************************************/
{
  SYMBOL Top;

  Top    = term_TopSymbol(Term);

  /* Term = p */
  if (symbol_IsPredicate(Top)) {
    return eml_RplacPropByFoAtom(Term, eml_PROP, 1, list_List(term_Create(Var,list_Nil())), Flags, Precedence);
  }
  /* Term = not(phi), Term = or(...) or Term = and(...) */
  else  if (eml_IsPropJunctor(Top)) {
    LIST Scan;
    for (Scan=term_ArgumentList(Term); !list_Empty(Scan); Scan=list_Cdr(Scan))
      eml_RelationalTranslationProp(list_Car(Scan),Var, Flags, Precedence);
    return Term;
  }
  /* Term = dia(alpha,phi)  or  Term = box(alpha,phi) */
  else if (eml_IsModalOpSymbol(Top)) {
    SYMBOL newVar;
    TERM   helpTerm;          
    newVar   = symbol_CreateStandardVariable();
    helpTerm = term_Create(eml_FoJunctorAssocWithPropSymbol(Top), 
        list_Cons(eml_RelationalTranslationRel(term_FirstArgument(Term),Var,newVar, Flags, Precedence),
        list_List(eml_RelationalTranslationProp(term_SecondArgument(Term),newVar, Flags, Precedence))));
    list_Delete(term_ArgumentList(Term));
    return eml_RplacWithQuantifier(Term, eml_FoQuantAssocWithPropSymbol(Top), list_List(term_Create(newVar,list_Nil())),
		    list_List(helpTerm));
    /* exists y tr(alpha)(x,y) /\ tr(phi)(y)  or
     * forall y tr(alpha)(x,y) -> tr(phi)(y)  */
  }
  /* Term = domain(alpha)  or  Term = range(alpha) */
  else if (symbol_Equal(Top,eml_Domain()) || symbol_Equal(Top,eml_Range())) {
    SYMBOL newVar;
    TERM   helpTerm;          
    newVar   = symbol_CreateStandardVariable();
    if (symbol_Equal(Top,eml_Domain())) 
      helpTerm = eml_RelationalTranslationRel(term_FirstArgument(Term),Var,newVar, Flags, Precedence);
    else
      helpTerm = eml_RelationalTranslationRel(term_FirstArgument(Term),newVar,Var, Flags, Precedence);

    list_Delete(term_ArgumentList(Term));
    return eml_RplacWithQuantifier(Term, eml_FoQuantAssocWithPropSymbol(Top), list_List(term_Create(newVar,list_Nil())),
		    list_List(helpTerm));
    /* exists y tr(alpha)(x,y)  or  exists y tr(alpha)(y,x) */
  }
  else {
    fprintf(stderr, "\nNo case for %s in eml_RelationalTranslationProp()\n",
             symbol_Name(Top));
    misc_Error();
    return NULL;
  }

}

static TERM eml_RelationalTranslationRel(TERM Term, SYMBOL VarX, SYMBOL VarY, 
	FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   An eml (relational) formula, two variables (the free
	   variables), a string encoding of the sorts of the arguments
	   and the modal depth.
  RETURNS: Its standard (relational) translation.
  CAUTION: The term is destructively changed.
***************************************************************/
{
  SYMBOL Top;

  Top    = term_TopSymbol(Term);

  /* Term = r */
  if (symbol_IsPredicate(Top)) {
    return eml_RplacPropByFoAtom(Term, eml_REL, 2, list_Cons(term_Create(VarX,list_Nil()),list_List(term_Create(VarY,list_Nil()))), Flags, Precedence);
  }
  /* Term = not(alpha), Term = or(...) or Term = and(...) */
  else  if (eml_IsPropJunctor(Top)) {
    LIST Scan;
    for (Scan=term_ArgumentList(Term); !list_Empty(Scan); Scan=list_Cdr(Scan))
      eml_RelationalTranslationRel(list_Car(Scan),VarX,VarY, Flags, Precedence);
    return Term;
  }
  /* Term = conv(alpha) */
  else  if (symbol_Equal(Top,eml_Conv())) {
    TERM helpTerm;          
    helpTerm = eml_RelationalTranslationRel(term_FirstArgument(Term),VarY,VarX, Flags, Precedence);
    list_Delete(term_ArgumentList(Term));
    Term     = eml_RplacWithOpAndArgs(Term, term_TopSymbol(helpTerm),
		    term_ArgumentList(helpTerm));
    term_Free(helpTerm);
    return Term;
    /* tr(alpha)(y,x) */
  }
  /* Term = comp(...)  or  sum(...)*/
  else if (symbol_Equal(Top,eml_Comp()) || symbol_Equal(Top,eml_Sum())) {
    SYMBOL var, newVar;
    TERM   helpTerm;          
    LIST   Scan, varList;
    var = VarX;
    varList = list_Nil();
    for (Scan=term_ArgumentList(Term); !list_Empty(list_Cdr(Scan)); Scan=list_Cdr(Scan)) {
      newVar   = symbol_CreateStandardVariable();
      varList  = list_Nconc(varList, list_List(term_Create(newVar,list_Nil())));
      eml_RelationalTranslationRel(list_Car(Scan),var,newVar, Flags, Precedence);
      var = newVar;
    }
    eml_RelationalTranslationRel(list_Car(Scan),var,VarY, Flags, Precedence);

    helpTerm = term_Create(eml_FoJunctorAssocWithPropSymbol(Top), term_ArgumentList(Term)); /* Sharing including argument list */
    return eml_RplacWithQuantifier(Term,
		eml_FoQuantAssocWithPropSymbol(Top), varList, list_List(helpTerm));
      /* exists z1,z2 tr(alpha1)(x,z1) /\ tr(alpha2)(z1,z2) /\ tr(alpha3)(z2,y) ...  or
       * forall z1,z2 tr(alpha1)(x,z1) \/ tr(alpha2)(z1,z2) \/ tr(alpha3)(z2,y) ... */
  }
  /* Term = test(phi) */
  else if (symbol_Equal(Top,eml_Test())) {
    TERM helpTerm;          
    helpTerm = eml_RelationalTranslationProp(term_FirstArgument(Term),VarY, Flags, Precedence);
    list_Delete(term_ArgumentList(Term));
    return eml_RplacWithOpAndArgs(Term, fol_And(),
	list_Cons(term_Create(fol_Equality(), 
			      list_Cons(term_Create(VarX,list_Nil()), list_List(term_Create(VarY,list_Nil())))),
	list_List(helpTerm)));
    /* equal(x,y) /\ tr(phi)(y) */
  }
  /* Term = domrestr(alpha,phi)  or  ranrestr(alpha,phi) */
  else if (symbol_Equal(Top,eml_DomRestr()) || symbol_Equal(Top,eml_RanRestr())) {
    TERM helpTerm, helpTermRel;          
    helpTermRel = eml_RelationalTranslationRel(term_FirstArgument(Term),VarX,VarY, Flags, Precedence);
    if (symbol_Equal(Top,eml_DomRestr()))
      helpTerm    = eml_RelationalTranslationProp(term_SecondArgument(Term),VarX, Flags, Precedence);
    else
      helpTerm    = eml_RelationalTranslationProp(term_SecondArgument(Term),VarY, Flags, Precedence);
    list_Delete(term_ArgumentList(Term));
    return eml_RplacWithOpAndArgs(Term, fol_And(),
	     list_Cons(helpTermRel, list_List(helpTerm)));
    /* tr(alpha)(x,y) /\ tr(phi)(x)  or  tr(alpha)(x,y) /\ tr(phi)(y) */
  }
  else {
    fprintf(stderr, "\nNo case for %s in eml_RelationalTranslationRel()\n",
             symbol_Name(Top));
    misc_Error();
    return NULL;
  }

}

static TERM eml_RelationalTranslation(TERM Term, FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   An eml (propositional) formula.
  RETURNS: Its standard (relational) translation.
  CAUTION: The term is destructively changed.
***************************************************************/
{
  TERM newVarX, helpTerm;

  symbol_ResetStandardVarCounter();

  newVarX = term_Create(symbol_CreateStandardVariable(), list_Nil());

  /* Embedding into the relational calculus */
  if (flag_GetFlagIntValue(Flags, flag_EML2REL)) {
    TERM newVarY;

    Term = eml_EmbedInRelCalculusProp(Term);
    if (flag_GetFlagIntValue(Flags, flag_EMLPTRANS)) {
      printf("\n [Rel] \t");fol_PrintDFG(Term);
    }

    newVarY  = term_Create(symbol_CreateStandardVariable(), list_Nil());
    helpTerm = eml_RelationalTranslationRel(Term,term_TopSymbol(newVarX),term_TopSymbol(newVarY), Flags, Precedence);

    if (flag_GetFlagIntValue(Flags, flag_EMLPTRANS))
      printf("\n [RelTr] \t");
    return term_Create(fol_All(), list_Cons(term_Create(fol_Varlist(),list_Cons(newVarX,list_List(newVarY))), list_List(helpTerm)));
    /* forall x y tr(Term,x,y) */
  } 
  /* Relational functional/path-based translation */
  else if (flag_GetFlagIntValue(Flags, flag_EMLFNARY)) {
    char nameSuffix[] = "";

    flag_SetFlagIntValue(Flags, flag_EMLFFSORTS, flag_EMLFFSORTSOFF);  
		/* Don't use explicit sorts!  Important for creation of
		 * new atoms in eml_RplacPropByNaryAtom() */

    helpTerm = eml_RelFunctTranslProp(Term, term_TopSymbol(newVarX), nameSuffix, Flags, Precedence);

    if (flag_GetFlagIntValue(Flags, flag_EMLPTRANS))
      printf("\n [RelFuncTr] \t");
    return term_Create(fol_All(), list_Cons(term_Create(fol_Varlist(),list_List(newVarX)), list_List(helpTerm)));
  }
  /* Standard relational translation */
  else {
    helpTerm = eml_RelationalTranslationProp(Term,term_TopSymbol(newVarX), Flags, Precedence);

    if (flag_GetFlagIntValue(Flags, flag_EMLPTRANS))
      printf("\n [RelTr] \t");
    return term_Create(fol_All(), list_Cons(term_Create(fol_Varlist(),list_List(newVarX)), list_List(helpTerm)));
    /* forall x tr(Term,x) */
  }
}

static TERM eml_FunctTranslItoNaryPredProp(TERM Term, LIST VarList,
		char* SortEncoding, int VarListLength, 
		FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   An eml (propositional) formula, a variable list, a string
           encoding of the sorts of the arguments and the length of the
           list of variables.
  RETURNS: Its functional translation ito n-ary predicates.
  CAUTION: The term is destructively changed.
***************************************************************/
{
  SYMBOL Top;

  Top    = term_TopSymbol(Term);
  
#ifdef DEBUG
  if (VarListLength != list_Length(VarList)) {
    fprintf(stderr, "Arity VarListLength conflict\n");
    misc_Error();
    return NULL;
  }
#endif

  /* Term = p */
  if (symbol_IsPredicate(Top)) {
    VarList = list_CopyWithElement(VarList,(POINTER (*)())term_Copy);
    return eml_RplacPropByNaryAtom(Term, eml_NARYPROP, SortEncoding,
		    VarListLength, VarList, Flags, Precedence);
  }
  /* Term = not(phi), Term = or(...) or Term = and(...) */
  else  if (eml_IsPropJunctor(Top)) {
    LIST Scan;
    for (Scan=term_ArgumentList(Term); !list_Empty(Scan); Scan=list_Cdr(Scan))
      eml_FunctTranslItoNaryPredProp(list_Car(Scan), VarList,
		      SortEncoding, VarListLength, Flags, Precedence);
    return Term;
  }
  /* Term = dia(r,phi)  or  Term = box(r,phi) */
  else if (eml_IsModalOpSymbol(Top)) {
    TERM relSubTerm, newVar, helpTerm, anotherHelpTerm;          
    LIST newVarList;
    char* newSortEncoding;
    int size;

    relSubTerm   = term_FirstArgument(Term);
    newVar     = term_Create(symbol_CreateStandardVariable(), list_Nil());
    newVarList = list_Nconc(list_CopyWithElement(VarList,(POINTER (*)())term_Copy), list_List(newVar));
    
    size = strlen(SortEncoding)+strlen(symbol_Name(term_TopSymbol(relSubTerm)))+1;
    newSortEncoding = memory_Malloc(size*sizeof(char));
    sprintf(newSortEncoding,"%s%s", SortEncoding, symbol_Name(term_TopSymbol(relSubTerm)));
    
    if (flag_GetFlagIntValue(Flags, flag_EMLFUNCNDEQ)) {
      helpTerm = eml_BuildSortQuantifierFor(Term, Top, term_TopSymbol(relSubTerm),
		term_TopSymbol(newVar), flag_GetFlagIntValue(Flags, flag_EMLFFSORTS), 
		list_List(eml_FunctTranslItoNaryPredProp(term_SecondArgument(Term),
			newVarList, newSortEncoding, VarListLength+1, Flags, Precedence)),
		(POINTER (*)())eml_CreateQuantifier, Precedence);
      list_Delete(term_ArgumentList(Term));
      anotherHelpTerm = eml_RplacWithOpAndArgs(Term, eml_FoJunctorAssocWithPropSymbol(Top), 
		list_Cons(eml_FunctTranslItoNaryPredRel(relSubTerm,
			VarList, SortEncoding, VarListLength, Flags, Precedence), 
		list_List(helpTerm)));
      list_DeleteWithElement(newVarList,(void (*)())term_Delete);
      return anotherHelpTerm;
      /* nde_r(vl) /\ exists y_r tr(phi)(vl,y)  or
       * nde_r(vl) -> forall y_r tr(phi)(vl,y)  */
    }
    else {
      TERM relSubTermCopy = term_Copy(relSubTerm);  /* make a copy; important */
      helpTerm = term_Create(eml_FoJunctorAssocWithPropSymbol(Top),
		list_Cons(eml_FunctTranslItoNaryPredRel(relSubTerm,
		    VarList, SortEncoding, VarListLength, Flags, Precedence), 
		  list_List(eml_FunctTranslItoNaryPredProp(term_SecondArgument(Term),
		    newVarList, newSortEncoding, VarListLength+1, Flags, Precedence))));
      list_Delete(term_ArgumentList(Term));
      anotherHelpTerm = eml_BuildSortQuantifierFor(Term, Top, 
	          term_TopSymbol(relSubTermCopy), term_TopSymbol(newVar),
		  flag_GetFlagIntValue(Flags, flag_EMLFFSORTS),
		  list_List(helpTerm), (POINTER (*)())eml_RplacWithQuantifier, Precedence);
      list_DeleteWithElement(newVarList,(void (*)())term_Delete);
      term_Delete(relSubTermCopy);
      return anotherHelpTerm;
      /* exists y_r nde_r(vl) /\ tr(phi)(vl,y)  or
       * forall y_r nde_r(vl) -> tr(phi)(vl,y)  */
    }
    
    memory_Free(newSortEncoding,size*sizeof(char));
  }
  else {
    fprintf(stderr, "\nNo case for %s in eml_FunctTranslItoNaryPredProp()\n",
             symbol_Name(Top));
    misc_Error();
    return NULL;
  }

}

static TERM eml_FunctTranslItoNaryPredRel(TERM Term, LIST VarList,
		char* SortEncoding, int VarListLength, 
		FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   An EML relational formula, a variable list, a string (encoding
           the sorts of the accessibility relations in the scope), and
	   the length of the variable list (corresponds to the modal
	   depth).
  RETURNS: Its functional translation ito n-ary predicates.
  CAUTION: The term is destructively changed.
***************************************************************/
{
  SYMBOL Top;

  Top    = term_TopSymbol(Term);

  /* Term = r */
  if (symbol_IsPredicate(Top)) {
    switch (flag_GetFlagIntValue(Flags, flag_EMLTHEORY)) {
      case flag_EMLTHEORYKD:
        return eml_RplacWithDesignatedPropAtom(Term, fol_True());
        break;
      case flag_EMLTHEORYK:
        VarList = list_CopyWithElement(VarList,(POINTER (*)())term_Copy);
        return eml_RplacPropByNaryAtom(Term, eml_NARYNDE,
  		        SortEncoding, VarListLength, VarList, Flags, Precedence);
        break;
      default:
        fprintf(stderr, "\nThe option EMLTheory = %d is not implemented in eml_FunctTranslItoNaryPredRel()\n",
             flag_GetFlagIntValue(Flags, flag_EMLTHEORY));
        misc_Error();
        return NULL;
        break;
    }
  }
  else {
    fprintf(stderr, "\nNo case for %s in eml_FunctTranslItoNaryPredRel()\n",
             symbol_Name(Top));
    misc_Error();
    return NULL;
  }
}

static TERM eml_FunctTranslProp(TERM Term, TERM WorldPath, 
	FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   An eml (propositional) formula, a variable list and its
           length.
  RETURNS: Its functional translation.
  CAUTION: The term is destructively changed.
  NOTE:    WorldPath needs to be copied before it is "used"
***************************************************************/
{
  SYMBOL Top;

  Top    = term_TopSymbol(Term);

  /* Term = p */
  if (symbol_IsPredicate(Top)) {
    return eml_RplacPropByFoAtom(Term, eml_PROP, 1, list_List(term_Copy(WorldPath)), Flags, Precedence);
  }
  /* Term = not(phi), Term = or(...) or Term = and(...) */
  else  if (eml_IsPropJunctor(Top)) {
    LIST Scan;
    for (Scan=term_ArgumentList(Term); !list_Empty(Scan); Scan=list_Cdr(Scan))
      eml_FunctTranslProp(list_Car(Scan), WorldPath, Flags, Precedence);
    return Term;
  }
  /* Term = dia(alpha,phi)  or  Term = box(alpha,phi) */
  else if (eml_IsModalOpSymbol(Top)) {
    TERM newVar, newWorldPath, relSubTerm, helpTerm, anotherHelpTerm;          
    relSubTerm   = term_FirstArgument(Term);
    newVar       = term_Create(symbol_CreateStandardVariable(), list_Nil());
    newWorldPath = term_Create(eml_ApplyFunct(), list_Cons(term_Copy(WorldPath),
			list_List(newVar)));
    if (flag_GetFlagIntValue(Flags, flag_EMLFUNCNDEQ)) {
      helpTerm = eml_BuildSortQuantifierFor(Term, Top, term_TopSymbol(relSubTerm), 
                term_TopSymbol(newVar), TRUE, 
		list_List(eml_FunctTranslProp(term_SecondArgument(Term),
			newWorldPath, Flags, Precedence)), (POINTER (*)())eml_CreateQuantifier, Precedence);
      /*printf("\n [*]\t"); fol_PrintDFG(helpTerm);*/
      list_Delete(term_ArgumentList(Term)); 
      anotherHelpTerm = eml_RplacWithOpAndArgs(Term, eml_FoJunctorAssocWithPropSymbol(Top), 
		list_Cons(eml_FunctTranslRel(relSubTerm, WorldPath, Flags, Precedence), 
		list_List(helpTerm)));
      /*printf("\n [**]\t"); fol_PrintDFG(anotherHelpTerm);*/
      term_Delete(newWorldPath);
      return anotherHelpTerm;
      /* nde_r(wp) /\ exists y_r tr(phi)(app(wp,y_r))  or
       * nde_r(wp) -> forall y_r tr(phi)(app(wp,y_r))  */
    }
    else {
      helpTerm = term_Create(eml_FoJunctorAssocWithPropSymbol(Top), 
	  list_Cons(eml_FunctTranslRel(relSubTerm, WorldPath, Flags, Precedence),
		    list_List(eml_FunctTranslProp(term_SecondArgument(Term), newWorldPath, Flags, Precedence))));
      list_Delete(term_ArgumentList(Term));
      anotherHelpTerm = eml_BuildSortQuantifierFor(Term, Top, term_TopSymbol(relSubTerm), 
                  term_TopSymbol(newVar), TRUE,
		  list_List(helpTerm), (POINTER (*)())eml_RplacWithQuantifier, Precedence);
      term_Delete(newWorldPath);
      return anotherHelpTerm;
      /* exists y_r nde_r(wp) /\ tr(phi)(app(wp,y_r))  or
       * forall y_r nde_r(wp) -> tr(phi)(app(wp,y_r))  */
    }
  }
  else {
    fprintf(stderr, "\nNo case for %s in eml_FunctTranslProp()\n",
             symbol_Name(Top));
    misc_Error();
    return NULL;
  }

}

static TERM eml_FunctTranslRel(TERM Term, TERM WorldPath, 
	FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   An eml (propositional) formula, a variable list and its
           length.
  RETURNS: Its functional translation.
  CAUTION: The term is destructively changed.
***************************************************************/
{
  SYMBOL Top;

  Top    = term_TopSymbol(Term);

  /* Term = r */
  if (symbol_IsPredicate(Top)) {
    switch (flag_GetFlagIntValue(Flags, flag_EMLTHEORY)) {
      case flag_EMLTHEORYKD:
        return eml_RplacWithDesignatedPropAtom(Term, fol_True());
        break;
      case flag_EMLTHEORYK:
        return eml_RplacPropByFoAtom(Term, eml_NDE, 1, list_List(term_Copy(WorldPath)), Flags, Precedence);
        break;
      default:
        fprintf(stderr, "\nThe option EMLTheory = %d is not implemented in eml_FunctTranslRel()\n",
             flag_GetFlagIntValue(Flags, flag_EMLTHEORY));
        misc_Error();
        return NULL;
        break;
    }
  }
  else {
    fprintf(stderr, "\nNo case for %s in eml_FunctTranslRel()\n",
             symbol_Name(Top));
    misc_Error();
    return NULL;
  }
}


static TERM eml_FunctionalTranslation(TERM Term, BOOL ItoNaryPreds, 
	FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   An eml (propositional) formula and a flag.
  RETURNS: Its functional translation.  If the flag is true the
           translation is into ordered first-order logic or the
           Bernays-Schoenfinkel class, meaning n-ary predicates
	   are generated.
  CAUTION: The term is destructively changed.
***************************************************************/
{
  symbol_ResetStandardVarCounter();

  if (ItoNaryPreds) {
    char nameSuffix[] = "";
    
    if (flag_GetFlagIntValue(Flags, flag_EMLPTRANS))
      printf("\n [FuncFTr] \t");
    Term = eml_FunctTranslItoNaryPredProp(Term, list_Nil(), nameSuffix, 0, Flags, Precedence);
    /* Check whether sharing is possible on the name suffixes,
     * e.g. whether the iterative allocation of new memory in
     * eml_FunctTranslItoNaryPredProp is necessary. */
    return  Term;
    /* note: without world quantifier */
  }
  else {
    TERM newVar, helpTerm;
    if (flag_GetFlagIntValue(Flags, flag_EMLPTRANS))
      printf("\n [FuncTr] \t");
    newVar   = term_Create(symbol_CreateStandardVariable(), list_Nil());
    helpTerm = eml_FunctTranslProp(Term,newVar, Flags, Precedence); 
    /*printf("\n [**]\t"); fol_PrintDFG(helpTerm);*/
    return term_Create(fol_All(), list_Cons(term_Create(fol_Varlist(),list_List(newVar)), list_List(helpTerm)));
    /* forall x tr(Term,x) */
  }

}

static TERM eml_SemiFunctTranslProp(TERM Term, TERM WorldTerm, int Polarity, 
	FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   An eml (propositional) formula, term of type world, and the
           polarity of the formula (Term) in the original formula
           length.
  RETURNS: Its functional translation.
  CAUTION: The term is destructively changed.
***************************************************************/
{
  SYMBOL Top;

  Top    = term_TopSymbol(Term);

  /* Term = p */
  if (symbol_IsPredicate(Top)) {
    return eml_RplacPropByFoAtom(Term, eml_PROP, 1, list_List(term_Copy(WorldTerm)), Flags, Precedence);
  }
  /* Term = not(phi) */
  else  if (eml_IsNotSymbol(Top)) {
      eml_SemiFunctTranslProp(term_FirstArgument(Term),
                      WorldTerm, Polarity * -1, Flags, Precedence);
    return Term;
  }
  /* Term = implies(...) */
  else  if (eml_IsImpliesSymbol(Top)) { 
    eml_SemiFunctTranslProp(term_FirstArgument(Term), WorldTerm, Polarity * -1, Flags, Precedence);
    eml_SemiFunctTranslProp(term_SecondArgument(Term), WorldTerm, Polarity, Flags, Precedence);
    return Term;
  }
  /* Term = implied(...) */
  else  if (eml_IsImpliedSymbol(Top)) { 
    eml_SemiFunctTranslProp(term_SecondArgument(Term), WorldTerm, Polarity * -1, Flags, Precedence);
    eml_SemiFunctTranslProp(term_FirstArgument(Term), WorldTerm, Polarity, Flags, Precedence);
    return Term;
  }
  /* Term = equiv(...) */
  else  if (eml_IsEquivSymbol(Top)) {
    TERM termL, termR, termLneg, termRneg;
    termL    = term_FirstArgument(Term);
    termR    = term_SecondArgument(Term);
    termLneg = term_Create(fol_Not(), list_List(term_Copy(termL)));
    termRneg = term_Create(fol_Not(), list_List(term_Copy(termR)));
    if ( Polarity == 1 ) {
      term_RplacTop(Term, fol_And());
      list_Rplaca(term_ArgumentList(Term), 
                  term_Create(fol_Or(), list_Cons(termLneg, list_List(termR))));
      list_RplacSecond(term_ArgumentList(Term),
                      term_Create(fol_Or(), list_Cons(termRneg, list_List(termL))));
    } 
    else if (Polarity == -1) {
      term_RplacTop(Term, fol_Or());
      list_Rplaca(term_ArgumentList(Term),
                  term_Create(fol_And(), list_Cons(termLneg, list_List(termRneg))));
      list_RplacSecond(term_ArgumentList(Term),
                  term_Create(fol_And(), list_Cons(termL, list_List(termR))));
    }
  eml_SemiFunctTranslProp(Term, WorldTerm, Polarity, Flags, Precedence);
  return Term;
  }
  /* Term = or(...), Term = and(...) */
  /* must be tested after eml_IsNotSymbol, eml_IsImpliesSymbol amd eml_IsEquivSymbol */
  else  if (eml_IsPropJunctor(Top)) {
    LIST Scan;
    for (Scan = term_ArgumentList(Term); 
         !list_Empty(Scan); 
         Scan = list_Cdr(Scan))
      eml_SemiFunctTranslProp(list_Car(Scan), WorldTerm, Polarity, Flags, Precedence);
    return Term;
  }
  else if (eml_IsModalOpSymbol(Top)) {
    /* Term = dia(alpha,phi), in the negation normal form (!) */
    if ( ( eml_IsDiaSymbol(Top) && (Polarity == 1) ) || 
         ( eml_IsBoxSymbol(Top) && (Polarity == -1) ) ) {
      TERM newVar, newWorldTerm, relSubTerm, helpTerm, anotherHelpTerm;
      relSubTerm   = term_FirstArgument(Term);
      newVar       = term_Create(symbol_CreateStandardVariable(), list_Nil());
      newWorldTerm = term_Create(eml_ApplyFunct(), 
                        list_Cons(term_Copy(WorldTerm), list_List(newVar)));
      if (flag_GetFlagIntValue(Flags, flag_EMLFUNCNDEQ)) {
        helpTerm = eml_BuildSortQuantifierFor(Term, Top, term_TopSymbol(relSubTerm), term_TopSymbol(newVar), TRUE, 
		list_List(eml_SemiFunctTranslProp(term_SecondArgument(Term),
			newWorldTerm, Polarity, Flags, Precedence)), 
                (POINTER (*)())eml_CreateQuantifier, Precedence);
        list_Delete(term_ArgumentList(Term)); 
        anotherHelpTerm = eml_RplacWithOpAndArgs(Term, eml_FoJunctorAssocWithPropSymbol(Top), 
		list_Cons(eml_SemiFunctTranslRelDia(relSubTerm, WorldTerm, Flags, Precedence), 
		list_List(helpTerm)));
        term_Delete(newWorldTerm);
        return anotherHelpTerm;
        /* nde_r(w) /\ exists y_r tr(phi)(app(w,y_r))  */
      }
      else {
        helpTerm = term_Create(eml_FoJunctorAssocWithPropSymbol(Top), 
	    list_Cons(eml_SemiFunctTranslRelDia(relSubTerm, WorldTerm, Flags, Precedence),
            list_List(eml_SemiFunctTranslProp(term_SecondArgument(Term), newWorldTerm, Polarity, Flags, Precedence))));
        list_Delete(term_ArgumentList(Term));
        anotherHelpTerm = eml_BuildSortQuantifierFor(Term, Top, term_TopSymbol(relSubTerm), 
		  term_TopSymbol(newVar), TRUE,
		  list_List(helpTerm), (POINTER (*)())eml_RplacWithQuantifier, Precedence);
        term_Delete(newWorldTerm);
        return anotherHelpTerm;
        /* exists y_r nde_r(w) /\ tr(phi)(app(w,y_r))  */
      }
    }
    /* Term = box(alpha,phi), in the negation normal form (!) */
    else {
      TERM newVar, helpTerm;          
      newVar   = term_Create(symbol_CreateStandardVariable(), list_Nil());
      helpTerm = term_Create(eml_FoJunctorAssocWithPropSymbol(Top), 
          list_Cons(eml_SemiFunctTranslRelBox(term_FirstArgument(Term), WorldTerm, newVar, Flags, Precedence),
          list_List(eml_SemiFunctTranslProp(term_SecondArgument(Term), newVar, Polarity, Flags, Precedence))));
      list_Delete(term_ArgumentList(Term));
      return eml_RplacWithQuantifier(Term, eml_FoQuantAssocWithPropSymbol(Top), list_List(newVar),
		    list_List(helpTerm));
      /* forall y tr(alpha)(w,y) -> tr(phi)(y)  */
    }
  }
  else {
    fprintf(stderr, "\nNo case for %s in eml_SemiFunctTranslProp()\n",
             symbol_Name(Top));
    misc_Error();
    return NULL;
  }

}


static TERM eml_SemiFunctTranslRelDia(TERM Term, TERM WorldTerm, 
	FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   An eml (propositional) formula, argument term.
  RETURNS: Its semi functional translation.
  CAUTION: The term is destructively changed.
***************************************************************/
{
  SYMBOL Top;

  Top    = term_TopSymbol(Term);

  /* Term = r */
  if (symbol_IsPredicate(Top)) {
    switch (flag_GetFlagIntValue(Flags, flag_EMLTHEORY)) {
      case flag_EMLTHEORYKD:
        return eml_RplacWithDesignatedPropAtom(Term, fol_True());
        break;
      case flag_EMLTHEORYK:
        return eml_RplacPropByFoAtom(Term, eml_SEMIFNDE, 1, list_List(term_Copy(WorldTerm)), Flags, Precedence);
        break;
      default:
        fprintf(stderr, "\nThe option EMLTheory = %d is not implemented in eml_SemiFunctTranslRel()\n",
             flag_GetFlagIntValue(Flags, flag_EMLTHEORY));
        misc_Error();
        return NULL;
        break;
    }
  }
  else {
    fprintf(stderr, "\nNo case for %s in eml_SemiFunctTranslRel()\n",
             symbol_Name(Top));
    misc_Error();
    return NULL;
  }
}

static TERM eml_SemiFunctTranslRelBox(TERM Term, TERM WorldTerm, TERM VarY, 
	FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   An eml (relational) formula, a term and a variable 
           (the free variable).
  RETURNS: Its standard (relational) translation.
  CAUTION: The term is destructively changed.
***************************************************************/
{
  SYMBOL Top;

  Top    = term_TopSymbol(Term);

  /* Term = r */
  if (symbol_IsPredicate(Top)) {
    return eml_RplacPropByFoAtom(Term, eml_REL, 2,
                    list_Cons(term_Copy(WorldTerm), list_List(term_Copy(VarY))), Flags, Precedence);
  }
  else {
    fprintf(stderr, "\nNo case for %s in eml_SemiFunctTranslRelBox()\n",
             symbol_Name(Top));
    misc_Error();
    return NULL;
  }

}



static TERM eml_SemiFunctionalTranslation(TERM Term, int Polarity, 
	FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   An eml (propositional) formula.
  RETURNS: Its semi functional translation.  
  CAUTION: The term is destructively changed.
***************************************************************/
{
  TERM newVar, helpTerm;

  symbol_ResetStandardVarCounter();

  if (flag_GetFlagIntValue(Flags, flag_EMLPTRANS))
    printf("\n [SemiFuncTr] \t");
  newVar   = term_Create(symbol_CreateStandardVariable(), list_Nil());
  helpTerm = eml_SemiFunctTranslProp(Term, newVar, Polarity, Flags, Precedence);
    return term_Create(fol_All(), list_Cons(term_Create(fol_Varlist(),list_List(newVar)), list_List(helpTerm)));
    /* forall x tr(Term,x) */

}



static TERM eml_RelFunctTranslProp(TERM Term, SYMBOL Var, 
		char* SortEncoding, FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   An eml (propositional) formula, a variable, a string
           encoding of the sorts of the arguments and the modal depth.
  RETURNS: Its translation using the relational functional/path-based
           encoding of de Rijke et al.
  CAUTION: The term is destructively changed.
***************************************************************/
{
  SYMBOL Top;

  Top    = term_TopSymbol(Term);
  
  /* Term = p */
  if (symbol_IsPredicate(Top)) {
    return eml_RplacPropByNaryAtom(Term, eml_PROP, SortEncoding,
		    1, list_List(term_Create(Var,list_Nil())), Flags, Precedence);
  }
  /* Term = not(phi), Term = or(...) or Term = and(...) */
  else  if (eml_IsPropJunctor(Top)) {
    LIST Scan;
    for (Scan=term_ArgumentList(Term); !list_Empty(Scan); Scan=list_Cdr(Scan))
      eml_RelFunctTranslProp(list_Car(Scan), Var,
		      SortEncoding, Flags, Precedence);
    return Term;
  }
  /* Term = dia(r,phi)  or  Term = box(r,phi) */
  else if (eml_IsModalOpSymbol(Top)) {
    SYMBOL newVar;
    TERM   helpTerm;          
    SYMBOL relSubTop;
    char* newSortEncoding;
    int size;
    newVar   = symbol_CreateStandardVariable();

    relSubTop = term_TopSymbol(term_FirstArgument(Term));
    size = strlen(SortEncoding)+strlen(symbol_Name(relSubTop))+1;
    newSortEncoding = (char*)memory_Malloc(size*sizeof(char));
    sprintf(newSortEncoding,"%s%s", SortEncoding, symbol_Name(relSubTop));
    helpTerm = term_Create(eml_FoJunctorAssocWithPropSymbol(Top), 
        list_Cons(eml_RelFunctTranslRel(term_FirstArgument(Term),Var,newVar,newSortEncoding, Flags, Precedence),
        list_List(eml_RelFunctTranslProp(term_SecondArgument(Term),newVar,newSortEncoding, Flags, Precedence))));
    list_Delete(term_ArgumentList(Term));    
    memory_Free(newSortEncoding,size*sizeof(char));
    return eml_RplacWithQuantifier(Term, eml_FoQuantAssocWithPropSymbol(Top), list_List(term_Create(newVar,list_Nil())),
		    list_List(helpTerm));
    /* exists y R_r(x,y) /\ tr(phi)(y)  or
     * forall y R_r(x,y) -> tr(phi)(y)  */
  }
  else {
    fprintf(stderr, "\nNo case for %s in eml_RelFunctTranslProp()\n",
             symbol_Name(Top));
    misc_Error();
    return NULL;
  }

}

static TERM eml_RelFunctTranslRel(TERM Term, SYMBOL VarX, SYMBOL VarY,
		char* SortEncoding, FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   An EML relational formula, a variable list, a string (encoding
           the sorts of the accessibility relations in the scope), and
	   the length of the variable list (corresponds to the modal
	   depth).
  RETURNS: Its functional translation ito n-ary predicates.
  CAUTION: The term is destructively changed.
***************************************************************/
{
  SYMBOL Top;

  Top    = term_TopSymbol(Term);

  /* Term = r */
  if (symbol_IsPredicate(Top)) {
    switch (flag_GetFlagIntValue(Flags, flag_EMLTHEORY)) {
      case flag_EMLTHEORYK:
        return eml_RplacPropByNaryAtom(Term, eml_REL,
  		        SortEncoding, 2,  list_Cons(term_Create(VarX,list_Nil()),list_List(term_Create(VarY,list_Nil()))), Flags, Precedence);
        break;
      default:
        fprintf(stderr, "\nThe option EMLTheory = %d is not implemented in eml_RelFunctTranslRel()\n",
             flag_GetFlagIntValue(Flags, flag_EMLTHEORY));
        misc_Error();
        return NULL;
        break;
    }
  }
  else {
    fprintf(stderr, "\nNo case for %s in eml_RelFunctTranslRel()\n",
             symbol_Name(Top));
    misc_Error();
    return NULL;
  }
}




static TERM  eml_TranslateToFol(TERM Term, int Polarity, 
	FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   An eml formula.
  RETURNS: Its translation as specified by flag_EMLTR, provided
           the formula is provided, otherwise the simplification of the
           original formula.
  CAUTION: The term is destructively changed.
***************************************************************/
{
  if (!eml_IsPropositional(Term))
    fol_NormalizeVars(Term);

  Term = eml_ObviousSimplifications(Term, Flags);
  /* Term = eml_MakeOneAndOrCompSum(Term); */
  if (flag_GetFlagIntValue(Flags, flag_EMLPTRANS)) {
    printf("\n [Simpl]\t"); 
    if ( (Polarity == -1) ) {   /* Conjecture */
      symbol_Print(fol_Not());
      putchar('(');
      /* printf("(not "); */
      /* putchar(' ');*/
      fol_PrintDFG(Term);
      printf(")");
    }
    else                        /* Axiom */
      fol_PrintDFG(Term);
  }

  if (eml_IsPropositional(Term)) {
    switch (flag_GetFlagIntValue(Flags, flag_EMLTR)) {
      case flag_EMLTRREL:
        Term = eml_RelationalTranslation(Term, Flags, Precedence);
        break;
      case flag_EMLTRFUNC:
      case flag_EMLTROPTFUNC:
        Term = eml_FunctionalTranslation(Term, flag_GetFlagIntValue(Flags, flag_EMLFNARY), Flags, Precedence);
        break;
      case flag_EMLTRSEMIFUNC:
        Term = eml_SemiFunctionalTranslation(Term, Polarity, Flags, Precedence);
        break;
      default:
        fprintf(stderr, "\nFlag = %d is not implemented in eml_TranslateToFol()\n",
             flag_GetFlagIntValue(Flags, flag_EMLTR));
        misc_Error();
        break;
    }
  }
  else
    if (flag_GetFlagIntValue(Flags, flag_EMLPTRANS))
      printf("\n [FOL]\t");

  return Term;
}

static TERM eml_CreateTheoryTerm(int Flag, SYMBOL Symbol)
/**************************************************************
  INPUT:   A flag, must be of a logic determined by one axiom schema
  RETURNS: A single theory formula, e.g. for the flag flag_EMLTHEORYKD it
           returns  forall x exists y R(x,y)
  REMARK:  Prototypical.  Only implemented for the relational
           translation and a few axiom schemas (D, T, B, 4, 5)
***************************************************************/
{   
  TERM newVarX, newVarY, newVarZ, helpTerm;          
  TERM term;                   /* theory formula */
  
  switch (Flag) {

    case flag_EMLTHEORYKD:
      /*printf("\nD");*/
      newVarX  = term_Create(symbol_CreateStandardVariable(), list_Nil());
      newVarY  = term_Create(symbol_CreateStandardVariable(), list_Nil());
      term     = term_Create(Symbol,        /* R(x,y) */
                      list_Nconc(list_List(newVarX), list_List(newVarY)));
      /*printf("\n [*]\t"); fol_PrintDFG(term);*/
      helpTerm = term_Create(fol_Exist(),   /* exists y R(x,y) */
    		 list_Cons(term_Create(fol_Varlist(), list_List(term_Copy(newVarY))), 
                               list_List(term)));
      /*printf("\n [**]\t"); fol_PrintDFG(helpTerm);*/
      term     = term_Create(fol_All(),     /* forall x exists y R(x,y) */
    		     list_Cons(term_Create(fol_Varlist(), list_List(term_Copy(newVarX))), 
                           list_List(helpTerm) ));
      /*printf("\n [***]\t"); fol_PrintDFG(term);*/
      return term;
      break;

    case flag_EMLTHEORYKT:
      /*printf("\nT");*/
      newVarX  = term_Create(symbol_CreateStandardVariable(), list_Nil());
      helpTerm = term_Create(Symbol,        /* R(x,x) */
                      list_Nconc(list_List(newVarX), list_List(term_Copy(newVarX))));
      /*printf("\n [*]\t"); fol_PrintDFG(helpTerm);*/
      term     = term_Create(fol_All(),     /* forall x R(x,x) */
    		     list_Cons(term_Create(fol_Varlist(), list_List(term_Copy(newVarX))), 
                           list_List(helpTerm) ));
      return term;
      break;

    case flag_EMLTHEORYKB:
      /*printf("\nB");*/
      newVarX  = term_Create(symbol_CreateStandardVariable(), list_Nil());
      newVarY  = term_Create(symbol_CreateStandardVariable(), list_Nil());
      term     = term_Create(Symbol,        /* R(x,y) */
                      list_Nconc(list_List(newVarX), list_List(newVarY)));
      /*printf("\n [*]\t"); fol_PrintDFG(term);*/
      helpTerm     = term_Create(Symbol,        /* R(y,x) */
                      list_Nconc(list_List(term_Copy(newVarY)), list_List(term_Copy(newVarX))));
      /*printf("\n [**]\t"); fol_PrintDFG(helpTerm);*/
      helpTerm = term_Create(fol_Implies(),   /* R(x,y) -> R(y,x) */
    		 list_Cons(term, list_List(helpTerm)));
      /*printf("\n [***]\t"); fol_PrintDFG(helpTerm);*/
      term     = term_Create(fol_All(),     /* forall x y R(x,y) -> R(y,x) */
    		     list_Cons(term_Create(fol_Varlist(), 
                             list_Cons(term_Copy(newVarX), list_List(term_Copy(newVarY)))), 
                       list_List(helpTerm) ));
      /*printf("\n [****]\t"); fol_PrintDFG(term);*/
      return term;
      break;

    case flag_EMLTHEORYK4:
      /*printf("\n4");*/
      newVarX  = term_Create(symbol_CreateStandardVariable(), list_Nil());
      newVarY  = term_Create(symbol_CreateStandardVariable(), list_Nil());
      newVarZ  = term_Create(symbol_CreateStandardVariable(), list_Nil());
      term     = term_Create(Symbol,        /* R(x,y) */
                      list_Nconc(list_List(newVarX), list_List(newVarY)));
      /*printf("\n [*]\t"); fol_PrintDFG(term);*/
      helpTerm     = term_Create(Symbol,        /* R(y,z) */
                      list_Nconc(list_List(term_Copy(newVarY)), list_List(newVarZ)));
      /*printf("\n [**]\t"); fol_PrintDFG(helpTerm);*/
      helpTerm = term_Create(fol_And(),   /* R(x,y) /\ R(y,z) */
    		 list_Cons(term, list_List(helpTerm)));
      /*printf("\n [***]\t"); fol_PrintDFG(helpTerm);*/
      term     = term_Create(Symbol,        /* R(x,z) */
                      list_Nconc(list_List(term_Copy(newVarX)), list_List(term_Copy(newVarZ))));
      /*printf("\n [****]\t"); fol_PrintDFG(term);*/
      helpTerm = term_Create(fol_Implies(),   /* (R(x,y) /\ R(y,z)) -> R(x,z) */
    		 list_Cons(helpTerm, list_List(term)));
      /*printf("\n [*****]\t"); fol_PrintDFG(helpTerm);*/
      term     = term_Create(fol_All(),     /* forall x y z (R(x,y) /\ R(y,z)) -> R(x,z) */
    		     list_Cons(term_Create(fol_Varlist(), 
                             list_Cons(term_Copy(newVarX),
                                     list_Cons(term_Copy(newVarY),
                                             list_List(term_Copy(newVarZ))))), 
                       list_List(helpTerm) ));
      /*printf("\n [******]\t"); fol_PrintDFG(term);*/
      return term;
      break;

    case flag_EMLTHEORYK5:
      /*printf("\n5");*/
      newVarX  = term_Create(symbol_CreateStandardVariable(), list_Nil());
      newVarY  = term_Create(symbol_CreateStandardVariable(), list_Nil());
      newVarZ  = term_Create(symbol_CreateStandardVariable(), list_Nil());
      term     = term_Create(Symbol,        /* R(x,y) */
                      list_Nconc(list_List(newVarX), list_List(newVarY)));
      /*printf("\n [*]\t"); fol_PrintDFG(term);*/
      helpTerm     = term_Create(Symbol,        /* R(x,z) */
                      list_Nconc(list_List(term_Copy(newVarX)), list_List(newVarZ)));
      /*printf("\n [**]\t"); fol_PrintDFG(helpTerm);*/
      helpTerm = term_Create(fol_And(),   /* R(x,y) /\ R(x,z) */
    		 list_Cons(term, list_List(helpTerm)));
      /*printf("\n [***]\t"); fol_PrintDFG(helpTerm);*/
      term     = term_Create(Symbol,        /* R(y,z) */
                      list_Nconc(list_List(term_Copy(newVarY)), list_List(term_Copy(newVarZ))));
      /*printf("\n [****]\t"); fol_PrintDFG(term);*/
      helpTerm = term_Create(fol_Implies(),   /* (R(x,y) /\ R(x,z)) -> R(y,z) */
    		 list_Cons(helpTerm, list_List(term)));
      /*printf("\n [*****]\t"); fol_PrintDFG(helpTerm);*/
      term     = term_Create(fol_All(),     /* forall x y z (R(x,y) /\ R(x,z)) -> R(y,z) */
    		     list_Cons(term_Create(fol_Varlist(), 
                             list_Cons(term_Copy(newVarX),
                                     list_Cons(term_Copy(newVarY),
                                             list_List(term_Copy(newVarZ))))), 
                       list_List(helpTerm) ));
      /*printf("\n [******]\t"); fol_PrintDFG(term);*/
      return term;
      break;

    default:
      fprintf(stderr, "\nFlag = %d is not implemented in eml_CreateTheoryTerm()\n",
           Flag);
      misc_Error();
      return NULL;
  }
}

static void eml_AddBackgroundTheory(LIST* Axiomlist, FLAGSTORE Flags)
/**************************************************************
  INPUT:   The list of axioms
  RETURNS: Nothing
  EFFECT:  Additional axioms may be added to the list of axioms.
  REMARK:  Prototypical.  Only implemented for the relational
           translation and a few logics (K, KD, KT, KB, K4, K5, S4, S5 )
***************************************************************/
{   
  int i, flag;
  char L[100];
  char* label;
  SYMBOL symbol;
  LIST pair   = list_Nil();    /* theory formula plus label */
  LIST theory = list_Nil();    /* list of theory formulae */

          /*printf("\n Beginning of eml_AddBackgroundTheory()\t");*/

  if ( (flag_GetFlagIntValue(Flags, flag_EMLTHEORY) != flag_EMLTHEORYK) && 
       (flag_GetFlagIntValue(Flags, flag_EMLTR) == flag_EMLTRREL) ) {
    
    for (i = 1; i < symbol_ActIndex(); i++) {
      if (!symbol_IsFreed(i)) {
        symbol = symbol_Signature(i)->info;
        /* printf("\n []\t"); symbol_Print(symbol); */
        if ( symbol_IsPredicate(symbol) &&
             (symbol_Signature(i)->arity == 2) &&
             !symbol_Equal(symbol, fol_Equality())) {
          flag = flag_GetFlagIntValue(Flags, flag_EMLTHEORY);
          switch (flag) {

            case flag_EMLTHEORYKD:
            case flag_EMLTHEORYKT:
            case flag_EMLTHEORYKB:
            case flag_EMLTHEORYK4:
            case flag_EMLTHEORYK5:
              sprintf(L,"%s%s", eml_RELPROPERTYLABELS[flag], symbol_Name(symbol));
	      label = memory_Malloc(sizeof(char) * (strlen(L) + 1));
	      sprintf(label, "%s", L);
              pair     = list_PairCreate(label, eml_CreateTheoryTerm(flag, symbol));
    /*printf("\nLabel : %s", (char*) list_PairFirst(pair)); */
    /*printf("\nTheory formula : "); fol_PrintDFG(list_PairSecond(pair));*/
              theory   = list_Nconc(theory, list_List(pair));
              break;

            case flag_EMLTHEORYS4:
              /* T / reflexivity */
              sprintf(L,"%s%s", eml_RELPROPERTYLABELS[flag_EMLTHEORYKT], symbol_Name(symbol));
	      label = memory_Malloc(sizeof(char) * (strlen(L) + 1));
	      sprintf(label, "%s", L);
              pair     = list_PairCreate(label, eml_CreateTheoryTerm(flag_EMLTHEORYKT, symbol));
    /*printf("\nLabel : %s", (char*) list_PairFirst(pair)); */
    /*printf("\nTheory formula : "); fol_PrintDFG(list_PairSecond(pair));*/
              theory   = list_Nconc(theory, list_List(pair));
              /* 4 / transitivity */
              sprintf(L,"%s%s", eml_RELPROPERTYLABELS[flag_EMLTHEORYK4], symbol_Name(symbol));
	      label = memory_Malloc(sizeof(char) * (strlen(L) + 1));
	      sprintf(label, "%s", L);
              pair     = list_PairCreate(label, eml_CreateTheoryTerm(flag_EMLTHEORYK4, symbol));
    /*printf("\nLabel : %s", (char*) list_PairFirst(pair)); */
    /*printf("\nTheory formula : "); fol_PrintDFG(list_PairSecond(pair));*/
              theory   = list_Nconc(theory, list_List(pair));
              break;

            case flag_EMLTHEORYS5:
              /* T / reflexivity */
              sprintf(L,"%s%s", eml_RELPROPERTYLABELS[flag_EMLTHEORYKT], symbol_Name(symbol));
	      label = memory_Malloc(sizeof(char) * (strlen(L) + 1));
	      sprintf(label, "%s", L);
              pair     = list_PairCreate(label, eml_CreateTheoryTerm(flag_EMLTHEORYKT, symbol));
    /*printf("\nLabel : %s", (char*) list_PairFirst(pair)); */
    /*printf("\nTheory formula : "); fol_PrintDFG(list_PairSecond(pair));*/
              theory   = list_Nconc(theory, list_List(pair));
              /* B / symmetry */
              sprintf(L,"%s%s", eml_RELPROPERTYLABELS[flag_EMLTHEORYKB], symbol_Name(symbol));
	      label = memory_Malloc(sizeof(char) * (strlen(L) + 1));
	      sprintf(label, "%s", L);
              pair     = list_PairCreate(label, eml_CreateTheoryTerm(flag_EMLTHEORYKB, symbol));
    /*printf("\nLabel : %s", (char*) list_PairFirst(pair)); */
    /*printf("\nTheory formula : "); fol_PrintDFG(list_PairSecond(pair));*/
              theory   = list_Nconc(theory, list_List(pair));
              /* 4 / transitivity */
              sprintf(L,"%s%s", eml_RELPROPERTYLABELS[flag_EMLTHEORYK4], symbol_Name(symbol));
	      label = memory_Malloc(sizeof(char) * (strlen(L) + 1));
	      sprintf(label, "%s", L);
              pair     = list_PairCreate(label, eml_CreateTheoryTerm(flag_EMLTHEORYK4, symbol));
    /*printf("\nLabel : %s", (char*) list_PairFirst(pair)); */
    /*printf("\nTheory formula : "); fol_PrintDFG(list_PairSecond(pair));*/
              theory   = list_Nconc(theory, list_List(pair));
              break;
            default:
              fprintf(stderr, "\nThe option EMLTheory = %d is not implemented in eml_AddBackgroundTheory()\n",
                   flag_GetFlagIntValue(Flags, flag_EMLTHEORY));
              misc_Error();
              break;
          }
#ifdef DEBUG
            if (list_Empty(theory)) {
              fprintf(stderr, "In eml_AddBackgroundTheory(): theory is empty\n");
              misc_Error();
            } 
            if (list_Empty(*Axiomlist)) {
              fprintf(stderr, "In eml_AddBackgroundTheory(): *Axiomlist is empty\n");
              misc_Error();
            } 
#endif
        }
      } 
    }
    *Axiomlist = list_Nconc(theory, *Axiomlist);
  }
}


void eml_TranslateToFolMain(LIST* AxiomList, LIST* ConjectureList, 
                 BOOL  KeepProofSearch, FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A list of axiom formulae,
           a list of conjecture formulae,
           a hasharray in which for every clause the list of labels 
           of the terms used for deriving the clause is stored (if DocProof 
           is set),
	   a flag store
  RETURNS: If KeepProofSearch is TRUE, then the ProofSearch object is not
           freed but returned.
           Else, NULL is returned.
***************************************************************/
{
  LIST  Scan, Pair;
  TERM  helpTerm;
  int	flag_emltr;

  /* NOTE: EMLAUTO mode not as yet implemented */
  
  /* Print warnings for non-sensical flag combinations */
  flag_emltr = flag_GetFlagIntValue(Flags, flag_EMLTR);
  if (flag_GetFlagIntValue(Flags, flag_DOCPROOF) || flag_GetFlagIntValue(Flags, flag_EMLPTRANS)) {
	  flag_CheckEmlTranslationFlags(Flags, flag_emltr);
  }

  /* Set additional flags */
  if (flag_emltr == flag_EMLTROPTFUNC) {
    flag_SetFlagIntValue(Flags, flag_CNFQUANTEXCH, flag_CNFQUANTEXCHON);
  }
  /* This caused a memory leak because of the allocation done in
   * cnf_Init; fixed with warnings
   * if (flag_GetFlagIntValue(Flags, flag_CNFQUANTEXCH)) {      
    flag_SetFlagIntValue(Flags, flag_CNFOPTSKOLEM, flag_CNFOPTSKOLEMOFF);
    flag_SetFlagIntValue(Flags, flag_CNFSTRSKOLEM, flag_CNFSTRSKOLEMOFF);
  } */
  /*
  if ((flag_emltr == flag_EMLTRFUNC || flag_emltr == flag_EMLTROPTFUNC) &&
      flag_GetFlagIntValue(Flags, flag_EMLFFSORTS)) {
    flag_SetFlagIntValue(Flags, flag_SORTS, flag_SORTSMONADICWITHVARIABLE); 
  } */
  /* RS: Check with Christoph?? */

  /* Limitations of current implementation */
  Scan = *AxiomList;
  if ( !list_Empty(Scan) && !list_Empty(list_Car(Scan)) ) {
    if ( flag_emltr == flag_EMLTROPTFUNC ) {
        fprintf(stderr, "ERROR: With the current implementation of the translation routines\n");
        fprintf(stderr, "       no axioms are allowed when the optimised functional translation\n");
        fprintf(stderr, "       is used (EMLTranslation=2)\n");
        misc_Error();
      }
    else {
      if ( flag_GetFlagIntValue(Flags, flag_EMLFNARY) ) {
        fprintf(stderr, "ERROR: With the current implementation of the translation routines\n");
        fprintf(stderr, "       no axioms are allowed when the encoding is in terms of\n");
        fprintf(stderr, "       n-ary predicates (EMLFuncNary=1)\n");
        misc_Error();
      }
    }
  }
  
  if (flag_GetFlagIntValue(Flags, flag_EMLPTRANS)) {
    printf("\n--------------------EML-TRANSLATION-START-------------------------");
    printf("\n Special   : "); 
    flag_PrintEmlTranslationMethod(flag_emltr);
    flag_PrintEmlTranslationFlags(Flags);
    printf("\n Precedence: "); symbol_PrintPrecedence(Precedence);
    printf("\nAxioms:");
  }

  /* Initialisation */
  switch (flag_GetFlagIntValue(Flags, flag_EMLTR)) {
    case flag_EMLTRFUNC:
    case flag_EMLTROPTFUNC:
    case flag_EMLTRSEMIFUNC:
      eml_InitFunctTransl(Flags, Precedence);
      break;
    default:
      /* Do nothing */
      break;
  }
      
  for(Scan=*AxiomList; !list_Empty(Scan); Scan = list_Cdr(Scan)) {
    Pair = list_Car(Scan);
    helpTerm = (TERM) list_PairSecond(Pair);
    if (flag_GetFlagIntValue(Flags, flag_EMLPTRANS)) {
      if ( !list_Empty(list_PairFirst(Pair)) ) {
        printf("\nLabel : %s", (char*) list_PairFirst(Pair)); 
      }
      if (eml_IsPropositional(helpTerm))
        printf("\n [%s]\t", flag_Name(flag_EML));
      else
        printf("\n [FOL]\t");
      fol_PrintDFG(helpTerm);
    }
    helpTerm = eml_TranslateToFol(helpTerm, 1, Flags, Precedence);         
                                  /* positive polarity */
    if (flag_GetFlagIntValue(Flags, flag_EMLPTRANS)) {
      fol_PrintDFG(helpTerm);
      printf("\n");
    }
    list_Rplacd(Pair, (LIST) helpTerm);
  }

  if (flag_GetFlagIntValue(Flags, flag_EMLPTRANS))
    printf("\n\nConjecture:");

  for(Scan=*ConjectureList; !list_Empty(Scan); Scan = list_Cdr(Scan)) {
    Pair = list_Car(Scan);
    helpTerm = (TERM) list_PairSecond(Pair);
    if (flag_GetFlagIntValue(Flags, flag_EMLPTRANS)) {
      if ( !list_Empty(list_PairFirst(Pair)) ) {
        printf("\nLabel : %s", (char*) list_PairFirst(Pair)); 
      }
      if (eml_IsPropositional(helpTerm))
        printf("\n [%s]\t", flag_Name(flag_EML));
      else
        printf("\n [FOL]\t");
      fol_PrintDFG(helpTerm);
    }
#ifdef DEBUG
    if (!symbol_Equal(term_TopSymbol(helpTerm),fol_Not())) {
      fprintf(stderr, "In eml_TranslateToFolMain(): The list of conjectures should have been negated\n");
      misc_Error();
    } 
#endif

    term_RplacFirstArgument(helpTerm,
			    eml_TranslateToFol(term_FirstArgument(helpTerm), -1, Flags, Precedence));                
                                                /* negative polarity */
    if (flag_GetFlagIntValue(Flags, flag_EMLPTRANS)) {
      fol_PrintDFG(helpTerm);
    }
    list_Rplacd(Pair, (LIST) helpTerm);
  }
  if (flag_GetFlagIntValue(Flags, flag_EMLPTRANS))
      printf("\n");
  
  eml_AddBackgroundTheory(AxiomList, Flags);

  /* Check whether the following does the job (as it is supposed to).
   * Deleting the symbol list causes a segmentation fault later on in cnf.c.
   * Is this possbily due to a missing
   *  if (!list_PointerMember(symbol_FREEDSYMBOLS,(POINTER)Index))
   * ??
   * */
}
