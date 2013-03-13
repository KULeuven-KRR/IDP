/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                  PARSER FOR DFG SYNTAX                 * */
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
/* $Revision: 1.13 $                                        * */
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


/* $RCSfile: dfgparser.y,v $ */

%{

#include <ctype.h>
#include "dfg.h"
#include "symbol.h"
#include "term.h"
#include "foldfg.h"
#include "strings.h"
#include "eml.h"
	
  /* Used for delayed parsing of plain clauses */
  typedef struct {
    LIST constraint;
    LIST antecedent;
    LIST succedent;
    TERM selected;
  } DFG_PLAINCLAUSEHELP, *DFG_PLAINCLAUSE;

  static DFGDESCRIPTION      dfg_DESC;
  static LIST                dfg_AXIOMLIST;
  static LIST                dfg_CONJECLIST;
  static LIST                dfg_SORTDECLLIST;
  static LIST                dfg_INCLUDELIST;
  /* symbol precedence explicitly defined by user */
  static LIST                dfg_USERPRECEDENCE;
  static LIST                dfg_USERSELECTION;
  static LIST                dfg_CLAXRELATION;
  static LIST                dfg_CLAXAXIOMS;
  static LIST                dfg_AXCLAUSES;
  static LIST                dfg_CONCLAUSES;
  static LIST                dfg_PROOFLIST;     /* list_of_proofs */
  static LIST                dfg_TERMLIST;      /* list_of_terms  */
  static LIST                dfg_PLAINAXCLAUSES; /* list of DFG_PLAINCLAUSE */
  static LIST                dfg_PLAINCONCLAUSES; /* list of DFG_PLAINCLAUSE */
  static LIST                dfg_TEMPPLAINCLAUSES; /* temporal list of DFG_PlAINCLAUSE, until they are concatenated to dfg_PLAINAXCLAUSES or dfg_PLAINCONCLAUSES */
  static TERM                dfg_SELECTED_LITERAL; /* last selected literal */
  static BOOL                dfg_IGNORE;      /* tokens are ignored while TRUE */
  static BOOL		   dfg_IGNORESETTINGS; /* issue a warning for included file with SPASS settings */
  static FLAGSTORE           dfg_FLAGS;
  static PRECEDENCE          dfg_PRECEDENCE;

  /* used also in the scanner */
  NAT                        dfg_LINENUMBER;
  BOOL			   dfg_IGNORETEXT;

  void yyerror(const char*);
  int  yylex(void);		/* Defined in dfgscanner.l */

  static void   dfg_SymbolDecl(int, char*, int);
  static SYMBOL dfg_Symbol(char*, NAT);
  static void   dfg_SubSort(char*, char*);
  static void   dfg_SymbolGenerated(SYMBOL, BOOL, LIST);
  static void   dfg_TranslPairDecl(char*, char*);
  static TERM   dfg_CreateQuantifier(SYMBOL, LIST, TERM);

  static __inline__ TERM dfg_TermCreate(char* Name, LIST Arguments)
    /***************************************************************
  INPUT:   A string <Name> and a list of terms <Arguments>.
  RETURNS: Does a symbol lookup for <Name> and creates out of
           the found or otherwise created new symbol and <Arguments>
           a new term.
  CAUTION: Deletes the string <Name>.
    ***************************************************************/
    {
      SYMBOL s;
      NAT    arity;
      arity = list_Length(Arguments);
      s = dfg_Symbol(Name, arity); /* Frees the string */
      if (!symbol_IsVariable(s) && !symbol_IsFunction(s)) {
	misc_StartUserErrorReport();
	misc_UserErrorReport("\n Line %d: is not a function.\n", dfg_LINENUMBER);
	misc_FinishUserErrorReport();
      }
      return term_Create(s, Arguments);
    }

  static __inline__ TERM dfg_AtomCreate(char* Name, LIST Arguments)
    /* Look up the symbol, check its arity and create the atom term */
    {
      SYMBOL s;
      s = dfg_Symbol(Name, list_Length(Arguments)); /* Frees the string */
      if (symbol_IsVariable(s) || !symbol_IsPredicate(s)) {
	misc_StartUserErrorReport();
	misc_UserErrorReport("\n Line %d: Symbol is not a predicate.\n", dfg_LINENUMBER);
	misc_FinishUserErrorReport();
      }
      return term_Create(s, Arguments);
    }

  static __inline__ void dfg_DeleteStringList(LIST List)
    {
      list_DeleteWithElement(List, (void (*)(POINTER)) string_StringFree);
    }

  /**************************************************************/
  /* Functions that handle symbols with unspecified arity       */
  /**************************************************************/

  /* The symbol list holds all symbols whose arity wasn't       */
  /* specified in the symbol declaration section.               */
  /* If the arity of a symbol was not specified in this section */
  /* it is first set to 0. If the symbol occurs with always the */
  /* same arity 'A' the arity of this symbol is set to 'A'.     */
  static LIST   dfg_SYMBOLLIST;

  static void dfg_SymAdd(SYMBOL);
  static void dfg_SymCheck(SYMBOL, NAT);
  static void dfg_SymCleanUp(void);

  /**************************************************************/
  /* Functions that handle variable names                       */
  /**************************************************************/

  /* List of quantified variables in the current input formula. */
  /* This list is used to find symbols that by mistake weren't  */
  /* declared in the symbol declaration section                 */
  /* --> free variables                                         */
  /* This is a list of lists, since each time a quantifier is   */
  /* reached, a new list is added to the global list.           */
  static LIST dfg_VARLIST;
  static BOOL dfg_VARDECL;

  static void   dfg_VarStart(void);
  static void   dfg_VarStop(void);
  static void   dfg_VarBacktrack(void);
  static void   dfg_VarCheck(void);
  static SYMBOL dfg_VarLookup(char*);
  static DFG_PLAINCLAUSE dfg_PlainClauseCreate(void);
  static void dfg_PlainClauseFree(DFG_PLAINCLAUSE Clause);

  static char * dfg_StripQuotes(char * str); /* strip quotes from include filename */

  /*
    #define DEBUG_PARSER
  */
#ifdef DEBUG_PARSER
#define YYERROR_VERBOSE
#define YYDEBUG 1000

  dfg_debug = 1;
#endif
  %}

%union {
  int       number;
  char*     string;
  SYMBOL    symbol;
  SPROPERTY property;
  TERM      term;
  LIST      list;
  DFG_STATE state;
  BOOL      bool;
}

%token DFG_AND DFG_AUTHOR DFG_AXIOMS DFG_BEGPROB DFG_BY DFG_CLAUSE
%token DFG_CLOSEBRACE DFG_CLSLIST DFG_CNF DFG_CONJECS DFG_DATE DFG_DECLLIST DFG_INCLUDELIST DFG_INCLUDE
%token DFG_DESC DFG_DESCLIST DFG_DNF DFG_DOMPRED DFG_ENDLIST DFG_ENDPROB
%token DFG_EQUAL DFG_EQUIV DFG_EXISTS DFG_FALSE DFG_FORMLIST DFG_FORMULA
%token DFG_FORALL DFG_FREELY DFG_FUNC DFG_GENERATED DFG_GENSET DFG_HYPOTH
%token DFG_IMPLIED DFG_IMPLIES DFG_LOGIC DFG_NAME DFG_NOT DFG_OPENBRACE
%token DFG_XOR DFG_NOR DFG_NAND
%token DFG_OPERAT DFG_OR DFG_PREC DFG_CLFORE DFG_PRED DFG_PRDICAT DFG_PRFLIST DFG_QUANTIF
%token DFG_SATIS DFG_SELECT DFG_SETFLAG DFG_SETTINGS DFG_SYMLIST DFG_SORT DFG_SORTS
%token DFG_STATUS DFG_STEP DFG_SUBSORT DFG_TERMLIST DFG_TRUE DFG_UNKNOWN
%token DFG_UNSATIS DFG_VERSION DFG_END_OF_CONSTRAINT_LITERALS DFG_END_OF_ANTECEDENT_LITERALS
%token DFG_LITERAL_IS_SELECTED

 /* Extension for extended modal logics */
%token DFG_EML DFG_PROP_FORMULA DFG_REL_FORMULA DFG_SPECIALFORMLIST DFG_TRANSLPAIRS 
%token DFG_BOX DFG_DIA DFG_TEST DFG_DOMAIN DFG_RANGE
%token DFG_COMP DFG_SUM DFG_CONV DFG_IDENTITY DFG_DIVERSITY DFG_DOMRESTR DFG_RANRESTR

%token <number>  DFG_NUM DFG_MINUS1
%token <string>  DFG_ID DFG_TEXT
%token <string>  single_quoted
%type <number>	 assoclistopt assoclist number
%type <string>	 id labelopt id_or_formula anysymbol filename
%type <symbol>   binsymbol nsymbol quantsymbol
%type <property> statopt
%type <term>	 atom clause cnfclause cnfclauseopt cnfclausebody formula
%type <term>     formulaopt lit qterm term
%type <list>	 arglist termlist atomlist cnfclausesopt formulalistopt
%type <list>     labellist litlist litlist_ws selected_litlist_ws parentlist qtermlist 
%type <list>     sortlist funclist
%type <list>     formula_selection name_list include includes
%type <state>	 log_state
%type <bool>     origin freelyopt optargs

 /* Extension for extended modal logics */
%type <symbol>  relpropbinsymbol relnsymbol
%type <term>    propformula propformulaopt relformula relformulaopt
%type <list>	proparglist relarglist specialformulalistopt

%error-verbose

%%

 /****************/
 /* TOP RULE     */
 /****************/

problem:	DFG_BEGPROB '(' id ')' '.'
description
logicalpart
includelistopt
settinglistsopt
DFG_ENDPROB '.'		{ string_StringFree($3);
		  YYACCEPT; /* Stop immediately */ }
;

/****************/
/* DESCRIPTION  */
/****************/

description:	DFG_DESCLIST '.'
name
author
versionopt
logicopt
status
desctext
dateopt
DFG_ENDLIST '.'
;

name: 		DFG_NAME '(' DFG_TEXT ')' '.'
{ dfg_DESC->name = $3; }
;

author:		DFG_AUTHOR '(' DFG_TEXT ')' '.'
{ dfg_DESC->author = $3; }
;

status:		DFG_STATUS '(' log_state ')' '.'
{ dfg_DESC->status = $3; }
;

desctext:	DFG_DESC '(' DFG_TEXT ')' '.'
{ dfg_DESC->description = $3; }
;

versionopt:	/* empty */
| DFG_VERSION '(' DFG_TEXT ')' '.'
{ dfg_DESC->version = $3; }
;

logicopt:	/* empty */
| DFG_LOGIC '(' DFG_TEXT ')' '.'
{ dfg_DESC->logic = $3; }
;

dateopt:	/* empty */
| DFG_DATE '(' DFG_TEXT ')' '.'
{ dfg_DESC->date = $3; }
;

log_state:	DFG_SATIS		{ $$ = DFG_SATISFIABLE; }
| DFG_UNSATIS		{ $$ = DFG_UNSATISFIABLE; }
| DFG_UNKNOWN		{ $$ = DFG_UNKNOWNSTATE; }
;

/****************/
/* LOGICAL PART */
/****************/

