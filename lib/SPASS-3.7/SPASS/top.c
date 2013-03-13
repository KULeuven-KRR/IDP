/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *              TOP MODULE OF SPASS                       * */
/* *                                                        * */
/* *  $Module:   TOP                                        * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001      * */
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
/* *                                                        * */
/* $Revision: 1.21 $                                       * */
/* $State: Exp $                                            * */
/* $Date: 2010-02-22 14:10:00 $                             * */
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


/* $RCSfile: top.c,v $ */

/*** MAIN LOOP *************************************************/


/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include "dfg.h"
#include "description.h"
#include "defs.h"
#include "ia.h"
#include "rules-inf.h"
#include "rules-sort.h"
#include "rules-split.h"
#include "terminator.h"
#include "rules-red.h"
#include "analyze.h"
#include "clock.h"
#include "strings.h"
#include "cmdline.h"
#include "context.h"
#include "cnf.h"
#include "search.h"
#include "hasharray.h"
#include "closure.h"
#include "eml.h"
#include "tptp.h"
#include <errno.h>
#include <stdlib.h>


/**************************************************************/
/* Types and Variables                                        */
/**************************************************************/

static const char *top_InputFile;
static char *top_DiscoveredFile = (char*)NULL;
static int top_NoAlarm = 0;                     /* prevent race between handler and poll: 0 no result, 1 poll printed, -1 SIGNAL printed */
BOOL top_RemoveInputFile;


typedef enum {top_PROOF, top_COMPLETION, top_RESOURCE} top_SEARCHRESULT;


/**************************************************************/
/* Catch Signals Section                                      */
/**************************************************************/

#ifdef SPASS_SIGNALS
#include <signal.h>

static PROOFSEARCH *top_PROOFSEARCH;

static void top_SigHandler(int Signal)
/**************************************************************
  INPUT:   
  RETURNS: 
  EFFECT:  
***************************************************************/
{
  if (Signal == SIGSEGV || Signal == SIGBUS) {
    puts("\n\n\tSPASS is going to crash. This is probably caused by a");
    puts("\tbug in SPASS.  Please send a copy of the input file(s) together");
    puts("\twith the used options to spass@mpi-inf.mpg.de, and someone will");
    puts("\ttry to fix the problem.\n");
    abort();
  }

  if (Signal == SIGINT || Signal == SIGTERM || (Signal == SIGALRM && top_NoAlarm == 0)) {
    top_NoAlarm = -1;
    clock_StopPassedTime(clock_OVERALL);
    printf("\nSPASS %s ", vrs_VERSION);
    puts("\nSPASS beiseite: Ran out of time. SPASS was killed.");
    printf("Problem: %s ", 
	   (top_DiscoveredFile != (char*)NULL ? top_DiscoveredFile : "Read from stdin."));
    printf("\nSPASS derived %d clauses, backtracked %d clauses, performed %d splits, "
	   "and kept %d clauses.",
	   (*top_PROOFSEARCH == (PROOFSEARCH)NULL ? 0 : prfs_DerivedClauses(*top_PROOFSEARCH)),
	   (*top_PROOFSEARCH == (PROOFSEARCH)NULL ? 0 : prfs_BacktrackedClauses(*top_PROOFSEARCH)),
	   (*top_PROOFSEARCH == (PROOFSEARCH)NULL ? 0 : prfs_SplitCounter(*top_PROOFSEARCH)),
	   (*top_PROOFSEARCH == (PROOFSEARCH)NULL ? 0 : prfs_KeptClauses(*top_PROOFSEARCH)));
    printf("\nSPASS allocated %lu KBytes.", memory_DemandedBytes()/1024);
    fputs("\nSPASS spent\t", stdout);
    clock_PrintTime(clock_OVERALL);
    fputs(" on the problem.\n\t\t", stdout);
    clock_PrintTime(clock_INPUT);
    fputs(" for the input.\n\t\t", stdout);
    clock_PrintTime(clock_CNF);
    fputs(" for the FLOTTER CNF translation.\n\t\t", stdout);
    clock_PrintTime(clock_INFERENCE);
    fputs(" for inferences.\n\t\t", stdout);
    clock_PrintTime(clock_BACKTRACK);
    fputs(" for the backtracking.\n\t\t", stdout);
    clock_PrintTime(clock_REDUCTION);
    puts(" for the reduction.");
  }

  if (top_RemoveInputFile) /* Only files in current directory might get removed, not with respect to environment variables */ 
    remove(top_InputFile);

  exit(EXIT_FAILURE);
}

#endif


/**************************************************************/
/* Clause Selection Functions                                 */
/**************************************************************/

static CLAUSE top_SelectClauseDepth(LIST List, FLAGSTORE Flags)
/**************************************************************
  INPUT:   A list of clauses and a flag store.
  RETURNS: A clause selected from the list.
  EFFECT:  This function selects a clause from the list according
           to the following criteria:
	   1. minimal depth
           2. minimal weight
	   3a. maximal number of variable occurrences, if the flag
	       'PrefVar' is TRUE
	   3b. minimal number of variable occurrences, if 'PrefVar'
	       is FALSE
***************************************************************/
{
  CLAUSE Result;
  NAT    Vars,NewVars,Weight,Depth,NewDepth;

  Result = (CLAUSE)list_Car(List);
  Depth  = clause_Depth(Result);
  Weight = clause_Weight(Result);
  Vars   = clause_NumberOfVarOccs(Result);
  List   = list_Cdr(List);

  while (!list_Empty(List)) {
    NewDepth =  clause_Depth(list_Car(List));
    if (NewDepth <= Depth) {
      if (NewDepth < Depth || clause_Weight(list_Car(List)) < Weight) {
	Depth  = NewDepth;
	Result = (CLAUSE)list_Car(List);
	Weight = clause_Weight(Result);
	Vars   = clause_NumberOfVarOccs(list_Car(List));
      }
      else
	if (clause_Weight(list_Car(List)) == Weight) {
	  NewVars = clause_NumberOfVarOccs(list_Car(List));
	  if (flag_GetFlagIntValue(Flags, flag_PREFVAR)) {
	    if (Vars < NewVars) {
	      Depth  = NewDepth;
	      Result = (CLAUSE)list_Car(List);
	      Weight = clause_Weight(Result);
	      Vars   = NewVars;
	    }
	  }
	  else
	    if (Vars > NewVars) {
	      Depth  = NewDepth;
	      Result = (CLAUSE)list_Car(List);
	      Weight = clause_Weight(Result);
	      Vars   = NewVars;
	    }
	}
    }
    List = list_Cdr(List);
  }

  return Result;
}


static CLAUSE top_SelectMinimalWeightClause(LIST List, FLAGSTORE Flags)
/**************************************************************
  INPUT:   A list of clauses and a flag store.
  RETURNS: A clause selected from the list.
  EFFECT:  This function selects a clause with minimal weight.
           If more than one clause has minimal weight and the flag
	   'PrefVar' is TRUE, a clause with maximal number of variable
	   occurrences is selected. If 'PrefVar' is FALSE, a clause with
	   minimal number of variable occurrences is selected.
	   If two clauses are equal with respect to the two criteria
	   the clause with the smaller list position is selected.
  CAUTION: THE LIST HAS TO BY SORTED BY WEIGHT IN ASCENDING ORDER!
***************************************************************/
{
  CLAUSE Result;
  NAT    Vars, NewVars, Weight;

#ifdef CHECK
  /* Check invariant: List has to be sorted by weight (ascending) */
  LIST Scan;
  Weight = clause_Weight(list_Car(List));
  for (Scan = list_Cdr(List); !list_Empty(Scan); Scan = list_Cdr(Scan)) {
    NAT NewWeight;
    NewWeight = clause_Weight(list_Car(Scan));
    if (NewWeight < Weight) {
      misc_StartErrorReport();
      misc_ErrorReport("\n In top_SelectMinimalConWeightClause: clause list ");
      misc_ErrorReport("isn't sorted by weight");
      misc_FinishErrorReport();
    }
    Weight = NewWeight;
  }
#endif

  Result = (CLAUSE)list_Car(List);
  Weight = clause_Weight(Result);
  Vars   = clause_NumberOfVarOccs(Result);
  List   = list_Cdr(List);

  while (!list_Empty(List)) {
    if (clause_Weight(list_Car(List)) == Weight) {
      NewVars = clause_NumberOfVarOccs(list_Car(List));
      if (flag_GetFlagIntValue(Flags, flag_PREFVAR)) {
	if (Vars < NewVars) {
	  Result = (CLAUSE)list_Car(List);
	  Weight = clause_Weight(Result);
	  Vars   = NewVars;
	}
      }
      else
	if (Vars > NewVars) {
	  Result = (CLAUSE)list_Car(List);
	  Weight = clause_Weight(Result);
	  Vars   = NewVars;
	}
    }
    else
      return Result;
    List = list_Cdr(List);
  }
  return Result;
}


static CLAUSE top_SelectMinimalConWeightClause(LIST List, FLAGSTORE Flags)
/**************************************************************
  INPUT:   A non-empty list of clauses and a flag store.
  RETURNS: A clause selected from the list.
  EFFECT:  This function selects a clause from the list in a
           similar way as the function top_SelectMinimalWeightClause.
	   The only difference is that conjecture clauses are
	   preferred over axiom clauses, because their weight
	   is divided by a factor given by the flag 'PrefCon'.
***************************************************************/
{
  CLAUSE Result;
  NAT    NewWeight,Weight, NewVars, Vars, Factor;

  Result = (CLAUSE)list_Car(List);
  Factor = flag_GetFlagIntValue(Flags, flag_PREFCON);
  Weight = clause_Weight(Result);
  if (clause_GetFlag(Result, CONCLAUSE))
    Weight = Weight / Factor;
  Vars   = clause_NumberOfVarOccs(list_Car(List));
  List   = list_Cdr(List);

  while (!list_Empty(List)) {
    NewWeight = clause_Weight(list_Car(List));
    if (clause_GetFlag(list_Car(List),CONCLAUSE))
      NewWeight = NewWeight / Factor;
    if (NewWeight < Weight) {
      Weight = NewWeight;
      Result = list_Car(List);
      Vars   = clause_NumberOfVarOccs(list_Car(List));
    }
    else {
      if (NewWeight == Weight) {
	NewVars = clause_NumberOfVarOccs(list_Car(List));
	if (flag_GetFlagIntValue(Flags, flag_PREFVAR)) {
	  if (Vars < NewVars) {
	    Result = (CLAUSE)list_Car(List);
	    Weight = NewWeight;
	    Vars   = NewVars;
	  }
	}
	else
	  if (Vars > NewVars) {
	    Result = (CLAUSE)list_Car(List);
	    Weight = NewWeight;
	    Vars   = NewVars;
	  }
      }
    }

    List = list_Cdr(List);
  }
  return Result;
}


