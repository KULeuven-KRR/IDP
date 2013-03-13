/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                  CLAUSE APPROXIMATION                  * */
/* *                                                        * */
/* *  $Module:   APPROX                                     * */ 
/* *                                                        * */
/* *  Copyright (C) 1998, 1999, 2000, 2001                  * */
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


/* $RCSfile: approx.c,v $ */

#include <math.h>
#include "approx.h"
#include "clause.h"

static NAT    approx_PREDICATECOUNTER; /* Used by approx_MonadicFlattenHeads */

static SYMBOL approx_TRUESYMBOL; /* Used by approx_MonadicByTermEncoding */
const  char   approx_TRUENAME[] = "True";

typedef struct {
  SYMBOL original;
  LIST   newSymbols;
} APPROX_NODE, *APPROX_TRANSLATION;

void approx_Init(void)
/**************************************************************
  INPUT:   None.
  RETURNS: Nothing.
  EFFECT:  Initialize the module's variables.
***************************************************************/
{
  approx_PREDICATECOUNTER = 0;
  approx_TRUESYMBOL = symbol_Null();
}

static void approx_DeleteTranslation(LIST Translation)
/**************************************************************
  INPUT:   A list of APPROX_TRANSLATIONs.
  RETURNS: Nothing.
  EFFECT:  All memory used by the translation is freed.
***************************************************************/
{
  for ( ; !list_Empty(Translation); Translation = list_Pop(Translation)) {
    APPROX_TRANSLATION t;
    t = list_Car(Translation);
    list_Delete(t->newSymbols);
    memory_Free(t, sizeof(APPROX_NODE));
  }
}

static APPROX_TRANSLATION approx_LookupTranslationSymbols(LIST Translation,
							  SYMBOL Symbol)
/**************************************************************
  INPUT:   A list of APPROX_TRANSLATIONs and a symbol.
  RETURNS: If the list contains a translation for the <Symbol>,
           the corresponding APPROX_TRANSLATION is returned,
           otherwise NULL is returned.
***************************************************************/
{
  APPROX_TRANSLATION trans;
  for (trans = NULL; trans == NULL && !list_Empty(Translation);
       Translation = list_Cdr(Translation)) {
    SYMBOL s = ((APPROX_TRANSLATION)list_Car(Translation))->original;
    if (symbol_Equal(Symbol, s))
      trans = list_Car(Translation);
  }
  return trans;
}


/**************************************************************/
/* Help functions for approx_MonadicByProjection              */
/**************************************************************/