logicalpart:	symbollistopt
declarationlistopt
formulalistsopt
specialformulalistsopt
clauselistsopt
prooflistsopt
listOfTermsopt
;

/****************/
/* SYMBOLS      */
/****************/

symbollistopt:	/* empty */
| DFG_SYMLIST '.'
functionsopt
predicatesopt
sortsopt
operatorsopt
quantifiersopt
translpairsopt
DFG_ENDLIST '.'
;

functionsopt:	/* empty */
| DFG_FUNC '[' functionlist ']' '.'
;

functionlist:	func
| functionlist ',' func
;

func:		id			{ dfg_SymbolDecl(DFG_FUNC, $1, -2); }
| '(' id ',' number ')'
{ dfg_SymbolDecl(DFG_FUNC, $2, $4); }
;

predicatesopt:	/* empty */
| DFG_PRDICAT '[' predicatelist ']' '.'
;

predicatelist:	pred
| predicatelist ',' pred
;

pred:		id			{ dfg_SymbolDecl(DFG_PRDICAT, $1, -2); }
| '(' id ',' number ')'	{ dfg_SymbolDecl(DFG_PRDICAT, $2, $4); }
;

sortsopt:	/* empty */
| DFG_SORTS '[' sortlist ']' '.'
;

sortlist:	id			{ dfg_SymbolDecl(DFG_PRDICAT, $1, 1); }
| sortlist ',' id	{ dfg_SymbolDecl(DFG_PRDICAT, $3, 1); }
;

operatorsopt:	/* empty */
| DFG_OPERAT '[' operatorlist ']' '.'
;

operatorlist:	op
| operatorlist ',' op
;

op:		id			{ dfg_SymbolDecl(DFG_OPERAT, $1, -2); }
| '(' id ',' number ')'	{ dfg_SymbolDecl(DFG_OPERAT, $2, $4); }
;

quantifiersopt:	/* empty */
| DFG_QUANTIF '[' quantifierlist ']' '.'
;

quantifierlist:	quant
| quantifierlist ',' quant
;

quant:		id			{ dfg_SymbolDecl(DFG_QUANTIF, $1, -2); }
| '(' id ',' number ')'	{ dfg_SymbolDecl(DFG_QUANTIF, $2, $4); }
;

number:		DFG_MINUS1		{ $$ = -1; }
| DFG_NUM		{ $$ = $1; }
;

translpairsopt:	/* empty */
| DFG_TRANSLPAIRS '[' translpairlist ']' '.'
;

translpairlist:	translpair
| translpairlist ',' translpair
;

translpair:	| '(' id ',' id ')'		{ dfg_TranslPairDecl($2, $4); }
;

/****************/
/* DECLARATIONS */
/****************/

declarationlistopt:	/* empty */
| DFG_DECLLIST '.' 
decllistopt
DFG_ENDLIST '.'
;

decllistopt:	/* empty */
| decllistopt decl
;

decl:		gendecl
| DFG_SUBSORT '(' id ',' id ')' '.'
{ dfg_SubSort($3,$5); }
| atom '.'		{ dfg_SORTDECLLIST = list_Nconc(dfg_SORTDECLLIST,list_List(list_PairCreate(NULL,$1))); }
| DFG_PRED '(' id ',' sortdecl ')' '.'	/* ignored */
{ string_StringFree($3); }
| DFG_FORALL '(' '['	{ dfg_VarStart(); }
qtermlist 		{ dfg_VarStop();  }
']' ',' atom ')' '.'	{ TERM term;
  dfg_VarBacktrack();
  dfg_VarCheck();
  term = dfg_CreateQuantifier(fol_All(),$5,$9);
  dfg_SORTDECLLIST = list_Nconc(dfg_SORTDECLLIST,list_List(list_PairCreate(NULL,term)));
}
;

gendecl:	DFG_SORT id freelyopt DFG_GENERATED DFG_BY '[' funclist ']' '.'
{ dfg_SymbolGenerated(dfg_Symbol($2,1), $3, $7); }
;

freelyopt:	/* empty */		{ $$ = FALSE; }
| DFG_FREELY		{ $$ = TRUE; }
;

funclist:	id			{ $$ = list_List($1); }
| funclist ',' id       { $$ = list_Cons($3, $1); }
;

sortdecl:	id			{ string_StringFree($1); }
| sortdecl ',' id	{ string_StringFree($3); }
;

/****************/
/* FORMULAS     */
/****************/

formulalist:	DFG_FORMLIST '(' origin ')' '.'
formulalistopt
DFG_ENDLIST '.'	{ list_NReverse($6);
		  if ($3) /* Axioms */
		    dfg_AXIOMLIST = list_Nconc(dfg_AXIOMLIST, $6);
		  else
		    dfg_CONJECLIST = list_Nconc(dfg_CONJECLIST, $6);
}
;

origin:		DFG_AXIOMS		{ $$ = TRUE;  }
| DFG_CONJECS		{ $$ = FALSE; }
;

formulalistsopt:	/* empty */
| formulalistsopt formulalist
;

formulalistopt:	/* empty */		{ $$ = list_Nil(); }
| formulalistopt DFG_FORMULA '(' formulaopt labelopt ')' '.'
{ LIST pair;
  if ($4 == NULL) { /* No term */
    if ($5 != NULL)
      string_StringFree($5);
    $$ = $1;
  } else {
    pair = list_PairCreate($5, $4);
    $$ = list_Cons(pair, $1);
  }
  dfg_VarCheck();
}
;

labelopt:	/* empty */		{ $$ = NULL; }
| ',' id		{ $$ = $2;   }
;

formula:	atom		{ $$ = $1; }
| DFG_NOT '(' formula ')'
{ $$ = dfg_IGNORE ? NULL : term_Create(fol_Not(),list_List($3)); }
| binsymbol '(' formula ',' formula ')'
{ $$ = dfg_IGNORE ? NULL : term_Create($1, list_Cons($3, list_List($5))); }
| nsymbol '(' arglist ')'
{ $$ = dfg_IGNORE ? NULL : term_Create($1, $3); }
| quantsymbol '(' '[' 	{ dfg_VarStart(); }
qtermlist 		{ dfg_VarStop(); }
']' ',' formula ')'
{ dfg_VarBacktrack();
  $$ = dfg_IGNORE ? NULL : dfg_CreateQuantifier($1,$5,$9);
}
;

formulaopt:	/* empty */		{ $$ = NULL; }
| formula		{ $$ = $1; }
;

arglist:	formula
{ $$ = dfg_IGNORE ? list_Nil() : list_List($1); }
| arglist ',' formula
{ $$ = dfg_IGNORE ? $1 : list_Nconc($1, list_List($3)); }
;

binsymbol:	DFG_EQUIV		{ $$ = fol_Equiv();    }
| DFG_IMPLIED		{ $$ = fol_Implied();  }
| DFG_IMPLIES		{ $$ = fol_Implies();  }
| DFG_XOR			{ $$ = fol_Xor();      }
| DFG_NOR			{ $$ = fol_Nor();      }
| DFG_NAND          { $$ = fol_Nand();     }
;

nsymbol:	DFG_AND			{ $$ = fol_And(); }
| DFG_OR		{ $$ = fol_Or();  }
;

quantsymbol:	DFG_EXISTS		{ $$ = fol_Exist(); }
| DFG_FORALL		{ $$ = fol_All(); }
;

id:		DFG_ID			{ if (dfg_IGNORE) {
		    string_StringFree($1);
		    $$ = NULL;
		  } else
					    $$ = $1;
}
| DFG_NUM
{ $$ = dfg_IGNORE ? NULL : string_IntToString($1); }
| DFG_SETFLAG
{ $$ = dfg_IGNORE ? NULL : string_StringCopy("set_flag"); }
| DFG_DOMPRED
{ $$ = dfg_IGNORE ? NULL : string_StringCopy("set_DomPred"); }
| DFG_PREC
{ $$ = dfg_IGNORE ? NULL : string_StringCopy("set_precedence"); }
| DFG_CLFORE
{ $$ = dfg_IGNORE ? NULL : string_StringCopy("set_ClauseFormulaRelation"); }
| DFG_SELECT
{ $$ = dfg_IGNORE ? NULL : string_StringCopy("set_selection"); }
;

qtermlist:	qterm
{ $$ = dfg_IGNORE ? list_Nil() : list_List($1); }
| qtermlist ',' qterm
{ $$ = dfg_IGNORE ? $1 : list_Nconc($1, list_List($3)); }
;

qterm:		id
{ if (!dfg_IGNORE) {
    SYMBOL s = dfg_Symbol($1,0);
    if (!symbol_IsVariable(s)) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %d: Symbol is not a variable.\n",dfg_LINENUMBER);
      misc_FinishUserErrorReport();
    }
    $$ = term_Create(s, list_Nil());
  }
}
| id '(' id ')'
{ if (!dfg_IGNORE) {
    SYMBOL p, v;
    p = dfg_Symbol($1, 1);
    if (!symbol_IsPredicate(p)) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %d: Symbol is not a predicate.\n",dfg_LINENUMBER);
      misc_FinishUserErrorReport();
    }
    v = dfg_Symbol($3, 0);
    if (!symbol_IsVariable(v)) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %d: Symbol is not a variable.\n",dfg_LINENUMBER);
      misc_FinishUserErrorReport();
    }
    $$ = term_Create(p, list_List(term_Create(v,list_Nil())));
  }
}
;

/************************/
/* SPECIAL FORMULAE     */
/************************/

specialformulalist:	DFG_SPECIALFORMLIST '(' origin ',' DFG_EML ')' '.'
specialformulalistopt
DFG_ENDLIST '.'	{ list_NReverse($8);
			  if ($3) /* Axioms */
			    dfg_AXIOMLIST = list_Nconc(dfg_AXIOMLIST, $8);
			  else
			    dfg_CONJECLIST = list_Nconc(dfg_CONJECLIST, $8);
			  flag_SetFlagIntValue(dfg_FLAGS, flag_EML, flag_EMLON);
}
;

specialformulalistsopt:	/* empty */
| specialformulalistsopt specialformulalist
;

specialformulalistopt:	/* empty */		{ $$ = list_Nil(); }
| specialformulalistopt DFG_FORMULA '(' formulaopt labelopt ')' '.'
  /* First order formula for specifying additonal (frame) properties */
{ LIST pair;
  if ($4 == NULL) { /* No term */
    if ($5 != NULL)
      string_StringFree($5);
    $$ = $1;
  } else {
    pair = list_PairCreate($5, $4);
    $$ = list_Cons(pair, $1);
  }
  dfg_VarCheck();
}
| specialformulalistopt DFG_PROP_FORMULA '(' propformulaopt labelopt ')' '.'
  /* Formula of extended (propsitional) modal logic */
{ LIST pair;
  if ($4 == NULL) { /* No term */
    if ($5 != NULL)
      string_StringFree($5);
    $$ = $1;
  } else {
    pair = list_PairCreate($5, $4);
    $$ = list_Nconc($1, list_List(pair));
  }
}
| specialformulalistopt DFG_REL_FORMULA '(' relformulaopt labelopt ')' '.'
  /* Formula of extended (propsitional) modal logic */
{ LIST pair;
  if ($4 == NULL) { /* No term */
    if ($5 != NULL)
      string_StringFree($5);
    $$ = $1;
  } else {
    pair = list_PairCreate($5, $4);
    $$ = list_Nconc($1, list_List(pair));
  }
}
;

