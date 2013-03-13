/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *              DFG2TPTP                                  * */
/* *                                                        * */
/* *  $Module:    DFG2TPTP                                  * */ 
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
/* $Revision: 1.8 $                                         * */
/* $State: Exp $                                            * */
/* $Date: 2010-02-22 14:09:57 $                             * */
/* $Author: weidenb $                                       * */
/* *                                                        * */
/* *             Contact:                                   * */
/* *             Christoph Weidenbach                       * */
/* *             MPI fuer Informatik                        * */
/* *             Stuhlsatzenhausweg 85                      * */
/* *             66123 Saarbruecken                         * */
/* *             Email: weidenb@mpi-inf.mpg.de              * */
/* *             Germany                                    * */
/* *                                                        * */
/* ********************************************************** */
/**************************************************************/


/* $RCSfile: dfg2tptp.c,v $ */


/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include "dfg.h"
#include "clause.h"
#include "foldfg.h"
#include "eml.h"

#include "description.h"

#include <errno.h>

#define DFG2TPTP__VERSION "0.49"

static void TPTPOutput(FILE* File, TERM Term);

static void TPTPSymbolOutput(FILE* File, SYMBOL Symbol)
{
  if (symbol_Equal(Symbol,fol_All())) {
    fputs("\n!", File);
    return;
  }
  if (symbol_Equal(Symbol,fol_Exist())) {
    fputs("\n?", File);
    return;
  }
  if (symbol_Equal(Symbol,fol_Implies())) {
    fputs("=>\n", File);
    return;
  }
  if (symbol_Equal(Symbol,fol_Equiv())) {
    fputs("<=>\n", File);
    return;
  }
  if (symbol_Equal(Symbol,fol_Not())) {
    fputs("~", File);
    return;
  }
  if (symbol_Equal(Symbol,fol_And())) {
    fputs("&\n", File);
    return;
  }
  if (symbol_Equal(Symbol,fol_Or())) {
    fputs("|\n", File);
    return;
  }
  if (symbol_Equal(Symbol,fol_False())) {
    fputs("$false\n", File);
    return;
  }
  if (symbol_Equal(Symbol,fol_True())) {
    fputs("$true\n", File);
    return;
  }
  if (symbol_Equal(Symbol,fol_Equality())) {
    fputs(" = ", File);
    return;
  }
  if (symbol_Equal(Symbol,fol_Xor())) {
    fputs(" <~> ", File);
    return;
  }  
  if (symbol_Equal(Symbol,fol_Nor())) {
    fputs(" ~| ", File);
    return;
  }  
  if (symbol_Equal(Symbol,fol_Nand())) {
    fputs(" ~& ", File);
    return;
  }  
  if (symbol_Equal(Symbol,fol_Implied())) {
    fputs(" <= ", File);
    return;
  }  
  symbol_FPrint(File,Symbol);
  return;
}

static void TPTPListOutput(FILE* File, LIST List)
/**************************************************************
  INPUT:   A list of terms.
  RETURNS: Nothing.
***************************************************************/
{
  for (; !list_Empty(List); List=list_Cdr(List)) {
    TPTPOutput(File,list_Car(List));
    if (!list_Empty(list_Cdr(List)))
      putc(',', File);
  }
}


static void TPTPOutput(FILE* File, TERM Term)
/**************************************************************
  INPUT:   A file and a term.
  RETURNS: Nothing.
  SUMMARY: Prints the term in prefix notation to the file. 
***************************************************************/
{
  SYMBOL Top;
  Top = term_TopSymbol(Term);
  if (term_IsComplex(Term)) {
    if (fol_IsQuantifier(Top)) {
      TPTPSymbolOutput(File,Top);
      fputs(" [", File);
      TPTPListOutput(File,fol_QuantifierVariables(Term));
      fputs("] : ", File);
      TPTPOutput(File, term_SecondArgument(Term));
    }
    else
      if (symbol_Equal(Top,fol_And()) || symbol_Equal(Top,fol_Or()) ||
	  symbol_Equal(Top,fol_Implies()) || symbol_Equal(Top,fol_Equiv()) || 
	  symbol_Equal(Top,fol_Implied()) || symbol_Equal(Top,fol_Xor()) ||
	  symbol_Equal(Top,fol_Nand()) || symbol_Equal(Top,fol_Nor())
	  ) {
	LIST Scan;
	fputs("(", File);
	for (Scan = term_ArgumentList(Term);!list_Empty(Scan);Scan=list_Cdr(Scan)) {
	  TPTPOutput(File,list_Car(Scan));
	  if (!list_Empty(list_Cdr(Scan))) {
	    putc(' ', File);
	    TPTPSymbolOutput(File,Top);
	    putc(' ', File);
	  }
	}
	putc(')', File);
      }
      else if( symbol_Equal(Top,fol_Equality()))
	{
	  /*putc('(', File);*/
	  TPTPOutput(File, term_FirstArgument(Term));
	  TPTPSymbolOutput(File,Top);
	  TPTPOutput(File, term_SecondArgument(Term));
	  /*putc(')', File);*/
	}
      else
	{
	  TPTPSymbolOutput(File,Top);
	  putc('(', File);
	  TPTPListOutput(File,term_ArgumentList(Term));
	  putc(')', File);
	}
  }
  else 
    TPTPSymbolOutput(File,Top);
}