/*static CLAUSE top_SelectClauseDepth(LIST List)*/
/**************************************************************
  INPUT:   A list of clauses.
  RETURNS: A clause selected from the list.
  EFFECT:  
***************************************************************/
/*{
  CLAUSE Result;
  int    Min, Depth;

  Result = (CLAUSE)list_Car(List);
  Depth  = clause_Depth(Result);
  Min    = Depth * clause_Weight(Result);
  List   = list_Cdr(List);

  if (Depth == 0)
  return Result;

  while (!list_Empty(List)) {
  Depth =  clause_Depth(list_Car(List));
  if (Min > Depth * clause_Weight(list_Car(List))) {
  Result = list_Car(List);
  if (Depth == 0)
  return Result;
  Min    = clause_Depth(Result) * clause_Weight(Result);
  }
  List = list_Cdr(List);
  }

  return Result;
  }*/


static LIST top_GetLiteralsForSplitting(CLAUSE Clause, BOOL SuccedentOnly)
/**************************************************************
  INPUT:   A clause and a flag whether only succedent literals should be
           considered
  RETURNS: A list of literal sets where every single
           set doesn't share any variables with other literal sets..
           If SuccedentOnly is TRUE, only succedent lits are considered.
           The literal sets contain  the full splits.
           Returns list_Nil() if the clause has only one component.
***************************************************************/
{
  LIST* Variables;  /* An array, mapping literal index to list of variables */
  LIST* Cluster;    /* potential variable sharing clusters of considered literals */
  int   i, j, EndIndex;
  LIST  Result,Scan1, Scan2;
  BOOL  NeedTest;

  Result   = list_Nil();
  EndIndex = (SuccedentOnly ? clause_FirstSuccedentLitIndex(Clause) : clause_FirstLitIndex());

  /* Special case: clause is ground, so return all literals */
  if (clause_IsGround(Clause)) {
    for (i = clause_LastLitIndex(Clause); i >= EndIndex; i--)
      Result = list_Cons(list_List(clause_GetLiteral(Clause,i)), Result);
    return Result;
  }

  
  Variables = memory_Malloc(sizeof(LIST) * clause_Length(Clause));
  Cluster   = memory_Malloc(sizeof(LIST) * clause_Length(Clause)); /* not necessarily all needed */
  /* Initialize the arrays */
  for (i = clause_FirstLitIndex(); i <= clause_LastLitIndex(Clause); i++) {
    Variables[i] = term_VariableSymbols(clause_GetLiteralAtom(Clause, i));
    Cluster[i]   = list_List((POINTER)i);
  }

  for (i = EndIndex; i <= clause_LastLitIndex(Clause); i++) {
    if (!list_Empty(Cluster[i])) {   /* not already clustered */
      NeedTest = TRUE;
      while (NeedTest) {
	NeedTest = FALSE;
	/* We don't know yet whether the literal shares variables */
	for (j = clause_FirstLitIndex(); j <= clause_LastLitIndex(Clause); j++) {
	  if (i != j && list_HasIntersection(Variables[i], Variables[j])) {
	    Variables[i] = list_Nconc(Variables[j],Variables[i]);
	    Variables[j] = list_Nil();
	    Cluster[i]   = list_Nconc(Cluster[i], Cluster[j]);
	    Cluster[j]   = list_Nil();
	    NeedTest     =  TRUE;
	  }
	}
      }
    }
  }

  
  /* Compute Results */
  for (i = clause_FirstLitIndex(); i <= clause_LastLitIndex(Clause); i++) {
    list_Delete(Variables[i]);
    if (i < EndIndex) 
      list_Delete(Cluster[i]);
    else 
      if (!list_Empty(Cluster[i]))
	Result = list_Cons(Cluster[i], Result);
  }

  memory_Free(Variables, sizeof(LIST) * clause_Length(Clause));
  memory_Free(Cluster, sizeof(LIST) * clause_Length(Clause));

  if (list_Length(Result) == 1) {
    list_Delete(list_Car(Result));
    list_Delete(Result);
    Result = list_Nil();
  }


  for (Scan1=Result;!list_Empty(Scan1);Scan1=list_Cdr(Scan1))
    for (Scan2=list_Car(Scan1);!list_Empty(Scan2);Scan2=list_Cdr(Scan2))
      list_Rplaca(Scan2,clause_GetLiteral(Clause,(int)list_Car(Scan2)));
  

 /*  printf("\nSplits: "); clause_Print(Clause); printf("\n"); */
/*   for(Scan2=Result;!list_Empty(Scan2);Scan2=list_Cdr(Scan2)) { */
/*     printf("[ "); */
/*     for(Scan1=list_Car(Scan2);!list_Empty(Scan1);Scan1=list_Cdr(Scan1)) { */
/*       clause_LiteralPrint(list_Car(Scan1)); printf(" "); */
/*     } */
/*     printf("] "); */
/*   } */

  return Result;
}

#ifdef CHECK
static BOOL top_ClauseIsSplittable(CLAUSE Clause, BOOL SuccedentOnly)
{
  LIST Lits;
  BOOL Result;

  Lits   = top_GetLiteralsForSplitting(Clause, SuccedentOnly);
  Result = (Lits != list_Nil());
  while (!list_Empty(Lits)) {
    list_Delete(list_Car(Lits));
    Lits = list_Pop(Lits);
  }
  return Result;
}
#endif

static float top_ComputeSplitPotential(PROOFSEARCH Search, CLAUSE Clause)
/**************************************************************
  INPUT:   A proofsearch object <Search> and a clause.
  RETURNS: If the clause is a split clause, the average number of instances
           of all split literals from <Clause>
****************************************************************/
{
  LIST    SplitLitSets,Literals;
  float   NrOfInstances;
  LITERAL Literal;
  TERM    Atom;
  int     i;

  SplitLitSets  = list_Nil();
  NrOfInstances = 0.0;
  i             = 0;

  if (clause_HasSolvedConstraint(Clause) && 
      clause_Length(Clause) > 1 &&
      (!clause_IsHornClause(Clause) || 
       (!ana_PUREPROPOSITIONAL &&
	clause_GetFlag(Clause, CONCLAUSE) &&  /* Split non-Horn clauses or input conjecture clauses */
	clause_Depth(Clause) == 0))) {
    /* Get a list of splittable literal indices */
    SplitLitSets = top_GetLiteralsForSplitting(Clause, !clause_IsHornClause(Clause));
  }
  if (!list_Empty(SplitLitSets)) {
    for ( ; !list_Empty(SplitLitSets); SplitLitSets = list_Pop(SplitLitSets)) {
      Literals   = list_Car(SplitLitSets);
      while (!list_Empty(Literals)) {
	i++;
	Literal        = list_Car(Literals);
	Atom           = clause_LiteralAtom(Literal);
	NrOfInstances += prfs_GetNumberOfInstances(Search, Literal, TRUE);
	Literals       = list_Cdr(Literals);
      }
      list_Delete(list_Car(SplitLitSets));
    }
    NrOfInstances = NrOfInstances / i; /* Average Hits */
  }
  return NrOfInstances;
}
    

static LIST top_GetBestSplitLiterals(PROOFSEARCH Search, CLAUSE Clause, BOOL Usables)
/**************************************************************
  INPUT:   A proofsearch object <Search>, a <Clause> 
           and a bool <Usables> telling whether the Usable set schould also be
           considered for computing the best split
  RETURNS: The literal list out of the SplitLitSets
           from <Clause>, such that the positive
           split literals together with this literal have the maximal number of instances 
           in clauses from <Search> where <Usables> determins whether the
           usable clauses are considered too as instance candidates
****************************************************************/
{
  LITERAL Literal;
  LIST    Literals, SplitLitSets, ActLiterals;
  int     j;
  float   SetAverage, MaxSetAverage;
  TERM    Atom;

  MaxSetAverage  = 0.0;
  ActLiterals    = list_Nil();

  SplitLitSets = top_GetLiteralsForSplitting(Clause, !clause_IsHornClause(Clause));

  if (!list_Empty(SplitLitSets)) {
    for ( ; !list_Empty(SplitLitSets); SplitLitSets = list_Pop(SplitLitSets)) {
      Literals   = list_Car(SplitLitSets);
      SetAverage = 0.0;
      j          = 0;
      while (!list_Empty(Literals)) {
	j++;
	Literal     = list_Car(Literals);
	Atom        = clause_LiteralAtom(Literal);
	SetAverage += prfs_GetNumberOfInstances(Search, Literal, Usables);
	Literals    = list_Cdr(Literals);
      }
      SetAverage     = SetAverage / j;
      if (ActLiterals == list_Nil() || SetAverage > MaxSetAverage) {
	  MaxSetAverage   = SetAverage;
	  list_Delete(ActLiterals);
	  ActLiterals = list_Car(SplitLitSets);
      }
      else
	list_Delete(list_Car(SplitLitSets));
    }
  }

  return ActLiterals;
}



static LIST top_GetSplitLiterals(PROOFSEARCH Search, CLAUSE Clause, BOOL Usables)
/**************************************************************
  INPUT:   A proofsearch object, a clause and a boolean flag.
  RETURNS: The index of the positive literal from <Clause>
           with the greatest number of instances (maybe 0) within
	   the WorkedOff/Usable sets of the proof search object.
	   The literal mustn't share any variables with other literals.
	   If the clause doesn't have a suitable literal, a negative
	   number is returned.
  EFFECT:  If <Usables> is FALSE, only the number of instances
	   within the WorkedOff set is considered, otherwise
	   the number of instances within both clause sets is considered.
***************************************************************/
{
  float   MaxInstances;
  LIST    Result;
  
  MaxInstances  = 0.0;
  Result        = list_Nil();

  MaxInstances  = top_ComputeSplitPotential(Search, Clause);

  if (MaxInstances > 0.0 &&
      (ana_PUREPROPOSITIONAL || MaxInstances >= flag_GetFlagIntValue(prfs_Store(Search), flag_SPLITMININST)))
    Result = top_GetBestSplitLiterals(Search, Clause, Usables);

  return Result;
}