/* FORMULAE for extended (propsitional) modal logics */

propformulaopt:	/* empty */	{ $$ = NULL; }
| propformula	{ $$ = $1; }
;

relformulaopt:	/* empty */	{ $$ = NULL; }
| relformula	{ $$ = term_Create(eml_Box(), 
				   list_Cons(term_Create(fol_Not(), list_List($1)), 
					     list_List(term_Create(fol_False(), list_Nil())))); 
    /* since r = 1 iff (r':1)' = 1 */
}
;

proparglist:	propformula			{ $$ = list_List($1); }
| proparglist ',' propformula	{ $$ = list_Nconc($1, list_List($3)); }
;

relarglist:	relformula				{ $$ = list_List($1); }
| relarglist ',' relformula	{ $$ = list_Nconc($1, list_List($3)); }
;

propformula:	id		
{ $$ = dfg_IGNORE ? NULL : dfg_AtomCreate($1, list_Nil()); }
| DFG_TRUE	
{ $$ = dfg_IGNORE ? NULL : term_Create(fol_True(),list_Nil()); }
| DFG_FALSE	
{ $$ = dfg_IGNORE ? NULL : term_Create(fol_False(),list_Nil()); }
| DFG_NOT '(' propformula ')'
{ $$ = dfg_IGNORE ? NULL : term_Create(fol_Not(),list_List($3)); }
| DFG_DOMAIN '(' relformula ')'
{ $$ = dfg_IGNORE ? NULL : term_Create(eml_Domain(),list_List($3)); }
| DFG_RANGE '(' relformula ')'
{ $$ = dfg_IGNORE ? NULL : term_Create(eml_Range(),list_List($3)); }
| binsymbol '(' propformula ',' propformula ')'
{ $$ = dfg_IGNORE ? NULL : term_Create($1, list_Cons($3, list_List($5))); }
| nsymbol '(' proparglist ')'
{ $$ = dfg_IGNORE ? NULL : term_Create($1, $3); }
| relpropbinsymbol '(' relformula ',' propformula ')'
{ $$ = dfg_IGNORE ? NULL : term_Create($1, list_Cons($3, list_List($5))); }
;

relformula:	id		
{ $$ = dfg_IGNORE ? NULL : dfg_AtomCreate($1, list_Nil()); }
| DFG_TRUE	
{ $$ = dfg_IGNORE ? NULL : term_Create(fol_True(),list_Nil()); }
| DFG_FALSE	
{ $$ = dfg_IGNORE ? NULL : term_Create(fol_False(),list_Nil()); }
| DFG_NOT '(' relformula ')'
{ $$ = dfg_IGNORE ? NULL : term_Create(fol_Not(),list_List($3)); }
| DFG_IDENTITY	
{ $$ = dfg_IGNORE ? NULL : term_Create(eml_Id(),list_Nil()); }
| DFG_DIVERSITY	
{ $$ = dfg_IGNORE ? NULL : term_Create(eml_Div(),list_Nil()); }
| DFG_CONV '(' relformula ')'
{ $$ = dfg_IGNORE ? NULL : term_Create(eml_Conv(),list_List($3)); }
| DFG_TEST '(' propformula ')'
{ $$ = dfg_IGNORE ? NULL : term_Create(eml_Test(),list_List($3)); }
| DFG_DOMRESTR '(' relformula ',' propformula ')'
{ $$ = dfg_IGNORE ? NULL : term_Create(eml_DomRestr(), list_Cons($3, list_List($5))); }
| DFG_RANRESTR '(' relformula ',' propformula ')'
{ $$ = dfg_IGNORE ? NULL : term_Create(eml_RanRestr(), list_Cons($3, list_List($5))); }
| binsymbol '(' relformula ',' relformula ')'
{ $$ = dfg_IGNORE ? NULL : term_Create($1, list_Cons($3, list_List($5))); }
| relnsymbol '(' relarglist ')'
{ $$ = dfg_IGNORE ? NULL : term_Create($1, $3); }
;

relpropbinsymbol:	DFG_BOX		{ $$ = eml_Box();  }
| DFG_DIA	{ $$ = eml_Dia();  }
;

relnsymbol:	DFG_AND			{ $$ = fol_And();  }
| DFG_OR		{ $$ = fol_Or();   }
| DFG_COMP		{ $$ = eml_Comp(); }
| DFG_SUM		{ $$ = eml_Sum();  }
;


/****************/
/* CLAUSES      */
/****************/

clauselistsopt:	/* empty */
| clauselistsopt clauselist
;

clauselist:	DFG_CLSLIST '(' origin ',' DFG_CNF ')' '.'
cnfclausesopt
DFG_ENDLIST '.'
{ list_NReverse($8);
  if (!list_Empty(dfg_TEMPPLAINCLAUSES)) {
    /* Plain clauses are temporarily stored in the list dfg_TEMPPLAINCLAUSES.
       They need to be concatenated to their respective axiom or conjecture 
       plain clause lists. */
    if ($3) /* Axioms */
      dfg_PLAINAXCLAUSES = list_Nconc(dfg_PLAINAXCLAUSES, list_NReverse(dfg_TEMPPLAINCLAUSES));
    else
      dfg_PLAINCONCLAUSES = list_Nconc(dfg_PLAINCONCLAUSES, list_NReverse(dfg_TEMPPLAINCLAUSES));
    dfg_TEMPPLAINCLAUSES = list_Nil();
  }
  if (!list_Empty($8)) {
    if ($3) /* Axioms */
      dfg_AXCLAUSES = list_Nconc(dfg_AXCLAUSES, $8);
    else
      dfg_CONCLAUSES = list_Nconc(dfg_CONCLAUSES, $8);
  }
}
| DFG_CLSLIST '(' origin ',' DFG_DNF ')' '.'
{ stack_Push((POINTER)dfg_IGNORE); dfg_IGNORE = TRUE; }
dnfclausesopt
DFG_ENDLIST '.'
{ dfg_IGNORE = (BOOL)stack_PopResult(); }
;

cnfclausesopt:	/* empty */		{ $$ = list_Nil(); }
| cnfclausesopt DFG_CLAUSE '(' cnfclauseopt labelopt ')' '.'
{ LIST pair;
  if ($4 == NULL) { /* No clause */
    if ($5 != NULL)
      string_StringFree($5);
    $$ = $1;
  } else {
    pair = list_PairCreate($5, $4);
    $$ = list_Cons(pair, $1);
  }
  dfg_VarCheck();
}
;

cnfclauseopt:	/* empty */		{ $$ = NULL; }
| cnfclause		{ $$ = $1; }
| {dfg_VarStart();} litlist_ws DFG_END_OF_CONSTRAINT_LITERALS selected_litlist_ws DFG_END_OF_ANTECEDENT_LITERALS litlist_ws {dfg_VarStop();}
{ dfg_VarBacktrack();
  if (dfg_IGNORE) 
    $$ = NULL;
  else {
    /* C || A -> S */
    DFG_PLAINCLAUSE clause = dfg_PlainClauseCreate();
    clause->constraint = $2;
    clause->antecedent = $4;
    clause->succedent = $6;
    clause->selected = dfg_SELECTED_LITERAL;
    dfg_SELECTED_LITERAL = (TERM) NULL;
    dfg_TEMPPLAINCLAUSES = list_Cons(clause,dfg_TEMPPLAINCLAUSES);
    $$ = NULL;
  }
}
;

cnfclause:	cnfclausebody		{ $$ = $1; }
| DFG_FORALL '(' '['	{ dfg_VarStart(); }
qtermlist		{ dfg_VarStop();  }
']' ',' cnfclausebody ')'
{ dfg_VarBacktrack();
  $$ = dfg_IGNORE ? NULL : dfg_CreateQuantifier(fol_All(),$5,$9);
}
;

cnfclausebody:	DFG_OR '(' litlist ')'
{ $$ = dfg_IGNORE ? NULL : term_Create(fol_Or(), $3); }
;

litlist:	/* empty */ {$$ = list_Nil();}
| lit
{ $$ = dfg_IGNORE ? list_Nil() : list_List($1); }
| litlist ',' lit
{ $$ = dfg_IGNORE ? $1 : list_Nconc($1, list_List($3)); }
;

litlist_ws: /* empty */ {$$ = list_Nil();}
| litlist_ws lit
{ $$ = dfg_IGNORE ? $1 : list_Nconc($1, list_List($2)); }	
;

selected_litlist_ws: /* empty */ {$$ = list_Nil();}
| selected_litlist_ws lit
{ $$ = dfg_IGNORE ? $1 : list_Nconc($1, list_List($2)); }	
| selected_litlist_ws lit DFG_LITERAL_IS_SELECTED
{ if (dfg_IGNORE)
    $$ = $1;
  else {
    if(dfg_SELECTED_LITERAL != NULL) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Trying to select two literals in a clause.");
      misc_FinishUserErrorReport();
    } 
    dfg_SELECTED_LITERAL = $2;
    $$ = list_Nconc($1, list_List($2));
  } 
}
;

lit:		atom			{ $$ = $1; }
| DFG_NOT '(' atom ')'
{ $$ = dfg_IGNORE ? $3 : term_Create(fol_Not(),list_List($3)); }
;

atomlist:	atom			{ $$ = list_List($1); }
| atomlist ',' atom	{ $$ = list_Nconc($1, list_List($3)); }
;

atom:		id
{ $$ = dfg_IGNORE ? NULL : dfg_AtomCreate($1,list_Nil()); }
| DFG_TRUE
{ $$ = dfg_IGNORE ? NULL : term_Create(fol_True(),list_Nil()); }
| DFG_FALSE
{ $$ = dfg_IGNORE ? NULL : term_Create(fol_False(),list_Nil()); }
| DFG_EQUAL '(' term ',' term ')'
{ $$ = dfg_IGNORE ? NULL : term_Create(fol_Equality(),list_Cons($3,list_List($5))); }
| id '(' termlist ')'
{ $$ = dfg_IGNORE ? NULL : dfg_AtomCreate($1, $3); }
;


/* Ignore DNF clauses, since SPASS can only handle CNF */

dnfclausesopt:	/* empty */
| dnfclausesopt DFG_CLAUSE '(' dnfclauseopt labelopt ')' '.'
;

dnfclauseopt:	/* empty */
| dnfclause
;

dnfclause:	dnfclausebody
| DFG_EXISTS '(' '[' atomlist ']' ',' dnfclausebody ')'
;

dnfclausebody:	DFG_AND '(' litlist ')'
;

/****************/
/* TERMS        */
/****************/

term:		id
{ $$ = dfg_IGNORE ? NULL : dfg_TermCreate($1,list_Nil()); }
| id '(' termlist ')'
{ $$ = dfg_IGNORE ? NULL : dfg_TermCreate($1, $3); }
;

termlist:	term
{ $$ = dfg_IGNORE ? list_Nil() : list_List($1); }
| termlist ',' term
{ $$ = dfg_IGNORE ? $1 : list_Nconc($1,list_List($3)); }
;

/****************/
/* PROOFS       */
/****************/