void TPTPCommentOutput(FILE *File, const char* String)
/**************************************************************
  INPUT:   A file and a comment string.
  RETURNS: none.
  SUMMARY: Prints the comment string  to the file: if <String>
           string  contains a new line character it is automatically
           followed by a '%' and some space according to TPTP
           standards
***************************************************************/
{
  int i;
  for (i = 0; String[i] != '\0'; i++)
    if (String[i] == 10 && String[i+1] != '%') {
      putc(String[i], File);
      fputs("%            ",File);
    }
    else
      putc(String[i], File);
}

static void TPTPFPrintInclude(FILE* File, char* filename, LIST selection) 
/**************************************************************
  INPUT:   A file, name of file included, list of formule secelection.
  EFFECTS: Print a line in DFG format denoting the given inclusion.
 **************************************************************/
{
	int len;
	
	fputs("\tinclude('",File);
	
	len = strlen(filename);
	if ((len >= 4) && (filename[len-1] == 'g') && (filename[len-2] == 'f') && (filename[len-3] == 'd') && (filename[len-4] == '.')  ) { /*changing *.dfg to *.p */
		filename[len-3] = '\0';
		fputs(filename,File);
		fputs("p",File);
		filename[len-3] = 'd';
	} else {
		fputs(filename,File);
	}

	fputs("'",File);
	
	if (list_Exist(selection)) {
		fputs(",[",File);
		fputs((char*)list_Car(selection),File);
		selection = list_Cdr(selection);
		
		while(list_Exist(selection)) {
			fputs(",",File);
			fputs((char*)list_Car(selection),File);
			selection = list_Cdr(selection);
		}
		fputs("]",File);
	}

	fputs(").\n",File);
}

/**************************************************************/
/* Main Function                                              */
/**************************************************************/

