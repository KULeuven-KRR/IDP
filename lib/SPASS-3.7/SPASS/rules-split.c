/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *               SPLITTING OF CLAUSES                     * */
/* *                                                        * */
/* *  $Module:   SPLIT                                      * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1997, 1998, 2000                  * */
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
/* $Revision: 1.6 $                                         * */
/* $State: Exp $                                            * */
/* $Date: 2010-02-22 14:09:59 $                             * */
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


/* $RCSfile: rules-split.c,v $ */

/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include "rules-split.h"

static LIST  split_DeleteClausesDependingOnLevelFromList(PROOFSEARCH,LIST, int, LIST*);
static LIST  split_DeleteInvalidClausesFromList(PROOFSEARCH, int, LIST);
/* static void  split_RemoveUnnecessarySplits(PROOFSEARCH, SPLITFIELD, unsigned, int, LIST*, unsigned*); */
static void  split_RemoveBackjumpedSplits(PROOFSEARCH, int, LIST*);
static void  split_RemoveTopSplit(PROOFSEARCH, LIST*);
static void  split_RemoveLeftTopSplit(PROOFSEARCH, CLAUSE, CLAUSE*, LIST*);


/**************************************************************/
/* Functions                                                  */
/**************************************************************/

static SPLITFIELD __inline__ prfs_GetTopSplitfield(PROOFSEARCH PS, unsigned *Length)
{
  SPLIT TopSplit;

  if (!prfs_SplitStackEmpty(PS)) {
    TopSplit = prfs_SplitStackTop(PS);
    if (prfs_SplitIsLeft(TopSplit))
      return prfs_LeftSplitfield(TopSplit, Length);
    else
      return prfs_RightSplitfield(TopSplit, Length);
  } else {
    return NULL;
  }
}


static void __inline__ prfs_SetTopSplitfield(PROOFSEARCH PS, SPLITFIELD SF, unsigned SF_length)
{
  SPLIT TopSplit;

  if (!prfs_SplitStackEmpty(PS)) {
    TopSplit = prfs_SplitStackTop(PS);
    if (prfs_SplitIsLeft(TopSplit))
      prfs_SetLeftSplitfield(TopSplit, SF, SF_length);
    else
      prfs_SetRightSplitfield(TopSplit, SF, SF_length);
  }
}

static unsigned __inline__ prfs_ComputeDeepestRightBranchLevel(PROOFSEARCH PS)
{
  LIST ScanStack;

  ScanStack = prfs_SplitStack(PS);
  while (!list_StackEmpty(ScanStack) && prfs_SplitIsLeft(list_Car(ScanStack)))
    ScanStack = list_Cdr(ScanStack);

  if (!list_StackEmpty(ScanStack))
    return prfs_SplitSplitLevel(list_Car(ScanStack));
  else
    return 0;
}

static void __inline__ prfs_PrintStack(PROOFSEARCH PS)
{
  LIST ScanStack;
  SPLIT Split;

  ScanStack = prfs_SplitStack(PS);
  while (!list_StackEmpty(ScanStack)) {
    Split = list_Car(ScanStack);

    if (prfs_SplitIsUnused(Split))
      if (prfs_SplitIsLeft(Split))
        printf(".L%d",prfs_SplitSplitLevel(Split));
      else
        printf(".R%d",prfs_SplitSplitLevel(Split));
    else
      if (prfs_SplitIsLeft(Split))
        printf(".[L%d]",prfs_SplitSplitLevel(Split));
      else
        printf(".[R%d]",prfs_SplitSplitLevel(Split));
    ScanStack = list_Cdr(ScanStack);
  }
  printf("\n");

}

static SPLITFIELD prfs_CopySplitfield(SPLITFIELD Src, unsigned SrcLength)
{
  SPLITFIELD Dst;
  unsigned i;

  if (SrcLength > 0) {
    Dst = memory_Malloc(SrcLength * sizeof(SPLITFIELDENTRY));
    for (i=0; i < SrcLength; i++)
      Dst[i] = Src[i];
    return Dst;
  } else
    return NULL;
}

