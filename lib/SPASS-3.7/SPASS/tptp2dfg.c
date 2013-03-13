/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *              TPTP2DFG                                  * */
/* *                                                        * */
/* ********************************************************** */
/**************************************************************/


/**************************************************************/
/* *  This program is free software; you can redistribute   * */
/* *  it and/or modify it under the terms of the FreeBSD    * */
/* *  Licence.                                              * */
/* *                                                        * */
/* *  This program is distributed in the hope that it will  * */
/* *  be useful, but WITHOUT ANY WARRANTY; without even     * */
/* *  the implied warranty of MERCHANTABILITY or FITNESS    * */
/* *  FOR A PARTICULAR PURPOSE.  See the LICENCE file       * */
/* *  for more details.                                     * */
/* Includes                                                   */
/**************************************************************/

#include "memory.h"
#include "term.h"
#include "clause.h"
#include "foldfg.h"
#include "symbol.h"
#include "cmdline.h"

#include "tptp.h"

#define TPTP2DFG__VERSION "0.1"

/*-----------------------------------------------------------------------------*/

static void tptp_FPrintDFGProblem(FILE*, DFGDESCRIPTION, LIST, LIST, LIST, LIST, LIST, BOOL);

/*-----------------------------------------------------------------------------*/

static void tptp_DeleteStrListOfStrPair(POINTER pair) {
  string_StringFree((char *)list_PairFirst(pair));
  list_DeleteWithElement(list_PairSecond(pair),(void (*)(POINTER))string_StringFree);  
  list_PairFree(pair);
}

/*-----------------------------------------------------------------------------*/

int main(int argc, const char* argv[]) {

  LIST Clauses, Axioms, Conjectures, SortDecls, UserDefinedPrecedence, UserDefinedSelection, ClAxRelation, Includes;
  PRECEDENCE Precedence;
  FLAGSTORE  Flags;
  DFGDESCRIPTION Description;
  FILE* InFile;
  FILE* OutFile;
  BOOL Dummy;

  /*Set up modules*/
  
  memory_Init(memory__UNLIMITED);
  symbol_Init(TRUE);
  stack_Init();
  flag_Init(flag_SPASS);
  flag_Init(flag_TPTP2DFG);   /* declare include flag */
  tptp_Init();

  Flags = flag_CreateStore();
  flag_InitStoreByDefaults(Flags);

  Precedence = symbol_CreatePrecedence();
  term_Init();
  fol_Init(TRUE, Precedence);
  clause_Init();
  cmdlne_Init();

  if (!cmdlne_Read(argc, argv) ||
      cmdlne_GetInputFile() == (char*)NULL || 
      cmdlne_GetOutputFile() == (char*)NULL) {
    /* print options */
    fputs("\n\t          tptp2dfg Version ", stdout);
    fputs(TPTP2DFG__VERSION, stdout);
    puts("\n\t       Usage: tptp2dfg [-include] <input-file> <output-file>\n");

    puts("-include makes the parser parse the files in include directive,");
    puts("\tby default the directives are just translated.");
    return EXIT_FAILURE;
  }

  if (!cmdlne_SetFlags(Flags))
    return EXIT_FAILURE;
  
  /*Set up variables*/

  Axioms = list_Nil();
  Conjectures = list_Nil();
  SortDecls = list_Nil();
  UserDefinedPrecedence = list_Nil(); 
  UserDefinedSelection = list_Nil();
  ClAxRelation = list_Nil();
  Includes = list_Nil();
  
  Description = desc_Create();

  InFile = tptp_OpenFile(cmdlne_GetInputFile(),NULL);
  OutFile = misc_OpenFile(cmdlne_GetOutputFile(),"w");
  tptp_SetParserOutput(OutFile); /*for the header comments*/

  /*Parse the thing*/
  if (flag_GetFlagIntValue(Flags, flag_INCLUDE) == flag_INCLUDEON) {
    Clauses = tptp_TPTPParser(InFile, Flags, Precedence, Description,
			      &Axioms, &Conjectures, &SortDecls,
			      &UserDefinedPrecedence, &UserDefinedSelection, &ClAxRelation,
			      &Dummy);
  } else {
    Clauses = tptp_TPTPParserIncludesExplicit(InFile, Flags, Precedence, Description,
					      &Axioms, &Conjectures, &SortDecls,
					      &UserDefinedPrecedence, &UserDefinedSelection, &ClAxRelation,
					      &Includes, &Dummy);
  }

  /* Print the thing*/
  tptp_FPrintDFGProblem(OutFile,Description,Clauses,ClAxRelation,Axioms,Conjectures,Includes,tptp_WasParsingCNF());

  misc_CloseFile(InFile,cmdlne_GetInputFile());
  misc_CloseFile(OutFile,cmdlne_GetOutputFile());

  /* Free the variables */
  desc_Delete(Description);

  list_DeleteWithElement(Clauses, (void (*)(POINTER))clause_Delete);
  list_DeleteWithElement(Axioms, tptp_DeleteLabelTermPair);
  list_DeleteWithElement(Conjectures, tptp_DeleteLabelTermPair);
  list_DeleteWithElement(Includes, tptp_DeleteStrListOfStrPair);

  dfg_DeleteClAxRelation(ClAxRelation);

#ifdef CHECK
  if (list_Exist(SortDecls) || list_Exist(UserDefinedPrecedence)
      || list_Exist(UserDefinedSelection)) {
    misc_StartUserErrorReport();
    misc_ErrorReport("SortDecls, UserDefinedPrecedence, and UserDefinedSelection should all be empty!\n");
    misc_FinishUserErrorReport();
  }
#endif

  /*Free the modules*/
  tptp_Free();
  cmdlne_Free();
  flag_DeleteStore(Flags);
  symbol_DeletePrecedence(Precedence);
  fol_Free();
  symbol_FreeAllSymbols();
#ifdef CHECK
  memory_Print();
#endif
  
  return EXIT_SUCCESS;
}