prooflistsopt:	/* empty */
| prooflistsopt prooflist
;

prooflist:	DFG_PRFLIST '(' id ')' '.'
{ if (!string_Equal($3,"SPASS")) {
    stack_Push((POINTER)dfg_IGNORE);
    dfg_IGNORE = TRUE;
  }
}
prooflistopt
DFG_ENDLIST '.'
{ if (!string_Equal($3,"SPASS"))
    dfg_IGNORE = (BOOL)stack_PopResult();
  string_StringFree($3);
}
;

prooflistopt:	/* empty */
| prooflistopt DFG_STEP '(' id_or_formula ',' clause ','
id ',' '[' parentlist ']' assoclistopt ')' '.'
{ if (!dfg_IGNORE && $4!=NULL && $6!=NULL && !list_Empty($11)) {
    LIST tupel;
    RULE Rule = clause_GetOriginFromString($8);
    string_StringFree($8);
    /* Build a tuple (label,clause,parentlist,split level,origin) */
    tupel = list_Cons((POINTER)$13,list_List((POINTER)Rule));
    tupel = list_Cons($4,list_Cons($6,list_Cons($11,tupel)));
    dfg_PROOFLIST = list_Cons(tupel, dfg_PROOFLIST);
  } else {
    /* ignore DNF clauses and clauses with incomplete data */
    if ($4 != NULL) string_StringFree($4);
    if ($6 != NULL) term_Delete($6);
    if ($8 != NULL) string_StringFree($8);
    dfg_DeleteStringList($11);
  }
  dfg_VarCheck();
}
;

parentlist:	id_or_formula
{ $$ = (dfg_IGNORE||$1==NULL) ? list_Nil() : list_List($1); }
| parentlist ',' id_or_formula
{ $$ = (dfg_IGNORE||$3==NULL) ? $1 : list_Nconc($1, list_List($3)); }
;

/* returns the split level */
assoclistopt:	/* empty */		{ $$ = 0; }
| ',' '[' assoclist ']' { $$ = $3; }
;

assoclist:	id_or_formula ':' id_or_formula
{ if (!dfg_IGNORE && $1!=NULL && $3!=NULL && string_Equal($1,"splitlevel"))
    string_StringToInt($3, TRUE, &$$);
  else
    $$ = 0;
  if ($1 != NULL) string_StringFree($1);
  if ($3 != NULL) string_StringFree($3);
}
| assoclist ',' id_or_formula ':' id_or_formula
{ if (!dfg_IGNORE && $3!=NULL && $5!=NULL && string_Equal($3,"splitlevel"))
    string_StringToInt($5, TRUE, &$$);
  else
    $$ = $1;
  if ($3 != NULL) string_StringFree($3);
  if ($5 != NULL) string_StringFree($5);
}
;

id_or_formula:	anysymbol
{ stack_Push((POINTER) dfg_IGNORE); dfg_IGNORE = TRUE; }
optargs
{ dfg_IGNORE = (BOOL) stack_PopResult();
  if ($3) {
    if ($1 != NULL) string_StringFree($1);
    $$ = NULL;
  } else
    $$ = $1;
}
;

anysymbol:	id			{ $$ = $1; }
| DFG_NOT		{ $$ = NULL; }
| DFG_EQUIV		{ $$ = NULL; }
| DFG_IMPLIED		{ $$ = NULL; }
| DFG_IMPLIES		{ $$ = NULL; }
| DFG_AND		{ $$ = NULL; }
| DFG_OR		{ $$ = NULL; }
| DFG_FORALL		{ $$ = NULL; }
| DFG_EXISTS		{ $$ = NULL; }
;

optargs:	/* empty */		{ $$ = FALSE; }
| '(' arglist ')'	{ $$ = TRUE; }
| '(' '[' qtermlist ']' ',' formula ')'	{ $$ = TRUE; }
;

clause:		cnfclause		{ $$ = $1;   }
| dnfclause		{ $$ = NULL; }
;


/****************/
/* TERM LIST    */
/****************/

listOfTermsopt: /* empty */
| listOfTermsopt listOfTerms
;

listOfTerms:    DFG_TERMLIST '.'        { dfg_VarStart(); }
terms DFG_ENDLIST '.' {
  dfg_VarStop();
  dfg_VarBacktrack();
  dfg_VarCheck(); }
;

terms:		/* empty */
| terms term '.'	{ dfg_TERMLIST = list_Nconc(dfg_TERMLIST, list_List($2)); }
; 

/****************/
/* INCLUDES     */
/****************/

includelistopt:	/* empty */
| includelistopt includelist
;

includelist: DFG_INCLUDELIST '.' includes DFG_ENDLIST '.' { dfg_INCLUDELIST = list_Nconc(dfg_INCLUDELIST,$3); }
;

includes: /* empty */ { $$ = list_Nil(); }
| includes include { $$ = list_Cons($2,$1); }
;

include: DFG_INCLUDE '(' filename formula_selection ')' '.' { $$ = list_PairCreate($3,$4); }
;

formula_selection : ',' '[' name_list ']' { $$ = $3; }
| /* empty */ { $$ = list_Nil(); }
;

name_list : id { $$ = list_List($1); }
| id ',' name_list {
  $$ = list_Cons($1,$3);
}
;
		  
filename: single_quoted { $$ = dfg_StripQuotes($1); }
;

/****************/
/* SETTINGS     */
/****************/

settinglistsopt:	/* empty */
| settinglistsopt settinglist {
  if (dfg_IGNORESETTINGS) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Settings not allowed in included files\n");			
    misc_FinishUserErrorReport();
  }
}
;

settinglist:	DFG_GENSET '.' gsettings DFG_ENDLIST '.'
| DFG_SETTINGS '(' id	{ if (string_Equal($3,"SPASS"))
		    dfg_IGNORETEXT = FALSE;
		  string_StringFree($3);
}
')' '.' flags DFG_ENDLIST '.'
{ dfg_IGNORETEXT = TRUE; }
;

flags:		DFG_TEXT	{ /* no SPASS flags */
  string_StringFree($1);
}
| DFG_OPENBRACE spassflags DFG_CLOSEBRACE
;

spassflags:	/* empty */
| spassflags spassflag '.'
;

spassflag:	DFG_PREC '(' preclist ')'
| DFG_SELECT '(' selectlist ')'
| DFG_CLFORE '(' clfolist ')'
| DFG_DOMPRED '(' labellist ')'
{ SYMBOL s;
  for ( ; !list_Empty($3); $3 = list_Pop($3)) {
    s = symbol_Lookup(list_Car($3));
    if (s == 0) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Undefined symbol %s", list_Car($3));
      misc_UserErrorReport(" in DomPred list.\n"); 
      misc_FinishUserErrorReport(); 
    }
    if (!symbol_IsPredicate(s)) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Symbol %s isn't a predicate", list_Car($3));
      misc_UserErrorReport(" in DomPred list.\n");
      misc_FinishUserErrorReport();
    }
    string_StringFree(list_Car($3)); 
    symbol_AddProperty(s, DOMPRED);
  }
}
| DFG_SETFLAG '(' DFG_ID ',' number ')'
{ int flag = flag_Id($3);
  if (flag == -1) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Found unknown flag %s", $3);
    misc_FinishUserErrorReport();
  }
  string_StringFree($3);
  flag_SetFlagIntValue(dfg_FLAGS, flag, $5);
}
;

preclist:	precitem
| preclist ',' precitem
;

precitem:	id
{ SYMBOL s = symbol_Lookup($1);
  if (s == 0) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Undefined symbol %s ", $1);
    misc_UserErrorReport(" in precedence list.\n"); 
    misc_FinishUserErrorReport(); 
  }
  string_StringFree($1);
  symbol_SetIncreasedOrdering(dfg_PRECEDENCE, s);
  dfg_USERPRECEDENCE = list_Cons((POINTER)s, dfg_USERPRECEDENCE);
}
| '(' id ',' DFG_NUM statopt ')'
{ SYMBOL s = symbol_Lookup($2);
  if (s == 0) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Undefined symbol %s", $2);
    misc_UserErrorReport("in precedence list.\n");
    misc_FinishUserErrorReport(); 
  }
  string_StringFree($2);
  symbol_SetIncreasedOrdering(dfg_PRECEDENCE, s);
  dfg_USERPRECEDENCE = list_Cons((POINTER)s, dfg_USERPRECEDENCE);
  symbol_SetWeight(s, $4);
  if ($5 != 0)
    symbol_AddProperty(s, $5);
}
;

clfolist:	clfoitem
| clfolist ',' clfoitem
;

clfoitem:       '(' DFG_NUM  ',' clfoaxseq ')'
{
  dfg_CLAXRELATION = list_Cons(list_Cons((POINTER)$2, dfg_CLAXAXIOMS), dfg_CLAXRELATION);
  dfg_CLAXAXIOMS   = list_Nil();
}   
;

clfoaxseq:      clfoaxseqitem
| clfoaxseq  ',' clfoaxseqitem
;

clfoaxseqitem:  id
{
  dfg_CLAXAXIOMS = list_Cons((POINTER)$1, dfg_CLAXAXIOMS);
}
;

selectlist:	selectitem
| selectlist ',' selectitem
;

selectitem:	id
{ SYMBOL s = symbol_Lookup($1);
  if (s == 0) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Undefined symbol %s ", $1);
    misc_UserErrorReport(" in selection list.\n"); 
    misc_FinishUserErrorReport(); 
  }
  if (!symbol_IsPredicate(s)) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Symbol %s isn't a predicate", $1);
    misc_UserErrorReport(" in selection list.\n");
    misc_FinishUserErrorReport();
  }
  string_StringFree($1);
  dfg_USERSELECTION = list_Cons((POINTER)s, dfg_USERSELECTION);
}	
;

statopt:	/* empty */		{ $$ = 0; /* left */ }
| ',' DFG_ID
{ if ($2[1] != '\0' ||
      ($2[0]!='l' && $2[0]!='m' && $2[0]!='r')) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Invalid symbol status %s", $2);
    misc_UserErrorReport(" in precedence list.");
    misc_FinishUserErrorReport();
  }
  switch ($2[0]) {
  case 'm': $$ = ORDMUL;   break;
  case 'r': $$ = ORDRIGHT; break;
  default:  $$ = 0;
  }
  string_StringFree($2);
}
;

gsettings:	gsetting
| gsettings gsetting
;

gsetting:	DFG_HYPOTH '[' labellist ']' '.'
{ dfg_DeleteStringList($3); }
;

labellist:	id			{ $$ = list_List($1); }
| labellist ',' id	{ $$ = list_Nconc($1, list_List($3)); }
;

%%

void yyerror(const char *s)
{
  misc_StartUserErrorReport();
  misc_UserErrorReport("\n Line %i: %s\n", dfg_LINENUMBER, s);
  misc_FinishUserErrorReport();
}