static void __inline__ prfs_FreeSplitfield(SPLITFIELD *SF, unsigned *Length)
{
  if (*SF != NULL) {
    memory_Free(*SF,
                sizeof(SPLITFIELDENTRY) * (*Length));
    *SF = NULL;
    *Length = 0;
  }
}

static void __inline__ prfs_RemoveUsedSplitsFromTop(PROOFSEARCH PS)
{
  SPLIT Top;

  while (!prfs_SplitStackEmpty(PS) && (!prfs_SplitIsUnused(prfs_SplitStackTop(PS)))) {
    Top = prfs_SplitStackTop(PS);
    prfs_SplitStackPop(PS);
    prfs_SplitDelete(Top);
  }
  if (!prfs_SplitStackEmpty(PS))
    prfs_SetValidLevel(PS,prfs_SplitSplitLevel(prfs_SplitStackTop(PS)));
  else
    prfs_SetValidLevel(PS,0);
}

LIST split_Backtrack(PROOFSEARCH PS, CLAUSE EmptyClause, CLAUSE* SplitClause)
{
  SPLIT      ActBacktrackSplit;
  int        Backtracklevel;
  SPLITFIELD CurrentSplitfield;
  unsigned   CurrentSplitfield_length;
  LIST       RecoverList = list_Nil();
  int        DeepestRightBranchLevel = prfs_ComputeDeepestRightBranchLevel(PS);
  LIST       Scan;
  BOOL       Done;

  Done = FALSE;

  CurrentSplitfield = clause_GetSplitfield(EmptyClause, &CurrentSplitfield_length);
  prfs_SetTopSplitfield(PS, CurrentSplitfield, CurrentSplitfield_length);

  /* loop: reducing to normal form */
  while (!Done) {
    if (!prfs_SplitStackEmpty(PS)) {
      CurrentSplitfield = prfs_GetTopSplitfield(PS, &CurrentSplitfield_length);
      CurrentSplitfield = prfs_CopySplitfield(CurrentSplitfield,CurrentSplitfield_length);
      Backtracklevel = prfs_SplitfieldHighestLevel(CurrentSplitfield,CurrentSplitfield_length);
      if (prfs_ValidLevel(PS) > Backtracklevel) {
        /* apply backjump ----> */
        split_RemoveBackjumpedSplits(PS, Backtracklevel, &RecoverList);
        prfs_RemoveUsedSplitsFromTop(PS);
        prfs_SetTopSplitfield(PS, CurrentSplitfield, CurrentSplitfield_length);
        DeepestRightBranchLevel = prfs_ComputeDeepestRightBranchLevel(PS);
        /* <---- apply backjump */
      } else if (prfs_SplitIsLeft(prfs_SplitStackTop(PS))) {
        /* left */
        /* apply branch-condense ----> */
	/* deleted       unsigned Removed = 0;

        split_RemoveUnnecessarySplits(PS, CurrentSplitfield, CurrentSplitfield_length, DeepestRightBranchLevel, &RecoverList, &Removed);
        if (Removed > 0)
          prfs_SetTopSplitfield(PS, CurrentSplitfield, CurrentSplitfield_length);
         <---- apply branch-condense 
        else */ {
          /* apply enter-right ----> */
          split_RemoveLeftTopSplit(PS, EmptyClause, SplitClause, &RecoverList);
          prfs_SetLastBacktrackLevel(PS, prfs_ValidLevel(PS));
          /* remove invalid clauses from RecoverList */
          RecoverList = split_DeleteInvalidClausesFromList(PS, prfs_ValidLevel(PS), RecoverList);
          /* stack is in normal form: */
          Done = TRUE;
          /* <---- apply enter-right */
        }
      } else {
        /* right */
        /* apply right-collapse ----> */
        SPLITFIELD LeftSplitfield, RightSplitfield, NewSplitfield;
        unsigned   LeftSplitfield_length, RightSplitfield_length, NewSplitfield_length;

        ActBacktrackSplit = prfs_SplitStackTop(PS);
        LeftSplitfield = prfs_LeftSplitfield(ActBacktrackSplit,&LeftSplitfield_length);
        RightSplitfield = prfs_RightSplitfield(ActBacktrackSplit,&RightSplitfield_length);
        NewSplitfield = prfs_CombineSplitfields(LeftSplitfield, LeftSplitfield_length, RightSplitfield, RightSplitfield_length,
          prfs_SplitSplitLevel(ActBacktrackSplit),
          &NewSplitfield_length);
        split_RemoveTopSplit(PS, &RecoverList);
        prfs_RemoveUsedSplitsFromTop(PS);
        prfs_SetTopSplitfield(PS, NewSplitfield, NewSplitfield_length);

        prfs_FreeSplitfield(&NewSplitfield,&NewSplitfield_length);

        DeepestRightBranchLevel = prfs_ComputeDeepestRightBranchLevel(PS);
        /* <---- apply right-collapse */
      }
      prfs_FreeSplitfield(&CurrentSplitfield,&CurrentSplitfield_length);
    } else {
      /* stack empty */
      for (Scan = RecoverList; !list_Empty(Scan); Scan = list_Cdr(Scan))
        prfs_InsertDocProofClause(PS, list_Car(Scan));
      list_Delete(RecoverList);
      RecoverList = list_Nil();
      Done = TRUE;
    }
  }
  prfs_MoveInvalidClausesDocProof(PS);
  return RecoverList;
}


