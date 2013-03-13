/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *              GENERATING APPROXIMATIONS                 * */
/* *                                                        * */
/* *  $Module:   DFG2DFG                                    * */ 
/* *                                                        * */
/* *  Copyright (C) 2000, 2001 MPI fuer Informatik          * */
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
/* $Revision: 1.7 $                                         * */
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


/* $RCSfile: dfg2dfg.c,v $ */


#include "dfg.h"
#include "cmdline.h"
#include "approx.h"
#include "eml.h"
#include "description.h"

#define DFG2DFG__VERSION "1.6"

static void dfg2dfg_ResetSelectedLiterals(LIST Clauses)
/**********************************************************
  INPUT:   A list of clauses.
  RETURNS: Nothing.
***********************************************************/
{
  LIST Scan;

  for (Scan = Clauses; !list_Empty(Scan); Scan = list_Cdr(Scan)) {
    NAT i;
    CLAUSE Clause;

    Clause = (CLAUSE) list_Car(Scan);

    /* For each clause in the list, if the clause contains selected
       literals, remove the selected status of the literals. */
    if (clause_GetFlag(Clause, CLAUSESELECT)) {
      for (i = clause_FirstAntecedentLitIndex(Clause); i <= clause_LastAntecedentLitIndex(Clause); i++)
        clause_LiteralClearFlag(clause_GetLiteral(Clause, i), LITSELECT);
      clause_RemoveFlag(Clause, CLAUSESELECT);
    }
  }
}