static void dfg_Init(FILE* Input, FLAGSTORE Flags, PRECEDENCE Precedence, DFGDESCRIPTION Description)
/**************************************************************
  INPUT:   The input file stream for the parser, a flag store,
           a precedence and a problem description store.
  RETURNS: Nothing.
  EFFECT:  The parser and scanner are initialized.
           The parser will use the flag store and the precedence
	   to memorize the settings from the input file.
***************************************************************/
{
  extern FILE* dfg_in;  /* declared in dfgscanner */

  dfg_in               = Input;
  dfg_LINENUMBER       = 1;
  dfg_IGNORETEXT       = TRUE;
  dfg_AXIOMLIST        = list_Nil();
  dfg_CONJECLIST       = list_Nil();
  dfg_INCLUDELIST      = list_Nil();
  dfg_SORTDECLLIST     = list_Nil();
  dfg_USERPRECEDENCE   = list_Nil();
  dfg_USERSELECTION    = list_Nil();
  dfg_CLAXRELATION     = list_Nil();
  dfg_CLAXAXIOMS       = list_Nil();
  dfg_AXCLAUSES        = list_Nil();
  dfg_CONCLAUSES       = list_Nil();
  dfg_PROOFLIST        = list_Nil();
  dfg_TERMLIST         = list_Nil();
  dfg_SYMBOLLIST       = list_Nil();
  dfg_VARLIST          = list_Nil();
  dfg_PLAINAXCLAUSES   = list_Nil();
  dfg_PLAINCONCLAUSES  = list_Nil();
  dfg_TEMPPLAINCLAUSES = list_Nil();
  dfg_SELECTED_LITERAL = (TERM) NULL;
  dfg_VARDECL          = FALSE;
  dfg_IGNORE           = FALSE;
  dfg_FLAGS            = Flags;
  dfg_PRECEDENCE       = Precedence;
  dfg_DESC             = Description;
}

void dfg_DeleteClAxRelation(LIST ClAxRelation)
{
  LIST Scan1, Scan2;

  for (Scan1=ClAxRelation;!list_Empty(Scan1);Scan1=list_Cdr(Scan1)) {
    for (Scan2=list_Cdr(list_Car(Scan1));!list_Empty(Scan2);Scan2=list_Cdr(Scan2))
      string_StringFree((char *)list_Car(Scan2));
    list_Delete(list_Car(Scan1));
  }
  list_Delete(ClAxRelation);
}

FILE* dfg_OpenFile(const char * FileName, char ** const DiscoveredName)
/**************************************************************
  INPUT: A string filename, and a pointer to char pointer
           to hold the fullpath name of the file 
           that was opened in the end (can be null).
  RETURNS: A opened file.
  EFFECT:
  Opens a file for reading.
  The file is searched for using the extended search mechanism
  wrt SPASSINPUT environment variable.  
***************************************************************/
{  
  return misc_OpenFileEnv(FileName,"r","SPASSINPUT",DiscoveredName);
}

static LIST dfg_TestList; /* list of strings used in nonmember tests */
	
static BOOL dfg_LabelFormulaPairIsNonmember(POINTER pair) {
  if (list_PairFirst(pair) == NULL)
    return TRUE;
	
  return !list_Member(dfg_TestList,list_PairFirst(pair),(BOOL (*)(POINTER, POINTER))string_Equal);
}

static void dfg_DeleteLabelTermPair(POINTER pair)	{
  term_Delete(list_PairSecond(pair));
  if (list_PairFirst(pair) != NULL)
    string_StringFree(list_PairFirst(pair));  /* Free the label */
  list_PairFree(pair);
}

void dfg_FilterClausesBySelection(LIST* Clauses, LIST* ClAxRelation, LIST selection)
/**************************************************************
  INPUT: A pointer to a clause list, a pointer to a ClAxRelation list
         and a char* list.
  RETURNS: Nothing.
  EFFECT: The clause list is filtered in such a way that clauses
          with names not in the selection list are deleted.
          The name for the clauses is found in the ClAxRelation structure.
          And the corresponding entry in ClAxRelation is also 
          deleted if the clause is.
          The clause list and the ClAxRelation list are assumed to be coherent.
  NOTE: The coherence is assured when the lists come from
        an included file, because in includes settings sections are forbidden
        and so the user cannot specify ClAxRelation herself.
***************************************************************/
{
  /* result lists */
  LIST RClauses;
  LIST RClAxRelation;

  /* pointers to last element in the result */
  LIST RLClauses = list_Nil();
  LIST RLClAxRelation = list_Nil();

  RClauses = list_Nil();
  RClAxRelation = list_Nil();

  /*
    Performs parallel tranversal of Clauses and ClAxRelation.
    Either deleting from both or keeping both for the result.
  */

  while (!list_Empty(*Clauses)) {  
    CLAUSE Clause;
    LIST ClAx;
    LIST Labels;
    char* Label;

    /* current list elements */
    LIST CClause;
    LIST CClAxRelation;

    CClause = *Clauses;
    CClAxRelation = *ClAxRelation;

    Clause = (CLAUSE) list_Car(CClause);

#ifdef CHECK
    if (list_Empty(CClAxRelation)) {
      misc_StartUserErrorReport();
      misc_ErrorReport("\n In dfg_FilterClausesBySelection: ClAxRelation too short!\n");
      misc_FinishUserErrorReport();
    }
#endif

    ClAx = (LIST) list_Car(CClAxRelation);

#ifdef CHECK
    if ((int)list_Car(ClAx) != clause_Number(Clause)) {
      misc_StartUserErrorReport();
      misc_ErrorReport("\n In dfg_FilterClausesBySelection: Incompatible ClAxRelation!\n");
      misc_FinishUserErrorReport();
    }
#endif

    Labels = list_Cdr(ClAx);
    if (list_Empty(Labels)) 
      Label = NULL;
    else
      Label = (char*)list_Car(Labels);

    /*and we can already move on*/
    *Clauses = list_Cdr(*Clauses);
    *ClAxRelation = list_Cdr(*ClAxRelation);

    if (!Label || !list_Member(selection,Label,(BOOL (*)(POINTER, POINTER))string_Equal)) {
      /* deleting */
      if (RClauses) { /*if RClauses exists so does RLClauses and RLClAxRelation*/
        list_Rplacd(RLClauses,*Clauses);
        list_Rplacd(RLClAxRelation,*ClAxRelation);
      }

      clause_Delete(Clause);
      list_DeleteWithElement(Labels,(void (*)(POINTER))string_StringFree);
      list_Free(ClAx);
      list_Free(CClause);
      list_Free(CClAxRelation);

    } else {
      /* keeping */
      if (!RClauses) {
        /* the result will contain at least one element */
        RClauses = CClause;
        RClAxRelation = CClAxRelation;
      }

      /* if we delete the next, whis will need to be updated */
      RLClauses = CClause;
      RLClAxRelation = CClAxRelation;
    }
  }

  *Clauses = RClauses;
  *ClAxRelation = RClAxRelation;
}

LIST dfg_DFGParser(FILE* File, FLAGSTORE Flags, PRECEDENCE Precedence, DFGDESCRIPTION Description,
		   LIST* Axioms, LIST* Conjectures, LIST* SortDecl,
		   LIST* UserDefinedPrecedence, LIST* UserDefinedSelection,
		   LIST* ClAxRelation, BOOL* HasPlainClauses)
/**************************************************************
  A stub around the real parser that takes care of following
  include directives.
  
  NOTE: This function is almost the same as tptp_TPTPParser form tptpparser.y
    any change in its implementation should propably be also propagated there.
***************************************************************/
{
  LIST Includes, Clauses;
  LIST FilesRead;
  DFGDESCRIPTION description_dummy;

  FilesRead = list_Nil();
  Includes = list_Nil();	

  dfg_IGNORESETTINGS = FALSE; 
  Clauses = dfg_DFGParserIncludesExplicit(File,Flags,Precedence,Description,FALSE,Axioms,Conjectures,SortDecl,UserDefinedPrecedence,UserDefinedSelection,ClAxRelation,&Includes,HasPlainClauses);
	
  while (list_Exist(Includes)) {
    LIST pair;
    char* filename;
    LIST selection;
  
    pair      = list_Top(Includes);
    filename  = (char*)list_PairFirst(pair);
    selection = (LIST)list_PairSecond(pair);
    list_PairFree(pair);
    Includes = list_Pop(Includes);
		
    if (list_Member(FilesRead,filename,(BOOL (*)(POINTER, POINTER))string_Equal)) {
      misc_UserWarning("File %s already included, skipped!\n",filename);
      string_StringFree(filename);
    } else {
      LIST Axs, Conjs, Cls, UDS, CAR;
      BOOL HPC;
      FILE* FileToInclude;      
				
      FileToInclude = dfg_OpenFile(filename, NULL);

      Axs = Conjs = UDS = CAR = list_Nil();

      dfg_IGNORESETTINGS = TRUE; 
      description_dummy = desc_Create();
      Cls = dfg_DFGParserIncludesExplicit(FileToInclude, Flags, Precedence, description_dummy, TRUE, &Axs, &Conjs, SortDecl, UserDefinedPrecedence, &UDS, &CAR, &Includes, &HPC);
      desc_Delete(description_dummy);

      if (list_Exist(selection)) { /*use the selection to filter Axs and Conjs */
        dfg_FilterClausesBySelection(&Cls,&CAR,selection);
      
        dfg_TestList = selection;
        Axs = list_DeleteElementIfFree(Axs,dfg_LabelFormulaPairIsNonmember,dfg_DeleteLabelTermPair);
	Conjs = list_DeleteElementIfFree(Conjs,dfg_LabelFormulaPairIsNonmember,dfg_DeleteLabelTermPair);
      }

      Clauses = list_Nconc(Clauses, Cls); 

      *Axioms = list_Nconc(*Axioms,Axs);
      *Conjectures = list_Nconc(*Conjectures,Conjs);

      /* "Summing up" UserDefinedSelection and ClAxRelation from all the files included. */
      *UserDefinedSelection = list_Nconc(*UserDefinedSelection, UDS);
      /*
	No explicit (user specified) ClAxRelation coming in dfg from includes!      
	*ClAxRelation = list_Nconc(*ClAxRelation, CAR);
	*/
      dfg_DeleteClAxRelation(CAR);

      /*The whole thing has plain clauses only if all the files have!*/			
      if (!HPC)
	*HasPlainClauses = FALSE;

      misc_CloseFile(FileToInclude,filename);
      FilesRead = list_Cons(filename,FilesRead);
    }

    list_DeleteWithElement(selection,(void (*)(POINTER))string_StringFree);		
  }
		
  list_DeleteWithElement(FilesRead,(void (*)(POINTER))string_StringFree);		
  return Clauses;
}

void dfg_InitExplicitParser() 
/**************************************************************
  This function should be called 
  prior to calling dfg_DFGParserIncludesExplicit
  from outside the dfg module.
***************************************************************/
{
  dfg_IGNORESETTINGS = FALSE; 
}

void dfg_CreateClausesFromTerms(LIST* Axioms, LIST* Conjectures,
				LIST* ClAxRelation, BOOL BuildClAx,
				FLAGSTORE Flags, PRECEDENCE Precedence)
