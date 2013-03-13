/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                CONTEXTS FOR VARIABLES                  * */
/* *                                                        * */
/* *  $Module:   CONTEXT                                    * */ 
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
/* $Revision: 1.4 $                                         * */
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


/* $RCSfile: context.c,v $ */

#include "context.h"

/**************************************************************/
/* Global Variables                                           */
/**************************************************************/

int     cont_NOOFCONTEXTS;
LIST    cont_LISTOFCONTEXTS;
int     cont_BINDINGS;

SYMBOL  cont_INDEXVARSCANNER;

CONTEXT cont_LASTBINDING;     /* The last binding made. */
CONTEXT cont_CURRENTBINDING;  /* Help variable. */

CONTEXT cont_LEFTCONTEXT;
CONTEXT cont_RIGHTCONTEXT;
CONTEXT cont_INSTANCECONTEXT;

cont_STACK_TYPE cont_STACK;
int             cont_STACKPOINTER;

cont_CHECKSTACK_TYPE cont_CHECKSTACK;
int                  cont_CHECKSTACKPOINTER;

CONTEXT cont_STATELASTBINDING; /* Storage to save state of trails. */
int     cont_STATEBINDINGS;    /* Storage to save number of current bindings. */
int     cont_STATESTACK;       /* Storage to save state of stack. */
int     cont_STATETOPSTACK;    /* Storage to save state of the top element of the stack. */


/**************************************************************/
/* ********************************************************** */
/* *							    * */
/* *  INITIALIZATION           			            * */
/* *							    * */
/* ********************************************************** */
/**************************************************************/


void cont_Init(void)
/**********************************************************
  INPUT:   None.
  RETURNS: None.
  EFFECT:  Initializes the unify module.
********************************************************/
{
  cont_LASTBINDING     = (CONTEXT)NULL;

  cont_ResetIndexVarScanner();

  cont_NOOFCONTEXTS    = 0;
  cont_LISTOFCONTEXTS  = list_Nil();
  cont_BINDINGS        = 0;

  cont_INSTANCECONTEXT = (CONTEXT)memory_Malloc(sizeof(CONTEXT_NODE));

  cont_LEFTCONTEXT     = cont_Create();
  cont_RIGHTCONTEXT    = cont_Create();

  cont_StackInit();
  cont_StackPush(0);
  cont_StackPop();
}


void cont_Check(void)
/**********************************************************
  INPUT:   None.
  RETURNS: None.
  EFFECT:  Frees internal structures of the unify module.
********************************************************/
{
#ifdef CHECK
  if (cont_LASTBINDING || (cont_BINDINGS != 0) ||
      !symbol_Equal(cont_INDEXVARSCANNER,
		    symbol_GetInitialIndexVarCounter())) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In cont_Check: There are variable bindings not reset.\n");
    misc_FinishErrorReport();
  }
#endif
}


void cont_Free(void)
/**********************************************************
  INPUT:   None.
  RETURNS: None.
  EFFECT:  Frees internal structures of the unify module.
********************************************************/
{
  cont_Check();

  while (cont_NOOFCONTEXTS > 0)
    cont_Delete(list_Car(cont_LISTOFCONTEXTS)); /* Decreases NOOFCONTEXTS */

  cont_BINDINGS = 0;

  memory_Free(cont_INSTANCECONTEXT, sizeof(CONTEXT_NODE));
}

void cont_InitBinding(CONTEXT C, SYMBOL Var)
/**********************************************************
  INPUT:   A context and a variable.
  RETURNS: Nothing.
  EFFECT:  Initializes a binding of variable in the 
           given context.
********************************************************/
{
  cont_CURRENTBINDING = cont_Binding(C, Var);
  cont_SetBindingLink(cont_CURRENTBINDING, (CONTEXT)NULL);
  cont_SetBindingTerm(cont_CURRENTBINDING, (TERM)NULL);
  cont_SetBindingSymbol(cont_CURRENTBINDING, Var);
  cont_SetBindingRenaming(cont_CURRENTBINDING, symbol_Null());
  cont_SetBindingContext(cont_CURRENTBINDING, (CONTEXT)NULL);
}