static LIST split_DeleteClausesDependingOnLevelFromList(PROOFSEARCH Search,
							LIST ClauseList,
							int Level, LIST* New)
/**************************************************************
  INPUT:   A proof search object, a list of unshared clauses
           and a split level.
  EFFECT:  Deletes all clauses depending on split level from
           <ClauseList>.
           All split stored deleted clauses from the level of
           the deleted clauses from <ClauseList> are stored in
           <*New>.
  RETURNS: The updated list and the recover clauses in <*New>.
***************************************************************/
{
  LIST   Scan;
  CLAUSE Clause;
  SPLIT  Reinsert;

  for (Scan = ClauseList; !list_Empty(Scan); Scan = list_Cdr(Scan)) {
    Clause = list_Car(Scan);
    if (clause_DependsOnSplitLevel(Clause, Level)) {
      if (prfs_ExistsSplitOfLevel(clause_SplitLevel(Clause), Search)) {
        Reinsert = prfs_GetSplitOfLevel(clause_SplitLevel(Clause), Search);
        if (prfs_SplitDeletedClauses(Reinsert) != list_Nil()) {
          *New = list_Nconc(prfs_SplitDeletedClauses(Reinsert), *New);
          prfs_SplitSetDeletedClauses(Reinsert, list_Nil());
        }
      }
      prfs_InsertDocProofClause(Search,Clause);
      list_Rplaca(Scan, NULL);
    }
  }
  return list_PointerDeleteElement(ClauseList, NULL);
}
 

static LIST split_DeleteClausesDependingOnLevelFromSet(PROOFSEARCH PS,
						       LIST ClauseList,
						       int SplitLevel)
/**************************************************************
  INPUT:   A PROOFSEARCH object, a list of shared clauses
           and a split level.
  RETURNS: A list of clauses that have to be recovered possibly.
  EFFECT:  Clauses from the clause list depending on <SplitLevel>
           are moved to the doc proof index of <PS>.
           All formerly redundant clauses that were reduced by a clause
           of the same split level as a clause from the list depending
           on <SplitLevel> are returned.
***************************************************************/
{
  LIST   scan, delList, recover;
  CLAUSE clause;
  SPLIT  reinsert;
  int    l;

  delList = recover = list_Nil();

  for (scan = ClauseList; !list_Empty(scan); scan = list_Cdr(scan)){
    clause = list_Car(scan);
    if (clause_DependsOnSplitLevel(clause, SplitLevel)) {
      l = clause_SplitLevel(clause);
      if (prfs_ExistsSplitOfLevel(l, PS)) {
        reinsert = prfs_GetSplitOfLevel(l, PS); /* this crashes */
        if (reinsert != NULL) {
          recover = list_Nconc(prfs_SplitDeletedClauses(reinsert), recover);
          prfs_SplitSetDeletedClauses(reinsert, list_Nil());
          delList = list_Cons(clause, delList);
        }
      }
    }
  }

  /* WARNING: The following move operations change the worked off */
  /* and usable sets of the proof search object destructively.    */
  /* So it's impossible to move those function calls into the     */
  /* loop above.                                                  */
  for ( ; !list_Empty(delList); delList = list_Pop(delList)) {
    clause = list_Car(delList);
    if (clause_GetFlag(clause, WORKEDOFF))
      prfs_MoveWorkedOffDocProof(PS, clause);
    else
      prfs_MoveUsableDocProof(PS, clause);
  }
  return recover;
}



