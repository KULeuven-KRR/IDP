/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *              DFG2ASCII                                 * */
/* *                                                        * */
/* *  $Module:    DFG2ASCII                                 * */ 
/* *                                                        * */
/* *  Copyright (C) 1999, 2000, 2001 MPI fuer Informatik    * */
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


/* $RCSfile: dfg2ascii.c,v $ */


/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include "dfg.h"
#include "clause.h"
#include "foldfg.h"
#include "flags.h"
#include "cmdline.h"
#include "eml.h"
#include "description.h"

#include <errno.h>

#define DFG2ASCII__VERSION "0.47"

/**************************************************************/
/* Main Function                                              */
/**************************************************************/

int main(int argc, const char* argv[])
{
  LIST       Clauses,Axioms,Conjectures,Sorts,Scan, 
             UserPrecedence,UserSelection,ClAxRelation;
  FILE       *Input;
  CLAUSE     Clause;
  const char *Filename;
  FLAGSTORE  Flags;
  PRECEDENCE Precedence;
  BOOL       HasPlainClauses;
  DFGDESCRIPTION Description;

  memory_Init(memory__UNLIMITED);
  atexit(memory_FreeAllMem);
  symbol_Init(TRUE);
  stack_Init();
  term_Init();
  flag_Init(flag_SPASS);
  cmdlne_Init();

  Flags = flag_CreateStore();
  flag_InitStoreByDefaults(Flags);
  Precedence = symbol_CreatePrecedence();
  Description = desc_Create();

  fol_Init(TRUE, Precedence);
  eml_Init(Precedence);
  clause_Init();

  if (argc < 2 || !cmdlne_Read(argc, argv)) {
    fputs("\n\t          dfg2ascii Version ", stdout);
    fputs(DFG2ASCII__VERSION, stdout);
    puts("\n\t       Usage: dfg2ascii <input-file>\n");
    return EXIT_FAILURE;
  }
  
  if (!cmdlne_SetFlags(Flags))
    return EXIT_FAILURE;

  Axioms         = list_Nil();
  Conjectures    = list_Nil();
  Sorts          = list_Nil();
  UserPrecedence = list_Nil();
  UserSelection  = list_Nil();
  ClAxRelation   = list_Nil();

  Filename = cmdlne_GetInputFile();
  Input    = misc_OpenFile(Filename,"r");
  Clauses  = dfg_DFGParser(Input, Flags, Precedence, Description, &Axioms, &Conjectures,
			   &Sorts, &UserPrecedence, &UserSelection, &ClAxRelation,
                           &HasPlainClauses);
  misc_CloseFile(Input,Filename);

  Axioms = list_Nconc(Axioms, Sorts);

  if (!list_Empty(Axioms) || !list_Empty(Conjectures)) {
    puts("\n\n\t\t Axioms:\n");
    if (list_Empty(Axioms))
      puts("None.\n");
    else
      for (Scan=Axioms; !list_Empty(Scan);Scan=list_Cdr(Scan)) {
	if (list_PairFirst(list_Car(Scan)) != NULL)
	  printf("%s:\n",(char *)list_PairFirst(list_Car(Scan)));
	fol_PrettyPrintDFG(list_PairSecond(list_Car(Scan)));
	puts("\n");
      }
    puts("\n\n\t\t Conjectures:\n");
    if (list_Empty(Conjectures))
      puts("None.\n");
    else
      for (Scan=Conjectures; !list_Empty(Scan);Scan=list_Cdr(Scan)) {
	if (list_PairFirst(list_Car(Scan)) != NULL)
	  printf("%s:\n",(char *)list_PairFirst(list_Car(Scan)));
	fol_PrettyPrintDFG(list_PairSecond(list_Car(Scan)));
	puts("\n");
      }
  }
  else {
    BOOL SetExist;
    LIST ClauseScan;

    /* Before we sort the clauses, we need to make sure that they have been
       assigned a weight.
    */
    for (ClauseScan = Clauses; !list_Empty(ClauseScan); ClauseScan = list_Cdr(ClauseScan)) {
      clause_UpdateWeight((CLAUSE) list_Car(ClauseScan), Flags);
    }
    
    Clauses   = clause_ListSortWeighed(Clauses);
    clause_SetCounter(1);
    for (Scan = Clauses;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
      Clause = (CLAUSE)list_Car(Scan);
      clause_SetSortConstraint(Clause, FALSE, Flags, Precedence);
      clause_NewNumber(Clause);
      clause_OrientAndReInit(Clause, Flags, Precedence);
    }
    puts("\n\n\t\t Axiom Clauses:\n");
    SetExist = FALSE;
    for (Scan = Clauses;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
      Clause = (CLAUSE)list_Car(Scan);
      if (!clause_GetFlag(Clause,CONCLAUSE)) {
	SetExist = TRUE;
	clause_Print(Clause);
	putchar('\n');
      }
    }
    if (SetExist)
      SetExist = FALSE;
    else
      puts("None.\n");
    puts("\n\n\t\t Conjecture Clauses:\n");
    for (Scan = Clauses;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
      Clause = (CLAUSE)list_Car(Scan);
      if (clause_GetFlag(Clause,CONCLAUSE)) {
	SetExist = TRUE;
	clause_Print(Clause);
	putchar('\n');
      }
    }
    if (SetExist)
      SetExist = FALSE;
    else
      puts("None.\n");
  }

  clause_DeleteClauseList(Clauses);
  dfg_StripLabelsFromList(Axioms);
  dfg_StripLabelsFromList(Conjectures);
  term_DeleteTermList(Axioms);
  term_DeleteTermList(Conjectures);

  eml_Free();
  flag_DeleteStore(Flags);
  symbol_DeletePrecedence(Precedence);
  list_Delete(UserPrecedence);
  list_Delete(UserSelection);
  dfg_DeleteClAxRelation(ClAxRelation);
  desc_Delete(Description);
  
  /*symbol_Dump();*/
  cmdlne_Free();
  fol_Free();
  symbol_FreeAllSymbols();
#ifdef CHECK
  memory_Print();
#endif
  return 0;
}