/**************************************************************/
/* ********************************************************** */
/* *							    * */
/* *  TERM EQUALITY WITH RESPECT TO BOUND VARIABLES         * */
/* *							    * */
/* ********************************************************** */
/**************************************************************/


BOOL cont_TermEqual(CONTEXT GlobalContext1, CONTEXT TermContext1, TERM Term1, 
                    CONTEXT GlobalContext2, CONTEXT TermContext2, TERM Term2)
/*********************************************************
  INPUT:   Two terms and two local contexts for the terms and
           two global contexts
  RETURNS: TRUE iff the two terms are equal, where
           variables are interpreted with respect to
	   the bindings in the contexts.
  CAUTION: Variables of <Term1> and <Term2> are bound in 
           <TermContext1> and <TermContext2> respectively and
           the index variables are bound in <GlobalContext1>
           and <GlobalContext2> respectively.
********************************************************/
{
#ifdef CHECK
  if (!(term_IsTerm(Term1) && term_IsTerm(Term2))) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In cont_TermEqual: Input terms are corrupted.\n");
    misc_FinishErrorReport();
  }
#endif

  Term1 = cont_Deref(GlobalContext1,&TermContext1,Term1);
  Term2 = cont_Deref(GlobalContext2,&TermContext2,Term2);

  if (!term_EqualTopSymbols(Term1, Term2))
    return FALSE;
  else if (term_ArgumentList(Term1)) {
    LIST Scan1, Scan2;
    for (Scan1=term_ArgumentList(Term1), Scan2=term_ArgumentList(Term2);
	 list_Exist(Scan1) && list_Exist(Scan2);
	 Scan1=list_Cdr(Scan1), Scan2=list_Cdr(Scan2))
      if (!cont_TermEqual(GlobalContext1, TermContext1,list_Car(Scan1), 
                            GlobalContext2, TermContext2,list_Car(Scan2)))
	return FALSE;
    return (list_Empty(Scan1) ? list_Empty(Scan2) : FALSE);
  } else
    return TRUE;
}


BOOL cont_TermEqualModuloBindings(CONTEXT IndexContext, CONTEXT CtL, TERM TermL,
				  CONTEXT CtR, TERM TermR)
/*********************************************************
  INPUT:   Two contexts, two terms.
  RETURNS: The boolean value TRUE if the terms are equal.
  CAUTION: EQUAL FUNCTION- OR PREDICATE SYMBOLS SHARE THE
           SAME ARITY. THIS IS NOT VALID FOR JUNCTORS!
*******************************************************/
{   
#ifdef CHECK
  if (!(term_IsTerm(TermL) && term_IsTerm(TermR))) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In cont_TermEqualModuloBindings: Input terms are corrupted.\n");
    misc_FinishErrorReport();
  }