static LIST split_DeleteInvalidClausesFromList(PROOFSEARCH Search, int Level,
					       LIST ClauseList)
/**************************************************************
  INPUT:   A proof search object, a split level and a list of clauses.
  RETURNS: The list where invalid clauses wrt 'Level' are deleted.
  EFFECT:  The invalid clauses are stored in the doc proof index
           of the proof search object if necessary.
***************************************************************/
{
  LIST   Scan;
  CLAUSE Clause;

  for (Scan = ClauseList; !list_Empty(Scan); Scan = list_Cdr(Scan)) {
    Clause = list_Car(Scan);
    if ( clause_SplitLevel(Clause) != 0 && !prfs_ExistsSplitOfLevel(clause_SplitLevel(Clause), Search) ) {
      prfs_InsertDocProofClause(Search,Clause);
      list_Rplaca(Scan, NULL);
    }
  }
  return list_PointerDeleteElement(ClauseList, NULL);
}

/*
static void split_RemoveUnnecessarySplits(PROOFSEARCH PS, SPLITFIELD SF, unsigned SF_length, int DeepestRightBranchLevel, LIST *Recover, unsigned *Removed)
**************************************************************
  INPUT:    A proof search object, a splitfield and the last backtrack
            level (deepest right branch)
  RETURNS:  The list of clauses depending on the unncecessary splits
            (splits with levels not in SF) are added to <*Recover>,
            <*Removed> is set to the number of unncecessary splits.
  EFFECT:   All unnecessary splits are marked as used (not deleted!)
            and their blocked and deleted sets are set to Nil.
***************************************************************
{
  LIST Scan;
  LIST New;
  LIST Deleted;
  LIST ScanStack;

  int SplitLevel;
  SPLIT Split,ScanSplit;

  Scan               = prfs_SplitStack(PS);
  SplitLevel         = prfs_ValidLevel(PS); * TopLevel; *

  *Removed = 0;

  while (SplitLevel > DeepestRightBranchLevel) {
    if (prfs_SplitIsUnused(list_Car(Scan)) && !prfs_SplitfieldContainsLevel(SF, SF_length, SplitLevel)) {
      New   = list_Nil();
      Split = list_Car(Scan);

      clause_DeleteClauseList(prfs_SplitBlockedClauses(Split));
      prfs_SplitSetBlockedClauses(Split, list_Nil());

      *Recover = list_Nconc(prfs_SplitDeletedClauses(Split), *Recover);
      prfs_SplitSetDeletedClauses(Split, list_Nil());

      if (prfs_SplitFatherClause(Split) != (CLAUSE)NULL) {
        *Recover = list_Cons(prfs_SplitFatherClause(Split),*Recover);
        prfs_SplitSetFatherClause(Split,NULL);
      }
      *Recover = split_DeleteClausesDependingOnLevelFromList(PS, *Recover, SplitLevel, &New);

      ScanStack = prfs_SplitStack(PS);
      while (!list_StackEmpty(ScanStack)) {
        ScanSplit = (SPLIT)list_Car(ScanStack);
        Deleted = prfs_SplitDeletedClauses(ScanSplit);
        prfs_SplitSetDeletedClauses(ScanSplit, list_Nil()); * IMPORTANT!, see next line *
        Deleted = split_DeleteClausesDependingOnLevelFromList(PS, Deleted, SplitLevel, &New);
        prfs_SplitSetDeletedClauses(ScanSplit, Deleted);
        ScanStack = list_Cdr(ScanStack);
      }

      while (!list_Empty(New)) {
        Deleted = list_Nil();
        *Recover = list_Nconc(split_DeleteClausesDependingOnLevelFromList(PS, New, SplitLevel, &Deleted),
                             *Recover);
        New     = Deleted;
      }
      *Recover = list_Nconc(*Recover,
                           split_DeleteClausesDependingOnLevelFromSet(PS, prfs_UsableClauses(PS), SplitLevel));
      *Recover = list_Nconc(*Recover,
                           split_DeleteClausesDependingOnLevelFromSet(PS, prfs_WorkedOffClauses(PS), SplitLevel));

      prfs_SplitSetUsed(Split);

      *Removed = *Removed + 1;
    }

    SplitLevel--;
    Scan = list_Cdr(Scan);
  }
}
*/