/**************************************************************
  INPUT:   Pointers to two lists of term-label pairs,
            pointer to ClAx list,
            boolean flag and
            a flag store and a precedence.
  EFFECT:  Applies dfg_CreateClauseFromTerm to the 
           pairs in the lists treating the firsts as axioms 
           and second as conjectures, releasing the pair struct
           and the labels.
           If BuildClAx is set the labels are not deleted,
           but instead used to record the relation 
           between the clauses and their original names
           in the ClAxRelation structure
           (otherwise ClAxRelation remains intact).
***************************************************************/
{
  LIST    scan, tupel;
  TERM    clauseTerm;
  CLAUSE  clause;
  LIST    ClAxContribution, labels;

  ClAxContribution = list_Nil();
 
  /* Remove clause labels and create clauses from the terms */
  for (scan = *Axioms; !list_Empty(scan); scan = list_Cdr(scan)) {
    tupel = list_Car(scan);
    clauseTerm = list_PairSecond(tupel);
    clause = dfg_CreateClauseFromTerm(clauseTerm,TRUE, Flags, Precedence);
    list_Rplaca(scan, clause);

    if (BuildClAx) {
      if (list_PairFirst(tupel) != NULL)
        labels = list_List(list_PairFirst(tupel));
      else
        labels = list_Nil();
      
      ClAxContribution = list_Cons(list_Cons((POINTER)clause_Number(clause),labels),ClAxContribution);
    } else {
      if (list_PairFirst(tupel) != NULL)        /* Label is defined */
        string_StringFree(list_PairFirst(tupel));  /* Delete the label */
    }
  
    list_PairFree(tupel);
  }
  /* Since dfg_CreateClauseFromTerm() returns NULL for trivial tautologies */
  /* we now delete those NULL pointers from the clause list.               */
  *Axioms = list_PointerDeleteElement(*Axioms, NULL);
  for (scan = *Conjectures; !list_Empty(scan); scan = list_Cdr(scan)) {
    tupel = list_Car(scan);
    clauseTerm = list_PairSecond(tupel);
    clause = dfg_CreateClauseFromTerm(clauseTerm,FALSE, Flags, Precedence);
    list_Rplaca(scan, clause);

    if (BuildClAx) {
      if (list_PairFirst(tupel) != NULL)
        labels = list_List(list_PairFirst(tupel));
      else
        labels = list_Nil();
      
      ClAxContribution = list_Cons(list_Cons((POINTER)clause_Number(clause),labels),ClAxContribution);
    } else {
      if (list_PairFirst(tupel) != NULL)        /* Label is defined */
        string_StringFree(list_PairFirst(tupel));  /* Delete the label */
    }         
  
    list_PairFree(tupel);
  }
  /* Since dfg_CreateClauseFromTerm() returns NULL for trivial tautologies */
  /* we now delete those NULL pointers from the clause list.               */
  *Conjectures = list_PointerDeleteElement(*Conjectures, NULL);

  if (BuildClAx) {
    /* appending the contribution to the end! */
    *ClAxRelation = list_Nconc(*ClAxRelation,list_NReverse(ClAxContribution));
  }
}

LIST dfg_DFGParserIncludesExplicit(FILE* File, FLAGSTORE Flags, PRECEDENCE Precedence,
				   DFGDESCRIPTION Description, BOOL AppendImplicitClAx,
				   LIST* Axioms, LIST* Conjectures, LIST* SortDecl,
				   LIST* UserDefinedPrecedence, LIST* UserDefinedSelection,
				   LIST* ClAxRelation, LIST* Includes, BOOL* HasPlainClauses)
/**************************************************************
  INPUT:   The input file containing clauses or formulae in DFG syntax,
           a flag store, a precedence used to memorize settings
	       from the file, and Description to store problem description.
           Boolean flag determinig whether the implicit ClAxRelation
           from Clauses should be appended to the explicit one 
           coming from settings.
           Axioms, Conjectures, SortDecl, UserDefinedPrecedence
           UserDefinedSelection, and ClAxRelation, and Includes are
	   pointers to lists used as return values.
           HasPlainClauses is a return value to indicate if
           the problem had clauses in the plain format.
  RETURNS: The list of clauses from File.
  EFFECT:  Reads formulae and clauses from the input file.
           The axioms, conjectures, sort declarations, user-defined
	   precedences, and includes are appended to the respective lists,
       the lists are not deleted!
	   The Includes list contains pairs (filename, selection),
	   where selection is a list of names of formulas chosen to include.
	   All lists except the returned clause list contain pairs
	   (label, term), where <label> may be NULL, if no
	   label was specified for that term.
	   <UserDefinedPrecedence> contains symbols sorted by decreasing
	   precedence. This list will only be changed, if the precedence
	   is explicitly defined in the input file. This can be done
	   by the 'set_precedence' flag in the SPASS settings list in
	   the DFG input file.
           <UserDefinedSelection> is set to a list of predicates to be
           selected in clause inferences by the 'set_selection' flag
           <ClAxRelation> is set to a clause, axiom relation where the
           clauses are given by their number, the axioms by name (string)
  CAUTION: The weight of the clauses is not correct and the literals
           are not oriented!
***************************************************************/
{
  LIST  scan,to_delete;
  NAT   bottom;

  dfg_Init(File, Flags, Precedence, Description);  /* Initialize the parser and scanner */
  bottom = stack_Bottom();
  dfg_parse();          /* Invoke the parser */
#ifdef CHECK 
  if (!stack_Empty(bottom)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In dfg_DFGParser: Stack not empty!\n");
    misc_FinishErrorReport();
  }
#endif
  dfg_SymCleanUp();

  dfg_CreateClausesFromTerms(&dfg_AXCLAUSES,&dfg_CONCLAUSES,&dfg_CLAXRELATION,AppendImplicitClAx,Flags,Precedence);

  /* Delete the proof list */
  dfg_DeleteProofList(dfg_PROOFLIST);

  /* Delete the list_of_terms, since it'll be ignored */
  term_DeleteTermList(dfg_TERMLIST);

  if (list_Empty(dfg_PLAINAXCLAUSES) && list_Empty(dfg_PLAINCONCLAUSES))
    *HasPlainClauses = FALSE;
  else
    *HasPlainClauses = TRUE;
  
  scan = dfg_PLAINAXCLAUSES;
  while(!list_Empty(scan)) {
    DFG_PLAINCLAUSE clause = list_Car(scan);
    CLAUSE newclause = clause_CreateFromLiteralLists(clause->constraint, clause->antecedent, 
                                                     clause->succedent, FALSE, clause->selected);
    dfg_AXCLAUSES = list_Cons(newclause,dfg_AXCLAUSES);
  
    dfg_PlainClauseFree(clause);
    to_delete = scan;
    scan = list_Cdr(scan);    
    list_Free(to_delete);
  }  
  
  scan = dfg_PLAINCONCLAUSES;
  while(!list_Empty(scan)) {   
    DFG_PLAINCLAUSE clause = list_Car(scan);
    CLAUSE newclause = clause_CreateFromLiteralLists(clause->constraint, clause->antecedent, 
                                                     clause->succedent, TRUE, clause->selected);
    dfg_CONCLAUSES = list_Cons(newclause,dfg_CONCLAUSES);
     
    dfg_PlainClauseFree(clause);
    to_delete = scan;
    scan = list_Cdr(scan);    
    list_Free(to_delete);
  }  
   
  scan = list_Nconc(list_NReverse(dfg_AXCLAUSES), list_NReverse(dfg_CONCLAUSES));

  *Axioms      = list_Nconc(*Axioms, dfg_AXIOMLIST);
  *Conjectures = list_Nconc(*Conjectures, dfg_CONJECLIST);
  *SortDecl    = list_Nconc(*SortDecl, dfg_SORTDECLLIST);
  *Includes    = list_Nconc(*Includes, dfg_INCLUDELIST);
  dfg_USERPRECEDENCE = list_NReverse(dfg_USERPRECEDENCE);
  *UserDefinedPrecedence = list_Nconc(*UserDefinedPrecedence, dfg_USERPRECEDENCE);
  *UserDefinedSelection  = dfg_USERSELECTION;
  *ClAxRelation          = dfg_CLAXRELATION;

  return scan;
}

LIST dfg_ProofParser(FILE* File, FLAGSTORE Flags, PRECEDENCE Precedence, DFGDESCRIPTION Description)
/**************************************************************
  INPUT:   The input file containing clauses in DFG syntax,
           a flag store and a precedence used to memorize settings
           from the file.
  RETURNS: A list of tuples (label,clause,justificationlist,splitlevel,origin)
           representing a proof.
  EFFECT:  Reads inputs clauses with labels and the proof lists
           from the input file.
           The elements of the list are lists with five items.
	   1. the label (a string) of a clause,
	   2. the clause in TERM format,
           3. the list of justification labels (strings, too),
           4. the split level of the clause,
           5. the origin of the clause (RULE struct from clause.h).
	   Note that the justification list is empty for input
	   clauses.
***************************************************************/
{
  LIST  scan, tupel;
  TERM  term;
  NAT   bottom; 
  
  dfg_Init(File, Flags, Precedence, Description);  /* Initialize the parser and scanner */
  bottom = stack_Bottom();
  
  dfg_parse();          /* Invoke the parser */
#ifdef CHECK 
  if (!stack_Empty(bottom)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In dfg_ProofParser: Stack not empty!\n");
    misc_FinishErrorReport();
  }
#endif

  dfg_SymCleanUp();

  /* Build the union of axiom and conjecture clauses */
  dfg_AXCLAUSES  = list_Nconc(dfg_AXCLAUSES, dfg_CONCLAUSES);
  dfg_CONCLAUSES = list_Nil();
  for (scan = dfg_AXCLAUSES; !list_Empty(scan); scan = list_Cdr(scan)) {
    tupel = list_Car(scan);
    term  = list_PairSecond(tupel);
    if (list_PairFirst(tupel) == NULL) {
      /* Ignore input clauses without label */
      term_Delete(term);
      list_PairFree(tupel);
      list_Rplaca(scan, NULL);
    } else
      /* Expand the pair to a tuple                            */
      /* (label,clause,justificationlist, split level, origin) */
      /* For input clauses the justificationlist is empty.     */
      /* Input clauses have split level 0.                     */
      list_Rplacd(tupel, list_Cons(term,list_Cons(list_Nil(),list_Cons(0, list_List((POINTER)INPUT)))));
  }
  /* Now delete the list items without labels */
  dfg_AXCLAUSES = list_PointerDeleteElement(dfg_AXCLAUSES, NULL);

  /* Delete the formula lists */
  dfg_DeleteFormulaPairList(dfg_AXIOMLIST);
  dfg_DeleteFormulaPairList(dfg_CONJECLIST);
  /* Delete includes list*/
  dfg_DeleteIncludePairList(dfg_INCLUDELIST);	  
  /* Delete the list of sort declarations */
  dfg_DeleteFormulaPairList(dfg_SORTDECLLIST);
  /* Delete the list_of_terms, since it'll be ignored */
  term_DeleteTermList(dfg_TERMLIST);

  /* Finally append the proof list to the list of input clauses with labels */
  dfg_PROOFLIST = list_NReverse(dfg_PROOFLIST);
  dfg_AXCLAUSES = list_Nconc(dfg_AXCLAUSES, dfg_PROOFLIST);

  return dfg_AXCLAUSES;
}


