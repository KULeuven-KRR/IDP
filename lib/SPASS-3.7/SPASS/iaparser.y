/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *          PARSER FOR SPASS INTERACTIVE MODUL            * */
/* *                                                        * */
/* *  $Module:   KIV                                        * */ 
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
/* $Revision: 1.3 $                                         * */
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


/* $RCSfile: iaparser.y,v $ */

%{

#include "flags.h"
#include "ia.h"
#include "symbol.h"
#include "term.h"
#include "foldfg.h"
#include "clause.h"

extern NAT dfg_LINENUMBER;    /* Defined in dfgparser.y */
LIST       ia_PROOFREQUEST;   /* A pair! */
FLAGSTORE  ia_FLAGS;

void yyerror(const char*);
int  yylex(void);		/* Defined in iascanner.l */

static SYMBOL ia_Symbol(char*, NAT);
static TERM   ia_CreateQuantifier(SYMBOL, LIST, TERM);

static __inline__ void  ia_StringFree(char* String)
{
  memory_Free(String, sizeof(char)*(strlen(String)+1));
}

static __inline__ TERM ia_TermCreate(char* Name, LIST Arguments)
/* Look up the symbol, check its arity and create the term */
{
  return term_Create(ia_Symbol(Name,list_Length(Arguments)), Arguments);
}

/**************************************************************/
/* Functions to check the arity of symbols                    */
/**************************************************************/

static void ia_SymCheck(SYMBOL, NAT);

/**************************************************************/
/* Functions that handle variable names                       */
/**************************************************************/

/* List of quantified variables in the current input formula. */
/* This list is used to find symbols that by mistake weren't  */
/* declared in the symbol declaration section                 */
/* --> free variables                                         */
/* This is a list of lists, since each time a quantifier is   */
/* reached, a new list is added to the global list.           */
static LIST ia_VARLIST;
static BOOL ia_VARDECL;

static void   ia_VarStart(void);
static void   ia_VarStop(void);
static void   ia_VarBacktrack(void);
static void   ia_VarCheck(void);
static SYMBOL ia_VarLookup(char*);

#define YY_INPUT(buf,result,max_size) \
{ \
  int c = getc(ia_in); \
  result = (c == EOF) ? YY_NULL : (buf[0] = c, 1); \
}

#define YYERROR_VERBOSE

%}

%union {
  int       number;
  char*     string;
  SYMBOL    symbol;
  TERM      term;
  LIST      list;
}

%token IA_AND IA_EQUAL IA_EQUIV IA_EXISTS IA_FALSE IA_FORALL IA_IMPLIED
%token IA_IMPLIES IA_NOT IA_OR IA_PROVE IA_TRUE
%token <number> IA_NUM
%token <string> IA_ID
%type <string>	id
%type <symbol>  binsymbol nsymbol quantsymbol
%type <term>    qterm term
%type <list>    labellist labellistopt qtermlist termlist

%%

/****************/
/* TOP RULE     */
/****************/

proofrequest:	/* empty, means EOF */	{ YYABORT; }
		| IA_PROVE '(' term ',' labellistopt ',' IA_NUM ')' '.' {
					  ia_VarCheck();
					  ia_PROOFREQUEST = list_PairCreate($3,$5);
					  flag_SetFlagIntValue(ia_FLAGS,flag_TIMELIMIT,$7);
					  YYACCEPT;
                                        }
;

/****************/
/* TERMS        */
/****************/

termlist:	term			{ $$ = list_List($1); }
		| termlist ',' term	{ $$ = list_Nconc($1, list_List($3)); }
;