static void  split_RemoveBackjumpedSplits(PROOFSEARCH PS, int BacktrackLevel, LIST *Recover)
/**************************************************************
  INPUT:    A proof search object and the last backtrack level
            (deepest right branch)
  RETURNS:  The list of clauses depending on the backjumped splits
            are added to <*Recover>
  EFFECT:   All splits with levels greater than BacktrackLevel
            are marked as used and their blocked and deleted
            sets are set to Nil; the valid level of PS is
            updated.
***************************************************************/
{
  LIST Scan;
  LIST New;
  LIST Deleted;
  LIST ScanStack;

  int SplitLevel;
  SPLIT Split,ScanSplit;

  Scan               = prfs_SplitStack(PS);
  SplitLevel         = prfs_ValidLevel(PS); /* TopLevel; */

  while (!list_Empty(Scan) && SplitLevel > BacktrackLevel) {
    if (prfs_SplitIsUnused(list_Car(Scan))) {
      New   = list_Nil();
      Split = list_Car(Scan);

      clause_DeleteClauseList(prfs_SplitBlockedClauses(Split));
      prfs_SplitSetBlockedClauses(Split, list_Nil());

      *Recover = list_Nconc(prfs_SplitDeletedClauses(Split), *Recover);
      prfs_SplitSetDeletedClauses(Split, list_Nil());

      if (prfs_SplitFatherClause(Split) != (CLAUSE)NULL) {
        *Recover = list_Cons(prfs_SplitFatherClause(Split),*Recover);
        prfs_SplitSetFatherClause(Split,NULL);
      }
      *Recover = split_DeleteClausesDependingOnLevelFromList(PS, *Recover, SplitLevel, &New);

      ScanStack = prfs_SplitStack(PS);
      while (!list_StackEmpty(ScanStack)) {
        ScanSplit = (SPLIT)list_Car(ScanStack);
        Deleted = prfs_SplitDeletedClauses(ScanSplit);
        prfs_SplitSetDeletedClauses(ScanSplit, list_Nil()); /* IMPORTANT!, see next line */
        Deleted = split_DeleteClausesDependingOnLevelFromList(PS, Deleted, SplitLevel, &New);
        prfs_SplitSetDeletedClauses(ScanSplit, Deleted);
        ScanStack = list_Cdr(ScanStack);
      }

      while (!list_Empty(New)) {
        Deleted = list_Nil();
        *Recover = list_Nconc(split_DeleteClausesDependingOnLevelFromList(PS, New, SplitLevel, &Deleted),
                             *Recover);
        New     = Deleted;
      }
      *Recover = list_Nconc(*Recover,
                           split_DeleteClausesDependingOnLevelFromSet(PS, prfs_UsableClauses(PS), SplitLevel));
      *Recover = list_Nconc(*Recover,
                           split_DeleteClausesDependingOnLevelFromSet(PS, prfs_WorkedOffClauses(PS), SplitLevel));

      prfs_SplitSetUsed(Split);

    }

    SplitLevel--;
    prfs_DecValidLevel(PS);
    Scan = list_Cdr(Scan);
  }

}