static APPROX_TRANSLATION approx_CreateProjectionSymbols(SYMBOL Symbol,
							 PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A n-ary predicate symbol and a precedence.
  RETURNS: An APPROX_TRANSLATION containing n new,
           unary predicate symbols.
  EFFECT:  When the symbol's name is <x> then n new predicates
           with the names <x>1, <x>2, ..., <x>n are created.
	   The precedence of the new symbols is set in <Precedence>.
***************************************************************/
{
  APPROX_TRANSLATION Result;
  SYMBOL Predicate;
  NAT    Suffix, Arity;
  int    i;

  Result = (APPROX_TRANSLATION) memory_Malloc(sizeof(APPROX_NODE));
  Result->original   = Symbol;
  Result->newSymbols = list_Nil();

  Arity = symbol_Arity(Symbol);  
  /* If <Symbol> has arity <n> create <n> new symbols */
  for (i = 1; i <= Arity; ++i) {
    char* approx_BUFFER = NULL;
    
    Suffix = i;
    /* Ensure that new symbol wasn't previously defined */      
    do {
      char* SuffixStr = string_IntToString(Suffix++);
      
      if (approx_BUFFER)
        string_StringFree(approx_BUFFER);
      approx_BUFFER = string_Conc(symbol_Name(Symbol),SuffixStr);
      
      string_StringFree(SuffixStr);
    } while (symbol_Lookup(approx_BUFFER) != 0);
    Predicate = symbol_CreatePredicate(approx_BUFFER,1,symbol_STATLEX,Precedence);
    Result->newSymbols = list_Cons((POINTER)Predicate, Result->newSymbols);
    
    string_StringFree(approx_BUFFER);
  }
  Result->newSymbols = list_NReverse(Result->newSymbols);
  return Result;
}

static LIST approx_ApplyProjection(APPROX_TRANSLATION Translation,
				   CLAUSE Clause, int LitIndex,
				   FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   An APPROX_TRANSLATION, a clause, a literal index
           within the clause, a flag store and a precedence.
  RETURNS: A list of new clauses generated from <Clause>.
  EFFECT:  The non-monadic literal with arity n at index <LitIndex>
           is replaced by monadic literals, resulting in n new
           clauses.
	   The flag store and the precedence are needed to create
	   the new clauses.
***************************************************************/
{
  SYMBOL Symbol;
  LIST   Result, Scan, ArgScan;
  TERM   Atom, NewAtom;
  CLAUSE NewClause;
  
  Result = list_Nil();
  Atom   = clause_GetLiteralAtom(Clause, LitIndex);
  /* Now create the new clauses */
  if (LitIndex <= clause_LastAntecedentLitIndex(Clause)) {
    /* A negative literal, so create a single clause */
    LIST Constraint, Antecedent, Succedent;
    int i;

    Constraint = clause_CopyConstraint(Clause);
    Antecedent = list_Nil();
    Succedent  = clause_CopySuccedent(Clause);
    for (i = clause_FirstAntecedentLitIndex(Clause);
	 i <= clause_LastAntecedentLitIndex(Clause); i++) {
      if (i != LitIndex) {
	NewAtom = term_Copy(clause_GetLiteralAtom(Clause, i));
	Antecedent = list_Cons(NewAtom, Antecedent);
      } else {
	for (Scan = Translation->newSymbols, ArgScan = term_ArgumentList(Atom);
	     !list_Empty(Scan);
	     Scan = list_Cdr(Scan), ArgScan = list_Cdr(ArgScan)) {
	  Symbol = (SYMBOL) list_Car(Scan);  /* The new Predicate */
	  /* Create the new monadic atom */
	  NewAtom = term_Create(Symbol, list_List(term_Copy(list_Car(ArgScan))));
	  Antecedent = list_Cons(NewAtom, Antecedent);
	}
      }
    }
    /* create the new clause */
    NewClause = clause_Create(Constraint,Antecedent,Succedent,Flags,Precedence);
    if (clause_GetFlag(Clause, CONCLAUSE))
      clause_SetFlag(NewClause, CONCLAUSE);
    clause_Normalize(NewClause);
    clause_UpdateWeight(NewClause, Flags);
    clause_UpdateMaxVar(NewClause);
    Result = list_Cons(NewClause, Result);
    /* clean up */
    list_Delete(Constraint);
    list_Delete(Antecedent);
    list_Delete(Succedent);
  } else {
    /* A positive literal, so create a set of clauses */
    for (Scan = Translation->newSymbols, ArgScan = term_ArgumentList(Atom);
	 !list_Empty(Scan);
	 Scan = list_Cdr(Scan), ArgScan = list_Cdr(ArgScan)) {
      Symbol = (SYMBOL) list_Car(Scan);  /* The new Predicate */
      /* Create the new monadic atom */
      Atom = term_Create(Symbol, list_List(term_Copy(list_Car(ArgScan))));
      NewClause = clause_Copy(Clause);
      /* Delete the old literal */
      clause_LiteralDelete(clause_GetLiteral(NewClause,LitIndex));
      /* and replace it with the new one */
      clause_SetLiteral(NewClause, LitIndex,
			clause_LiteralCreate(Atom, NewClause));
      clause_Normalize(NewClause);
      clause_UpdateWeight(NewClause, Flags);
      clause_UpdateMaxVar(NewClause);
      Result = list_Nconc(Result, list_List(NewClause));
    }
  }
  return Result;
}

static LIST approx_ApplyProjectionToList(LIST* Translation, LIST Clauses,
					 FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A list of APPROX_TRANSLATIONs, a list of clauses
           that were approximated from the same "father" clause,
	   a flag store and a precedence.
  RETURNS: A list of clauses.
  EFFECT:  This recursive function is initially called with
           a one-element list containing a clause.
           It searches for a non-monadic literal in the first
           clause of the list. If there's no such literal
           the list is just returned. If a non-monadic literal
           was found, all clauses in the list are transformed
           in the same way, adding new clauses to the list.
           Finally the function is called recursively with the
           new list to check for other non-monadic literals.
	   The flag store and the precedence are needed to create
	   the new clauses.
***************************************************************/
{
  APPROX_TRANSLATION Trans;
  CLAUSE Clause;
  SYMBOL Symbol;
  LIST   Result;
  BOOL   Hit;
  NAT    i;

  /* Check whether projection is possible for the first clause */
  Clause = list_Car(Clauses);
  for (Hit = FALSE, i = clause_FirstLitIndex();
       !Hit && i <= clause_LastLitIndex(Clause); i++) {
    Symbol = term_TopSymbol(clause_GetLiteralAtom(Clause, i));
    if (!symbol_Equal(fol_Equality(),Symbol) && symbol_Arity(Symbol) > 1)
      Hit = TRUE;
  }
  --i;   /* Important! */
  /* If projection wasn't possible for the first clause, */
  /* it isn't possible for the other clauses.            */
  /* If projection was possible for the first clause,    */
  /* it's also applicable to the other clauses at the    */
  /* same literal index.                                 */
  if (!Hit) {
    return Clauses;     /* Stop recursion */
  } else {
    Symbol = term_TopSymbol(clause_GetLiteralAtom(Clause, i));
    /* Examine whether the symbol was translated earlier */
    Trans = approx_LookupTranslationSymbols(*Translation, Symbol);
    if (Trans == NULL) {  /* Symbol not found in translation list */
      Trans = approx_CreateProjectionSymbols(Symbol, Precedence);
      *Translation = list_Cons(Trans, *Translation);
    }
    /* Apply projection at literal index <i> */
    Result = list_Nil();
    for ( ; !list_Empty(Clauses); Clauses = list_Pop(Clauses)) {
      Result = list_Nconc(Result,
			  approx_ApplyProjection(Trans,list_Car(Clauses),i,
						 Flags, Precedence));
      /* Delete the old clause */
      clause_Delete(list_Car(Clauses));
    }
    /* recursion */
    return approx_ApplyProjectionToList(Translation, Result, Flags, Precedence);
  }
}
			  
LIST approx_MonadicByProjection(LIST Clauses, FLAGSTORE Flags,
				PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A list of clauses, a flag store and a precedence.
  RETURNS: A list of clauses with monadic literals. If a clause
           contains a n-ary atom P(t_1,...t_n), n clause copies
	   are created where the atom in the i-th new clause is
	   replaced by Pi(t_i).
           Equality is left unchanged.
  EFFECT:  The original clauses are not changed.
           Copies are created even if no projection was applied.
	   The flag store and the precedence are needed to create
	   the new clauses.
***************************************************************/
{
  LIST Result, Translation;

  Result = Translation = list_Nil();
  for ( ; !list_Empty(Clauses); Clauses = list_Cdr(Clauses)) {
    Result = list_Nconc(Result, approx_ApplyProjectionToList(&Translation,
                list_List(clause_Copy(list_Car(Clauses))), Flags, Precedence));
  }
  approx_DeleteTranslation(Translation);
  list_Apply((void (*)(POINTER)) clause_NewNumber, Result);
  return Result;
}


/**************************************************************/
/* Help functions for approx_MonadicByTermEncoding            */
/**************************************************************/

static APPROX_TRANSLATION approx_CreateFunctionForPredicate(SYMBOL Symbol,
							    PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A predicate and a precedence.
  RETURNS: A one-element list with an APPROX_TRANSLATION for
           the predicate.
  EFFECT:  If the predicate has the name "P", a function "P_f" is
           created for the predicate.
	   The precedence of the new symbol is set in <Precedence>.
***************************************************************/
{
  APPROX_TRANSLATION Result;
  SYMBOL Predicate;
  char* approx_BUFFER;
  char* base;
  int size, i;

  Result = (APPROX_TRANSLATION) memory_Malloc(sizeof(APPROX_NODE));
  Result->original   = Symbol;
  Result->newSymbols = list_Nil();

  size = strlen(symbol_Name(Symbol))+3;
  base = memory_Malloc(size*sizeof(char));
  sprintf(base,"%s_f",symbol_Name(Symbol));
  approx_BUFFER = string_StringCopy(base);
  
  /* If the predicate symbol name + "_f" is already defined, try additional numbers */
  i = 0;
  while (symbol_Lookup(approx_BUFFER) != 0) {
    char* Suffix = string_IntToString(++i);
    
    string_StringFree(approx_BUFFER);
    approx_BUFFER = string_Conc(base,Suffix);
    
    string_StringFree(Suffix);
  }
  Predicate = symbol_CreateFunction(approx_BUFFER, symbol_Arity(Symbol),
				    symbol_STATLEX, Precedence);
  Result->newSymbols = list_Cons((POINTER)Predicate, Result->newSymbols);

  string_StringFree(approx_BUFFER);
  memory_Free(base,size*sizeof(char));
  
  return Result;
}

static void approx_EncodeClause(LIST* Translation, CLAUSE Clause,
				FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A clause, a list of APPROX_TRANSLATIONs, a flag store
           and a precedence.
  RETURNS: Nothing.
  EFFECT:  All non-monadic literals of the clause are transformed into
           monadic literals.
           The clause is destructively changed!
***************************************************************/
{
  APPROX_TRANSLATION Trans;
  SYMBOL Symbol;
  TERM   Atom;
  NAT    i;

  for (i = clause_FirstLitIndex(); i <= clause_LastLitIndex(Clause); ++i) {
    Symbol = term_TopSymbol(clause_GetLiteralAtom(Clause, i));
    if (!symbol_Equal(fol_Equality(),Symbol) && symbol_Arity(Symbol) > 1) {
      Trans = approx_LookupTranslationSymbols(*Translation, Symbol);
      if (Trans == NULL) {
	Trans = approx_CreateFunctionForPredicate(Symbol, Precedence);
	*Translation = list_Cons(Trans, *Translation);
      }
      Atom = clause_GetLiteralAtom(Clause, i);
      term_RplacTop(Atom, (SYMBOL)list_Car(Trans->newSymbols));
      Atom = term_Create(approx_TRUESYMBOL, list_List(Atom));
      clause_LiteralSetAtom(clause_GetLiteral(Clause, i), Atom);
    }      
  }
  /* The weight of the clause has changed */
  clause_UpdateWeight(Clause, Flags);
}

LIST approx_MonadicByTermEncoding(LIST Clauses, FLAGSTORE Flags,
				  PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A list of clauses, a flag store and a precedence.
  RETURNS: A list of clauses with monadic literals. A non-monadic
           literal P(t1,t2,...) is translated to True(p(t1,t2,...))
           where p is a function with the same arity as P
           and True is a special predicate with boolean domain.
	   Equality literals are left unchanged.
  EFFECT:  The clauses are not changed.
           This transformation is satisfiability preserving.
	   The flag store and the precedence are needed to create
	   the new clauses.
***************************************************************/
{
  LIST Result, Translation;
  NAT  i;

  /* Ensure that the special predicate "True" is defined */
  if (symbol_Equal(approx_TRUESYMBOL, symbol_Null())) {
    char* approx_BUFFER;
    i = 0;
    
    approx_BUFFER = string_StringCopy(approx_TRUENAME);
        
    while (symbol_Lookup(approx_BUFFER) != 0) {
      char* Suffix = string_IntToString(++i);
      
      string_StringFree(approx_BUFFER);
      approx_BUFFER = string_Conc(approx_TRUENAME,Suffix);
      
      string_StringFree(Suffix);
    }
    approx_TRUESYMBOL = symbol_CreatePredicate(approx_BUFFER, 1, symbol_STATLEX,
					       Precedence);
               
    string_StringFree(approx_BUFFER);
  }

  Translation = list_Nil();
  Result = clause_CopyClauseList(Clauses);
  for (Clauses = Result; !list_Empty(Clauses); Clauses = list_Cdr(Clauses)) {
    approx_EncodeClause(&Translation, list_Car(Clauses), Flags, Precedence);
  }
  approx_DeleteTranslation(Translation);
  list_Apply((void (*)(POINTER)) clause_NewNumber, Result);
  return Result;
}


/**************************************************************/
/* Help functions for approx_MonadicMakeLinear                */
/**************************************************************/

static LIST approx_MakeTermLinear(TERM Term, NAT Mark)
/**************************************************************
  INPUT:   A TERM and the current term binding mark.
  RETURNS: A list of pairs (oldvar, newvar) of variable renamings.
  EFFECT:  The term is destructively changed.
           The variables within Term are made unique with
           respect to previous variable occurrences since
           the last term_StartBinding call.
***************************************************************/
{
  LIST   Scan, Result;
  SYMBOL s;

  Result = list_Nil();
  if (term_IsComplex(Term)) {
    for (Scan=term_ArgumentList(Term); !list_Empty(Scan); Scan=list_Cdr(Scan))
      Result = list_Nconc(approx_MakeTermLinear(list_Car(Scan),Mark), Result);
  } else {
    s = term_TopSymbol(Term);
    if (symbol_IsVariable(s)) {
      if (term_VarIsMarked(s, Mark)) {
	term_RplacTop(Term, symbol_CreateStandardVariable());
	Result = list_Cons(list_PairCreate((POINTER)s,(POINTER)term_TopSymbol(Term)),
			   Result);
      } else {
        term_CreateValueBinding(s, Mark, NULL);
      }
    }
  }
  return Result;
}

static CLAUSE approx_MakeClauseLinear(CLAUSE Clause, FLAGSTORE Flags,
				      PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A clause with monadic positive literals, a flag store
           and a precedence
  RETURNS: The linear approximation of the clause.
  EFFECT:  The original clause is not changed.
           The flag store and the precedence are needed to create
	   the new clause.
  CAUTION: The MaxVar info of the clause must be up to date!
***************************************************************/
{
  LIST   Renamings, Scan, Constraint, Antecedent, Succedent;
  SYMBOL NewVar, OldVar;
  CLAUSE NewClause;
  TERM   Atom, NewAtom;
  NAT    Mark;
  int    i;

#ifdef DEBUG
  if (term_InBindingPhase()) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In approx_makeClauseLinear: Function called inside binding phase.\n");
    misc_FinishErrorReport();
  }
#endif

  if (clause_NumOfSuccLits(Clause) == 0)
    return clause_Copy(Clause);

  for (i = clause_FirstLitIndex(); i <= clause_LastLitIndex(Clause); ++i) {
    Atom = clause_GetLiteralAtom(Clause, i);
    if (fol_IsEquality(Atom) || !term_IsDeclaration(Atom))
      return clause_Copy(Clause);
  }

  /* Ensure that all new variables don't occur in the clause */
  symbol_SetStandardVarCounter(clause_MaxVar(Clause));
  
  term_StartBinding();
  Mark = term_ActMark();

  Constraint = Antecedent = Succedent = list_Nil();
  Renamings = list_Nil();
  /* Make the positive literals linear */
  for (i = clause_LastSuccedentLitIndex(Clause);
       i >= clause_FirstSuccedentLitIndex(Clause); --i) {
    Atom = term_Copy(clause_GetLiteralAtom(Clause, i));
    Succedent = list_Cons(Atom, Succedent);
    Renamings = list_Nconc(approx_MakeTermLinear(Atom, Mark), Renamings);
  }
  term_StopBinding();

  /* Now examine the negative literals */
  for (i = clause_LastAntecedentLitIndex(Clause);
       i >= clause_FirstConstraintLitIndex(Clause); --i) {
    /* Keep a copy of the original literal */
    Atom = term_Copy(clause_GetLiteralAtom(Clause, i));
    if (i <= clause_LastConstraintLitIndex(Clause))
      Constraint = list_Cons(Atom, Constraint);
    else
      Antecedent = list_Cons(Atom, Antecedent);
    for (Scan = Renamings; !list_Empty(Scan); Scan = list_Cdr(Scan)) {
      OldVar = (SYMBOL) list_PairFirst(list_Car(Scan));
      NewVar = (SYMBOL) list_PairSecond(list_Car(Scan));
      if (term_ContainsSymbol(Atom, OldVar)) {
	NewAtom = term_Copy(Atom);
	term_ExchangeVariable(NewAtom, OldVar, NewVar);
	/* Add additional literals for renamed variables */
	if (i <= clause_LastConstraintLitIndex(Clause))
	  Constraint = list_Cons(NewAtom, Constraint);
	else
	  Antecedent = list_Cons(NewAtom, Antecedent);
      }
    }
  }
  /* Create the new clause */
  NewClause = clause_Create(Constraint,Antecedent,Succedent,Flags,Precedence);
  if (clause_GetFlag(Clause, CONCLAUSE))
    clause_SetFlag(NewClause, CONCLAUSE);
  /* clean up */
  list_Delete(Constraint);
  list_Delete(Antecedent);
  list_Delete(Succedent);
  list_DeletePairList(Renamings);
  return NewClause;
}


LIST approx_MonadicMakeLinear(LIST Clauses, FLAGSTORE Flags,
			      PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A list of clauses, a flag store and a precedence.
  RETURNS: A list of copies of the clauses.
  EFFECT:  The entire succedent of a clause <c> is made linear
           if all literals of <c> are monadic.
	   The original clauses are not changed, the clauses are
	   copied even if no transformation was applied.
           For every variable renaming X -> Y, that is necessary
           to make the positive literals linear, we collect
           all negative literals P(t) containing X and add new
           negative literals P(t) where X is replaced by Y.
	   The flag store and the precedence are needed to create
	   the new clauses.
  CAUTION: The MaxVar info of the clause must be up to date!
***************************************************************/
{
  LIST Result, Scan;
  
  Result = list_Copy(Clauses);
  for (Scan = Result; !list_Empty(Scan); Scan = list_Cdr(Scan))
    list_Rplaca(Scan, approx_MakeClauseLinear(list_Car(Scan),Flags,Precedence));
  return Result;
}


/**************************************************************/
/* Help functions for approx_MonadicFlattenHeads              */
/**************************************************************/

static TERM approx_ReplaceSubterms(TERM Term, LIST Duplicates, TERM NewVarTerm)
/**************************************************************
  INPUT:   A term, a list of terms that are possibly subterms of <Term>,
           and a third term, that consists of a variable.
  RETURNS: A copy of <Term> where all subterms that are element of
           the <Duplicates> list are replaced by a copy of <NewVarTerm>.
	   Note that pointers are used to check for equality of terms.
***************************************************************/
{
  if (list_PointerMember(Duplicates, Term))
    return term_Copy(NewVarTerm);
  else {
    LIST Arguments, Scan;

    Arguments = list_Copy(term_ArgumentList(Term));
    for (Scan = Arguments; !list_Empty(Scan); Scan = list_Cdr(Scan))
      list_Rplaca(Scan, approx_ReplaceSubterms(list_Car(Scan),Duplicates,
					       NewVarTerm));
    return term_Create(term_TopSymbol(Term), Arguments);
  }
}

static LIST approx_ApplyFlattening(CLAUSE Clause, LIST Duplicates,
				   BOOL CheckNegativeLiterals, FLAGSTORE Flags,
				   PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A horn clause with one positive literal, a list of terms that
           shall be replaced by some new variable, a boolean flag
	   indicating which transformation will be applied,
	   a flag store and a precedence.
	   Note that <Duplicates> contains all occurrences of a complex
	   subterm of the single positive atom of <Clause>.
  RETURNS: A list of two clauses.
  EFFECT:  The <Clause> is not changed.
           A new predicate with a name starting with "Fl"
	   is created.
	   See approx_MonadicFlattenHeads for more details.
***************************************************************/
{
  LIST   Constraint1, Antecedent1, Succedent1;  /* literals for 1st clause */
  LIST   Constraint2, Antecedent2, Succedent2;  /* literals for 2nd clause */
  LIST   VarsInTerm;
  TERM   Term, NewVarTerm;
  SYMBOL NewPredicate;
  CLAUSE Clause1, Clause2;
  int    i;
  char*  approx_BUFFER;
  const  char*  base;

  /* The set of variables of the subterm is required both for           */
  /* transformation 1) and 2) (CheckNegative=TRUE, CheckVar arbitrary). */
  if (CheckNegativeLiterals)
    VarsInTerm = term_VariableSymbols(list_Car(Duplicates));
  else
    VarsInTerm = list_Nil();
  
  Constraint1 = Antecedent1 = Succedent1 = list_Nil();
  Constraint2 = Antecedent2 = Succedent2 = list_Nil();
  for (i = clause_LastAntecedentLitIndex(Clause);
       i >= clause_FirstConstraintLitIndex(Clause); --i) {
    SYMBOL Var;

    Term = term_Copy(clause_GetLiteralAtom(Clause, i));  /* an atom */
    Var = term_TopSymbol(term_FirstArgument(Term));
    if (i <= clause_LastConstraintLitIndex(Clause)) {
      if (CheckNegativeLiterals) {
	if (list_PointerMember(VarsInTerm, (POINTER)Var))
	  Constraint2 = list_Cons(Term, Constraint2);
	else
	  Constraint1 = list_Cons(Term, Constraint1);
      } else {
	/* copy the constraint for both clauses */
	Constraint1 = list_Cons(Term, Constraint1);
	Constraint2 = list_Cons(term_Copy(Term), Constraint2);
      }
    } else {
      if (CheckNegativeLiterals) {
	if (list_PointerMember(VarsInTerm, (POINTER)Var))
	  Antecedent2 = list_Cons(Term, Antecedent2);
	else
	  Antecedent1 = list_Cons(Term, Antecedent1);
      } else {
	/* copy the antecedent for both clauses */
	Antecedent1 = list_Cons(Term, Antecedent1);
	Antecedent2 = list_Cons(term_Copy(Term), Antecedent2);
      }
    }
  }
  list_Delete(VarsInTerm);

  /* Create a new predicate */
  base = "Fl";
  approx_BUFFER = string_StringCopy(base);
  /* Ensure that new symbol wasn't previously defined */
  do {
    char* Suffix = string_IntToString(++approx_PREDICATECOUNTER);
    
    string_StringFree(approx_BUFFER);
    approx_BUFFER = string_Conc(base,Suffix);    
  
    string_StringFree(Suffix);
  } while (symbol_Lookup(approx_BUFFER) != 0);
  NewPredicate = symbol_CreatePredicate(approx_BUFFER,1,symbol_STATLEX,Precedence);
  string_StringFree(approx_BUFFER);
  /* Create a new variable not occurring in the clause */
  symbol_SetStandardVarCounter(clause_MaxVar(Clause));
  NewVarTerm = term_CreateStandardVariable();
  /* Add the new negative literal to the first clause */
  Antecedent1 = list_Cons(term_Create(NewPredicate, list_List(NewVarTerm)),
			  Antecedent1);

  /* Now create the succedent of the first result clause. To do this we have */
  /* to replace all occurrences of the subterm by the new variable.          */
  Term = clause_GetLiteralAtom(Clause, clause_FirstSuccedentLitIndex(Clause));
  Succedent1 = list_List(approx_ReplaceSubterms(Term, Duplicates, NewVarTerm));
  /* Create the succedent of the second result clause */
  Term = term_Copy(list_Car(Duplicates));
  Succedent2 = list_List(term_Create(NewPredicate, list_List(Term)));
  
  Clause1 = clause_Create(Constraint1, Antecedent1, Succedent1,Flags,Precedence);
  Clause2 = clause_Create(Constraint2, Antecedent2, Succedent2,Flags,Precedence);
  if (clause_GetFlag(Clause, CONCLAUSE)) {
    clause_SetFlag(Clause1, CONCLAUSE);
    clause_SetFlag(Clause2, CONCLAUSE);
  }
  /* cleanup */
  list_Delete(Constraint1);
  list_Delete(Antecedent1);
  list_Delete(Succedent1);
  list_Delete(Constraint2);
  list_Delete(Antecedent2);
  list_Delete(Succedent2);
  
  return list_Cons(Clause1, list_List(Clause2)); 
}

static LIST approx_GetDuplicatesAndCheckVariables(TERM Atom, TERM ComplexTerm,
						  BOOL CheckVariables)
/**************************************************************
  INPUT:   An atom, a subterm of the atom, and a boolean flag.
  RETURNS: Let <A'> be the <Atom> with all duplicates of <ComplexTerm>
           replaced by some constant.
	   This function returns the empty list if <CheckVariables> is true
	   and <A'> shares any variable with <ComplexTerm>.
	   Otherwise the list of all duplicates of <ComplexTerm>
	   in <Atom> is returned.
***************************************************************/
{
  NAT  Stack, ActMark;
  LIST Result;

#ifdef CHECK
  if (term_InBindingPhase()) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In approx_CheckVariables: Called while in variable ");
    misc_ErrorReport("binding phase.");
    misc_FinishErrorReport();
  }
#endif

  term_StartBinding();

  ActMark = term_ActMark();
  Result  = list_Nil();
  Stack   = stack_Bottom();

  /* Mark all variables from <ComplexTerm> */
  term_MarkVariables(ComplexTerm, ActMark);
  term_StopBinding();

  do {
    if (term_Equal(Atom, ComplexTerm))
      Result = list_Cons(Atom, Result);
    else {
      if (CheckVariables && term_IsVariable(Atom) &&
	  term_VarIsMarked(term_TopSymbol(Atom), ActMark)) {
	/* Variable from <ComplexTerm> occurs in the rest of <Term> */
	list_Delete(Result);
	stack_SetBottom(Stack);
	return list_Nil();
      } else
	stack_Push(term_ArgumentList(Atom));
    }
    while (!stack_Empty(Stack) && list_Empty(stack_Top()))
      stack_Pop();
    if (!stack_Empty(Stack)) {
      Atom = list_Car(stack_Top());
      stack_RplacTop(list_Cdr(stack_Top()));
    }
  } while (!stack_Empty(Stack));

  return Result;
}

static LIST approx_OneFlatteningStep(CLAUSE Clause, BOOL CheckVarCondition,
				     BOOL CheckNegativeLiterals,
				     FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A horn clause, two boolean flags, a flag store and a precedence.
  RETURNS: An empty list, if the clause wasn't flattened, else a list
           of clauses resulting from the flattening of the clause.
  EFFECT:  The <Clause> is not changed.
***************************************************************/
{
  TERM Atom, Term;
  LIST Result;
  NAT  Stack;

  /* Nothing to do if clause has no positive literal */
  if (clause_NumOfSuccLits(Clause) != 1)
    return list_Nil();
  /* Get the atom of the only positive literal */
  Atom = clause_GetLiteralAtom(Clause, clause_FirstSuccedentLitIndex(Clause));
  if (fol_IsEquality(Atom) || !term_IsDeclaration(Atom)) {
    /* positive literal is equality or isn't monadic */
    return list_Nil();
  }
  Term = term_FirstArgument(Atom); /* the term of the monadic literal */

  Stack   = stack_Bottom();
  /* Start with the arguments of the term, not the term itself */
  stack_Push(term_ArgumentList(Term));
  do {
    while (!stack_Empty(Stack) && list_Empty(stack_Top()))
      stack_Pop();
    if (!stack_Empty(Stack)) {
      Term = list_Car(stack_Top());
      stack_RplacTop(list_Cdr(stack_Top()));
      
      if (term_IsComplex(Term)) { /* replace only complex terms */
	LIST duplicates;
	duplicates = approx_GetDuplicatesAndCheckVariables(Atom, Term,
							   CheckVarCondition);
	if (list_Empty(duplicates))
	  /* Var condition had to be checked but was violated */
	  stack_Push(term_ArgumentList(Term));
	else {
	  /* Apply rule */
	  Result = approx_ApplyFlattening(Clause, duplicates,
					  CheckNegativeLiterals, Flags,
					  Precedence);
	  list_Delete(duplicates);
	  stack_SetBottom(Stack);
	  return Result;
	}
      }
    }
  } while (!stack_Empty(Stack));

  return list_Nil();
}


LIST approx_HornMonadicFlattenHeads(CLAUSE Clause, BOOL CheckVarCondition,
				    BOOL CheckNegativeLiterals,
				    FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A clause, two boolean flags, a flag store and a precedence.
  RETURNS: An empty list, if the clause wasn't flattened,
           else the list of clauses resulting from the
           flattening of the clause.
  EFFECT:  This function implements three kinds of flattening that
           require all literals in <Clause> to be monadic and <Clause> is Horn:
           1) CheckVarCondition=TRUE, CheckNegativeLiterals=TRUE
              C || D -> P(t[s]_p)
              ---------------------------
	      Fl(x) C1 || D1 -> P(t[p/x])
	            C2 || D2 -> Fl(s) 
	      all terms in C, D are variables,
              where s is a complex term at the non-top position p,
	      Var(s) is disjoint from Var(P(t[p/c])) (where c is an arbitrary constant),
	      x is a new variable, Fl is a new predicate,
	      L\in C2 if Var(L) \subset Var(s),
	      L\in D2 if Var(L) \subset Var(s),
	      C1 and C2 are a partition of C,
	      D1 and D2 are a partition of D
	      This transformation is equivalence preserving with respect
	      to the extension of P in the minimal model.
           2) CheckVarCondition=FALSE, CheckNegativeLiterals=TRUE
	      This setting does the same transformation as above,
              but doesn't require that Var(s) is disjoint from
              Var(P(t[p/c])).
	      This transformation is an upper approximation of P. 
           3) CheckVarCondition=FALSE, CheckNegativeLiterals=FALSE
	      C || D -> P(t[s]_p)
	      ------------------
	      Fl(x) C || D -> P(t[p/x])
	            C || D -> Fl(s)
              where C and D may contain non-variable terms and
	      s and t[p/c] may share variables
	      This transformation is an upper approximation of P. 

	   These transformations are recursively applied.
           The <Clause> itself is not changed.
	   If the clause doesn't meet the requirements an empty 
	   list is returned.
  MEMORY : <Clause> is not changed, the result consists of newly
           generated clauses.
***************************************************************/
{
  LIST   Result, Queue, NewClauses;
  CLAUSE ActClause;
  TERM   Term;
  BOOL   FirstTime;
  int    i;

#ifdef DEBUG
  if (CheckVarCondition && !CheckNegativeLiterals) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In approx_MonadicFlattenHeads: Illegal input parameters.\n");
    misc_FinishErrorReport();
  }
#endif

  /* Apply transformation only to clauses with a single positive literal */
  if (!clause_NumOfSuccLits(Clause) != 1)
    return list_Nil();

  /* Check the negative literals */
  for (i = clause_FirstConstraintLitIndex(Clause);
       i <= clause_LastAntecedentLitIndex(Clause); ++i) {
    Term = clause_GetLiteralAtom(Clause, i);   /* an atom */
    if (fol_IsEquality(Term) || !term_IsDeclaration(Term)) {
      /* negative literal isn't monadic */
      return list_Nil();
    }
    if (CheckNegativeLiterals && !term_IsVariable(term_FirstArgument(Term))) {
      /* negative literal hasn't simple variable as argument */
      return list_Nil();
    }
  }

  Result = list_Nil();
  FirstTime = TRUE;
  Queue = list_List(Clause);  /* Initialize the queue with the first clause */
  do {
    ActClause = list_NCar(&Queue);
    NewClauses = approx_OneFlatteningStep(ActClause, CheckVarCondition,
					  CheckNegativeLiterals, Flags,
					  Precedence);
    if (list_Empty(NewClauses)) {
      /* ActClause is finished */
      Result = list_Cons(ActClause, Result);
    } else {
      /* ActClause was transformed */
      Queue = list_Nconc(NewClauses, Queue);
      /* Delete temporary clauses */
      if (!FirstTime)  /* Avoid deletion of the initial clause */
	clause_Delete(ActClause);
    }
    FirstTime = FALSE;
  } while (!list_Empty(Queue));

  if (list_Empty(list_Cdr(Result))) {
    /* list contains only the original clause */
    list_Delete(Result);
    return list_Nil();
  } else
    return list_NReverse(Result);
}


/**************************************************************/
/* approx_MakeHorn                                            */
/**************************************************************/

LIST approx_MakeHorn(CLAUSE Clause, FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A clause, a flag store and a precedence.
  RETURNS: An empty list if <Clause> is already a horn clause,
           otherwise a list of horn clauses resulting from <Clause>.
  EFFECT:  A non-horn clause with n positive literals
           is replaced by n horn clauses. The i-th horn clause
           has the i-th positive literal from <Clause> as
           only positive literal.
	   The flag store and the precedence are needed to create
	   the new clauses.
***************************************************************/
{
  LIST Result, Constraint, Antecedent, Succedent;
  int  i;
  
  Result = list_Nil();
  if (!clause_IsHornClause(Clause)) {
    Constraint = Antecedent = list_Nil();
    /* Collect constraint atoms */
    for (i = clause_LastConstraintLitIndex(Clause);
	 i >= clause_FirstConstraintLitIndex(Clause); --i)
      Constraint = list_Cons(clause_GetLiteralAtom(Clause, i), Constraint);
    /* Collect antecedent atoms */
    for (i = clause_LastAntecedentLitIndex(Clause);
	 i >= clause_FirstAntecedentLitIndex(Clause); --i)
      Antecedent = list_Cons(clause_GetLiteralAtom(Clause, i), Antecedent);
    for (i = clause_FirstSuccedentLitIndex(Clause);
	 i <= clause_LastSuccedentLitIndex(Clause); ++i) {
      CLAUSE NewClause;
      /* Copy constraint and antecedent atoms */
      term_CopyTermsInList(Constraint);
      term_CopyTermsInList(Antecedent);
      Succedent = list_List(term_Copy(clause_GetLiteralAtom(Clause, i)));
      NewClause = clause_Create(Constraint,Antecedent,Succedent,Flags,Precedence);
      if (clause_GetFlag(Clause, CONCLAUSE))
	clause_SetFlag(NewClause, CONCLAUSE);
      list_Delete(Succedent);
      Result = list_Cons(NewClause, Result);
    }
    list_Delete(Constraint);
    list_Delete(Antecedent);
  }
  return list_NReverse(Result);
}