term:		id		{ $$ = ia_TermCreate($1, list_Nil()); }
		| IA_TRUE	{ $$ = term_Create(fol_True(),list_Nil()); }
		| IA_FALSE	{ $$ = term_Create(fol_False(),list_Nil()); }
		| IA_NOT '(' term ')'
				{ $$ = term_Create(fol_Not(),list_List($3)); }
		| binsymbol '(' term ',' term ')'
				{ $$ = term_Create($1, list_Cons($3, list_List($5))); }
		| nsymbol '(' termlist ')'
					{ $$ = term_Create($1, $3); }
		| id '(' termlist ')'
				{ $$ = ia_TermCreate($1, $3); }
		| quantsymbol '(' '[' 	{ ia_VarStart(); }
		  qtermlist 		{ ia_VarStop(); }
		  ']' ',' term ')'
			  	{ ia_VarBacktrack();
				  $$ = ia_CreateQuantifier($1,$5,$9);
				}
		| id '(' '['		{ ia_VarStart(); }
		  qtermlist		{ ia_VarStop(); }
		  ']' ',' term ')'
				{ misc_StartUserErrorReport();
				  misc_UserErrorReport("\n Line %d: SPASS can't handle the quantifier %s.\n", dfg_LINENUMBER, $1);
				  misc_FinishUserErrorReport();
				}
;

binsymbol:	IA_EQUAL		{ $$ = fol_Equality(); }
		| IA_EQUIV		{ $$ = fol_Equiv();    }
		| IA_IMPLIED		{ $$ = fol_Implied();  }
		| IA_IMPLIES		{ $$ = fol_Implies();  }
;

nsymbol:	IA_AND			{ $$ = fol_And(); }
		| IA_OR		{ $$ = fol_Or();  }
;

quantsymbol:	IA_EXISTS		{ $$ = fol_Exist(); }
		| IA_FORALL		{ $$ = fol_All(); }
;

id:		IA_ID			{ $$ = $1; }
		| IA_NUM		{ $$ = string_IntToString($1); }
;

qtermlist:	qterm			{ $$ = list_List($1); }
		| qtermlist ',' qterm	{ $$ = list_Nconc($1, list_List($3)); }
;

qterm:		id			{ SYMBOL s = ia_Symbol($1,0);
					  if (!symbol_IsVariable(s)) {
					    misc_StartUserErrorReport();
					    misc_UserErrorReport("\n Line %d: %s",dfg_LINENUMBER,
								 symbol_Name(s));
					    misc_UserErrorReport(" is not a variable.\n");
					    misc_FinishUserErrorReport();
					  }
					  $$ = term_Create(s, list_Nil());
					}
		| id '(' id ')'		{ SYMBOL p, v;
					  p = ia_Symbol($1, 1);
					  if (!symbol_IsPredicate(p)) {
					    misc_StartUserErrorReport();
					    misc_UserErrorReport("\n Line %d: %s",dfg_LINENUMBER,
								 symbol_Name(p));
					    misc_UserErrorReport(" is not a predicate.\n");
					    misc_FinishUserErrorReport();
					  }
					  v = ia_Symbol($3, 0);
					  if (!symbol_IsVariable(v)) {
					    misc_StartUserErrorReport();
					    misc_UserErrorReport("\n Line %d: %s",dfg_LINENUMBER,
								 symbol_Name(v));
					    misc_UserErrorReport(" is not a variable.\n");
					    misc_FinishUserErrorReport();
					  }
					  $$ = term_Create(p, list_List(term_Create(v,list_Nil())));
					}
;

labellistopt:	'[' ']'			{ $$ = list_Nil(); }
		| '[' labellist ']'	{ $$ = $2; }
;

labellist:	id		        { $$ = list_List($1); }
		| labellist ',' id	{ $$ = list_Nconc($1, list_List($3)); }
;

%%


void yyerror(const char *s)
{
  misc_StartUserErrorReport();
  misc_UserErrorReport("\n Line %i: %s\n", dfg_LINENUMBER, s);
  misc_FinishUserErrorReport();

}

LIST ia_GetNextRequest(FILE* Input, FLAGSTORE Flags)
/**************************************************************
  INPUT:   An input file containing one proof request from KIV.
  RETURNS: The proof request as pair (formula, labellist),
           list_Nil(), if EOF was reached.
  EFFECT:  Reads ONE proof request from the file.
           <Input> may also be a UNIX pipe.
***************************************************************/
{
  extern FILE* ia_in;  /* defined in kivscanner */

  ia_in           = Input;
  ia_PROOFREQUEST = list_Nil();
  ia_FLAGS        = Flags;
  ia_parse();
  
  return ia_PROOFREQUEST;
}