static CLAUSE top_GetPowerfulSplitClause(PROOFSEARCH Search, BOOL Usables, LIST* Literals, int MinInstances)
/**************************************************************
  INPUT:   A proofsearch object, a boolean flag, a pointer to a literal
           list which is used as return value and a minimum number
           of instances a potential split cause must match
  RETURNS: A clause from the usable set that was selected as given clause.
           Iff no suitable clause was found NULL is returned and <*LitIndex>
	   is set to -1.
	   Iff a suitable clause was found, this clause is returned and
	   <*LitIndex> is set to the index of the "optimal" literal, that
           is a literal that can be split off as unit witha high reduction
           potential
  EFFECT:  This function selects a clause from the "usable" set and
           a literal that are "optimal" for the application of the splitting
	   rule with respect to the following criteria:
           1) the literal must occur in the succedent of the non-horn clause,
	   2) the literal mustn't share any variables with other literals,
           3) the clause must have a solved constraint,
	   4) the atom must have the maximum number of instances
              a) within the set of "workedoff" clauses, iff <Usables>=FALSE
              b) within the set of "usable" and "workedoff" clauses,
                 iff "Usables"=TRUE
	   5) the atom must have at least one instance in another clause.
           6) if the split heuristic is flag_SPLITHEURISTICALWAYS the function
              always selects a non-Horn clause to be split if available
           7) if the split heutistic is  flag_SPLITHEURISTICGROUND then
              ground input conjecture clauses are split as well (even if
              the clause is Horn)
***************************************************************/
{
  LIST   Scan;
  CLAUSE Clause, OptimalClause;


  OptimalClause    = NULL;
  
  for (Scan = prfs_UsableClauses(Search); !list_Empty(Scan);
       Scan = list_Cdr(Scan)) {
    Clause = list_Car(Scan);
    if (clause_SplitPotential(Clause) > 0.0)
      if (OptimalClause == NULL || clause_SplitPotential(OptimalClause) < clause_SplitPotential(Clause))
	OptimalClause = Clause;
  }
  
  if (OptimalClause != (CLAUSE)NULL) {
    if (clause_SplitPotential(OptimalClause) > MinInstances) 
      *Literals = top_GetBestSplitLiterals(Search, OptimalClause, Usables);
    else
      OptimalClause = (CLAUSE)NULL;
  }

  return OptimalClause;
}


static LIST top_FullReductionSelectGivenComputeDerivables(PROOFSEARCH Search,
							  CLAUSE *SplitClause,
							  int *Counter, int *MinInstances)
/**************************************************************
  INPUT:   A proof search object, a pointer to a clause resulting from a
           previous splitting step, and a pointer to an integer counter and
           a pointer the minimum number of instances for a successful split application
  RETURNS: A list of derived clauses.
  EFFECT:  In this function a clause is selected from the set of
           "usable" clauses. After a clause was selected as "given clause",
	   inferences between the given clause and the "worked off" clauses
	   are made. The selection works as follows:
	   1) If <*SplitClause> is not NULL, the split clause
	      is selected as "given clause". <*SplitClause> should result
	      from splitting
	   2) If <*SplitClause> is NULL, we try to find a clause that is
	      "optimal" for splitting. This is done by selecting a literal
	      <L> in a clause, such that <L> is variable-disjoint from
	      the rest of the clause, and the atom of <L> has the maximum
	      number of instances within the set of "usable" and "workoff"
	      clauses.
	   3) If the previous steps failed, a clause is selected by weight
	      or by depth, depending on the parameters "WDRatio", "PrefVar"
	      and "PrefCon". Then splitting is tried on the selected clause.
	      If the clause is a non-horn clause, we try to find a positive
	      literal <L> and a set of negative literals <N>, such that <N>
	      and <L> are variable disjoint from the rest of the clause.
***************************************************************/
{
  CLAUSE     GivenClause, TerminatorClause;
  LIST       Derivables, SplitLits;
  FLAGSTORE  Flags;
  PRECEDENCE Precedence;

  GivenClause = NULL;
  Derivables  = list_Nil();
  Flags       = prfs_Store(Search);
  Precedence  = prfs_Precedence(Search);
  SplitLits   = list_Nil();

  /* 1) If the last given clause was split or if backtracking was applied, */
  /* then choose the clause resulting from the splitting step as        */
  /* given clause.                                                      */
  /* ATTENTION: Since the <SplitClause> might have been reduced since   */
  /* the time the variable was set, we have to check whether            */
  /* <SplitClause> is still element of the set of usable clauses.       */
  if (*SplitClause != NULL &&
      list_PointerMember(prfs_UsableClauses(Search), *SplitClause))
    GivenClause = *SplitClause;
  
  *SplitClause = NULL;

  if (GivenClause == NULL) {
    if (prfs_SplitsAvailable(Search, Flags)) {
      /* 2) Try to find an "optimal" splitting clause, that doesn't share */
      /*    variables with any other literal.                             */
      GivenClause = top_GetPowerfulSplitClause(Search, TRUE, &SplitLits, *MinInstances);
#ifdef CHECK
      if (GivenClause != (CLAUSE)NULL && !top_ClauseIsSplittable(GivenClause, FALSE)) {
	misc_StartErrorReport();
	misc_ErrorReport("\n In  top_FullReductionSelectGivenComputeDerivables: powerful split clause ");
	misc_ErrorReport("is not splittable.");
	misc_FinishErrorReport();
      }
#endif
    }
    if (GivenClause != NULL) {
      /* Found "optimal" split clause, so apply splitting */
      (*MinInstances)++;
      *SplitClause = prfs_DoSplitting(Search, GivenClause, SplitLits);
      list_Delete(SplitLits);      
    } else {
      /* 3) Splitting wasn't applied, so use the other strategies */
      *MinInstances = flag_GetFlagIntValue(Flags, flag_SPLITMININST);
      if ((*Counter) % flag_GetFlagIntValue(Flags, flag_WDRATIO) == 0)
	GivenClause = top_SelectClauseDepth(prfs_UsableClauses(Search), Flags);
      else {
	if (flag_GetFlagIntValue(Flags, flag_PREFCON) != flag_PREFCONUNCHANGED)
	  GivenClause = top_SelectMinimalConWeightClause(prfs_UsableClauses(Search),
							 Flags);
	else
	  GivenClause = top_SelectMinimalWeightClause(prfs_UsableClauses(Search),
						      Flags);
      }
      (*Counter)++; /* EK: hier lassen, oder eine Klammerebene nach aussen? */
    }
  }

  if (*SplitClause == NULL && prfs_SplitsAvailable(Search,Flags)) {
    /* Try to find the "optimal" literal for splitting the clause. */
    /* This makes sense for a clause that is the right part of a   */
    /* splitting step.                                             */
    SplitLits = top_GetSplitLiterals(Search, GivenClause, FALSE);
    if (!list_Empty(SplitLits)) {
#ifdef CHECK
      if (!top_ClauseIsSplittable(GivenClause, FALSE)) {
	misc_StartErrorReport();
	misc_ErrorReport("\n In  top_FullReductionSelectGivenComputeDerivables: powerful split clause ");
	misc_ErrorReport("is not splittable.");
	misc_FinishErrorReport();
      }
#endif
      *SplitClause = prfs_DoSplitting(Search, GivenClause, SplitLits);
      list_Delete(SplitLits);
    } else {
      /* Optimal splitting wasn't possible, so try the old-style splitting. */
      /* Here a split is done if a positive literal doesn't share variables */
      /* with another POSITIVE literal. */
      *SplitClause = (CLAUSE)NULL; /* only split optimal clauses prfs_PerformSplitting(Search, GivenClause);*/
    }
  }

  prfs_ExtractUsable(Search, GivenClause);
  clause_SelectLiteral(GivenClause, Flags);

  if (flag_GetFlagIntValue(Flags, flag_PGIVEN)) {
    fputs("\n\tGiven clause: ", stdout);
    clause_Print(GivenClause);
    fflush(stdout);
  }
  
  if (*SplitClause != NULL) {
    /*printf("\n Split: "); clause_Print(*SplitClause); printf(" from\n");
      clause_Print(GivenClause);printf("\n");*/
    Derivables = list_List(*SplitClause);
  }
  else {
    /* No splitting was applied */
    if (flag_GetFlagIntValue(Flags, flag_RTER) != flag_RTEROFF) {
      clock_StartCounter(clock_REDUCTION);
      TerminatorClause = red_Terminator(GivenClause,
					flag_GetFlagIntValue(Flags, flag_RTER),
					prfs_WorkedOffSharingIndex(Search),
					prfs_UsableSharingIndex(Search), Flags,
					Precedence);
      clock_StopAddPassedTime(clock_REDUCTION);
      
      if (TerminatorClause != NULL) {
	/* An empty clause was derived by the "terminator" rule */
	Derivables = list_List(TerminatorClause);
	prfs_InsertUsableClause(Search, GivenClause);
      }
    }
    if (list_Empty(Derivables)) {
      /* No splitting was applied, no empty clause was found by terminator */
      /*clause_SetSpecialFlags(GivenClause,ana_SortDecreasing());*/
      prfs_InsertWorkedOffClause(Search, GivenClause);
      clock_StartCounter(clock_INFERENCE);
      Derivables = inf_DerivableClauses(Search, GivenClause);
      clock_StopAddPassedTime(clock_INFERENCE);
    }
  }

  prfs_IncDerivedClauses(Search, list_Length(Derivables));

  return Derivables;
}


static LIST top_LazyReductionSelectGivenComputeDerivables(PROOFSEARCH Search,
							  CLAUSE *SplitClause,
							  int *Counter, int *MinInstances)