#endif

  while (term_IsVariable(TermL)) {
    SYMBOL TermTop;

    TermTop = term_TopSymbol(TermL);

    if (symbol_IsIndexVariable(TermTop))
      CtL = IndexContext;
    else if (CtL == cont_InstanceContext())
      break;

    if (cont_VarIsBound(CtL, TermTop)) {
      CONTEXT CHelp;

      CHelp = cont_ContextBindingContext(CtL, TermTop);
      TermL = cont_ContextBindingTerm(CtL, TermTop);
      CtL   = CHelp;
    } else
      break;
  }

  while (term_IsVariable(TermR)) {
    SYMBOL TermTop;

    TermTop = term_TopSymbol(TermR);

    if (symbol_IsIndexVariable(TermTop))
      CtR = IndexContext;
    else if (CtR == cont_InstanceContext())
      break;

    if (cont_VarIsBound(CtR, TermTop)) {
      CONTEXT CHelp;

      CHelp = cont_ContextBindingContext(CtR, TermTop);
      TermR = cont_ContextBindingTerm(CtR, TermTop);
      CtR   = CHelp;
    } else
      break;
  }

  if (!term_EqualTopSymbols(TermL, TermR))
    return FALSE;
  else 
    if (term_IsVariable(TermL)) {
      if (CtL == CtR)
	return TRUE;
      else
	return FALSE;
    }
    else 
      if (term_IsComplex(TermL)) {
	LIST ScanL, ScanR;
	
	for (ScanL=term_ArgumentList(TermL), ScanR=term_ArgumentList(TermR);
	     list_Exist(ScanL) && list_Exist(ScanR);
	     ScanL=list_Cdr(ScanL), ScanR=list_Cdr(ScanR))
	  if (!cont_TermEqualModuloBindings(IndexContext, CtL, list_Car(ScanL),
					    CtR, list_Car(ScanR)))
	    return FALSE;
	
	return (list_Empty(ScanL) ? list_Empty(ScanR) : FALSE);
	
      } 
      else
	return TRUE;
}


/**************************************************************/
/* ********************************************************** */
/* *							    * */
/* *  APPLY BINDINGS                                        * */
/* *							    * */
/* ********************************************************** */
/**************************************************************/


TERM cont_CopyAndApplyBindings(CONTEXT TermContext, TERM Term)
{
  while (term_IsVariable(Term)) {
    SYMBOL TermTop;

    TermTop = term_TopSymbol(Term);

    if (cont_VarIsBound(TermContext, TermTop)) {
      CONTEXT HelpContext;

      HelpContext = cont_ContextBindingContext(TermContext, TermTop);
      Term        = cont_ContextBindingTerm(TermContext, TermTop);
      TermContext = HelpContext;
    } else
      break;
  }

  if (term_IsComplex(Term)) {
    LIST Scan, ArgumentList;
    for (Scan = ArgumentList = list_Copy(term_ArgumentList(Term));
	 !list_Empty(Scan);
	 Scan = list_Cdr(Scan))
      list_Rplaca(Scan, cont_CopyAndApplyBindings(TermContext, list_Car(Scan)));
    return term_Create(term_TopSymbol(Term), ArgumentList);
  } else 
    return term_Create(term_TopSymbol(Term), list_Nil());
}

TERM cont_SymbolApplyBindings(CONTEXT TermContext, SYMBOL Symbol)
/**************************************************************
  INPUT:   A call-by-ref context and a variable symbol.
  RETURNS: The recursively dereferenced term and the corresponding context,
           NULL if the symbol is not bound
  SUMMARY: Dereferences bindings of variables.
  CAUTION: In general, the context of the returned term
           is different to the input context.
***************************************************************/
{
  TERM Term;

  Term = (TERM)NULL;

  while (symbol_IsVariable(Symbol)) {

    if (cont_VarIsBound(TermContext, Symbol)) {
      CONTEXT HelpContext;

      HelpContext = cont_ContextBindingContext(TermContext, Symbol);
      Term        = cont_ContextBindingTerm(TermContext, Symbol);
      TermContext = HelpContext;
      Symbol      = term_TopSymbol(Term);
    } else
      break;
  }

  if (Term != (TERM)NULL && term_IsComplex(Term)) {
    LIST Scan, ArgumentList;
    for (Scan = ArgumentList = list_Copy(term_ArgumentList(Term));
	 !list_Empty(Scan);
	 Scan = list_Cdr(Scan))
      list_Rplaca(Scan, cont_CopyAndApplyBindings(TermContext, list_Car(Scan)));
    return term_Create(term_TopSymbol(Term), ArgumentList);
  }
  
  return Term;
}