/**************************************************************/
/* Static Functions                                           */
/**************************************************************/

static SYMBOL ia_Symbol(char* Name, NAT Arity)
/**************************************************************
  INPUT:   The name of a symbol and the actual arity of the symbol.
  RETURNS: The corresponding SYMBOL.
  EFFECT:  This function checks if the <Name> was declared as
           symbol or variable. If not, an error message is printed
	   to stderr.
	   The <Name> is deleted.
***************************************************************/
{
  SYMBOL symbol;

  symbol = symbol_Lookup(Name);
  if (symbol != 0) {
    ia_StringFree(Name);
    ia_SymCheck(symbol, Arity); /* Check the arity */
  } else {
    /* Variable */
    if (Arity > 0) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %d: Undefined symbol %s.\n",dfg_LINENUMBER,Name);
      misc_FinishUserErrorReport();
    }
    symbol = ia_VarLookup(Name);
  }
  return symbol;
}


static TERM ia_CreateQuantifier(SYMBOL Symbol, LIST VarTermList, TERM Term)
/**************************************************************
  INPUT:   A quantifier symbol, a list possibly containing sorts,
           and a term.
  RETURNS: The created quantifier term..
***************************************************************/
{
  LIST varlist, sortlist, scan;
  TERM helpterm;

  /* First collect the variable symbols in varlist and the sorts in sortlist */
  varlist = sortlist = list_Nil();
  for ( ; !list_Empty(VarTermList); VarTermList = list_Pop(VarTermList)) {
    helpterm = list_Car(VarTermList);
    if (term_IsVariable(helpterm)) {
      varlist = list_Nconc(varlist, list_List((POINTER)term_TopSymbol(helpterm)));
      term_Delete(helpterm);
    } else {
      SYMBOL var = term_TopSymbol(term_FirstArgument(helpterm));
      varlist  = list_Nconc(varlist, list_List((POINTER)var));
      sortlist = list_Nconc(sortlist, list_List(helpterm));
    }
  }

  varlist = list_PointerDeleteDuplicates(varlist);
  /* Now create terms from the variables */
  for (scan = varlist; !list_Empty(scan); scan = list_Cdr(scan))
    list_Rplaca(scan, term_Create((SYMBOL)list_Car(scan), list_Nil()));

  if (!list_Empty(sortlist)) {
    if (symbol_Equal(fol_All(), Symbol)) {
      /* The conjunction of all sortterms implies the Term */
      if (symbol_Equal(fol_Or(), term_TopSymbol(Term))) {
	/* Special treatment if <Term> is a term with "or" like */
	/* in clauses: add all sort terms negated to the args    */
	/* of the "or" */
	for (scan = sortlist; !list_Empty(scan); scan = list_Cdr(scan))
	  /* Negate the sort terms */
	  list_Rplaca(scan, term_Create(fol_Not(), list_List(list_Car(scan))));
	sortlist = list_Nconc(sortlist, term_ArgumentList(Term));
	term_RplacArgumentList(Term, sortlist);
      } else {
	/* No "or" term, so build the implication term */
	if (list_Empty(list_Cdr(sortlist))) {
	  /* Only one sort term */
	  list_Rplacd(sortlist, list_List(Term));
	  Term = term_Create(fol_Implies(), sortlist);
	} else {
	  /* More than one sort term */
	  helpterm = term_Create(fol_And(), sortlist);
	  Term = term_Create(fol_Implies(), list_Cons(helpterm, list_List(Term)));
	}
      }
    } else if (symbol_Equal(fol_Exist(), Symbol)) {
      /* Quantify the conjunction of all sort terms and <Term> */
      if (symbol_Equal(fol_And(), term_TopSymbol(Term))) {
	/* Special treatment if <Term> has an "and" as top symbol: */
	/* just add the sort terms to the args of the "and".       */
	sortlist = list_Nconc(sortlist, term_ArgumentList(Term));
	term_RplacArgumentList(Term, sortlist);
      } else {
	sortlist = list_Nconc(sortlist, list_List(Term));
	Term = term_Create(fol_And(), sortlist);
      }
    }
  }
  helpterm = fol_CreateQuantifier(Symbol, varlist, list_List(Term));
  return helpterm;
}