/**************************************************************
  INPUT:   A proof search object, a pointer to a clause resulting from a
           previous splitting step, and a pointer to an integer counter
           and a pointer to the minimum number of instances for successful split clause selection.
  RETURNS: A list of derived clauses.
  EFFECT:  In this function a clause is selected from the set of
           "usable" clauses. After a clause was selected as "given clause",
	   inferences between the given clause and the "worked off" clauses
	   are made. Take a look at the description of the function
	   top_FullReduction... for more details.
	   This function is more complicated than the other function,
	   since clauses in the set of usable clauses may be reducible.
	   Because of this fact, the selection of the given clause
	   has to be done in a loop. After picking a "candidate" clause
	   the clause is inter-reduced with the "worked off" set.
	   If the candidate still exists after the reduction it is
	   selected as given clause, else another usable clause is picked
	   as candidate.
***************************************************************/
{
  CLAUSE     GivenClause, TerminatorClause;
  LIST       Derivables;
  FLAGSTORE  Flags;
  PRECEDENCE Precedence;
  LIST       SplitLits;

  GivenClause      = (CLAUSE)NULL;
  TerminatorClause = (CLAUSE)NULL;
  Derivables       = list_Nil();
  Flags            = prfs_Store(Search);
  Precedence       = prfs_Precedence(Search);
  SplitLits        = list_Nil();

  while (GivenClause == (CLAUSE)NULL &&
	 !list_Empty(prfs_UsableClauses(Search))) {
    /* The selected clause may be redundant */

    if (*SplitClause != NULL &&
	list_PointerMember(prfs_UsableClauses(Search), *SplitClause))
      GivenClause = *SplitClause;

    *SplitClause  = NULL;

    /* Try selecting a clause that is optimal for splitting */
    if (GivenClause == NULL) {
      if (prfs_SplitsAvailable(Search,Flags)) {
	GivenClause = top_GetPowerfulSplitClause(Search, TRUE, &SplitLits, *MinInstances);
	list_Delete(SplitLits);  /* GivenClause has to be reduced first */
      }

      if (GivenClause == NULL) {
	/* No optimal clause for splitting found */
	*MinInstances = flag_GetFlagIntValue(Flags, flag_SPLITMININST);
	if ((*Counter) % flag_GetFlagIntValue(Flags, flag_WDRATIO) == 0)
	  GivenClause = top_SelectClauseDepth(prfs_UsableClauses(Search), Flags);
	else {
	  if (flag_GetFlagIntValue(Flags, flag_PREFCON) != flag_PREFCONUNCHANGED)
	    GivenClause = top_SelectMinimalConWeightClause(prfs_UsableClauses(Search),
							   Flags);
	  else
	    GivenClause = top_SelectMinimalWeightClause(prfs_UsableClauses(Search),
							Flags);
	}
	(*Counter)++;
      }
      else
	(*MinInstances)++;
    }
    prfs_ExtractUsable(Search, GivenClause);

    /* Reduce the selected clause */
    clock_StartCounter(clock_REDUCTION);
    GivenClause = red_CompleteReductionOnDerivedClause(Search, GivenClause, 
						       red_WORKEDOFF);
    clock_StopAddPassedTime(clock_REDUCTION);
  }

  if (GivenClause == (CLAUSE)NULL)
    /* Set of usable clauses is empty */
    return list_Nil();

  
  if (clause_IsEmptyClause(GivenClause)) {
    Derivables = list_List(GivenClause);
    return Derivables;
  }
  else {
    /* Reduce Workedoff clauses with selected clause */
    clock_StartCounter(clock_REDUCTION);
    Derivables = red_BackReduction(Search, GivenClause, red_WORKEDOFF);
    clock_StopAddPassedTime(clock_REDUCTION);
  }

  clause_SelectLiteral(GivenClause, Flags);
  /* Print selected clause */
  if (flag_GetFlagIntValue(Flags, flag_PGIVEN)) {
    fputs("\n\tGiven clause: ", stdout);
    clause_Print(GivenClause); 
    fflush(stdout);
  }     

  /* Try splitting */
  if (prfs_SplitsAvailable(Search,Flags)) {
    /* First try "optimal" splitting on the selected clause */
    SplitLits = top_GetSplitLiterals(Search, GivenClause, FALSE);

    if (!list_Empty(SplitLits)) {
      *SplitClause = prfs_DoSplitting(Search, GivenClause, SplitLits);
      list_Delete(SplitLits);      
    } else {
      /* Try the old splitting that allows negative literals   */
      /* sharing variables with the selected positive literal. */
      *SplitClause = prfs_PerformSplitting(Search, GivenClause);
    }
  } 
  
  if (*SplitClause != NULL) {
    Derivables = list_Cons(*SplitClause, Derivables);
  } else {
    /* Try terminator reduction only for a clause that wasn't splitted. */
    if (flag_GetFlagIntValue(Flags, flag_RTER) != flag_RTEROFF) {
      TerminatorClause = red_Terminator(GivenClause,
					flag_GetFlagIntValue(Flags, flag_RTER),
					prfs_WorkedOffSharingIndex(Search),
					prfs_UsableSharingIndex(Search),
					Flags, Precedence);
      if (TerminatorClause != NULL) {
	Derivables = list_Cons(TerminatorClause, Derivables);
	prfs_InsertUsableClause(Search, GivenClause);
      }
    }
    if (TerminatorClause == (CLAUSE)NULL) {   
      /* clause_SetSpecialFlags(GivenClause,ana_SortDecreasing());*/
      prfs_InsertWorkedOffClause(Search, GivenClause);
      clock_StartCounter(clock_INFERENCE);
      Derivables = list_Nconc(Derivables,
			      inf_DerivableClauses(Search, GivenClause));
      clock_StopAddPassedTime(clock_INFERENCE);
    }
  }
  
  prfs_IncDerivedClauses(Search, list_Length(Derivables));

  return Derivables;
}


static PROOFSEARCH top_ProofSearch(PROOFSEARCH Search, LIST ProblemClauses,
				   FLAGSTORE InputFlags, LIST UserPrecedence, 
                                   int *BoundApplied, BOOL NativeClauseInput)