int main(int argc, const char* argv[])
{
  LIST       Clauses, Axioms, Conjectures, SortDecls, 
             UserPrecedence, UserSelection, ClAxRelation;
  const char *Filename;
  const char *Creator = "{* dfg2dfg Version " DFG2DFG__VERSION " *}";
  FILE       *File;
  int        value;
  FLAGSTORE  Flags;
  PRECEDENCE Precedence;
  BOOL       HasPlainClauses;
  DFGDESCRIPTION Description;

  /* Initialization */
  memory_Init(memory__UNLIMITED);
  atexit(memory_FreeAllMem);
  symbol_Init(TRUE);
  stack_Init();
  term_Init();
  flag_Init(flag_SPASS);
  flag_Init(flag_DFG2DFG);

  Flags = flag_CreateStore();
  flag_InitStoreByDefaults(Flags);
  Precedence = symbol_CreatePrecedence();
  Description = desc_Create();

  fol_Init(TRUE, Precedence);
  eml_Init(Precedence);
  clause_Init();
  approx_Init();
  cmdlne_Init(flag_SPASS);
  cmdlne_Init(flag_DFG2DFG);

  if (!cmdlne_Read(argc, argv) ||
      cmdlne_GetInputFile() == (char*)NULL) {
    /* print options */
    fputs("\n\t          dfg2dfg Version ", stdout);
    fputs(DFG2DFG__VERSION, stdout);
    fputs("\nUsage: dfg2dfg [-horn] [-linear] [-monadic[=n]]", stdout);
    puts(" [-shallow[=m]] input [output]\n");
    puts("See the man page or the postscript documentation for more details.");
    return EXIT_FAILURE;
  }

  if (!cmdlne_SetFlags(Flags))
    return EXIT_FAILURE;

  Filename = cmdlne_GetInputFile();
  File     = misc_OpenFile(Filename, "r");

  /* Call the parser */
  Axioms         = list_Nil();
  Conjectures    = list_Nil();
  SortDecls      = list_Nil();
  UserPrecedence = list_Nil();
  UserSelection  = list_Nil();
  ClAxRelation   = list_Nil();
  
  Clauses = dfg_DFGParser(File,Flags,Precedence,Description,&Axioms,&Conjectures,
			  &SortDecls, &UserPrecedence, &UserSelection,
			  &ClAxRelation, &HasPlainClauses);

  misc_CloseFile(File, Filename);

  /* Formulae are ignored */
  dfg_DeleteFormulaPairList(Axioms);
  dfg_DeleteFormulaPairList(Conjectures);
  dfg_DeleteFormulaPairList(SortDecls);

  if (list_Empty(Clauses)) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n No clauses found in input file!\n");
    misc_FinishUserErrorReport();
  }
  
  /* Apply some approximations */
  /* 1. the transformation to horn clauses */
  if (flag_GetFlagIntValue(Flags, flag_DFG2DFGHORN) == flag_ON) {
    Conjectures = list_Nil();
    for ( ; !list_Empty(Clauses); Clauses = list_Pop(Clauses)) {
      Axioms = approx_MakeHorn(list_Car(Clauses), Flags, Precedence);
      if (list_Empty(Axioms))
	Conjectures = list_Nconc(Conjectures, list_List(list_Car(Clauses)));
      else {
	clause_Delete(list_Car(Clauses));
	Conjectures = list_Nconc(Conjectures, Axioms);
      }
    }
    Clauses = Conjectures;
  }

  /* 2. the transformation to monadic literals */
  if (flag_GetFlagIntValue(Flags, flag_DFG2DFGMONADIC) != flag_OFF) {
    if (flag_GetFlagIntValue(Flags, flag_DFG2DFGMONADIC) == flag_DFG2DFGMONADICPROJECT) {
      Conjectures = approx_MonadicByProjection(Clauses, Flags, Precedence);
      clause_DeleteClauseList(Clauses);
      Clauses = Conjectures;
    }
    else if (flag_GetFlagIntValue(Flags, flag_DFG2DFGMONADIC) == flag_DFG2DFGMONADICTERM) {
      Conjectures = approx_MonadicByTermEncoding(Clauses, Flags, Precedence);
      clause_DeleteClauseList(Clauses);
      Clauses = Conjectures;
    }
    else {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\nError: argument of option 'monadic' must be 0, 1 or 2\n");
      misc_FinishUserErrorReport();
    }
  }

  /* 3. the linear transformation */
  if (flag_GetFlagIntValue(Flags, flag_DFG2DFGLINEAR) == flag_ON) {
    Conjectures = approx_MonadicMakeLinear(Clauses, Flags, Precedence);
    clause_DeleteClauseList(Clauses);
    Clauses = Conjectures;
  }

  /* 4. the shallow transformation */
  if (flag_GetFlagIntValue(Flags, flag_DFG2DFGSHALLOW) != flag_OFF) {
    BOOL flag1, flag2;
    flag1 = flag2 = TRUE;
    value = flag_GetFlagIntValue(Flags, flag_DFG2DFGSHALLOW);
    if (value == 2)
      flag1 = FALSE;
    else if (value == 3)
      flag1 = flag2 = FALSE;
    else if (value != 0 && value != 1) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("Error: argument of option 'shallow' must be 0, 1, 2 or 3\n");
      misc_FinishUserErrorReport();
    }

    if (value != 0) {
      Conjectures = list_Nil();
      for ( ; !list_Empty(Clauses); Clauses = list_Pop(Clauses)) {
	Axioms = approx_HornMonadicFlattenHeads(list_Car(Clauses), flag1,
						flag2, Flags, Precedence);
	if (list_Empty(Axioms))
	  Conjectures = list_Nconc(Conjectures, list_List(list_Car(Clauses)));
	else {
	  clause_Delete(list_Car(Clauses));
	  Conjectures = list_Nconc(Conjectures, Axioms);
	}
      }
      Clauses = Conjectures;
    }
  }
    
  /* Print transformed clauses to stdout by default */
  File = stdout;
  if (cmdlne_GetOutputFile()) {
    /* Name of output file is given */
    File = misc_OpenFile(cmdlne_GetOutputFile(), "w");
  }

  /* Do not print the selected status of the literals. */
  dfg2dfg_ResetSelectedLiterals(Clauses);

  clause_FPrintCnfDFGProblem(File, FALSE, "{**}", 
			     Creator,
			     "unknown",
			     "{**}", Clauses, NULL, 
                             Flags, Precedence, NULL, FALSE, FALSE);
  
  if (cmdlne_GetOutputFile()) {
    /* Name of output file is given */
    misc_CloseFile(File, cmdlne_GetOutputFile());
  }
  clause_DeleteClauseList(Clauses);
  eml_Free();
  flag_DeleteStore(Flags);
  symbol_DeletePrecedence(Precedence);
  list_Delete(UserPrecedence);
  list_Delete(UserSelection);
  dfg_DeleteClAxRelation(ClAxRelation);
  desc_Delete(Description);
  
  cmdlne_Free();
  fol_Free();
  symbol_FreeAllSymbols();

#ifdef CHECK
  memory_Print();
  memory_PrintLeaks();
#endif

  putchar('\n');
  return 0;
}