TERM cont_CopyAndApplyBindingsCom(const CONTEXT Context, TERM Term)
{
  while (term_IsVariable(Term) && cont_VarIsBound(Context, term_TopSymbol(Term)))
    Term = cont_ContextBindingTerm(Context, term_TopSymbol(Term));

  if (term_IsComplex(Term)) {
    LIST Scan, ArgumentList;
    for (Scan = ArgumentList = list_Copy(term_ArgumentList(Term));
	 !list_Empty(Scan);
	 Scan = list_Cdr(Scan))
      list_Rplaca(Scan, cont_CopyAndApplyBindingsCom(Context, list_Car(Scan)));
    return term_Create(term_TopSymbol(Term), ArgumentList);
  } else 
    return term_Create(term_TopSymbol(Term), list_Nil());
}


TERM cont_ApplyBindingsModuloMatching(const CONTEXT Context, TERM Term,
				      BOOL VarCheck)
/**********************************************************
  INPUT:   A context, a term, and a boolean flag.
  RETURNS: <Term> is destructively changed with respect to
           established bindings in the context.
	   If <VarCheck> is true, all variables in <Term>
	   must be bound in the context. When compiled with
	   "CHECK" on, this condition is in fact checked.
	   This function only makes sense after a matching operation.
***********************************************************/
{
  TERM   RplacTerm;
  LIST   Arglist;
  SYMBOL Top;

#ifdef CHECK
  if (VarCheck && symbol_IsVariable(term_TopSymbol(Term)) &&
      !cont_VarIsBound(Context, term_TopSymbol(Term))) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In cont_ApplyBindingsModuloMatching:");
    misc_ErrorReport(" Used in forbidden context.\n");
    misc_FinishErrorReport();
  }
#endif

  Top = term_TopSymbol(Term);

  if (symbol_IsVariable(Top)) {
    
    if (cont_VarIsBound(Context, Top)) {
      RplacTerm = cont_ContextBindingTerm(Context, Top);
      Arglist   = term_CopyTermList(term_ArgumentList(RplacTerm));
      term_RplacTop(Term, term_TopSymbol(RplacTerm));
      term_DeleteTermList(term_ArgumentList(Term));
      term_RplacArgumentList(Term, Arglist); 
    }
  }
  else {
    
    for (Arglist = term_ArgumentList(Term);
	 !list_Empty(Arglist);
	 Arglist = list_Cdr(Arglist))
      cont_ApplyBindingsModuloMatching(Context, list_Car(Arglist), VarCheck);
  }     

  return Term;
}


static TERM cont_CopyAndApplyIndexVariableBindings(const CONTEXT Context, TERM Term)
{
  SYMBOL TermTop;

#ifdef CHECK
  if (symbol_IsIndexVariable(term_TopSymbol(Term)) &&
      !cont_VarIsBound(Context, term_TopSymbol(Term))) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In cont_CopyAndApplyIndexVariableBindings:");
    misc_ErrorReport(" Expected bound index variable.");
    misc_FinishErrorReport();
  }
#endif

  TermTop = term_TopSymbol(Term);

  while (symbol_IsIndexVariable(TermTop)) {
    if (cont_VarIsBound(Context, TermTop)) {
      Term    = cont_ContextBindingTerm(Context, TermTop);
      TermTop = term_TopSymbol(Term);
    }
  }

  if (term_IsComplex(Term)) {
    LIST Scan, ArgumentList;
    for (Scan = ArgumentList = list_Copy(term_ArgumentList(Term));
	 !list_Empty(Scan);
	 Scan = list_Cdr(Scan))
      list_Rplaca(Scan, cont_CopyAndApplyIndexVariableBindings(Context, list_Car(Scan)));
    return term_Create(TermTop, ArgumentList);
  } else 
    return term_Create(TermTop, list_Nil());
}