static void  split_RemoveTopSplit(PROOFSEARCH PS, LIST *Recover)
/**************************************************************
***************************************************************/
{
  LIST Scan;
  LIST New;
  LIST Deleted;
  LIST ScanStack;

  int SplitLevel;
  SPLIT Split,ScanSplit;

  Scan               = prfs_SplitStack(PS);
  SplitLevel         = prfs_ValidLevel(PS); /* TopLevel; */

  if (prfs_SplitIsUnused(list_Car(Scan))) {
    New   = list_Nil();
    Split = list_Car(Scan);

    clause_DeleteClauseList(prfs_SplitBlockedClauses(Split));
    prfs_SplitSetBlockedClauses(Split, list_Nil());

    *Recover = list_Nconc(prfs_SplitDeletedClauses(Split), *Recover);
    prfs_SplitSetDeletedClauses(Split, list_Nil());

    if (prfs_SplitFatherClause(Split) != (CLAUSE)NULL) {
      *Recover = list_Cons(prfs_SplitFatherClause(Split),*Recover);
      prfs_SplitSetFatherClause(Split,NULL);
    }
    *Recover = split_DeleteClausesDependingOnLevelFromList(PS, *Recover, SplitLevel, &New);

    ScanStack = prfs_SplitStack(PS);
    while (!list_StackEmpty(ScanStack)) {
      ScanSplit = (SPLIT)list_Car(ScanStack);
      Deleted = prfs_SplitDeletedClauses(ScanSplit);
      prfs_SplitSetDeletedClauses(ScanSplit, list_Nil()); /* IMPORTANT!, see next line */
      Deleted = split_DeleteClausesDependingOnLevelFromList(PS, Deleted, SplitLevel, &New);
      prfs_SplitSetDeletedClauses(ScanSplit, Deleted);
      ScanStack = list_Cdr(ScanStack);
    }

    while (!list_Empty(New)) {
      Deleted = list_Nil();
      *Recover = list_Nconc(split_DeleteClausesDependingOnLevelFromList(PS, New, SplitLevel, &Deleted),
                            *Recover);
      New     = Deleted;
    }
    *Recover = list_Nconc(*Recover,
                          split_DeleteClausesDependingOnLevelFromSet(PS, prfs_UsableClauses(PS), SplitLevel));
    *Recover = list_Nconc(*Recover,
                          split_DeleteClausesDependingOnLevelFromSet(PS, prfs_WorkedOffClauses(PS), SplitLevel));

    prfs_SplitSetUsed(Split);

  }

  prfs_DecValidLevel(PS);

}



static void  split_RemoveLeftTopSplit(PROOFSEARCH PS, CLAUSE EmptyClause, CLAUSE *SplitClause, LIST *Recover)
/**************************************************************
***************************************************************/
{
  LIST  Scan;
  LIST  New;
  LIST  Deleted;
  LIST  ScanStack;
  LIST  RightClauses;
  SPLIT ScanSplit;
  SPLIT TopSplit   = prfs_SplitStackTop(PS);
  int   SplitLevel = prfs_ValidLevel(PS);

  New   = list_Nil();

  *Recover = list_Nconc(prfs_SplitDeletedClauses(TopSplit), *Recover);
  prfs_SplitSetDeletedClauses(TopSplit, list_Nil());

  *Recover = split_DeleteClausesDependingOnLevelFromList(PS, *Recover, SplitLevel, &New);

  RightClauses = prfs_SplitBlockedClauses(TopSplit);
  prfs_SplitSetBlockedClauses(TopSplit, list_Nil());
  for (Scan = RightClauses; !list_Empty(Scan); Scan = list_Cdr(Scan)) {
    if (clause_Number(list_Car(Scan)) == 0) {
      /* Found the right clause, the negation clauses have number -1. */
      *SplitClause = list_Car(Scan);
    }
    clause_NewNumber((CLAUSE) list_Car(Scan));
    clause_AddParentClause((CLAUSE) list_Car(Scan), clause_Number(EmptyClause));
    clause_AddParentLiteral((CLAUSE) list_Car(Scan), 0);  /* dummy literal */
  }
  *Recover = list_Nconc(RightClauses, *Recover);

  ScanStack = prfs_SplitStack(PS);
  while (!list_StackEmpty(ScanStack)) {
    ScanSplit = (SPLIT)list_Car(ScanStack);
    Deleted = prfs_SplitDeletedClauses(ScanSplit);
    prfs_SplitSetDeletedClauses(ScanSplit, list_Nil()); /* IMPORTANT!, see next line */
    Deleted = split_DeleteClausesDependingOnLevelFromList(PS, Deleted, SplitLevel, &New);
    prfs_SplitSetDeletedClauses(ScanSplit, Deleted);
    ScanStack = list_Cdr(ScanStack);
  }

  while (!list_Empty(New)) {
    Deleted = list_Nil();
    *Recover = list_Nconc(split_DeleteClausesDependingOnLevelFromList(PS, New, SplitLevel, &Deleted),
                          *Recover);
    New     = Deleted;
  }
  *Recover = list_Nconc(*Recover,
                        split_DeleteClausesDependingOnLevelFromSet(PS, prfs_UsableClauses(PS), SplitLevel));
  *Recover = list_Nconc(*Recover,
                        split_DeleteClausesDependingOnLevelFromSet(PS, prfs_WorkedOffClauses(PS), SplitLevel));

  prfs_SplitSetRight(TopSplit);
}