/**************************************************************
  INPUT:   A proof search object, a list of clauses, a flag store
           containing the flags from the command line and from
           the input file, a list containing the precedence as
	   specified by the user, a pointer to an integer, and a 
           boolean indicating if the input clauses need to be 
           treated specially.
  RETURNS: The same proof search object
  EFFECTS: 
***************************************************************/
{
  LIST       Scan,EmptyClauses,Derivables;
  LIST       UsedEmptyClauses;
  CLAUSE     SplitClause,HighestLevelEmptyClause;
  FLAGSTORE  Flags;
  PRECEDENCE Precedence;
  int        Counter, ActBound, BoundMode, BoundLoops, MinInstances;

  HighestLevelEmptyClause = (CLAUSE)NULL;
  UsedEmptyClauses        = list_Nil();
  EmptyClauses            = list_Nil();
  Derivables              = list_Nil();
  Flags                   = prfs_Store(Search);
  Precedence              = prfs_Precedence(Search);
  Counter                 = 1;

  clock_InitCounter(clock_REDUCTION);
  clock_InitCounter(clock_BACKTRACK);
  clock_InitCounter(clock_INFERENCE);

  /* Important ! Recomputes Weight ! */
  ana_AnalyzeProblem(Search, ProblemClauses);
  if (flag_GetFlagIntValue(Flags, flag_AUTO)) {
    prfs_InstallFiniteMonadicPredicates(Search, ProblemClauses, ana_FinMonPredList());
    ana_AutoConfiguration(ProblemClauses, Flags, Precedence);
    /* File and input flags have higher precedence */
    flag_TransferSetFlags(InputFlags, Flags);
  }

  MinInstances = flag_GetFlagIntValue(Flags, flag_SPLITMININST);

  /* Rearrange automatically determined precedence according to user's specification. */
  symbol_RearrangePrecedence(Precedence, UserPrecedence);

  if (NativeClauseInput) {
    CLAUSE Clause;
    for (Scan = ProblemClauses; !list_Empty(Scan); Scan = list_Cdr(Scan)) {
      Clause = (CLAUSE)list_Car(Scan);
      clause_OrientEqualities(Clause, Flags, Precedence);
      clause_Normalize(Clause);
      clause_SetNativeMaxLitFlags(Clause, Flags, Precedence);
      clause_UpdateWeight(Clause, Flags);
      clause_UpdateMaxVar(Clause);
    }
  }
  else
    for (Scan = ProblemClauses; !list_Empty(Scan); Scan = list_Cdr(Scan))
      clause_OrientAndReInit(list_Car(Scan), Flags, Precedence);
  
  /* Must be called after ana_AnalyzeProblem and Reorientation */
  ana_AnalyzeSortStructure(ProblemClauses, Flags, Precedence);

  if (flag_GetFlagIntValue(Flags, flag_AUTO)) {
    ana_ExploitSortAnalysis(Flags);
    /* File and input flags have higher precedence */
    flag_TransferSetFlags(InputFlags, Flags);
  }

  ActBound       = flag_GetFlagIntValue(Flags, flag_BOUNDSTART);
  BoundMode      = flag_GetFlagIntValue(Flags, flag_BOUNDMODE);
  BoundLoops     = flag_GetFlagIntValue(Flags, flag_BOUNDLOOPS);
  *BoundApplied  = -1;

  if (flag_GetFlagIntValue(Flags, flag_PPROBLEM)) {
    puts("");
    puts("--------------------------SPASS-START-----------------------------");
    puts("Input Problem:");
    clause_ListPrint(ProblemClauses);
    ana_Print(Flags, Precedence);
  }

  if (!NativeClauseInput && flag_GetFlagIntValue(Flags, flag_SORTS) != flag_SORTSOFF) {
    BOOL Strong;
    Strong = (flag_GetFlagIntValue(Flags, flag_SORTS) == flag_SORTSMONADICALL);
    for (Scan = ProblemClauses; !list_Empty(Scan); Scan = list_Cdr(Scan))
      clause_SetSortConstraint((CLAUSE)list_Car(Scan),Strong,Flags, Precedence);
  }

  if (flag_GetFlagIntValue(Flags, flag_RINPUT)) {
    clock_StartCounter(clock_REDUCTION);
    EmptyClauses = red_ReduceInput(Search, ProblemClauses);
    clock_StopAddPassedTime(clock_REDUCTION);
    if (list_Empty(EmptyClauses) && flag_GetFlagIntValue(Flags, flag_SATINPUT))
      EmptyClauses = red_SatInput(Search);
  }
  else {
    for (Scan=ProblemClauses; !list_Empty(Scan); Scan=list_Cdr(Scan))
      if (red_ExplicitTautology(list_Car(Scan),Flags,Precedence))
	clause_Delete(list_Car(Scan));
      else
	prfs_InsertUsableClause(Search, list_Car(Scan));
  }
  Derivables = list_Nil();

  if (ana_SortRestrictions() ||
      (ana_UnsolvedSortRestrictions() &&
       flag_GetFlagIntValue(Flags,flag_SORTS) == flag_SORTSMONADICALL)) {
    if (flag_GetFlagIntValue(Flags, flag_RSST))
      prfs_SetStaticSortTheory(Search,sort_ApproxStaticSortTheory(prfs_UsableClauses(Search),Flags,Precedence));
    prfs_SetDynamicSortTheory(Search,sort_TheoryCreate());
  }

  /* Fix literal order in clauses in the usable set.
     Since they are shared, we have to extract them from
     the sharing before fixing them. Afterwards, we have to
     insert them in the sharing again.
  */ 
  for (Scan = prfs_UsableClauses(Search);
       !list_Empty(Scan);
       Scan = list_Cdr(Scan)) {
    CLAUSE clause;
    clause = list_Car(Scan);
    clause_MakeUnshared(clause,prfs_UsableSharingIndex(Search));
    clause_FixLiteralOrder(clause, Flags, Precedence);
    clause_InsertIntoSharing(clause, prfs_UsableSharingIndex(Search),
			     prfs_Store(Search), prfs_Precedence(Search));
  }

  /* Calculate the frequency counts and split potential for the symbols in the usable set. */
  for (Scan = prfs_UsableClauses(Search);
       !list_Empty(Scan);
       Scan = list_Cdr(Scan)) {
    CLAUSE Clause;
    Clause = list_Car(Scan);
    if (flag_GetFlagIntValue(Flags, flag_SPLITS) != flag_OFF)
      clause_SetSplitPotential(Clause, top_ComputeSplitPotential(Search, Clause));
    clause_CountSymbols(Clause);
  }

  /* Sort usable set. */
  prfs_SetUsableClauses(Search, 
			list_Sort(prfs_UsableClauses(Search), 
				  (BOOL (*) (void *, void *)) clause_CompareAbstractLEQ));

  if (flag_GetFlagIntValue(Flags, flag_SOS)) {
    Derivables = list_Copy(prfs_UsableClauses(Search));
    for (Scan=Derivables;!list_Empty(Scan);Scan=list_Cdr(Scan))
      if (!clause_GetFlag(list_Car(Scan), CONCLAUSE))
	prfs_MoveUsableWorkedOff(Search,list_Car(Scan));
    list_Delete(Derivables);
  }

  if (flag_GetFlagIntValue(Flags, flag_PPROBLEM)) {
    puts("\nProcessed Problem:");
    puts("\nWorked Off Clauses:");
    clause_ListPrint(prfs_WorkedOffClauses(Search));
    puts("\nUsable Clauses:");
    clause_ListPrint(prfs_UsableClauses(Search));
  }

  while ((list_Empty(EmptyClauses) || !prfs_SplitStackEmpty(Search)) && 
	 prfs_Loops(Search) != 0 &&
	 ((*BoundApplied != -1) || !list_Empty(prfs_UsableClauses(Search))) &&
	 (flag_GetFlagIntValue(Flags,flag_TIMELIMIT) == flag_TIMELIMITUNLIMITED ||
	  flag_GetFlagIntValue(Flags,flag_TIMELIMIT) > clock_GetSeconds(clock_OVERALL))) {

    Derivables    = list_Nil();
    SplitClause   = (CLAUSE)NULL;
    *BoundApplied = -1;

    while ((list_Empty(EmptyClauses) || !prfs_SplitStackEmpty(Search)) && 
	   prfs_Loops(Search) != 0 &&
	   (!list_Empty(prfs_UsableClauses(Search)) || !list_Empty(EmptyClauses)) &&
	   (flag_GetFlagIntValue(Flags,flag_TIMELIMIT) == flag_TIMELIMITUNLIMITED ||
	    flag_GetFlagIntValue(Flags,flag_TIMELIMIT) > clock_GetSeconds(clock_OVERALL))) {
      
      if (!list_Empty(EmptyClauses)) {
	/* Backtracking */
	clock_StartCounter(clock_BACKTRACK);
	Derivables = split_Backtrack(Search, HighestLevelEmptyClause,
				     &SplitClause);
	clock_StopAddPassedTime(clock_BACKTRACK);
	prfs_IncBacktrackedClauses(Search, list_Length(Derivables));
      
	if (prfs_SplitStackEmpty(Search)) {
	  EmptyClauses = list_PointerDeleteElement(EmptyClauses, HighestLevelEmptyClause); 
 	  list_DeleteWithElement(EmptyClauses, (void (*)(POINTER))clause_Delete);   /* make sure the right clause is used for doc proof */
	  Derivables = list_Cons(HighestLevelEmptyClause, Derivables);
	}
	else {
	  for ( ; !list_Empty(EmptyClauses); EmptyClauses = list_Pop(EmptyClauses))
	    if (list_Car(EmptyClauses) != HighestLevelEmptyClause)
	      clause_Delete(list_Car(EmptyClauses));
	  prfs_InsertDocProofClause(Search, HighestLevelEmptyClause);
	  /* Keep HighestLevelEmptyClause for finding the terms required */
	  /* for the proof.                                              */
	  if (flag_GetFlagIntValue(Flags, flag_DOCPROOF))
	    UsedEmptyClauses = list_Cons(HighestLevelEmptyClause, UsedEmptyClauses);
	  if (flag_GetFlagIntValue(Flags, flag_DOCSPLIT)) {
	    printf("\n\t Split Backtracking level %d:",prfs_ValidLevel(Search));
	    if (flag_GetFlagIntValue(Flags, flag_DOCSPLIT) == 2) {
	      LIST Scon;
	      printf("\n\t Backtracked clauses:");
	      for(Scon=Derivables;!list_Empty(Scon);Scon=list_Cdr(Scon)) {
		fputs("\n\tBclause: ", stdout);
		clause_Print((CLAUSE)list_Car(Scon));
		fflush(stdout);
	      }
	      printf("\n\t End Backtracked clauses:");
	    }
	  }
	}
	EmptyClauses            = list_Nil();
	HighestLevelEmptyClause = (CLAUSE)NULL;
      }
      else {   /* no empty clause found */

#ifdef CHECK
	if (!prfs_Check(Search)) {
	  misc_StartErrorReport();
	  misc_ErrorReport("\n In top_ProofSearch: Illegal state of search in SPASS.\n");
	  misc_FinishErrorReport();
	}
	if (!ana_Equations())
	  red_CheckSplitSubsumptionCondition(Search);
#endif 

	if (flag_GetFlagIntValue(Flags, flag_FULLRED))
	  Derivables = top_FullReductionSelectGivenComputeDerivables(Search, &SplitClause, &Counter, &MinInstances);
	else
	  Derivables = top_LazyReductionSelectGivenComputeDerivables(Search, &SplitClause, &Counter, &MinInstances);
      }

      /* Print the derived clauses, if required */
      if (flag_GetFlagIntValue(Flags, flag_PDER))
	for (Scan=Derivables; !list_Empty(Scan); Scan=list_Cdr(Scan)) {
	  fputs("\nDerived: ", stdout); 
	  clause_Print(list_Car(Scan));
	}

      /* Partition the derived clauses into empty and non-empty clauses */
      Derivables = split_ExtractEmptyClauses(Derivables, &EmptyClauses);

      /* Apply reduction rules */
      clock_StartCounter(clock_REDUCTION);
      if (flag_GetFlagIntValue(Flags, flag_FULLRED)) {
	EmptyClauses = 
	  list_Nconc(EmptyClauses,
		     red_CompleteReductionOnDerivedClauses(Search, Derivables,
							   red_ALL, ActBound,
							   BoundMode,
							   BoundApplied));
      } else {
	EmptyClauses =
	  list_Nconc(EmptyClauses,
		     red_CompleteReductionOnDerivedClauses(Search, Derivables,
							   red_WORKEDOFF,
							   ActBound, BoundMode,
							   BoundApplied));
      }
      clock_StopAddPassedTime(clock_REDUCTION);
      

      if (!list_Empty(EmptyClauses)) {
	HighestLevelEmptyClause = split_SmallestSplitLevelClause(EmptyClauses);
	if (flag_GetFlagIntValue(Flags, flag_PEMPTYCLAUSE)) {
	  fputs("\nEmptyClause: ", stdout);
	  clause_Print(HighestLevelEmptyClause);
	}
      }
      prfs_DecLoops(Search);
    }
  
    if (ActBound != flag_BOUNDSTARTUNLIMITED &&
	BoundMode != flag_BOUNDMODEUNLIMITED) {
      BoundLoops--;
      if (BoundLoops == flag_BOUNDLOOPSMIN)
	ActBound = flag_BOUNDSTARTUNLIMITED;
      else
	ActBound = *BoundApplied;
      if (*BoundApplied != -1) {
	prfs_SwapIndexes(Search);
	if  (flag_GetFlagIntValue(Flags,flag_PBINC))
	  printf("\n\n\t ---- New Clause %s Bound: %2d ----\n",
		 (BoundMode==flag_BOUNDMODERESTRICTEDBYDEPTH) ? "Term Depth" : "Weight",ActBound);
      }
    }
  }
  prfs_SetEmptyClauses(Search, EmptyClauses);
  prfs_SetUsedEmptyClauses(Search, UsedEmptyClauses);

  return Search;
}