TERM cont_ApplyBindingsModuloMatchingReverse(const CONTEXT Context, TERM Term)
/**********************************************************
  INPUT:   A term.
  RETURNS: <Term> is destructively changed with respect to
           established bindings in the leftmost context. This
           function only make sense after a matching operation (reverse).
***********************************************************/
{
  TERM   RplacTerm;
  LIST   Arglist;
  SYMBOL Top;

#ifdef CHECK
  if (symbol_IsVariable(term_TopSymbol(Term)) &&
      !cont_VarIsBound(Context, term_TopSymbol(Term))) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In cont_ApplyBindingsModuloMatchingReverse:");
    misc_ErrorReport(" Used in forbidden context.\n");
    misc_FinishErrorReport();
  }
#endif
    
  Top = term_TopSymbol(Term);

  if (symbol_IsVariable(Top)) {
    
    if (cont_VarIsBound(Context, Top)) {
      RplacTerm =
	cont_CopyAndApplyIndexVariableBindings(Context,
						  cont_ContextBindingTerm(Context, Top));
      term_RplacTop(Term, term_TopSymbol(RplacTerm));
      term_DeleteTermList(term_ArgumentList(Term));
      term_RplacArgumentList(Term, term_ArgumentList(RplacTerm));
      term_Free(RplacTerm);
    }
  }
  else {
    
    for (Arglist = term_ArgumentList(Term); !list_Empty(Arglist);
	 Arglist = list_Cdr(Arglist))
      cont_ApplyBindingsModuloMatchingReverse(Context, list_Car(Arglist));
  }     

  return Term;
}


BOOL cont_BindingsAreRenamingModuloMatching(const CONTEXT RenamingContext)
{
  CONTEXT Context;

#ifdef CHECK
  if (!cont_IsContextEmpty(RenamingContext)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In cont_BindingsAreRenamingModuloMatching:");
    misc_ErrorReport(" Renaming context contains bindings.\n");
    misc_FinishErrorReport();
  }
#endif

  cont_StartBinding();

  Context = cont_LastBinding();

  while (Context) {
    
    if (!symbol_IsIndexVariable(cont_BindingSymbol(Context))) {
      SYMBOL CodomainSymbol;

      CodomainSymbol = term_TopSymbol(cont_BindingTerm(Context));

      if (symbol_IsVariable(CodomainSymbol)) {
	if (cont_VarIsRenamed(RenamingContext, CodomainSymbol)) {
	  cont_BackTrack();
	  return FALSE;
	} else {
	  cont_CreateBinding(RenamingContext, CodomainSymbol, NULL, NULL);
	  cont_SetContextBindingRenaming(RenamingContext, CodomainSymbol, CodomainSymbol);
	}
      } else {
	cont_BackTrack();
	return FALSE;
      }
    }

    Context = cont_BindingLink(Context);
  }

  cont_BackTrack();
  return TRUE;
}


/**************************************************************/
/* ********************************************************** */
/* *							    * */
/* *  MISC FUNCTIONS                                        * */
/* *							    * */
/* ********************************************************** */
/**************************************************************/

void cont_CreateBindingHelp(CONTEXT C, SYMBOL Var, CONTEXT CTerm, TERM Term)
{
#ifdef CHECK
  if (cont_VarIsBound(C, Var)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In cont_CreateBindingHelp: Variable already bound.\n");
    misc_FinishErrorReport();
  }
#endif

  cont_CURRENTBINDING = cont_Binding(C,Var);
  cont_SetBindingTerm(cont_CURRENTBINDING, Term);
  cont_SetBindingContext(cont_CURRENTBINDING, CTerm);
  cont_SetBindingLink(cont_CURRENTBINDING, cont_LastBinding());
  cont_SetLastBinding(cont_CURRENTBINDING);
}

CONTEXT cont_ContextOfBinding(CONTEXT B)
{
  CONTEXT Result;
  LIST    Scan;

  for (Result = NULL, Scan = cont_LISTOFCONTEXTS;
       list_Exist(Scan);
       Scan = list_Cdr(Scan)) {
    if (cont_IsInContext(list_Car(Scan), cont_BindingSymbol(B), B)) {
      Result = list_Car(Scan);
      break;
    }
  }

#ifdef CHECK
  if (Result == NULL) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In cont_ContextOfBinding: Unknown context.\n");
    misc_FinishErrorReport();
  }