static void tptp_FPrintFormulaList(FILE* File, LIST Formulas, const char* Name)
/**************************************************************
  INPUT:   A file, a list of formulas, a name.
  EFFECTS: Print a list formulas in DFG format, with given list name.
**************************************************************/
{
  LIST scan;

  fputs("list_of_formulae(", File);
  fputs(Name, File);
  fputs(").\n", File);
  for (scan = Formulas; !list_Empty(scan); scan= list_Cdr(scan)) {
    LIST formula_pair = list_Car(scan);
  
    fputs("\tformula(", File);
    fol_FPrintDFG(File, list_PairSecond(formula_pair));
    fputs(",", File);
    fputs((char*)list_PairFirst(formula_pair), File);
    fputs(").\n", File);
  }
  fputs("end_of_list.\n\n", File);
}

static void tptp_FPrintClauseWithLabel(FILE* File, CLAUSE Clause, const char* label) {
  int     n,j;
  LITERAL Lit;
  TERM    Atom;
  LIST    Variables,Iter;
  
  n = clause_Length(Clause);

  fputs("\tclause(", File);
  Variables = list_Nil();

  for (j = 0; j < n; j++)
    Variables =
      list_NPointerUnion(Variables,
			 term_VariableSymbols(clause_GetLiteralAtom(Clause,j)));

  if (!list_Empty(Variables)) {
    symbol_FPrint(File, fol_All());
    fputs("([", File);
    for (Iter = Variables; !list_Empty(Iter); Iter = list_Cdr(Iter)) {
      symbol_FPrint(File, (SYMBOL) list_Car(Iter));
      if (!list_Empty(list_Cdr(Iter)))
	putc(',', File);
    }
    fputs("],", File);
  }
    
  symbol_FPrint(File, fol_Or());
  putc('(', File);  

  for (j = 0; j < n; j++) {
    Lit  = clause_GetLiteral(Clause,j);
    Atom = clause_LiteralSignedAtom(Lit);
    term_FPrintPrefix(File,Atom);
    if (j+1 < n)
      putc(',', File);
  }
  if (n==0)
    symbol_FPrint(File,fol_False());

  if (!list_Empty(Variables)) {
    list_Delete(Variables);
    putc(')', File);
  }
  
  fprintf(File, "),%s", label);
  
  fputs(").\n", File);
}

static void tptp_FPrintClauseList(FILE* File, LIST Clauses,
				  LIST ClAxRelation, BOOL PrintConjectures, const char* Name)