static void top_Flotter(int argc, const char* argv[], LIST InputClauses, HASH ClauseToTermLabelList, DFGDESCRIPTION Description)
/**************************************************************
  INPUT:  
  RETURNS: Nothing.
  EFFECT:  
***************************************************************/
{
  FILE *Output;
  char *description;
  const char *creator = "\n\tCNF generated by FLOTTER " vrs_VERSION " *}";
  int  size;
  int  creator_size;

  if (cmdlne_GetOutputFile() == (char*)NULL)
    Output = stdout;
  else
    Output = misc_OpenFile(cmdlne_GetOutputFile(),"w");

  creator_size = (int)strlen(creator);
  size        = (int)strlen(desc_Description(Description)) + creator_size;
  description = (char*)memory_Malloc(sizeof(char)*size);
  strncpy(description,desc_Description(Description),size-creator_size-3);
  strcpy(description+size-creator_size-3, creator);

  
  clause_FPrintCnfDFGProblem(Output, FALSE, desc_Name(Description), desc_Author(Description),
			     desc_StatusString(Description), description, InputClauses,
			     NULL, NULL, NULL, ClauseToTermLabelList, FALSE, TRUE);
  
  fputs("\nFLOTTER needed\t", stdout);
  clock_PrintTime(clock_INPUT);
  puts(" for the input.");
  fputs("\t\t", stdout);
  clock_PrintTime(clock_CNF);
  fputs(" for the  CNF translation.", stdout);
  

  if (Output != stdout)
    misc_CloseFile(Output,cmdlne_GetOutputFile());
  memory_Free(description, sizeof(char)*size);
}

static BOOL top_CalledFlotter(FLAGSTORE Flags, const char* Call)
{ 
  int  length;
  BOOL result;
  length = strlen(Call);
  result = string_Equal((Call + (length > 7 ? length - 7 : 0)), "FLOTTER");
  if (result)
    flag_SetFlagIntValue(Flags, flag_FLOTTER, flag_FLOTTERON);
  return result;
}

static void top_EstablishClAxRelation(LIST ClAxRelation, LIST InputClauses, LIST* Labellist, HASH ClauseToTermLabellist, BOOL DocProof)
/**************************************************************
  INPUT:   A list of relations between clause numbers and formula labels (strings) <ClAxRelation>
           a list of input clauses
           the list of used formula labels
           a hash array relating clauses to formula labels (strings)
           the doc proof flag
  RETURNS: Nothing.
  EFFECT:  If <DocProof> and the <ClAxRelation> is not empty, then the
           list representation of the clause to formula label relation is established
           in the hash array <ClauseToTermLabellist> and the labels are collected in  <Labellist>

           the <ClAxRelation> is eventually deleted
***************************************************************/
{
  LIST   Scan1, Scan2;
  CLAUSE Clause;

  if (!list_Empty(ClAxRelation)) {
    if (DocProof) {
      for (Scan1=ClAxRelation; !list_Empty(Scan1);Scan1=list_Cdr(Scan1)) {
	for (Scan2=InputClauses; 
	     !list_Empty(Scan2) &&
	       clause_Number((CLAUSE)list_Car(Scan2)) != (int)list_Car(list_Car(Scan1));
	     Scan2=list_Cdr(Scan2));
	if (list_Empty(Scan2)) {
	  misc_StartUserErrorReport();
	  misc_UserErrorReport("\n For clause %d a formula relation was defined but the clause was not found in the input file!\n", 
			       (int)list_Car(list_Car(Scan1)));
	  misc_FinishUserErrorReport();
	}
	Clause = (CLAUSE)list_Car(Scan2);
	for (Scan2=list_Cdr(list_Car(Scan1)); !list_Empty(Scan2); Scan2=list_Cdr(Scan2))
	  *Labellist = list_Cons(list_Car(Scan2), *Labellist);
	hsh_PutList(ClauseToTermLabellist, Clause, list_Cdr(list_Car(Scan1)));
	list_Free(list_Car(Scan1));   
      }
      *Labellist = list_PointerDeleteDuplicates(*Labellist);
      for (Scan1=InputClauses; !list_Empty(Scan1);Scan1=list_Cdr(Scan1))
	if (hsh_Get(ClauseToTermLabellist, list_Car(Scan1)) == NULL) {
	  misc_StartUserErrorReport();
	  misc_UserErrorReport("\n The formula relation  for clause %d is missing!\n", 
			       clause_Number((CLAUSE)list_Car(Scan1)));
	  misc_FinishUserErrorReport();
	}
    }
    else {
      for (Scan1=ClAxRelation;!list_Empty(Scan1);Scan1=list_Cdr(Scan1)) {
	for (Scan2=list_Cdr(list_Car(Scan1));!list_Empty(Scan2);Scan2=list_Cdr(Scan2))
	  string_StringFree((char *)list_Car(Scan2));
	list_Delete(list_Car(Scan1));
      }
    }
    list_Delete(ClAxRelation);
  }
}



/**************************************************************/
/* Main Function                                              */
/**************************************************************/