#endif

  return Result;
}

void cont_BindingOutput(CONTEXT C, SYMBOL Var)
{
  symbol_Print(cont_ContextBindingSymbol(C, Var));
  putchar(':');
  symbol_Print(Var);

  fputs(" -> ", stdout);

  if (cont_VarIsBound(C, Var)) {
    term_PrintPrefix(cont_ContextBindingTerm(C, Var));
  } else
    fputs("unbound", stdout);

  fputs(" in ", stdout);

  if (cont_VarIsUsed(C, Var)) {
    printf("%ld", (unsigned long)cont_ContextBindingContext(C, Var));
  } else
    fputs("NULL (unused)", stdout);

  fputs(". ", stdout);

  if (cont_VarIsClosed(C, Var)) {
    fputs("(closed)", stdout);
  }

  if (!cont_VarIsBound(C, Var) &&
      !cont_VarIsUsed(C, Var)) {
    fputs(",(free)", stdout);
  }

  if (cont_VarIsRenamed(C, Var)) {
    fputs(",(renamed): ", stdout);
    symbol_Print(Var);
    fputs(" -> ", stdout);
    symbol_Print(cont_ContextBindingRenaming(C, Var));
  } 
  
  fflush(stdout);
}

void cont_BackTrackLastBindingHelp(void)
{
  cont_CURRENTBINDING = cont_LastBinding();
  cont_SetLastBinding(cont_BindingLink(cont_CURRENTBINDING));
  cont_SetBindingTerm(cont_CURRENTBINDING, NULL);
  cont_SetBindingContext(cont_CURRENTBINDING, NULL);
  cont_SetBindingRenaming(cont_CURRENTBINDING, symbol_Null());
  cont_SetBindingLink(cont_CURRENTBINDING, NULL);

  cont_BINDINGS--;
}

SYMBOL cont_TermMaxVar(CONTEXT GlobalContext, CONTEXT TermContext, TERM Term)
/*********************************************************
  INPUT:   A global Context where Index variables are bound 
           a context and a term
  RETURNS: The maximal variable in <Term> with respect to
           the bindings in <Context>
  CAUTION: Variables of <Term1> are bound in 
           <TermContext1>  and
           the index variables are bound in <GlobalContext1>
           
********************************************************/
{
  LIST   scan;
  SYMBOL result;

#ifdef CHECK
  if (!term_IsTerm(Term)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In cont_TermMaxVar: Input term is corrupted.\n");
    misc_FinishErrorReport();
  }
#endif

  Term   = cont_Deref(GlobalContext,&TermContext,Term);
  result = symbol_Null();

  if (term_IsStandardVariable(Term)) {
    if (term_TopSymbol(Term) > result)
      result = term_TopSymbol(Term);
  } else {
    for (scan = term_ArgumentList(Term); !list_Empty(scan); scan = list_Cdr(scan)) {
      SYMBOL max = cont_TermMaxVar(GlobalContext, TermContext, list_Car(scan));
      
      if (max > result)
	result = max;
    }
  }

  return result;
}


NAT cont_TermSize(CONTEXT GlobalContext, CONTEXT TermContext, TERM Term)
/*********************************************************
  INPUT:   A global context where index variables are bound
           a context and a term.
  RETURNS: The number of symbols in <Term> with respect to
           the bindings in <Context>
  CAUTION: Variables of <Term1> are bound in 
           <TermContext1>  and
           the index variables are bound in <GlobalContext1>
********************************************************/
{
  NAT  result;
  LIST scan;

  Term = cont_Deref(GlobalContext, &TermContext, Term);
  result = 1;
  for (scan = term_ArgumentList(Term); !list_Empty(scan); scan = list_Cdr(scan))
    result += cont_TermSize(GlobalContext, TermContext, list_Car(scan));

  return result;
}