void split_DeleteClauseAtLevel(PROOFSEARCH PS, CLAUSE Clause, int Level)
/**************************************************************
  INPUT:   A clause, a level and a proofsearch object
  RETURNS: Nothing.
  EFFECT:  <Clause> is deleted from the usable or worked off set
           and made unshared.
***************************************************************/
{
  if (clause_GetFlag(Clause,WORKEDOFF)) 
    prfs_ExtractWorkedOff(PS, Clause);
  else 
    prfs_ExtractUsable(PS, Clause);

  split_KeepClauseAtLevel(PS, Clause, Level);
}


void split_KeepClauseAtLevel(PROOFSEARCH PS, CLAUSE Clause, int Level)
/**************************************************************
  INPUT:   A clause and a level as int.
  RETURNS: None.
  MEMORY:  A copy of clause is made and kept within the split stack.
***************************************************************/
{
  SPLIT  Split;

  if (prfs_ExistsSplitOfLevel(Level, PS)) {
    Split = prfs_GetSplitOfLevel(Level, PS);
    prfs_SplitSetDeletedClauses(Split,list_Cons(Clause, prfs_SplitDeletedClauses(Split)));
  }
  else {
    misc_StartErrorReport();
    misc_ErrorReport("\n Clause %d is redundant but can't be kept at split level %d.\n", clause_Number(Clause), Level);
    misc_FinishErrorReport();
  }
}


LIST split_ExtractEmptyClauses(LIST Clauses, LIST* EmptyClauses)
/**************************************************************
  INPUT:   A list of clauses and a pointer to a list of empty clauses.
  RETURNS: <Clauses> without all empty clauses where the empty clauses
           are moved to <EmptyClauses>
  MEMORY:  Destructive on <Clauses>.
***************************************************************/
{
  LIST   Scan;
  CLAUSE Clause;

  for (Scan=Clauses;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Clause = (CLAUSE)list_Car(Scan);
    if (clause_IsEmptyClause(Clause)) {
      *EmptyClauses = list_Cons(Clause,*EmptyClauses);
      list_Rplaca(Scan,NULL);
    }
  }
  Clauses = list_PointerDeleteElement(Clauses,NULL);

  return Clauses;
}

CLAUSE split_SmallestSplitLevelClause(LIST Clauses)
/**************************************************************
  INPUT:   A non-empty list of clauses.
  RETURNS: The clause with the smallest split level and
           among those the one with the fewest dependencies
***************************************************************/
{
  CLAUSE Result;
  NAT Dependencies, New;

  Result       = (CLAUSE)list_Car(Clauses);
  Dependencies = clause_SplitLevelDependencies(Result);
  Clauses      = list_Cdr(Clauses);
  
  while (!list_Empty(Clauses)) {
    if (clause_SplitLevel(Result) > clause_SplitLevel(list_Car(Clauses)))
      Result  = list_Car(Clauses);
    else
      if (clause_SplitLevel(Result) == clause_SplitLevel(list_Car(Clauses))) {
	New = clause_SplitLevelDependencies(list_Car(Clauses));
	if (New < Dependencies) {	 
	  Dependencies = New;
	  Result       = list_Car(Clauses);
	}
      }
    Clauses = list_Cdr(Clauses);
  }

  return Result;
}