int main(int argc, const char* argv[])
{
  LIST              InputClauses,Scan,Axioms,Conjectures, Sorts, QueryClauses, 
    LabelClauses, QueryPair, ProblemClauses, Labellist, Sortlabellist, 
    Symblist, UserPrecedence, UserSelection, ClAxRelation;
  PROOFSEARCH       Search, FlotterSearch;
  /* <InputFlags> are the flags from the problem file and the command line. */
  FLAGSTORE         InputFlags, Flags;
  /* <InputPrecedence> is the precedence after reading the problem file. */
  PRECEDENCE        InputPrecedence, Precedence;
  FILE*             InputStream; 
  HASH              TermLabelToClauselist, ClauseToTermLabellist;
  top_SEARCHRESULT  Result;
  BOOL              NativeClauseInput;
  DFGDESCRIPTION    Description; /*Problem description store*/

  Search = (PROOFSEARCH)NULL;
  NativeClauseInput = FALSE;
  
#ifdef SPASS_SIGNALS
  top_PROOFSEARCH = &Search;
  signal(SIGINT, top_SigHandler);
  signal(SIGTERM, top_SigHandler);
  signal(SIGSEGV, top_SigHandler);
  signal(SIGBUS, top_SigHandler);
  signal(SIGALRM, top_SigHandler);
#endif

  clock_Init();
  clock_StartCounter(clock_OVERALL);
  memory_Init(memory__UNLIMITED);
#ifdef CHECK
  atexit(memory_FreeAllMem);
#endif
  symbol_Init(TRUE);
  stack_Init();
  hash_Init();
  sort_Init();
  term_Init();

  InputPrecedence = symbol_CreatePrecedence();
  fol_Init(TRUE, InputPrecedence);
  eml_Init(InputPrecedence);
  cont_Init();
  unify_Init();
  flag_Init(flag_SPASS);
  subs_Init();
  clause_Init();
  red_Init();
  ren_Init();
  dp_Init();
  cmdlne_Init();
  ana_Init();
  cc_Init();
  tptp_Init();

  /* Build proof search object to store definitions in */
  Search      = prfs_Create();
  InputFlags  = flag_CreateStore();

  if(!cmdlne_Read(argc, argv)) {
    return EXIT_FAILURE;
  }

  /* Check whether flag_STDIN is set in the command line */
  flag_InitStoreByDefaults(InputFlags);
  if (!cmdlne_SetFlags(InputFlags))
    return EXIT_FAILURE;

  if (cmdlne_GetInputFile() == (char*) NULL &&
      !flag_GetFlagIntValue(InputFlags,flag_STDIN)) {
    /* No input file, no stdin input */
    printf("\n\t          %s %s",argv[0],vrs_VERSION);
    if (top_CalledFlotter(InputFlags, argv[0]) ||
	flag_GetFlagIntValue(InputFlags, flag_FLOTTER))
      puts("\n\t       Usage: FLOTTER [options] [<input-file>] [<output-file>]\n");
    else
      puts("\n\t       Usage: SPASS [options] [<input-file>] \n");
    puts("Possible options:\n");
    cmdlne_PrintSPASSNames();

    return EXIT_FAILURE;
  }
  FlotterSearch = NULL;

  Description = desc_Create();
  Axioms = Conjectures = Sorts = Labellist = Sortlabellist = UserPrecedence = UserSelection = ClAxRelation = list_Nil();
  
  cmdlne_SetFlags(InputFlags);   /* Needed for TPTP flag triggering TPTP parser */
    
  if (flag_GetFlagIntValue(InputFlags, flag_STDIN)) {
    top_InputFile = (char*)NULL;
    top_RemoveInputFile = FALSE;
    InputStream   = stdin;
  } else {
    top_InputFile = cmdlne_GetInputFile();
    top_RemoveInputFile = (flag_GetFlagIntValue(InputFlags, flag_GLOBALRIF) == flag_ON);
    if (flag_GetFlagIntValue(InputFlags,flag_TPTP) == flag_ON)
      InputStream = tptp_OpenFile(top_InputFile,&top_DiscoveredFile);
    else
      InputStream = dfg_OpenFile(top_InputFile,&top_DiscoveredFile);
  }
  
  clock_StartCounter(clock_INPUT);
  if (flag_GetFlagIntValue(InputFlags,flag_TPTP) == flag_ON) { /* TPTP parsing */
    flag_CleanStore(InputFlags);  /* Mark all flags as unset */
    InputClauses = tptp_TPTPParser(InputStream, InputFlags, InputPrecedence, Description, &Axioms,
				   &Conjectures, &Sorts, &UserPrecedence, &UserSelection, 
				   &ClAxRelation, &NativeClauseInput); 
  }
  else { /* DFG parsing */
    flag_CleanStore(InputFlags);  /* Mark all flags as unset */
    /* Now add flags from file to InputFlags and set precedence */
    InputClauses = dfg_DFGParser(InputStream, InputFlags, InputPrecedence, Description, &Axioms,
				 &Conjectures, &Sorts, &UserPrecedence, &UserSelection, 
				 &ClAxRelation, &NativeClauseInput); 
  }

  for(Scan=UserSelection;!list_Empty(Scan);Scan=list_Cdr(Scan))
    symbol_AddProperty((SYMBOL)list_Car(Scan), SELECTED);

  /* Add/overwrite with command line flags */
  cmdlne_SetFlags(InputFlags);
  
  Flags      = prfs_Store(Search);
  Precedence = prfs_Precedence(Search);
  /* The Flags were initialized with the default flag values.  */
  /* This values are now overwritten by command line flags and flags */
  /* from the input file. */
  flag_TransferSetFlags(InputFlags, Flags);
  /* From now on the input flags are not changed! */

#ifdef SPASS_SIGNALS
  if (flag_GetFlagIntValue(Flags,flag_TIMELIMIT) != flag_TIMELIMITUNLIMITED) 
    alarm(flag_GetFlagIntValue(Flags,flag_TIMELIMIT)+1);  /* No race with main loop polling */
#endif
  
  /* Transfer input precedence to search object */
  symbol_TransferPrecedence(InputPrecedence, Precedence);


  /* Complain about missing input clauses/formulae when in */
  /* non-interactive mode */
  if (!flag_GetFlagIntValue(Flags, flag_INTERACTIVE) && list_Empty(Axioms) &&
      list_Empty(Conjectures) && list_Empty(InputClauses)) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n No formulae and clauses found in input file!\n");
    misc_FinishUserErrorReport();
  }

  cnf_Init(Flags);  /* Depends on Strong Skolemization Flag */

  /* DocProof is required for interactive mode */
  if (flag_GetFlagIntValue(Flags, flag_INTERACTIVE)) {
    flag_SetFlagIntValue(Flags, flag_DOCPROOF, flag_DOCPROOFON);
  }

  if (flag_GetFlagIntValue(Flags, flag_DOCPROOF))
    prfs_AddDocProofSharingIndex(Search);

  /* Create necessary hasharrays */
  if (flag_GetFlagIntValue(Flags, flag_DOCPROOF)  || 
      top_CalledFlotter(Flags, argv[0]) || flag_GetFlagIntValue(Flags, flag_FLOTTER)) {
    TermLabelToClauselist = hsh_Create();
    ClauseToTermLabellist = hsh_Create();
  }
  else {
    TermLabelToClauselist = NULL;
    ClauseToTermLabellist = NULL;
  }

  /* Establish clause to term (formula) labels in case of doc proof */
  top_EstablishClAxRelation(ClAxRelation,InputClauses, &Labellist, ClauseToTermLabellist,flag_GetFlagIntValue(Flags, flag_DOCPROOF));
  
  /* Build conjecture formula and negate it: Conjectures are taken disjunctively!!*/
  for (Scan = Conjectures; !list_Empty(Scan); Scan = list_Cdr(Scan))
    list_Rplacd(list_Car(Scan), (LIST)term_Create(fol_Not(), 
						  list_List(list_PairSecond(list_Car(Scan)))));

  clock_StopPassedTime(clock_INPUT);

  if (top_DiscoveredFile) {
    misc_CloseFile(InputStream,top_DiscoveredFile);
    if (flag_GetFlagIntValue(Flags, flag_GLOBALRIF) == flag_ON)
      remove(top_InputFile);
  }

  clock_StartCounter(clock_CNF);

  if (list_Empty(InputClauses)) {
    NAT Termcount;

    Termcount  = 0;
    
    /* Create labels for formulae without them */
    for (Scan = Axioms; !list_Empty(Scan); Scan = list_Cdr(Scan), Termcount++) {
      if (list_PairFirst(list_Car(Scan)) == NULL) {
	char L[100];
	char* Label;
	sprintf(L, "axiom%d", Termcount);
	Label = string_StringCopy(L);
	list_Rplaca(list_Car(Scan), Label);
	if (flag_GetFlagIntValue(Flags, flag_DOCPROOF) &&
	    flag_GetFlagIntValue(Flags, flag_PLABELS)) {
	  printf("\nAdded label %s for axiom \n", Label);
	  fol_PrettyPrintDFG((TERM) list_PairSecond(list_Car(Scan)));
	}
      }
    }
    Termcount  = 0;
    for (Scan = Sorts; !list_Empty(Scan); Scan = list_Cdr(Scan), Termcount++) {
      char L[100];
      char* Label;
      sprintf(L, "declaration%d", Termcount);
      Label = string_StringCopy(L);
      list_Rplaca(list_Car(Scan), Label);
      if (flag_GetFlagIntValue(Flags, flag_DOCPROOF) &&
	  flag_GetFlagIntValue(Flags, flag_PLABELS)) {
	printf("\nAdded label %s for declaration \n", Label);
	fol_PrettyPrintDFG((TERM) list_PairSecond(list_Car(Scan)));
      }
      Sortlabellist = list_Cons(Label, Sortlabellist);
    }
    Axioms = list_Nconc(Axioms, Sorts);

    if (flag_GetFlagIntValue(Flags, flag_EML)) {
      clock_StartCounter(clock_TRANSL);

      /* Reduce EML special formulae to first-order logic */
      if (flag_GetFlagIntValue(Flags, flag_EML) ) {
        eml_TranslateToFolMain(&Axioms, &Conjectures, 
			       flag_GetFlagIntValue(Flags, flag_INTERACTIVE), Flags, Precedence);
      }

      clock_StopPassedTime(clock_TRANSL);
    }

    if (flag_GetFlagIntValue(Flags, flag_APPLYDEFS) != flag_APPLYDEFSOFF) {
      def_ExtractDefsFromTermlist(Search, Axioms, Conjectures); 
      Conjectures = def_ApplyDefinitionToTermList(prfs_Definitions(Search),
						  Conjectures, Flags,
						  Precedence);
    }

    /* We must keep the list of symbols because they can't be freed in cnf_Flotter */
    Symblist = list_Nil();

    /* Axioms is list of pairs, conjectures is list of terms */
    /* A ProofSearch object is only returned and the symbols kept in Symblist
       if flag_INTERACTIVE is set */
    FlotterSearch = cnf_Flotter(Axioms,Conjectures,&InputClauses, &Labellist,
				TermLabelToClauselist, ClauseToTermLabellist,
				Flags, Precedence, &Symblist);

    InputClauses = clause_ListSortWeighed(InputClauses);
    clause_SetCounter(1);
    for (Scan = InputClauses; !list_Empty(Scan); Scan = list_Cdr(Scan)) {
      clause_NewNumber(list_Car(Scan));
    }
  }
  else {
    dfg_DeleteFormulaPairList(Axioms);
    dfg_DeleteFormulaPairList(Sorts);
    dfg_DeleteFormulaPairList(Conjectures);
    if (flag_GetFlagIntValue(Flags, flag_APPLYDEFS) != flag_APPLYDEFSOFF) {
      /* Extract list of definitions */
      def_ExtractDefsFromClauselist(Search, InputClauses);
      def_FlattenDefinitionsDestructive(Search);
      for (Scan=prfs_Definitions(Search); !list_Empty(Scan); Scan=list_Cdr(Scan))
	InputClauses = def_ApplyDefToClauselist(Search, (DEF) list_Car(Scan),
						InputClauses, TRUE);
    }
  }

  clock_StopPassedTime(clock_CNF);

  if (top_CalledFlotter(Flags, argv[0]) || flag_GetFlagIntValue(Flags, flag_FLOTTER)) {
    if ( flag_GetFlagIntValue(Flags, flag_SORTS) != flag_SORTSOFF) {    /* Now native clause output, so lets print sorts already */
      for (Scan = InputClauses; !list_Empty(Scan); Scan = list_Cdr(Scan))
	clause_SetSortConstraint((CLAUSE)list_Car(Scan), FALSE, Flags, Precedence);
    }
    top_Flotter(argc,argv,InputClauses, ClauseToTermLabellist,Description);
    flag_SetFlagIntValue(Flags, flag_TIMELIMIT,   0);       /* Exit No Output */
    flag_SetFlagIntValue(Flags, flag_INTERACTIVE, flag_INTERACTIVEOFF);
    flag_SetFlagIntValue(Flags, flag_PPROBLEM,    flag_PPROBLEMOFF);
  }

  memory_Restrict(flag_GetFlagIntValue(Flags, flag_MEMORY));
  symbol_SeparateVariableSymbolNames();

  do {
    LIST deflist;
    int  BoundApplied;
    ProblemClauses = list_Nil();
    LabelClauses   = list_Nil();
    Result         = top_RESOURCE;

    if (flag_GetFlagIntValue(Flags, flag_INTERACTIVE)) {
      QueryPair = ia_GetNextRequest(InputStream, Flags);
      /* A pair (<formula,labellist>) */
      /* Get list of clauses derivable from formulae with labels in labellist */
      if (list_Empty(QueryPair)) {
	break;
      }
      for (Scan=list_PairSecond(QueryPair);!list_Empty(Scan);Scan=list_Cdr(Scan)) {
	LIST l;
	l = hsh_GetWithCompareFunc(TermLabelToClauselist, list_Car(Scan),
				   (BOOL (*)(POINTER, POINTER)) cnf_LabelEqual,
				   (unsigned long (*)(POINTER)) hsh_StringHashKey);

	l = list_PointerDeleteDuplicates(list_Copy(l));
	LabelClauses = list_Nconc(l, LabelClauses);
      }
      /* Get list of clauses derivable from sorts */
      for (Scan=Sortlabellist; !list_Empty(Scan); Scan=list_Cdr(Scan)) {
	LIST l;
	l = hsh_GetWithCompareFunc(TermLabelToClauselist, list_Car(Scan),
				   (BOOL (*)(POINTER, POINTER)) cnf_LabelEqual,
				   (unsigned long (*)(POINTER)) hsh_StringHashKey);

	l = list_PointerDeleteDuplicates(list_Copy(l));
	LabelClauses = list_Nconc(l, LabelClauses);
      }	

      /* For labelclauses copies are introduced */
      /* So an update to the clause to term mapping is necessary */
      for (Scan=LabelClauses; !list_Empty(Scan); Scan=list_Cdr(Scan)) {
	CLAUSE C;
	LIST l;
	C = (CLAUSE) list_Car(Scan);
	l = list_Copy(hsh_Get(ClauseToTermLabellist, C));
	l = cnf_DeleteDuplicateLabelsFromList(l);
	list_Rplaca(Scan, clause_Copy(C));
	hsh_PutList(ClauseToTermLabellist, list_Car(Scan), l);
      }
      QueryClauses   = cnf_QueryFlotter(FlotterSearch, list_PairFirst(QueryPair),
					&Symblist);
      ProblemClauses = list_Nconc(QueryClauses, LabelClauses);

      for (Scan=list_PairSecond(QueryPair); !list_Empty(Scan); Scan= list_Cdr(Scan))
	string_StringFree(list_Car(Scan)); /* Free the label strings */
      list_Delete(list_PairSecond(QueryPair));
      list_PairFree(QueryPair);
      clock_InitCounter(clock_OVERALL);
      clock_StartCounter(clock_OVERALL);
    }
    else {
      ProblemClauses = InputClauses;
      InputClauses   = list_Nil();
    }

    prfs_SetLoops(Search,flag_GetFlagIntValue(Flags, flag_LOOPS));
    prfs_SetBacktrackedClauses(Search, 0);
    BoundApplied = -1;
    if (flag_GetFlagIntValue(Flags, flag_LOOPS) != 0 && flag_GetFlagIntValue(Flags, flag_TIMELIMIT) != 0)
      Search = top_ProofSearch(Search, ProblemClauses, InputFlags, UserPrecedence, &BoundApplied, NativeClauseInput);
  
    if ((flag_GetFlagIntValue(Flags, flag_TIMELIMIT) == flag_TIMELIMITUNLIMITED ||
	 flag_GetFlagIntValue(Flags, flag_TIMELIMIT) > clock_GetSeconds(clock_OVERALL)) &&
	prfs_Loops(Search) != 0 &&
	(BoundApplied == -1 || !list_Empty(prfs_EmptyClauses(Search)))) {
      if (list_Empty(prfs_EmptyClauses(Search)))
	Result = top_COMPLETION;
      else
	Result = top_PROOF;
    }
   
    if (flag_GetFlagIntValue(Flags, flag_TIMELIMIT) != 0 && top_NoAlarm == 0) {
      /* Stop SPASS immediately */
      top_NoAlarm = 1; /* No race with SIGALRM handler */
      printf("\nSPASS %s ", vrs_VERSION);
      fputs("\nSPASS beiseite: ", stdout);
      switch (Result) {
      case top_RESOURCE:
	if (prfs_Loops(Search) != 0)
	  fputs("Ran out of time.", stdout);
	else
	  fputs("Maximal number of loops exceeded.", stdout);
	break;
      case top_PROOF:
	fputs("Proof found.", stdout);
	break;
      default:        /* Completion */
	fputs("Completion found.", stdout);
      }
      printf("\nProblem: %s ",
	     (top_DiscoveredFile != (char*)NULL ? top_DiscoveredFile : "Read from stdin."));
      if (flag_GetFlagIntValue(Flags, flag_PSTATISTIC)) {
	clock_StopPassedTime(clock_OVERALL);
	printf("\nSPASS derived %d clauses,", prfs_DerivedClauses(Search));
	printf(" backtracked %d clauses,", prfs_BacktrackedClauses(Search));
        printf(" performed %d splits", prfs_SplitCounter(Search));
	printf(" and kept %d clauses.", prfs_KeptClauses(Search));
	printf("\nSPASS allocated %lu KBytes.", memory_DemandedBytes()/1024);
	fputs("\nSPASS spent\t", stdout);
	clock_PrintTime(clock_OVERALL);
	fputs(" on the problem.\n\t\t", stdout);
	clock_PrintTime(clock_INPUT);
	fputs(" for the input.\n\t\t", stdout);
	clock_PrintTime(clock_CNF);
	fputs(" for the FLOTTER CNF translation", stdout);
        if(flag_GetFlagIntValue(Flags, flag_EML)) {
	  fputs(", of which", stdout);
	  fputs("\n\t\t", stdout); clock_PrintTime(clock_TRANSL);
	  fprintf(stdout, " for the translation from %s to FOL", flag_Name(flag_EML));
	}
	printf(".");
	printf("\n\t\t"); clock_PrintTime(clock_INFERENCE);
	fputs(" for inferences.\n\t\t", stdout);
        clock_PrintTime(clock_BACKTRACK);
	fputs(" for the backtracking.\n\t\t", stdout);
	clock_PrintTime(clock_REDUCTION);
	puts(" for the reduction.");
      }
      if (Result != top_PROOF &&
	  flag_GetFlagIntValue(Flags, flag_FPMODEL) != flag_FPMODELOFF) {
	FILE *Output;
	char name[100];
	const char * creator = "{*SPASS " vrs_VERSION " *}";
	BOOL PrintPotProductive;

	strcpy(name, (top_InputFile != (char*)NULL ? top_InputFile : "SPASS"));
	if (Result == top_COMPLETION)
	  strcat(name, ".model");
	else
	  strcat(name, ".clauses");
	Output = misc_OpenFile(name,"w");
	PrintPotProductive = (flag_GetFlagIntValue(Flags, flag_FPMODEL) ==
			      flag_FPMODELPOTENTIALLYPRODUCTIVECLAUSES);
	if (Result == top_COMPLETION)
	  clause_FPrintCnfDFGProblem(Output, PrintPotProductive,
				     "{*Completion_by_SPASS*}",
				     creator, "satisfiable",
				     desc_Description(Description),
				     prfs_WorkedOffClauses(Search),
				     list_Nil(), Flags, Precedence, NULL, TRUE, TRUE);
	else
	  clause_FPrintCnfDFGProblem(Output, PrintPotProductive,
				     "{*Clauses_generated_by_SPASS*}",
				     creator, "unknown",
				     desc_Description(Description),
				     prfs_WorkedOffClauses(Search),
				     prfs_UsableClauses(Search), Flags,
				     Precedence, NULL, FALSE, TRUE);
	misc_CloseFile(Output, name);
	if (Result == top_COMPLETION)
	  printf("\nCompletion printed to: %s\n", name);
	else
	  printf("\nClauses printed to: %s\n", name);
      }
    
      if (flag_GetFlagIntValue(Flags, flag_DOCPROOF) && Result != top_RESOURCE) {
	if (Result == top_COMPLETION) {
	  puts("\n\n The saturated set of worked-off clauses is :");
	  clause_ListPrint(prfs_WorkedOffClauses(Search));
	}
	else {
	  LIST UsedClauses, UsedTerms;	  
	  UsedClauses = dp_PrintProof(Search, prfs_EmptyClauses(Search),
				      top_DiscoveredFile ? top_DiscoveredFile : "SPASS");
	  /* Find terms required for proof */
	  UsedTerms = list_Nil();

	  for (Scan = UsedClauses; !list_Empty(Scan); Scan = list_Cdr(Scan))
	    if (clause_IsFromInput((CLAUSE) list_Car(Scan))) {
	      LIST L;
	      L = hsh_Get(ClauseToTermLabellist, list_Car(Scan));
	      L = list_Copy(L);
	      L = cnf_DeleteDuplicateLabelsFromList(L);
	      UsedTerms = list_Nconc(UsedTerms, L);
	    }
	  list_Delete(UsedClauses);
	  UsedTerms = cnf_DeleteDuplicateLabelsFromList(UsedTerms);
	  fputs("\nFormulae used in the proof :", stdout);
	  for (Scan = UsedTerms; !list_Empty(Scan); Scan = list_Cdr(Scan)) 
	    if (!(strncmp((char*) list_Car(Scan), "_SORT_", 6) == 0))
	      printf(" %s", (char*) list_Car(Scan));
	  putchar('\n');
	  list_Delete(UsedTerms);
	}
      }
    }
    
    /* Delete mapping for the clause copies of the labelclauses */
    for (Scan = LabelClauses; !list_Empty(Scan); Scan=list_Cdr(Scan))
      hsh_DelItem(ClauseToTermLabellist, list_Car(Scan));
    
    list_Delete(ProblemClauses);
 
    fflush(stdout);

    /* Keep definitions */
    deflist = prfs_Definitions(Search);
    prfs_SetDefinitions(Search, list_Nil());
    prfs_Clean(Search);
    prfs_SetDefinitions(Search, deflist);

    symbol_TransferPrecedence(InputPrecedence, Precedence);
    if (flag_GetFlagIntValue(Flags, flag_PPROBLEM))
      fputs("\n--------------------------SPASS-STOP------------------------------", stdout);
  } while (flag_GetFlagIntValue(Flags, flag_INTERACTIVE) &&
	   (flag_GetFlagIntValue(Flags, flag_TIMELIMIT) != 0));

  for (Scan = InputClauses; !list_Empty(Scan); Scan=list_Cdr(Scan))
    clause_OrientAndReInit(list_Car(Scan), Flags, Precedence);

  /* Cleanup Flotter data structures */
  if (flag_GetFlagIntValue(Flags, flag_INTERACTIVE)) {
    if (flag_GetFlagIntValue(Flags, flag_DOCPROOF))
      list_Delete(Symblist);
    else 
      symbol_DeleteSymbolList(Symblist);
    /*  symbol_ResetSkolemIndex(); */
    if (FlotterSearch != NULL) 
      prfs_Delete(FlotterSearch);
  }
  if (flag_GetFlagIntValue(Flags, flag_PFLAGS)) {
    putchar('\n');
    flag_Print(Flags);
  }
  if (TermLabelToClauselist != (HASH)NULL) {
    hsh_Delete(TermLabelToClauselist);
    hsh_Delete(ClauseToTermLabellist);
  }
  for (Scan = Labellist; !list_Empty(Scan); Scan = list_Cdr(Scan))
    string_StringFree(list_Car(Scan));
  list_Delete(Labellist);
  list_Delete(Sortlabellist);
  list_Delete(UserPrecedence);
  list_Delete(UserSelection);

  cnf_Free(Flags);
  eml_Free();

  if (top_DiscoveredFile)
    string_StringFree(top_DiscoveredFile);
  
  prfs_Delete(Search);
  clause_DeleteClauseList(InputClauses);
  flag_DeleteStore(InputFlags);
  symbol_DeletePrecedence(InputPrecedence);
  desc_Delete(Description);
  
  tptp_Free();
  cc_Free();
  ana_Free();
  sort_Free();
  subs_Free();
  unify_Free();
  cont_Free();
  fol_Free();
  symbol_FreeAllSymbols();
  red_Free();
  cmdlne_Free();
#ifdef CHECK
  memory_Print();
  memory_PrintLeaks();
#endif
  putchar('\n');
  return 0;
}