/**************************************************************/
/* Functions for the Symbol Table                             */
/**************************************************************/

static void ia_SymCheck(SYMBOL Symbol, NAT Arity)
/**************************************************************
  INPUT:   A symbol and the current arity of this symbol.
  RETURNS: Nothing.
  EFFECT:  This function compares the previous arity of 'Symbol'
           with the actual 'Arity'. If these values differ
	   a warning is printed to stderr and the program exits.
***************************************************************/
{
  /* Check if the specified arity corresponds with the actual arity */
  if (symbol_Arity(Symbol) != symbol_ArbitraryArity() &&
      symbol_Arity(Symbol) != Arity) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Line %u: Symbol %s", dfg_LINENUMBER, symbol_Name(Symbol));
    misc_UserErrorReport(" was declared with arity %u.\n", symbol_Arity(Symbol));
    misc_FinishUserErrorReport();
  }
}


/**************************************************************/
/* Functions for the Variable Table                           */
/**************************************************************/
  
typedef struct {
  char*  name;
  SYMBOL symbol;
} IA_VARENTRY, *IA_VAR;

static __inline__ char* ia_VarName(IA_VAR Entry)
{
  return Entry->name;
}

static __inline__ SYMBOL ia_VarSymbol(IA_VAR Entry)
{
  return Entry->symbol;
}

static __inline__ IA_VAR ia_VarCreate(void)
{
  return (IA_VAR) memory_Malloc(sizeof(IA_VARENTRY));
}

static void ia_VarFree(IA_VAR Entry)
{
  ia_StringFree(Entry->name);
  memory_Free(Entry, sizeof(IA_VARENTRY));
}

static void ia_VarStart(void)
{
  ia_VARLIST = list_Push(list_Nil(), ia_VARLIST);
  ia_VARDECL = TRUE;
}

static void ia_VarStop(void)
{
  ia_VARDECL = FALSE;
}

static void ia_VarBacktrack(void)
{
  list_DeleteWithElement(list_Top(ia_VARLIST), (void (*)(POINTER)) ia_VarFree);
  ia_VARLIST = list_Pop(ia_VARLIST);
}

static void ia_VarCheck(void)
/* Should be called after a complete clause or formula was parsed */
{
  if (!list_Empty(ia_VARLIST)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In ia_VarCheck: List of variables should be empty!\n");
    misc_FinishErrorReport();
  }
  symbol_ResetStandardVarCounter();
}

static SYMBOL ia_VarLookup(char* Name)
/**************************************************************
  INPUT:   A variable name.
  RETURNS: The corresponding variable symbol.
  EFFECT:  If the variable name was quantified before, the
           corresponding symbol is returned and the <Name> is freed.
	   If the variable name was not quantified, and <ia_VARDECL>
	   is TRUE, a new variable is created, else an error
	   message is printed and the program exits.
***************************************************************/
{
  LIST   scan, scan2;
  SYMBOL symbol;

  symbol = symbol_Null();

  scan  = ia_VARLIST;
  scan2 = list_Nil();
  while (!list_Empty(scan) && list_Empty(scan2)) {
    scan2 = list_Car(scan);
    while (!list_Empty(scan2) &&
	   !string_Equal(ia_VarName(list_Car(scan2)), Name))
      scan2 = list_Cdr(scan2);
    scan = list_Cdr(scan);
  }

  if (!list_Empty(scan2)) {
    /* Found variable */
    ia_StringFree(Name);
    symbol = ia_VarSymbol(list_Car(scan2));
  } else {
    /* Variable not found */
    if (ia_VARDECL) {
      IA_VAR newEntry = ia_VarCreate();
      newEntry->name   = Name;
      newEntry->symbol = symbol_CreateStandardVariable();
      /* Add <newentry> to the first list in ia_VARLIST */
      list_Rplaca(ia_VARLIST, list_Cons(newEntry,list_Car(ia_VARLIST)));
      symbol = ia_VarSymbol(newEntry);
    } else {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %u: Free Variable %s.\n", dfg_LINENUMBER, Name);
      misc_FinishUserErrorReport();
    }
  }
  return symbol;
}