/**************************************************************
  INPUT:   A file, a list of clauses, a flag and a name.
  EFFECTS: Print a list formulas in DFG format, with given list name.
           Prints the fromula only when
          ("the formula is a conjecture clause" == PrintConjectures)
**************************************************************/
{
  LIST Scan;
  
  fputs("list_of_clauses(", File);
  fputs(Name, File);
  fputs(",cnf).\n", File);
  
  for (Scan=Clauses; !list_Empty(Scan);Scan=list_Cdr(Scan)) {
    CLAUSE Clause;
    LIST ClAx;
    LIST Labels;

    Clause = (CLAUSE) list_Car(Scan);

#ifdef CHECK
    if (list_Empty(ClAxRelation)) {
      misc_StartUserErrorReport();
      misc_ErrorReport("ClAxRelation too short!\n");
      misc_FinishUserErrorReport();
    }
#endif

    ClAx = (LIST) list_Car(ClAxRelation);

#ifdef CHECK
    if ((int)list_Car(ClAx) != clause_Number(Clause)) {
      misc_StartUserErrorReport();
      misc_ErrorReport("Incompatible ClAxRelation!\n");
      misc_FinishUserErrorReport();
    }
#endif

    Labels = list_Cdr(ClAx);

#ifdef CHECK
    if (list_Empty(Labels)) {
      misc_StartUserErrorReport();
      misc_ErrorReport("Empty ClAxRelation entry!\n");
      misc_FinishUserErrorReport();
    }
#endif
    
    if (PrintConjectures == clause_GetFlag(Clause,CONCLAUSE))
      tptp_FPrintClauseWithLabel(File,Clause,(char*)list_Car(Labels) /*the first and the only one*/);

    ClAxRelation = list_Cdr(ClAxRelation);
  }
  fputs("end_of_list.\n\n", File);
}

static void tptp_FPrintInclude(FILE* File, char* filename, LIST selection) 
/**************************************************************
  INPUT:   A file, name of file included, list of formule secelection.
  EFFECTS: Print a line in DFG format denoting the given inclusion.
**************************************************************/
{
  int len;
	
  fputs("\tinclude('",File);
	
  len = strlen(filename);
  if ((len >= 2) && (filename[len-1] == 'p') && (filename[len-2] == '.')) { /*changing *.p to *.dfg */
    filename[len-1] = '\0';
    fputs(filename,File);
    fputs("dfg",File);
    filename[len-1] = 'p';
  } else if ((len >= 3) && (filename[len-1] == 'x') && (filename[len-2] == 'a') && (filename[len-3] == '.')) { /*changing *.ax to *.ax.dfg */
    fputs(filename,File);
    fputs(".dfg",File);
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
	
static __inline__ const char* Null2Empty(char * str) {
  return str ? str : "";
}

static void tptp_FPrintDFGProblem(FILE* File, DFGDESCRIPTION Description,
				  LIST Clauses, LIST ClAxRelation,
				  LIST Axioms, LIST Conjectures,
				  LIST Includes, BOOL IsCnf)
/**************************************************************
  INPUT:   A file, some info strings, two lists of formulas, and a boolean flag
  EFFECTS: Prints a complete DFG file containing these lists.
**************************************************************/
{
  fputs("begin_problem(Unknown).\n\n", File);
  
  fputs("list_of_descriptions.\n", File);
  fprintf(File,"name({*%s*}).\n",Null2Empty(Description->name));
  fprintf(File,"author({*%s*}).\n",Null2Empty(Description->author));
  if (Description->version)
    fprintf(File,"version({*%s*}).\n",Null2Empty(Description->version));
  if (Description->logic)
    fprintf(File,"logic({*%s*}).\n",Null2Empty(Description->logic));

  fprintf(File,"status(%s).\n",desc_StatusString(Description));

  fprintf(File,"description({*%s*}).\n",Null2Empty(Description->description));
  if (Description->date)
    fprintf(File,"date({*%s*}).\n",Null2Empty(Description->date));
  fputs("end_of_list.\n\n", File);
  
  fputs("list_of_symbols.\n", File);
  symbol_FPrintDFGSignature(File);
  fputs("end_of_list.\n\n", File);

  if (!IsCnf) {
    tptp_FPrintFormulaList(File, Axioms, "axioms");
    tptp_FPrintFormulaList(File, Conjectures, "conjectures");
  } else {
    tptp_FPrintClauseList(File, Clauses, ClAxRelation, FALSE, "axioms");
    tptp_FPrintClauseList(File, Clauses, ClAxRelation, TRUE, "conjectures");
  }

  if (list_Exist(Includes)) {
    LIST Scan;
	  
    fputs("list_of_includes.\n", File);

    for (Scan = Includes; !list_Empty(Scan); Scan = list_Cdr(Scan))	{
      LIST pair = list_Car(Scan);
      tptp_FPrintInclude(File,list_PairFirst(pair),list_PairSecond(pair));
    }
	
    fputs("end_of_list.\n\n", File);  
  }
     
  fputs("end_problem.\n", File);
}