LIST dfg_TermParser(FILE* File, FLAGSTORE Flags, PRECEDENCE Precedence, DFGDESCRIPTION Description)
/**************************************************************
  INPUT:   The input file containing a list of terms in DFG syntax,
           a flag store and a precedence used to memorize settings
	   from the file.
  RETURNS: The list of terms from <File>.
  EFFECT:  Reads terms from the list_of_terms from the input file.
***************************************************************/
{
  NAT bottom;
  
  dfg_Init(File, Flags, Precedence, Description);   /* Initialize the parser and scanner */
  bottom = stack_Bottom();
  dfg_parse();          /* Invoke the parser */
#ifdef CHECK 
  if (!stack_Empty(bottom)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In dfg_TermParser: Stack not empty!\n");
    misc_FinishErrorReport();
  }
#endif

  dfg_SymCleanUp();

  /* Delete the clause lists */
  dfg_DeleteFormulaPairList(dfg_AXCLAUSES);
  dfg_DeleteFormulaPairList(dfg_CONCLAUSES);
  /* Delete the formula lists */
  dfg_DeleteFormulaPairList(dfg_AXIOMLIST);
  dfg_DeleteFormulaPairList(dfg_CONJECLIST);
  /* Delete includes list*/
  dfg_DeleteIncludePairList(dfg_INCLUDELIST);	  
  /* Delete the proof list */
  dfg_DeleteProofList(dfg_PROOFLIST);
  /* Delete the list of sort declarations */
  dfg_DeleteFormulaPairList(dfg_SORTDECLLIST);

  return dfg_TERMLIST;
}

void dfg_DeleteFormulaPairList(LIST FormulaPairs)
/**************************************************************
  INPUT:   A list of pairs (label, formula).
  RETURNS: Nothing.
  EFFECT:  The list and the pairs with their strings and terms
           are completely deleted.
***************************************************************/
{
  LIST pair;

  for ( ; !list_Empty(FormulaPairs); FormulaPairs = list_Pop(FormulaPairs)) {
    pair = list_Car(FormulaPairs);  /* (label, term) */
    term_Delete(list_PairSecond(pair));
    if (list_PairFirst(pair) != NULL)
      string_StringFree(list_PairFirst(pair));  /* Free the label */
    list_PairFree(pair);
  }
}

void dfg_DeleteIncludePairList(LIST IncludePairs) 
/**************************************************************
  INPUT:   A list of pairs (filename, selectionlist).
  RETURNS: Nothing.
  EFFECT:  The list and the pairs with their strings
           are completely deleted.
***************************************************************/
{
  LIST pair;

  for ( ; !list_Empty(IncludePairs); IncludePairs = list_Pop(IncludePairs)) {
    pair = list_Car(IncludePairs);  /* (filename, selectionlist) */

    string_StringFree((char *)list_PairFirst(pair));
    list_DeleteWithElement(list_PairSecond(pair),(void (*)(POINTER))string_StringFree);
    list_PairFree(pair);
  }	
}

void dfg_StripLabelsFromList(LIST FormulaPairs)
/**************************************************************
  INPUT:   A list of pairs (label, formula).
  RETURNS: Nothing.
  EFFECT:  The pairs are replaced by the respective formula
           and the pairs with their label strings are deleted.
***************************************************************/
{
  LIST pair, scan;

  for (scan = FormulaPairs; !list_Empty(scan); scan = list_Cdr(scan)) {
    pair = list_Car(scan);  /* (label, term) */
    list_Rplaca(scan, list_PairSecond(pair));
    if (list_PairFirst(pair) != NULL)
      string_StringFree(list_PairFirst(pair));  /* Free the label */
    list_PairFree(pair);
  }
}

void dfg_DeleteProofList(LIST Proof)
/**************************************************************
  INPUT:   A list of tuples (label, term, justificationlist, split level).
  RETURNS: Nothing.
  EFFECT:  All memory used by the proof list is freed.
           The labels must NOT be NULL entries!
***************************************************************/
{
  /* Delete the proof list */
  for ( ; !list_Empty(Proof); Proof = list_Pop(Proof)) {
    LIST tupel = list_Car(Proof);
    string_StringFree(list_First(tupel));
    term_Delete(list_Second(tupel));
    dfg_DeleteStringList(list_Third(tupel));
    list_Delete(tupel);
  }
}

/**************************************************************/
/* Static Functions                                           */
/**************************************************************/

static void dfg_SymbolDecl(int SymbolType, char* Name, int Arity)
/**************************************************************
  INPUT:   The type of a symbol, the name, and the arity.
  RETURNS: Nothing.
  EFFECT:  This function handles the declaration of symbols.
           If <Arity> is -2, it means that the arity of the symbol
	   was not specified, if it is -1 the symbol is declared
	   with arbitrary arity. User defined symbols with arbitrary
           arity are not allowed.
	   The <Name> is deleted.
***************************************************************/
{
  NAT    arity;
  SYMBOL symbol;

  switch (Arity) {
  case -2: /* not specified */
    arity = 0;
    break;
  case -1:
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Line %u: symbols with arbitrary arity are not allowed.\n",
			 dfg_LINENUMBER);
    misc_FinishUserErrorReport();
  default:
    arity = Arity;
  }

  /* Check if this symbol was declared earlier */
  symbol = symbol_Lookup(Name);
  if (symbol != 0) {
    /* Symbol was declared before */
    /* Check if the old and new symbol type are equal */
    if ((SymbolType == DFG_FUNC && !symbol_IsFunction(symbol)) ||
	(SymbolType == DFG_PRDICAT && !symbol_IsPredicate(symbol)) ||
	((SymbolType == DFG_OPERAT || SymbolType == DFG_QUANTIF) && 
	 !symbol_IsJunctor(symbol))) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %u: symbol %s was already declared as ",
			   dfg_LINENUMBER, Name);
      switch (symbol_Type(symbol)) {
      case symbol_CONSTANT:
      case symbol_FUNCTION:
	misc_UserErrorReport("function.\n"); break;
      case symbol_PREDICATE:
	misc_UserErrorReport("predicate.\n"); break;
      case symbol_JUNCTOR:
	misc_UserErrorReport("junctor.\n"); break;
      default:
	misc_UserErrorReport("unknown type.\n");
      }
      misc_FinishUserErrorReport();
    }
    /* Now check the old and new arity if specified */
    if (Arity != -2 && Arity != symbol_Arity(symbol)) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %u: symbol %s was already declared with arity %d\n",
			   dfg_LINENUMBER, Name, symbol_Arity(symbol));
      misc_FinishUserErrorReport();
    }
  } else {
    /* Symbol was not declared before */
    switch (SymbolType) {
    case DFG_FUNC:
      symbol = symbol_CreateFunction(Name, arity, symbol_STATLEX,dfg_PRECEDENCE);
      break;
    case DFG_PRDICAT:
      symbol = symbol_CreatePredicate(Name, arity,symbol_STATLEX,dfg_PRECEDENCE);
      break;
    default:
      symbol = symbol_CreateJunctor(Name, arity, symbol_STATLEX, dfg_PRECEDENCE);
    }
    if (Arity == -2)
      /* Arity wasn't specified so check the arity for each occurrence */
      dfg_SymAdd(symbol);
  }

  string_StringFree(Name);  /* Name was copied */
}


static SYMBOL dfg_Symbol(char* Name, NAT Arity)
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
    string_StringFree(Name);
    dfg_SymCheck(symbol, Arity); /* Check the arity */
  } else {
    /* Variable */
    if (Arity > 0) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %d: Undefined symbol %s.\n",dfg_LINENUMBER,Name);
      misc_FinishUserErrorReport();
    }
    symbol = dfg_VarLookup(Name);
  }
  return symbol;
}


static TERM dfg_CreateQuantifier(SYMBOL Symbol, LIST VarTermList, TERM Term)
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


CLAUSE dfg_CreateClauseFromTerm(TERM Clause, BOOL IsAxiom, FLAGSTORE Flags,
				PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A clause term, a boolean value, a flag store and a precedence.
  RETURNS: The clause term converted to a CLAUSE.
  EFFECT:  This function converts a clause stored as term into an
           EARL clause structure.
	   If 'IsAxiom' is TRUE the clause is treated as axiom
	   clause else as conjecture clause.
           The function deletes the literals "false" and "not(true)"
           if they occur in <Clause>.
	   The contents of the flag store and the precedence are changed
	   because the parser read flag and precedence settings from
  MEMORY:  The clause term is deleted.
***************************************************************/
{
  LIST   literals, scan;
  TERM   literal;
  CLAUSE result;
  
  if (term_TopSymbol(Clause) == fol_All()) {
    /* Remove and free the quantifier and the OR term */
    literals = term_ArgumentList(term_SecondArgument(Clause));
    term_RplacArgumentList(term_SecondArgument(Clause), list_Nil());
  } else {
    /* Remove and free the OR term */
    literals = term_ArgumentList(Clause);
    term_RplacArgumentList(Clause, list_Nil());
  }
  term_Delete(Clause);

  for (scan = literals; !list_Empty(scan); scan = list_Cdr(scan)) {
    literal = (TERM) list_Car(scan);
    if (symbol_IsPredicate(term_TopSymbol(literal))) {  /* Positive literal */
      if (fol_IsFalse(literal)) {
	/* Ignore this literal */
	term_Delete(literal);
	list_Rplaca(scan, NULL); /* Mark the actual list element */
      }
    } else {
      /* Found a negative literal */
      TERM atom = term_FirstArgument(literal);
      if (fol_IsTrue(atom)) {
	/* Ignore this literal */
	term_Delete(literal);
	list_Rplaca(literals, NULL); /* Mark the actual list element */
      }
    }
  }
  

  literals = list_PointerDeleteElement(literals, NULL);
  /* Remove the special literals treated above from the list */
  result = clause_CreateFromLiterals(literals, FALSE, !IsAxiom, FALSE, Flags, Precedence);
  /* Don't create sorts! */
  list_Delete(literals);

  return result;
}


static void dfg_SubSort(char* Name1, char* Name2)
/**************************************************************
  INPUT:   Two sort symbol names.
  RETURNS: Nothing.
  EFFECT:  This functions adds the formula
           forall([U], implies(Name1(U), Name2(U)))
	   to the list of axiom formulas. Both <Name1> and <Name2>
	   are deleted.
***************************************************************/
{
  SYMBOL s1, s2;
  TERM   varterm, t1, t2, term;

  s1 = dfg_Symbol(Name1, 1);   /* Should be unary predicates */
  s2 = dfg_Symbol(Name2, 1);
  if (!symbol_IsPredicate(s1)) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Line %d: Symbol is not a sort predicate.\n", dfg_LINENUMBER);
    misc_FinishUserErrorReport();
  }
  if (!symbol_IsPredicate(s2)) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Line %d: Symbol is not a sort predicate.\n", dfg_LINENUMBER);
    misc_FinishUserErrorReport();
  }

  varterm = term_Create(symbol_CreateStandardVariable(), list_Nil());
  symbol_ResetStandardVarCounter();
  
  t1   = term_Create(s1, list_List(varterm));
  t2   = term_Create(s2, list_List(term_Copy(varterm)));
  term = term_Create(fol_Implies(), list_Cons(t1, list_List(t2)));
  term = fol_CreateQuantifier(fol_All(), list_List(term_Copy(varterm)),
			      list_List(term));
  dfg_SORTDECLLIST = list_Nconc(dfg_SORTDECLLIST, list_List(list_PairCreate(NULL,term)));
}