BOOL cont_TermContainsSymbol(CONTEXT GlobalContext, CONTEXT TermContext, TERM Term, 
                                SYMBOL Symbol)
/*********************************************************
  INPUT:   A global context where index varibles are bound
           a context, a term and a symbol.
  RETURNS: TRUE, if <Symbol> occurs in <Term> with respect to
           the bindings in <Context>, FALSE otherwise.
********************************************************/
{
  LIST scan;

  Term = cont_Deref(GlobalContext, &TermContext, Term);

  if (symbol_Equal(term_TopSymbol(Term), Symbol))
    return TRUE;
  else
    for (scan = term_ArgumentList(Term); !list_Empty(scan); scan = list_Cdr(scan)) {
      if (cont_TermContainsSymbol(GlobalContext, TermContext, list_Car(scan), Symbol))
	return TRUE;
    }

  return FALSE;
}

TERM cont_Deref(CONTEXT GlobalContext, CONTEXT* TermContext, TERM Term)
/******************************************************************
  INPUT:      A global context where the Index variables are bound,
              a term <Term> and a call-by-ref context for <Term>.
  RETURNS:    The dereferenced term and the corresponding context.
  SUMMARY:    Dereferences bindings of variables.
  CAUTION:    In general, the context of the returned term <TermContext>
              is different to the input context. 
  ASSUMPTION: All Index variables occuring in <Term> have to be 
              bound in <GlobalContext>,
              no Index variable is mapped to another index variable
*******************************************************************/
{

  if(term_IsIndexVariable(Term)) {
   
    SYMBOL TermTop;
    TermTop = term_TopSymbol(Term);

    #ifdef CHECK
    if(!cont_VarIsBound(GlobalContext, TermTop) ||
       term_IsIndexVariable(cont_ContextBindingTerm(GlobalContext, TermTop))) {
       misc_StartErrorReport();
       misc_ErrorReport("\ncont_Deref: Illegal Context!");
       misc_FinishErrorReport();
    }
    #endif
        
    Term         = cont_ContextBindingTerm(GlobalContext, TermTop);
    *TermContext = cont_ContextBindingContext(GlobalContext, TermTop);
    
    }
        
  while (term_IsVariable(Term) && *TermContext != cont_InstanceContext()) {
    SYMBOL TermTop;

    TermTop = term_TopSymbol(Term);

    if (cont_VarIsBound(*TermContext, TermTop)) {
      CONTEXT HelpContext;

      HelpContext = cont_ContextBindingContext(*TermContext, TermTop);
      Term        = cont_ContextBindingTerm(*TermContext, TermTop);
      *TermContext    = HelpContext;
    } 
    else
      return Term;
  }

  return Term;
}

/**************************************************************/
/* ********************************************************** */
/* *							    * */
/* *  OUTPUT             			            * */
/* *							    * */
/* ********************************************************** */
/**************************************************************/


void cont_TermPrintPrefix(CONTEXT GlobalContext, CONTEXT TermContext, TERM Term)
/**************************************************************
  INPUT:   A global context where index variables are bound,
           a context and a term.
  RETURNS: none.
  SUMMARY: Prints the term modulo the context to stdout. 
  CAUTION: Variables of <Term1> are bound in 
           <TermContext1>  and
           the index variables are bound in <GlobalContext1>
***************************************************************/
{
  Term = cont_Deref(GlobalContext,&TermContext, Term);

  symbol_Print(term_TopSymbol(Term));

  if (term_IsComplex(Term)) {
    LIST List;

    putchar('(');

    for (List = term_ArgumentList(Term); !list_Empty(List);
	 List = list_Cdr(List)) {
      cont_TermPrintPrefix(GlobalContext, TermContext, list_Car(List));

      if (!list_Empty(list_Cdr(List)))
	putchar(',');
    }

    putchar(')');
  }
}