int main(int argc, const char* argv[])
{
  LIST       Clauses,Axioms,Conjectures,Sorts,Scan, 
             UserPrecedence,UserSelection,ClAxRelation,Includes;
  FILE       *Output,*Input;
  int        i;
  CLAUSE     Clause;
  const char *Filename,*Outname;
  FLAGSTORE  FlagStore;
  PRECEDENCE Precedence;
  BOOL       HasPlainClauses;
  DFGDESCRIPTION Description;

  memory_Init(memory__UNLIMITED);
  symbol_Init(TRUE);
  stack_Init();
  term_Init();
  flag_Init(flag_SPASS);

  FlagStore = flag_CreateStore();
  flag_InitStoreByDefaults(FlagStore);
  Precedence = symbol_CreatePrecedence();
  Description = desc_Create();

  fol_Init(TRUE, Precedence);
  eml_Init(Precedence);
  clause_Init();

  if (argc < 3) {
    fputs("\n\t          dfg2tptp Version ", stdout);
    fputs(DFG2TPTP__VERSION, stdout);
    puts("\n\t       Usage: dfg2tptp <input-file> <output-file>\n");
    return EXIT_FAILURE;
  }

  Axioms         = list_Nil();
  Conjectures    = list_Nil();
  Sorts          = list_Nil();
  UserPrecedence = list_Nil();
  UserSelection  = list_Nil();
  ClAxRelation   = list_Nil();
  Includes       = list_Nil();

  Filename = *++argv;
  Input    = misc_OpenFile(Filename, "r");
  dfg_InitExplicitParser();
  Clauses  = dfg_DFGParserIncludesExplicit(Input, FlagStore, Precedence, Description, FALSE, &Axioms, &Conjectures,
			   &Sorts, &UserPrecedence, &UserSelection, &ClAxRelation, &Includes,
                           &HasPlainClauses);
  misc_CloseFile(Input,Filename);

  if (flag_GetFlagIntValue(FlagStore, flag_EML)) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n The input file contains EML formulae. Please perfrom a translation to FOL by FLOTTER first!\n\n");
    misc_FinishUserErrorReport();
  }

  Axioms = list_Nconc(Axioms, Sorts);
  dfg_StripLabelsFromList(Axioms);
  dfg_StripLabelsFromList(Conjectures);

  symbol_LowerSignature();

  Outname = *++argv;
  Output  = misc_OpenFile(Outname, "w");

  if (!list_Empty(Axioms) || !list_Empty(Conjectures)) {
    const char * status;

  if (desc_Status(Description)== DFG_UNSATISFIABLE)
    if (list_Empty(Conjectures))
      status = "Unsatisfiable";
    else 
      status = "Theorem";
  else if (desc_Status(Description)== DFG_SATISFIABLE)
    if (list_Empty(Conjectures))
      status = "Satisfiable"; 
    else 
      status = "CounterSatisfiable";
  else
    status = "Unknown";

    fprintf(Output,"%%--------------------------------------------------------------------------");
    fprintf(Output,"\n%% File     : No information");
    fprintf(Output,"\n%% Domain   : No information");
    fprintf(Output,"\n%% Problem  : %s",Filename);
    fprintf(Output,"\n%% Version  : No information");
    fprintf(Output,"\n%% English  : No information");
    fprintf(Output,"\n\n%% Refs     : No information");
    fprintf(Output,"\n%% Source   : No information");
    fprintf(Output,"\n%% Names    : No information");
    fprintf(Output,"\n\n%% Status   : %s",status);
    fprintf(Output,"\n%% Rating   : ?");
    fprintf(Output,"\n%% Syntax   : No information");
    fprintf(Output,"\n\n%% Comments : File generated by dfg2tptp Version ");
    fprintf(Output, DFG2TPTP__VERSION);
    fprintf(Output,"\n%%--------------------------------------------------------------------------");
    i=1;
    for (Scan = Axioms;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
      fprintf(Output,"\nfof(ax%d,axiom,(\n\t",i++);
      TPTPOutput(Output,list_Car(Scan));
      fputs(")).", Output);
    }
    i=1;
    for (Scan = Conjectures;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
      fprintf(Output,"\nfof(co%d,conjecture,(\n\t",i++);
      TPTPOutput(Output,list_Car(Scan));
      fputs(")).\n", Output);
    }
    fprintf(Output,"%%--------------------------------------------------------------------------\n");
  }
  else {
    fprintf(Output,"%%--------------------------------------------------------------------------");
    fprintf(Output,"\n%% File     : No information");
    fprintf(Output,"\n%% Domain   : No information");
    fprintf(Output,"\n%% Problem  : %s",Filename);
    fprintf(Output,"\n%% Version  : No information");
    fprintf(Output,"\n%% English  : No information");
    fprintf(Output,"\n\n%% Refs     : No information");
    fprintf(Output,"\n%% Source   : No information");
    fprintf(Output,"\n%% Names    : No information");
    fprintf(Output,"\n\n%% Status   : %s",( desc_Status(Description)== DFG_UNSATISFIABLE ? "Unsatisfiable" :
				    ( desc_Status(Description)== DFG_SATISFIABLE ? "Satisfiable" : "Unknown")));
    fprintf(Output,"\n%% Rating   : ?");
    fprintf(Output,"\n%% Syntax   : No information");
    fprintf(Output,"\n\n%% Comments : File generated by dfg2tptp Version ");
    fprintf(Output, DFG2TPTP__VERSION);
    fprintf(Output,"\n%%--------------------------------------------------------------------------");
    for (Scan = Clauses;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
      Clause = (CLAUSE)list_Car(Scan);
      fprintf(Output,"\ncnf(clause%d,%s,\n\t",clause_Number(Clause),
	      (clause_GetFlag(Clause,CONCLAUSE) ? "negated_conjecture" : "axiom"));
      putc('(', Output);
      for (i=clause_FirstLitIndex();i<clause_Length(Clause);i++) {
	if (i <= clause_LastAntecedentLitIndex(Clause))
	  fputs("~ ", Output);
	TPTPOutput(Output,clause_GetLiteralAtom(Clause,i));
	if (i+1 < clause_Length(Clause))
	  fputs("\n\t| ", Output);
      }
      fputs(")).\n", Output);
    }
    fprintf(Output,"%%--------------------------------------------------------------------------\n");
  }

  if (list_Exist(Includes)) {	  	
	for (Scan = Includes; !list_Empty(Scan); Scan = list_Cdr(Scan))	{
		LIST pair = list_Car(Scan);
		TPTPFPrintInclude(Output,list_PairFirst(pair),list_PairSecond(pair));
	}  
  
	fprintf(Output,"%%--------------------------------------------------------------------------\n");
  }
  
  misc_CloseFile(Output,Outname);

  clause_DeleteClauseList(Clauses);
  term_DeleteTermList(Axioms);
  term_DeleteTermList(Conjectures);

  eml_Free();
  flag_DeleteStore(FlagStore);
  symbol_DeletePrecedence(Precedence);
  list_Delete(UserPrecedence);
  list_Delete(UserSelection);
  dfg_DeleteClAxRelation(ClAxRelation);
  dfg_DeleteIncludePairList(Includes);
  desc_Delete(Description);

  /*symbol_Dump();*/
  fol_Free();
  symbol_FreeAllSymbols();
#ifdef CHECK
  memory_Print();
#endif
  return 0;
}