static void dfg_SymbolGenerated(SYMBOL SortPredicate, BOOL FreelyGenerated,
				LIST GeneratedBy)
/**************************************************************
  INPUT:   A sort predicate, a boolean flag, and a list of function
           symbol names.
  RETURNS: Nothing.
  EFFECT:  This function stores the information that the <SortPredicate>
           is generated by the function symbols from the <GeneratedBy>
           list. The list contains only symbol names!
	   The <SortPredicate> AND the symbols from the list get
           the property GENERATED. Additionally the symbols get
	   the property FREELY, if the flag <FreelyGenerated> is TRUE.
***************************************************************/
{
  SYMBOL symbol;
  LIST   scan;

  if (!symbol_IsPredicate(SortPredicate)) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Line %d: Symbol is not a sort predicate.\n", dfg_LINENUMBER);
    misc_FinishUserErrorReport();
  }
  /* First reset the old information */
  symbol_RemoveProperty(SortPredicate, GENERATED);
  symbol_RemoveProperty(SortPredicate, FREELY);
  list_Delete(symbol_GeneratedBy(SortPredicate));
  /* Now set the new information */
  symbol_AddProperty(SortPredicate, GENERATED);
  if (FreelyGenerated)
    symbol_AddProperty(SortPredicate, FREELY);
  for (scan = GeneratedBy; !list_Empty(scan); scan = list_Cdr(scan)) {
    symbol = symbol_Lookup(list_Car(scan));
    if (symbol == 0) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %d: undefined symbol %s.\n", dfg_LINENUMBER,
			   (char*)list_Car(scan));
      misc_FinishUserErrorReport();
    } else if (!symbol_IsFunction(symbol)) { /* must be function or constant */
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %d: Symbol is not a function.\n", dfg_LINENUMBER);
      misc_FinishUserErrorReport();
    }
    string_StringFree(list_Car(scan));
    list_Rplaca(scan, (POINTER)symbol);  /* change the name list to a symbol list */
    /* Set GENERATED properties for generating symbols */
    symbol_AddProperty(symbol, GENERATED);
    if (FreelyGenerated)
      symbol_AddProperty(symbol, FREELY);
  }
  symbol_SetGeneratedBy(SortPredicate, GeneratedBy);
}

static void dfg_TranslPairDecl(char* PropName, char* FoName)
/**************************************************************
  INPUT:   A propositional name/first order name pair
  RETURNS: Nothing.
  EFFECT:  Updates the appropriate list.
***************************************************************/
{
  SYMBOL PropSymbol, FoSymbol;
  
  PropSymbol = symbol_Lookup(PropName);
  FoSymbol = symbol_Lookup(FoName);
  
  if (PropSymbol == 0) {
    fprintf(stderr, "Line %u: Undefined symbol %s\n", 
	    dfg_LINENUMBER, PropName); 
    misc_Error(); 
  }
  else if (FoName == 0) {
    fprintf(stderr, "Line %u: Undefined symbol %s\n", 
	    dfg_LINENUMBER, FoName); 
    misc_Error(); 
  }
  else 
    eml_SetPropFoSymbolAssocList(PropSymbol,list_List((POINTER)FoSymbol));
  string_StringFree(PropName);  /* Names were copied */
  string_StringFree(FoName);  
}

/**************************************************************/
/* Functions for the Symbol Table                             */
/**************************************************************/

typedef struct {
  SYMBOL symbol;
  BOOL   valid;
  int    arity;
} DFG_SYMENTRY, *DFG_SYM;

static __inline__ DFG_SYM dfg_SymCreate(void)
{
  return (DFG_SYM) memory_Malloc(sizeof(DFG_SYMENTRY));
}

static __inline__ void dfg_SymFree(DFG_SYM Entry)
{
  memory_Free(Entry, sizeof(DFG_SYMENTRY));
}


static void dfg_SymAdd(SYMBOL Symbol)
/**************************************************************
  INPUT:   A symbol.
  RETURNS: Nothing.
  EFFECT:  This function adds 'Symbol' to the symbol list.
           The arity of these symbols will be checked every time
	   the symbol occurs.
***************************************************************/
{
  DFG_SYM newEntry = dfg_SymCreate();
  newEntry->symbol = Symbol;
  newEntry->valid  = FALSE;
  newEntry->arity  = 0;
  dfg_SYMBOLLIST = list_Cons(newEntry, dfg_SYMBOLLIST);
}


static void dfg_SymCheck(SYMBOL Symbol, NAT Arity)
/**************************************************************
  INPUT:   A symbol and the current arity of this symbol.
  RETURNS: Nothing.
  EFFECT:  This function compares the previous arity of 'Symbol'
           with the actual 'Arity'. If these values differ
	   the symbol's arity is set to arbitrary.
	   The arity of symbols whose arity was specified in
	   the symbol declaration section is checked and a warning
	   is printed to stderr in case of differences.
***************************************************************/
{
  LIST scan = dfg_SYMBOLLIST;
  while (!list_Empty(scan)) {
    DFG_SYM actEntry = (DFG_SYM) list_Car(scan);
    if (actEntry->symbol == Symbol) {
      if (actEntry->valid) {
	if (actEntry->arity != Arity) {
	  misc_StartUserErrorReport();
	  misc_UserErrorReport("\n Line %u:", dfg_LINENUMBER);
	  misc_UserErrorReport(" The actual arity %u", Arity);
	  misc_UserErrorReport(" of symbol %s differs", symbol_Name(Symbol));
	  misc_UserErrorReport(" from the previous arity %u.\n", actEntry->arity);
	  misc_FinishUserErrorReport();
	}
      } else {
	/* Not valid => first time */
	actEntry->arity = Arity;
	actEntry->valid = TRUE;
      }
      return;
    }
    scan = list_Cdr(scan);
  }

  /* Symbol isn't in SymbolList, so its arity was specified.        */
  /* Check if the specified arity corresponds with the actual arity */
  if (symbol_Arity(Symbol) != Arity) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n Line %u: Symbol %s was declared with arity %u.\n",
			 dfg_LINENUMBER, symbol_Name(Symbol), symbol_Arity(Symbol));
    misc_FinishUserErrorReport();
  }
}


static void dfg_SymCleanUp(void)
/**************************************************************
  INPUT:   None.
  RETURNS: Nothing.
  EFFECT:  This function corrects all symbols whose arity wasn't
           specified in the symbol declaration section but seem
	   to occur with always the same arity.
	   The memory for the symbol list is freed.
***************************************************************/
{
  while (!list_Empty(dfg_SYMBOLLIST)) {
    DFG_SYM actEntry  = (DFG_SYM) list_Car(dfg_SYMBOLLIST);
    SYMBOL  actSymbol = actEntry->symbol;

    if (actEntry->arity != symbol_Arity(actSymbol))
      symbol_SetArity(actSymbol, actEntry->arity);

    dfg_SymFree(actEntry);
    dfg_SYMBOLLIST = list_Pop(dfg_SYMBOLLIST);
  }
}


/**************************************************************/
/* Functions for the Variable Table                           */
/**************************************************************/
  
typedef struct {
  char*  name;
  SYMBOL symbol;
} DFG_VARENTRY, *DFG_VAR;

static __inline__ char* dfg_VarName(DFG_VAR Entry)
{
  return Entry->name;
}

static __inline__ SYMBOL dfg_VarSymbol(DFG_VAR Entry)
{
  return Entry->symbol;
}

static __inline__ DFG_VAR dfg_VarCreate(void)
{
  return (DFG_VAR) memory_Malloc(sizeof(DFG_VARENTRY));
}

static void dfg_VarFree(DFG_VAR Entry)
{
  string_StringFree(Entry->name);
  memory_Free(Entry, sizeof(DFG_VARENTRY));
}

static void dfg_VarStart(void)
{
  dfg_VARLIST = list_Push(list_Nil(), dfg_VARLIST);
  dfg_VARDECL = TRUE;
}

static void dfg_VarStop(void)
{
  dfg_VARDECL = FALSE;
}

static void dfg_VarBacktrack(void)
{
  list_DeleteWithElement(list_Top(dfg_VARLIST), (void (*)(POINTER)) dfg_VarFree);
  dfg_VARLIST = list_Pop(dfg_VARLIST);
}

static void dfg_VarCheck(void)
/* Should be called after a complete clause or formula was parsed */
{
  if (!list_Empty(dfg_VARLIST)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In dfg_VarCheck: List of variables should be empty!\n");
    misc_FinishErrorReport();
  }
  symbol_ResetStandardVarCounter();
}

static SYMBOL dfg_VarLookup(char* Name)
/**************************************************************
  INPUT:   A variable name.
  RETURNS: The corresponding variable symbol.
  EFFECT:  If the variable name was quantified before, the
           corresponding symbol is returned and the <Name> is freed.
	   If the variable name was not quantified, and <dfg_VARDECL>
	   is TRUE, a new variable is created, else an error
	   message is printed and the program exits.
***************************************************************/
{
  LIST   scan, scan2;
  SYMBOL symbol = symbol_Null();

  scan  = dfg_VARLIST;
  scan2 = list_Nil();
  while (!list_Empty(scan) && list_Empty(scan2)) {
    scan2 = list_Car(scan);
    while (!list_Empty(scan2) &&
	   (!string_Equal(dfg_VarName(list_Car(scan2)), Name)))
      scan2 = list_Cdr(scan2);
    scan = list_Cdr(scan);
  }

  if (!list_Empty(scan2)) {
    /* Found variable */
    string_StringFree(Name);
    symbol = dfg_VarSymbol(list_Car(scan2));
  } else {
    /* Variable not found */
    if (dfg_VARDECL) {
      DFG_VAR newEntry = dfg_VarCreate();
      newEntry->name   = Name;
      newEntry->symbol = symbol_CreateStandardVariable();
      /* Add <newentry> to the first list in dfg_VARLIST */
      list_Rplaca(dfg_VARLIST, list_Cons(newEntry,list_Car(dfg_VARLIST)));
      symbol = dfg_VarSymbol(newEntry);
    } else {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Line %u: Free Variable %s.\n", dfg_LINENUMBER, Name);
      misc_FinishUserErrorReport();
    }
  }
  return symbol;
}

/**************************************************************/
/* Functions for the plain clauses                            */
/**************************************************************/
static DFG_PLAINCLAUSE dfg_PlainClauseCreate(void)
{
  return (DFG_PLAINCLAUSE) memory_Malloc(sizeof(DFG_PLAINCLAUSEHELP));
}

static void dfg_PlainClauseFree(DFG_PLAINCLAUSE Clause)
{
  list_Delete(Clause->constraint);
  list_Delete(Clause->antecedent);
  list_Delete(Clause->succedent);
  memory_Free(Clause, sizeof(DFG_PLAINCLAUSEHELP));
}

/**************************************************************/
/* Miscelaneous                                               */
/**************************************************************/

/*Get rid of leading and trailing qoutes of a string.
  Returns a newly allocated thing and frees the one destroyed.*/
static char * dfg_StripQuotes(char * str) {
  char * newstr;
  int len = strlen(str);
    
  str[len-1] = '\0'; /*one characer shorter from end*/

  newstr = string_StringCopy(str+1);    /*start duplicating from the second char*/

  memory_Free(str,len+1);

  return newstr;
}
