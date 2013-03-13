/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                  SORTED REASONING                      * */
/* *                                                        * */
/* *  $Module:   SORT                                       * */
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
/* $Revision: 1.4 $                                         * */
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


/* $RCSfile: sort.c,v $ */

#include "sort.h"

/**************************************************************/
/* Global Variables                                           */
/**************************************************************/


/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *			HIGH LEVEL FUNCTIONS		    * */
/* *                                                        * */
/* ********************************************************** */
/**************************************************************/

void sort_Delete(SORT Sort)
/**************************************************************
  INPUT:   A sort.
  RETURNS: Nothing.
  MEMORY:  The memory of the sort is freed.
***************************************************************/
{
  list_Delete(Sort);
}

BOOL sort_Eq(SORT S1, SORT S2)
/**************************************************************
  INPUT:   Two sorts.
  RETURNS: TRUE iff the sorts <S1> and <S2> are the same base
           sort intersection
***************************************************************/
{
  LIST Scan1,Scan2;
  BOOL Found;

#ifdef CHECK
  if (!sort_IsSort(S1) || !sort_IsSort(S2)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In sort_Eq :");
    misc_ErrorReport(" Illegal input.\n");
    misc_FinishErrorReport();
  }
#endif

  if (list_Length(S1) !=  list_Length(S2))
    return FALSE;

  for (Scan1=S1; !list_Empty(Scan1); Scan1=list_Cdr(Scan1)) {
    Found = FALSE;
    for (Scan2=S2; !list_Empty(Scan2) && !Found; Scan2=list_Cdr(Scan2))
      Found = sort_NodeEqual(list_Car(Scan1),list_Car(Scan2));
    if (!Found)
      return FALSE;
  } 

  return TRUE;
}


LIST sort_GetSymbolsFromSort(SORT Sort)
/**************************************************************
  INPUT:   A sort.
  RETURNS: The list of sort symbols..
***************************************************************/
{
  LIST result = list_Nil();

  for ( ; !list_Empty(Sort); Sort = list_Cdr(Sort))
    result = list_Cons((POINTER)sort_NodeSymbol(list_Car(Sort)), result);

  return result;
}

BOOL sort_ContainsSymbol(SORT Sort, SYMBOL Symbol)
/**************************************************************
  INPUT:   A sort and a symbol.
  RETURNS: TRUE, if the sort contains the symbol, FALSE otherwise.
***************************************************************/
{
  for ( ; !list_Empty(Sort); Sort = list_Cdr(Sort))
    if (symbol_Equal(sort_NodeSymbol(list_Car(Sort)), Symbol))
      return TRUE;

  return FALSE;
}

BOOL sort_IsSort(SORT Sort)
/**************************************************************
  INPUT:   A sort.
  RETURNS: TRUE, if the sort contains not more than one node.
***************************************************************/
{
  LIST Scan1,Scan2;

  for (Scan1=Sort ; !list_Empty(Scan1); Scan1 = list_Cdr(Scan1))
    for (Scan2=list_Cdr(Scan1) ; !list_Empty(Scan2); Scan2 = list_Cdr(Scan2))
      if (sort_NodeEqual(list_Car(Scan1),list_Car(Scan2)))
	return FALSE;

  return TRUE;
}

BOOL sort_TheorySortEqual(SORTTHEORY Theory, SORT Sort1, SORT Sort2)
/**************************************************************
  INPUT:   
  RETURNS: 
***************************************************************/
{
  LIST Scan;

  if (list_Length(Sort1) != list_Length(Sort2))
    return FALSE;

  sort_TheoryIncrementMark(Theory);

  for (Scan=Sort1; !list_Empty(Scan); Scan=list_Cdr(Scan))
    sort_PutNodeExtraTrue(Theory,list_Car(Scan));
  for (Scan=Sort2; !list_Empty(Scan); Scan=list_Cdr(Scan))
    if (!sort_NodeExtraValue(Theory,list_Car(Scan)))
      return FALSE;

  return TRUE;
}

static BOOL sort_TheorySortMember(SORTTHEORY Theory, LIST List, SORT Sort)
/**************************************************************
  INPUT:   
  RETURNS: 
***************************************************************/
{
  while (!list_Empty(List)) {
    if (sort_TheorySortEqual(Theory,list_Car(List),Sort))
      return TRUE;
    List = list_Cdr(List);
  }
  return FALSE;
}


void sort_DeleteSortPair(SOJU Pair) 
/**************************************************************
  INPUT:   
  RETURNS: Nothing.
***************************************************************/
{                                         
  sort_DeleteOne(sort_PairSort(Pair));   
  list_Delete(sort_PairClauses(Pair));  
  list_PairFree(Pair);                 
}


static void sort_ClausesPrint(LIST Clauses)
/**************************************************************
  INPUT:   
  RETURNS: Nothing.
***************************************************************/
{
  LIST Scan;
  
  for (Scan=Clauses;!list_Empty(Scan); Scan=list_Cdr(Scan)){
    if (!list_Empty(list_Cdr(Scan))) 
      printf("%d,",clause_Number(list_Car(Scan)));	
    else      
      printf("%d",clause_Number(list_Car(Scan))); 
    
  }
}


static void sort_LinkPrint(SLINK Link)
/**************************************************************
  INPUT:   
  RETURNS: Nothing.
***************************************************************/
{
  LIST Scan;

  fputs("Input: (", stdout);
  for (Scan=sort_LinkInput(Link);!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    symbol_Print(sort_NodeSymbol(list_Car(Scan)));
    if (!list_Empty(list_Cdr(Scan)))
      putchar(',');
  }

  fputs(") Output: ", stdout);
  symbol_Print(sort_NodeSymbol(sort_LinkOutput(Link)));
  
  printf(" Clause: %d Card: %d Fire: %d Var: ", clause_Number(sort_LinkClause(Link)), sort_LinkCard(Link),
     sort_LinkFire(Link));
  symbol_Print(sort_LinkVar(Link));
}
  
    

void sort_PairPrint(SOJU Pair)
/**************************************************************
  INPUT:   
  RETURNS: Nothing.
***************************************************************/
{
  sort_Print(sort_PairSort(Pair));
  fputs(":[", stdout);
  sort_ClausesPrint(sort_PairClauses(Pair));
  putchar(']');
}


static NODE sort_NodeCreate(SYMBOL S)
/**************************************************************
  INPUT:   A sort symbol.
  RETURNS: A new initialized sort node for the symbol <S>.
***************************************************************/
{
  NODE Result;

  Result = (NODE)memory_Malloc(sizeof(NODE_NODE));

  sort_PutNodeLinks(Result, list_Nil());
  sort_PutNodeClauses(Result, list_Nil());
  sort_PutNodeMark(Result, 0);
  sort_PutNodeStart(Result, 0);
  sort_PutNodeExtra(Result, 0);
  sort_PutNodeSymbol(Result, S);

  return Result;
}

BOOL sort_NodeIsTop(SORTTHEORY Theory, NODE Node)
/**************************************************************
  INPUT:   A sort theory and a node.
  RETURNS: TRUE if the Node is declared to be equivalent to the
           top sort.
***************************************************************/
{
  LIST  Scan;
  SLINK Link;

  for (Scan=sort_TheorySuborigcls(Theory);!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Link = (SLINK)list_Third(list_Car(Scan));
    if (list_Empty(sort_LinkInput(Link)) && Node == sort_LinkOutput(Link))
      return TRUE;
  }
  return FALSE; 
}


static SLINK sort_TheoryLinkCreate(SORTTHEORY Theory, CLAUSE Origin,
				   CLAUSE Clause, LITERAL Lit)
/**************************************************************
  INPUT:   A sort theory, a clause its origin and a subsort declaration
           literal in the clause 
  RETURNS: A new link in <Theory> origin <Clause> and subsort declaration <Lit>
***************************************************************/
{
  SLINK  Result;
  SYMBOL Output,Var,Max;
  int    i;
  LIST   Input;


  Result = (SLINK)memory_Malloc(sizeof(SLINK_NODE));

  Output = term_TopSymbol(clause_LiteralSignedAtom(Lit));
  Var    = term_TopSymbol(term_FirstArgument(clause_LiteralSignedAtom(Lit)));
  Input  = list_Nil();
  Max    = clause_MaxVar(Clause);
  term_StartMaxRenaming(Max);
  Max    = symbol_CreateStandardVariable();

  for (i = clause_FirstConstraintLitIndex(Clause);
       i <= clause_LastConstraintLitIndex(Clause); i++)
    if (symbol_Equal(Var,term_TopSymbol(term_FirstArgument(clause_GetLiteralAtom(Clause,i)))))
      Input = list_Cons(sort_TheoryNode(Theory,term_TopSymbol(clause_GetLiteralAtom(Clause,i))),
			Input);
 
  sort_PutLinkInput(Result,Input);
  sort_PutLinkOutput(Result,sort_TheoryNode(Theory,Output));
  sort_PutLinkVar(Result,Max);
  sort_PutLinkCard(Result,list_Length(Input));
  sort_LinkResetFire(Result);
  sort_PutLinkClause(Result,Origin);

  while (!list_Empty(Input)) {
    sort_PutNodeLinks(list_Car(Input),list_Cons(Result,sort_NodeLinks(list_Car(Input))));
    Input = list_Cdr(Input);
  }

  return Result;
}


void sort_Init(void)
/**************************************************************
  INPUT:   None.
  RETURNS: None.
  SUMMARY: Initializes the sort Module.
  EFFECTS: Initializes global variables, i.e. the BASESORT-Array.
  CAUTION: MUST BE CALLED BEFORE ANY OTHER sort-FUNCTION.
***************************************************************/
{
  return;
}


void sort_Print(SORT Sort)
/**************************************************************
  INPUT:   
  RETURNS: Nothing.
***************************************************************/
{
  putchar('(');
  
  while (!list_Empty(Sort)) {
    symbol_Print(sort_NodeSymbol(list_Car(Sort)));
    Sort = list_Cdr(Sort);
    if (!list_Empty(Sort))
      putchar(',');
  }
  putchar(')');
}


void sort_Free(void)
/**************************************************************
  INPUT:   None.
  RETURNS: Nothing.
  SUMMARY: Frees the sort Module.
***************************************************************/
{
  return;
}

SORTTHEORY sort_TheoryCreate(void)
/**************************************************************
  INPUT:   None.
  RETURNS: A new sort theory.
  EFFECT:  A new sort theory is created and initialized.
***************************************************************/
{
  SORTTHEORY Result;
  int        i;
  SIGNATURE  Entry;
  SYMBOL     Symbol;

  Result = (SORTTHEORY)memory_Malloc(sizeof(SORTTHEORY_NODE));

  for (i = 1; i < symbol_ACTINDEX; i++) {
    Result->basesorttable[i] = (NODE)NULL;
    Entry = symbol_Signature(i);
    if (Entry != NULL) {
      Symbol = Entry->info;
      if (symbol_IsPredicate(Symbol) && Entry->arity == 1)
	Result->basesorttable[i] = sort_NodeCreate(Symbol);
    }
  }
  
  Result->index        = st_IndexCreate();
  Result->suborigcls   = list_Nil();
  Result->termorigcls  = list_Nil();
  Result->mark         = 0;

  return Result;
}

void sort_TheoryPrint(SORTTHEORY Theory)
/**************************************************************
  INPUT:   A sort theory
  RETURNS: None.
  EFFECT:  Prints the sort theory to stdout.
***************************************************************/
{
  LIST Scan;

  if (Theory == (SORTTHEORY)NULL) {
    fputs(" Empty Theory", stdout);
    return;
  }

  fputs("\n Subsort Clauses:", stdout);
  for (Scan=sort_TheorySuborigcls(Theory);!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    fputs("\n\t\t Clause:", stdout);
    clause_Print(list_Second(list_Car(Scan)));
    fputs("\n\t\t Link: ", stdout);
    sort_LinkPrint(list_Third(list_Car(Scan)));
  }
  fputs("\n Term Declaration Clauses:", stdout);
  for (Scan=sort_TheoryTermorigcls(Theory);!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    fputs("\n\t\t Clause:", stdout);
    clause_Print(list_Second(list_Car(Scan)));
  }
    
}

void sort_TheoryDelete(SORTTHEORY Theory)
/**************************************************************
  INPUT:   A sort theory
  RETURNS: None.
  EFFECT:  The complete sort theory is deleted.
***************************************************************/
{
  if (Theory != (SORTTHEORY)NULL) {
    int  i;
    LIST Scan,Tuple;
    TERM Term, Atom;

    for (Scan=Theory->suborigcls;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
      Tuple = (LIST)list_Car(Scan);
      sort_LinkDelete(list_Third(Tuple));
      clause_Delete(list_Second(Tuple));
      list_Delete(Tuple);
    }
    list_Delete(Theory->suborigcls); 
    for (Scan=Theory->termorigcls;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
      Tuple = (LIST)list_Car(Scan);
      Term  = (TERM)list_Third(Tuple);
      Atom  = (TERM)list_Car(term_SupertermList(Term));
      st_EntryDelete(Theory->index,Term,Term,cont_LeftContext());
      st_EntryDelete(Theory->index,Atom,Atom,cont_LeftContext());
      list_Delete(term_SupertermList(Atom));
      list_Delete(term_SupertermList(Term));
      term_RplacSupertermList(Term, list_Nil());
      term_RplacSupertermList(Atom, list_Nil());
      clause_Delete(list_Second(Tuple));
      list_Delete(Tuple);
    }
    list_Delete(Theory->termorigcls);
    st_IndexDelete(Theory->index);

    for (i=1;i<symbol_ACTINDEX;i++)
      if (Theory->basesorttable[i] != (NODE)NULL)
	sort_NodeDelete(Theory->basesorttable[i]);

    memory_Free(Theory,sizeof(SORTTHEORY_NODE));
  }
}

void sort_TheoryInsertClause(SORTTHEORY Theory, CLAUSE Origin, CLAUSE Clause, LITERAL L)
/**************************************************************
  INPUT:   A sort theory, two clauses and a declaration of the  second clause.
           <Origin> is the original clause and <Clause> is a possibly approximated
	   copy of <Origin>.
  RETURNS: None.
  EFFECT:  Inserts <Clause> with declaration <L> into the sort theory.
***************************************************************/
{
  TERM   Term, Atom;

  Term = term_FirstArgument(clause_LiteralSignedAtom(L));
  
  if (term_IsVariable(Term)) {
    SLINK Link;
    Link               = sort_TheoryLinkCreate(Theory,Origin,Clause,L);
    Theory->suborigcls = list_Cons(list_Cons(Origin,list_Cons(clause_Copy(Clause),list_List(Link))),
				   Theory->suborigcls);
  }

  /* Since currently Sort Resolution and Empty Sort require the subsort declaration clauses */
  /* also subsort clauses are introduced into the sort theory index                         */
  
  Atom = clause_LiteralSignedAtom(L);
  term_RplacSupertermList(Atom, list_List(L)); 
  term_RplacSupertermList(Term, list_List(Atom));   /* Must be empty before this operation */
  st_EntryCreate(Theory->index,Term,Term,cont_LeftContext());
  st_EntryCreate(Theory->index,Atom,Atom,cont_LeftContext());
  Theory->termorigcls = list_Cons(list_Cons(Origin,list_Cons(Clause,list_List(Term))),
				  Theory->termorigcls);
}

void sort_TheoryDeleteClause(SORTTHEORY Theory, CLAUSE Origin)
/**************************************************************
  INPUT:   A sort theory and a clause possibly inserted several times in the theory.
  RETURNS: None.
  EFFECT:  <Origin> is deleted from the sort theory, but not deleted itself.
***************************************************************/
{
  TERM Term,Atom;
  LIST Scan,Tuple;   

  for (Scan=Theory->suborigcls;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Tuple = list_Car(Scan);
    if ((CLAUSE)list_First(Tuple) == Origin) {
      list_Rplaca(Scan,NULL);
      sort_LinkDelete(list_Third(Tuple));
      clause_Delete(list_Second(Tuple));
      list_Delete(Tuple);
    }
  }        
  Theory->suborigcls = list_PointerDeleteElement(Theory->suborigcls,NULL);
  for (Scan=Theory->termorigcls;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Tuple = list_Car(Scan);
    if ((CLAUSE)list_First(Tuple) == Origin) {
      list_Rplaca(Scan,NULL);
      Term = (TERM)list_Third(Tuple);
      Atom = (TERM)list_Car(term_SupertermList(Term));
      st_EntryDelete(Theory->index,Term,Term,cont_LeftContext());
      st_EntryDelete(Theory->index,Atom,Atom,cont_LeftContext());
      list_Delete(term_SupertermList(Atom));
      list_Delete(term_SupertermList(Term));
      term_RplacSupertermList(Term, list_Nil());
      term_RplacSupertermList(Atom, list_Nil());
      clause_Delete(list_Second(Tuple));
      list_Delete(Tuple);
    }
  }
  Theory->termorigcls = list_PointerDeleteElement(Theory->termorigcls,NULL);
}


BOOL sort_IsBaseSortSymbol(SYMBOL Symbol)
/*********************************************************
  INPUT:   A Symbol.
  RETURNS: The boolean value TRUE, if 'Symbol' is a
           basesortsymbol, FALSE else.
*******************************************************/
{
#ifdef CHECK
  if (!symbol_IsSymbol(Symbol)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In sort_IsBaseSymbol :");
    misc_ErrorReport(" Illegal input. Input not a symbol.\n");
    misc_FinishErrorReport();
  }
#endif

  return(symbol_IsPredicate(Symbol) && symbol_Arity(Symbol) == 1);
}


SORT sort_TheorySortOfSymbol(SORTTHEORY Theory, SYMBOL Symbol)
/*********************************************************
  INPUT:   A sort theory and a base sort symbol.
  RETURNS: The Basesort of 'Symbol'.
*******************************************************/
{
#ifdef CHECK
  if (!sort_IsBaseSortSymbol(Symbol)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In sort_TheorySortOfSymbol :");
    misc_ErrorReport(" Illegal input. Input not a symbol.\n");
    misc_FinishErrorReport();
  }
#endif

  return (list_List(sort_TheoryNode(Theory, Symbol)));
}


static void sort_EvalSubsort(SORTTHEORY Theory, LIST Nodes)
/*********************************************************
  INPUT:   A sort theory and a list of nodes from this theory.
  RETURNS: None.
  EFFECT:  Starting from the nodes in <Nodes> the subsort
           graph is exploited as long as links fire and
	   new nodes become true. 
*******************************************************/
{
  NODE  Node,Head;
  LIST  Scan,Help,Clauses;
  SLINK Link;

  while (!list_Empty(Nodes)) {
    Node  = (NODE)list_Car(Nodes);
    Scan  = Nodes;
    Nodes = list_Cdr(Nodes);
    list_Free(Scan);

    for (Scan = sort_NodeLinks(Node);
	 !list_Empty(Scan);
	 Scan = list_Cdr(Scan)) {
      Link = (SLINK)list_Car(Scan);
      if (sort_LinkDecrementFire(Link)==0) {
	Head = sort_LinkOutput(Link);
	if (!sort_NodeValue(Theory, Head)) {
	  Clauses = list_List(sort_LinkClause(Link));
	  for (Help=sort_LinkInput(Link);!list_Empty(Help);Help=list_Cdr(Help))
	    if (!list_Empty(sort_NodeClauses(list_Car(Help))))
	      Clauses = 
		list_Nconc(list_Copy(sort_NodeClauses(list_Car(Help))),Clauses);

	  sort_DeleteNodeClauses(Head);
	  sort_PutNodeClauses(Head,Clauses);
	  sort_PutNodeTrue(Theory, Head);
	  Nodes = list_Cons(Head,Nodes);
	}
      }
    }
  }
}


static BOOL sort_TestSubsortSpecial(SORTTHEORY Theory, LIST Nodes, LIST Goal)
/*********************************************************
  INPUT:   A sort theory and a list of nodes from this theory and
           a list of goal nodes.
  RETURNS: TRUE if we can walk from at least one node of <Nodes> over
           a previously evaluated sort structure.
*******************************************************/
{
  NODE  Node,Head;
  LIST  Scan;
  SLINK Link;

  while (!list_Empty(Nodes)) {
    Node  = (NODE)list_NCar(&Nodes);
    if (list_PointerMember(Goal,Node)) {
      list_Delete(Nodes);
      return TRUE;
    }
    for (Scan = sort_NodeLinks(Node);!list_Empty(Scan);Scan = list_Cdr(Scan)) {
      Link = (SLINK)list_Car(Scan);
      if (sort_LinkFire(Link) == 0) {
	Head = sort_LinkOutput(Link);
	Nodes = list_Cons(Head,Nodes);
      }
    }
  }
  return FALSE;
}

static void sort_EvalSubsortSpecial(SORTTHEORY Theory, LIST Nodes)
/*********************************************************
  INPUT:   A sort theory and a list of nodes from this theory.
  RETURNS: None.
  EFFECT:  Starting from the nodes in <Nodes> the subsort
           graph is exploited as long as links fire and
	   new nodes become true. Only links without residues
	   are considered.
*******************************************************/
{
  NODE  Node,Head;
  LIST  Scan;
  SLINK Link;

  while (!list_Empty(Nodes)) {
    Node  = (NODE)list_NCar(&Nodes);
    for (Scan = sort_NodeLinks(Node);!list_Empty(Scan);Scan = list_Cdr(Scan)) {
      Link = (SLINK)list_Car(Scan);
      if (sort_LinkDecrementFire(Link) == 0) {
	Head = sort_LinkOutput(Link);
	if (!sort_NodeValue(Theory, Head)) {
	  sort_PutNodeTrue(Theory, Head);
	  Nodes = list_Cons(Head,Nodes);
	}
      }
    }
  }
}




LIST sort_TheoryIsSubsort(SORTTHEORY Theory, SORT Sort1, SORT Sort2, BOOL *SubsortTest)
/*********************************************************
  INPUT:   A sort theory, two sorts, and a boolean variable indicating weather the subsort relation holds.
  RETURNS: A list of clauses if <Sort1> is a subsort of <Sort2> and NULL otherwise.
*******************************************************/
{
  LIST   Scan,Clauses;
  SLINK  Link;
  NODE   Node;
  SORT   Top;

#ifdef CHECK
  if (!sort_IsSort(Sort1) || !sort_IsSort(Sort2)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In sort_TheoryIsSubsortOfNoResidues :");
    misc_ErrorReport(" Illegal sort input.\n");
    misc_FinishErrorReport();
  }
#endif 
  *SubsortTest = TRUE;
   Clauses     = list_Nil();
   Top         = sort_TopSort();

  sort_TheoryIncrementMark(Theory);


  /*fputs("\n Subsort Test: ", stdout); 
    sort_Print(Sort1); 
    putchar(' '); 
    sort_Print(Sort2);*/
    
    

  for (Scan=sort_TheorySuborigcls(Theory);!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Link = (SLINK)list_Third(list_Car(Scan));
    sort_LinkResetFire(Link);
  }

  for (Scan=Sort1;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Node = (NODE)list_Car(Scan);
    sort_DeleteNodeClauses(Node);
    sort_PutNodeTrue(Theory, Node);
  }

  for (Scan=sort_TheorySuborigcls(Theory);!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Link = (SLINK)list_Third(list_Car(Scan));
    
    if (list_Empty(sort_LinkInput(Link)) && clause_NumOfSuccLits(sort_LinkClause(Link))==1) {
      Node = sort_LinkOutput(Link);
      Top = sort_AddBaseNode(Top,Node);
      sort_DeleteNodeClauses(Node);
      sort_PutNodeClauses(Node, list_List(sort_LinkClause(Link)));
      sort_PutNodeTrue(Theory,Node);
    }
  }
  
  sort_EvalSubsort(Theory,sort_Intersect(Top,sort_Copy(Sort1)));
  
  for (Scan=Sort2;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Node = (NODE)list_Car(Scan);
    if (!sort_NodeValue(Theory,Node)) {
      *SubsortTest = FALSE;
      /*puts(" FALSE");*/
      list_Delete(Clauses);
      return list_Nil();
    }
    else 
       Clauses = list_Nconc(list_Copy(sort_NodeClauses(Node)),
			     Clauses);
  
  }
  
  /*printf(" TRUE %lu\n",(unsigned long)Clauses);*/
  return Clauses;
}


BOOL sort_TheoryIsSubsortOf(SORTTHEORY Theory, SORT Sort1, SORT Sort2)
/*********************************************************
  INPUT:   A sort theory and two sorts.
  RETURNS: TRUE if <Sort1> is a subsort of <Sort2> and NULL otherwise.
*******************************************************/
{
  LIST   Scan;
  SLINK  Link;
  NODE   Node;

  sort_TheoryIncrementMark(Theory);

  /*fputs("\n Subsort Test: ", stdout);
    sort_Print(Sort1);
    putchar(' ');
    sort_Print(Sort2);*/

  for (Scan=sort_TheorySuborigcls(Theory);!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Link = (SLINK)list_Third(list_Car(Scan));
    sort_LinkResetFire(Link);
  }

  for (Scan=Sort1;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Node = (NODE)list_Car(Scan);
    sort_PutNodeTrue(Theory, Node);
  }

  sort_EvalSubsortSpecial(Theory,sort_Copy(Sort1));

  for (Scan=Sort2;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Node = (NODE)list_Car(Scan);
    if (!sort_NodeValue(Theory,Node)) 
      return FALSE;
  }

  return TRUE;
    
}

BOOL sort_TheoryIsSubsortOfExtra(SORTTHEORY Theory, SORT Extra, SORT Sort1, SORT Sort2)
/*********************************************************
  INPUT:   A sort theory and three sorts.
  RETURNS: TRUE if <Sort1> is a subsort of <Sort2> and <Extra> is
                   useful for that purpose
*******************************************************/
{
  LIST   Scan;
  SLINK  Link;
  NODE   Node;

  sort_TheoryIncrementMark(Theory);

  /*fputs("\n Subsort Test: ", stdout);
    sort_Print(Sort1);
    putchar(' ');
    sort_Print(Sort2);*/

  for (Scan=sort_TheorySuborigcls(Theory);!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Link = (SLINK)list_Third(list_Car(Scan));
    sort_LinkResetFire(Link);
  }

  for (Scan=Sort1;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Node = (NODE)list_Car(Scan);
    sort_PutNodeTrue(Theory, Node);
  }

  sort_EvalSubsortSpecial(Theory,sort_Copy(Sort1));

  for (Scan=Sort2;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Node = (NODE)list_Car(Scan);
    if (!sort_NodeValue(Theory,Node)) 
      return FALSE;
  }

  return sort_TestSubsortSpecial(Theory,sort_Copy(Extra),Sort2);
    
}

LIST sort_TheoryComputeAllSubsortHits(SORTTHEORY Theory, SORT Sort1, SORT Sort2)
/*********************************************************
  INPUT:   A sort theory and two sorts.
  RETURNS: All possible sorts that are subsets of <Sort1> that are subsorts
           of <Sort2> together with all conditions.
*******************************************************/
{
  LIST   Scan,Result,Search,Scan2,Visited;
  SLINK  Link;
  NODE   Node;
  SOJU   Cand;
  BOOL   Valid,ValidStart;
  NAT    StartMark;

  sort_TheoryIncrementMark(Theory);
  StartMark = sort_TheoryMark(Theory);

  /*fputs("\n Exhaustive Subsort Test: ", stdout);
    sort_Print(Sort1);
    putchar(' ');
    sort_Print(Sort2);*/

  for (Scan=sort_TheorySuborigcls(Theory);!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Link = (SLINK)list_Third(list_Car(Scan));
    sort_LinkResetFire(Link);
  }

  for (Scan=Sort1;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Node = (NODE)list_Car(Scan);
    sort_DeleteNodeClauses(Node);
    sort_PutNodeClauses(Node,list_Nil());
    sort_PutNodeTrue(Theory, Node);
    sort_PutNodeStartTrue(Theory,Node);
  }

  sort_EvalSubsort(Theory,sort_Copy(Sort1));

  for (Scan=Sort2;!list_Empty(Scan);Scan=list_Cdr(Scan)) {/* Subsort condition must hold */
    Node = (NODE)list_Car(Scan);
    if (!sort_NodeValue(Theory,Node))
      return list_Nil();
  }
  
  Result  = list_Nil();
  Search  = list_List(sort_PairCreate(sort_Copy(Sort2),list_Nil()));
  Visited = list_Nil();
  fputs("\n\n Starting Soju Search:", stdout);

  while (!list_Empty(Search)) {
    Cand   = (SOJU)list_Car(Search);
    Scan   = Search;
    Search = list_Cdr(Search);
    list_Free(Scan);
    putchar('\n');
    sort_PairPrint(Cand);

    if (!sort_TheorySortMember(Theory,Visited,sort_PairSort(Cand))) {
      Visited = list_Cons(sort_Copy(sort_PairSort(Cand)),Visited);
      ValidStart = TRUE;
      Valid      = TRUE;
      for (Scan=sort_PairSort(Cand);!list_Empty(Scan) && (ValidStart || Valid);Scan=list_Cdr(Scan)) {
	Node = (NODE)list_Car(Scan);
	if (sort_NodeMark(Node) != StartMark) {
	  Valid      = FALSE;
	  ValidStart = FALSE;
	}
	else
	  if (sort_NodeStart(Node) != StartMark)
	    ValidStart = FALSE;
      }
      if (Valid) {
	if (ValidStart) 
	  Result = list_Cons(sort_PairCopy(Cand),Result);

	for (Scan=sort_PairSort(Cand);!list_Empty(Scan);Scan=list_Cdr(Scan)) {
	  Node = (NODE)list_Car(Scan);
	  for (Scan2=sort_TheorySuborigcls(Theory);!list_Empty(Scan2);Scan2=list_Cdr(Scan2)) {
	    Link = (SLINK)list_Third(list_Car(Scan2));
	    if (sort_LinkOutput(Link) == Node && !list_Empty(sort_LinkInput(Link)))
	      Search = list_Cons(sort_PairCreate(list_PointerDeleteDuplicates(sort_Intersect(sort_DeleteBaseNode(sort_Copy(sort_PairSort(Cand)),Node),
											     sort_Copy(sort_LinkInput(Link)))),
						 list_Cons(list_Copy(sort_PairClauses(Cand)),list_List(sort_LinkClause(Link)))),
				 Search);
	  }
	}
      }
      sort_PairDelete(Cand);
    }
  }
  list_DeleteWithElement(Visited,(void (*)(POINTER)) sort_Delete);
      
  return Result;    
}

static SORT sort_VarSort(SORTTHEORY Theory, SYMBOL Var, CLAUSE Clause, int i)
/*********************************************************
  INPUT:   A variable symbol, a clause and a literal index in the clause.
  RETURNS: The sort of  <Var> with respect to the sort constraint 
           literals (possibly in the antecedent)
           in <Clause> except literal <i> that is not considered.
  MEMORY:  Both Sorts are destroyed.
*******************************************************/
{
  SORT Result;
  int j;
  TERM Atom;
  
  Result = sort_TopSort();
  
  for (j=clause_FirstConstraintLitIndex(Clause);j<=clause_LastAntecedentLitIndex(Clause);j++) 
    if (j != i) {
      Atom = clause_LiteralAtom(clause_GetLiteral(Clause,j));
      if (symbol_IsBaseSort(term_TopSymbol(Atom)) &&
	  term_TopSymbol(term_FirstArgument(Atom)) == Var)
	Result = sort_Intersect(Result,sort_TheorySortOfSymbol(Theory,term_TopSymbol(Atom)));
    }
  
  return Result;
}


static BOOL sort_Match(SORTTHEORY Theory, TERM Term1, TERM Term2, CLAUSE Clause, LIST* Clauses)
/*********************************************************
  INPUT:   A sort theory, two terms, a clause and list of clauses
           as an additional return parameter.
  RETURNS: TRUE iff there exists a well-sorted matcher from <Term1> to <Term2>
           The sorts of variables in <Term1> is determined by the sort constraint in <Clause>
	   The sorts of subterms of <Term2> are assumed to be already computed and stored.
*******************************************************/
{
  int       l;
  SUBST     subst,scan;
  SORT      varsort;
  LIST      NewClauses, Cl;
  SOJU      Pair;
  BOOL      SubSort;
  
  l          = clause_Length(Clause);
  NewClauses = list_Nil();

  cont_StartBinding();
  unify_Match(cont_LeftContext(),Term1,Term2);
  subst = subst_ExtractMatcher();
  cont_BackTrack();
  
  /*putchar('\n'); term_Print(Term1);fputs("->",stdout);
    term_Print(Term2);putchar(':');subst_Print(subst);
    putchar('\n');*/

  for (scan=subst;!subst_Empty(scan);scan=subst_Next(scan)) {
    varsort = sort_VarSort(Theory,subst_Dom(scan),Clause,l);
    Pair    = hash_Get(subst_Cod(scan));
    
        
    SubSort     = TRUE;
    Cl          = sort_TheoryIsSubsort(Theory,sort_PairSort(Pair),varsort, &SubSort);

    if(!SubSort){
      sort_DeleteOne(varsort);
      list_Delete(NewClauses);
      subst_Free(subst);
      return FALSE;
    } else {
      NewClauses = list_Nconc(NewClauses,
			      list_Copy(sort_PairClauses(Pair)));
      NewClauses = list_Nconc(NewClauses,Cl);
          
    }
    sort_DeleteOne(varsort);
  }
    
  subst_Free(subst);
  *Clauses = list_Nconc(NewClauses,*Clauses);
  return TRUE;
}


static SOJU sort_ComputeSortIntern(SORTTHEORY Theory, TERM Term,
					     CLAUSE Clause, int i,
					     FLAGSTORE Flags,
					     PRECEDENCE Precedence)
/*********************************************************
  INPUT:   A sort theory, a term, a clause wrt that variable sorts are
	   computed, where the literal <i> is discarded,
	   a flag store and a precedence.
	   The sorts of 'Term's args have to be known.
  RETURNS: The sort of 'Term' wrt. the sort theory of the
           clause set. Be careful, the Sort-entries of
	   'Term' and its subterms are changed.
*******************************************************/
{
  SORT Sort;
  LIST HelpList, Scan, Clauses;
  TERM QueryTerm;
  
  Sort    = sort_TopSort();
  Clauses = list_Nil();

  if (term_IsVariable(Term))
    Sort = sort_VarSort(Theory,term_TopSymbol(Term),Clause,i);
  else {
    HelpList = st_GetGen(cont_LeftContext(), sort_TheoryIndex(Theory), Term);
    
    for (Scan=HelpList;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
      SYMBOL  Top;
      LITERAL ActLit;
      TERM    Atom;
      CLAUSE  ActClause;
      
      QueryTerm = (TERM)list_First(Scan);
           
      if (!term_IsVariable(QueryTerm)) { /* Currently also subort declarations are in the index ...*/ 
	Atom      = (TERM)list_First(term_SupertermList(QueryTerm));
	Top       = term_TopSymbol(Atom);
	ActLit    = (LITERAL)list_First(term_SupertermList(Atom));
	ActClause = clause_LiteralOwningClause(ActLit);
	

	if (!sort_ContainsSymbol(Sort,Top) &&
	    clause_IsSortTheoryClause(ActClause, Flags, Precedence) &&
	    sort_Match(Theory,QueryTerm,Term, ActClause,&Clauses)){
	  Sort        = sort_Intersect(Sort,sort_TheorySortOfSymbol(Theory,Top));
	  Clauses     = list_Cons(ActClause,Clauses);
	}
      }
    }
    list_Delete(HelpList);
  }
  return (sort_PairCreate(Sort,Clauses));
}
 

SOJU sort_ComputeSort(SORTTHEORY Theory, TERM Term, CLAUSE Clause,
				int i, FLAGSTORE Flags, PRECEDENCE Precedence) 
/*********************************************************
  INPUT:   A sort theory, a Term, a clause to access 
	   variable-sort-information where the literal <i>
	   is discarded, a flag store and a precedence.
  RETURNS: The sort of 'Term' wrt. the sort theory of the
           clause set and the clauses used for this 
	   computation.
	   Be careful, the Sort-entries of
	   'Term' and its subterms are changed, if they
	   already existed.
*******************************************************/
{
  int  SubtermStack;
  SOJU SortPair;

  SortPair     = (SOJU)NULL;
  SubtermStack = stack_Bottom();
  sharing_PushOnStack(Term);

    while (!stack_Empty(SubtermStack)){
    Term = stack_PopResult();
    
    if (!(SortPair = (SOJU)hash_Get(Term))) {
      
      SortPair = sort_ComputeSortIntern(Theory,Term,Clause,i,
					Flags, Precedence);
      
      /*fputs("\n Computed:",stdout);term_Print(Term);
	putchar(':');sort_PairPrint(SortPair);putchar('\n');*/

      hash_Put(Term,SortPair);
    }
  }

  SortPair = sort_PairCopy(SortPair);
  hash_ResetWithValue((void (*)(POINTER)) sort_DeleteSortPair);

  return(SortPair);
}


/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *	      Creating and Analyzing Sort Theories          * */
/* *                                                        * */
/* ********************************************************** */
/**************************************************************/


static BOOL sort_SortTheoryIsTrivial(SORTTHEORY Theory, LIST Clauses)
/*********************************************************
  INPUT:   A sort theory and a list of clauses that generated the theory.
  RETURNS: TRUE iff all considered base sorts are top.
*******************************************************/
{
  LIST      Sorts, Scan;
  CLAUSE    Clause;
  int       i,n;
  BOOL      SubsortTest;
 

  Sorts = list_Nil();

  /* fputs("\n\nAnalyzing Theory:", stdout); */

  while (!list_Empty(Clauses)) {
    Clause  = (CLAUSE)list_Car(Clauses);
    n       = clause_Length(Clause);
    Clauses = list_Cdr(Clauses);

    /* fputs("\n\t", stdout);clause_Print(Clause); */

    for (i=clause_FirstConstraintLitIndex(Clause); i<n; i++)
      Sorts = list_Cons((POINTER)term_TopSymbol(clause_LiteralAtom(
								   clause_GetLiteral(Clause,i))), Sorts);

    Sorts = list_PointerDeleteDuplicates(Sorts);
  }

  Clauses = Sorts;
  while (!list_Empty(Clauses)) {
    list_Rplaca(Clauses,sort_TheorySortOfSymbol(Theory,(SYMBOL)list_Car(Clauses)));
    Clauses = list_Cdr(Clauses);
  }

  n       = list_Length(Sorts);
  i       = 0;
  Clauses = Sorts;

  /* printf("\n\t  There are %d different sorts.",n); */

  while (!list_Empty(Clauses)) {
    if ((Scan = sort_TheoryIsSubsort(Theory,sort_TopSort(),list_Car(Clauses), &SubsortTest))) {
      list_Delete(Scan);
      i++;
    }
    sort_DeleteOne(list_Car(Clauses));
    Clauses = list_Cdr(Clauses);
  }

  list_Delete(Sorts);

  return (i == n);   /* All sorts are trivial */
}


static LIST sort_ApproxPseudoLinear(LIST Constraint, TERM Head, SYMBOL Var)
/**************************************************************
  INPUT:   A list of constraint literals, the head literal term
           and a variable maximal for all variables.
  RETURNS: The new list of constraint literals. 
  EFFECT:  The succedent literal is made pseudo-linear.
           The function is DESTRUCTIVE.
***************************************************************/
{
  TERM   Atom;
  LIST   RenVars,Scan1,Scan2,Lits;
  SYMBOL OldVar, NewVar;

  RenVars = term_RenamePseudoLinear(Head,Var);
  Lits    = list_Nil();

  for (Scan1=RenVars;!list_Empty(Scan1);Scan1=list_Cdr(Scan1)) {
    OldVar = (SYMBOL)list_PairFirst(list_Car(Scan1));
    NewVar = (SYMBOL)list_PairSecond(list_Car(Scan1));
    list_PairFree(list_Car(Scan1));
    for (Scan2=Constraint;!list_Empty(Scan2);Scan2=list_Cdr(Scan2)) {
      Atom = (TERM)list_Car(Constraint);
      if (symbol_Equal(term_TopSymbol(term_FirstArgument(Atom)),OldVar))
	Lits = list_Cons(term_Create(term_TopSymbol(Atom),
		 list_List(term_Create(NewVar,list_Nil()))), Lits);
    }
  }
  list_Delete(RenVars);

  Lits = list_Nconc(Constraint,Lits);

  return Lits;
}


static LIST sort_ApproxHornClauses(CLAUSE Clause, FLAGSTORE Flags,
				   PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A clause to make special horn clauses from, a flag
           store and a precedence.
  RETURNS: The list of clauses according to the rule
           ClauseDeletion.
  MEMORY:  Allocates memory for the clauses and the list.
***************************************************************/
{
  LIST    Result, NewConstraint,NewSuccedent;
  CLAUSE  NewClause;
  LITERAL LitK;
  int     i,length,j,lc,pli;

#ifdef CHECK
  if (!clause_IsClause(Clause, Flags, Precedence)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In sort_ApproxHornClauses :");
    misc_ErrorReport(" Illegal input. Input not a clause.\n");
    misc_FinishErrorReport();
  }
#endif

  Result = list_Nil();

  if (clause_HasOnlyVarsInConstraint(Clause, Flags, Precedence) &&
      clause_HasSortInSuccedent(Clause, Flags, Precedence)) {
    length = clause_Length(Clause);
    
    for (i = clause_FirstSuccedentLitIndex(Clause); i < length; i++) {
      LitK = clause_GetLiteral(Clause, i);
      
      if (symbol_Arity(term_TopSymbol(clause_LiteralSignedAtom(LitK))) == 1) {
	pli           = i;
	NewSuccedent  = list_List(term_Copy(clause_LiteralSignedAtom(LitK)));
	NewConstraint = list_Nil();
	lc            = clause_LastConstraintLitIndex(Clause);

	for (j = clause_FirstLitIndex(); j <= lc; j++)
	  if (clause_LitsHaveCommonVar(LitK, clause_GetLiteral(Clause, j)))
	    NewConstraint = list_Cons(term_Copy(clause_LiteralAtom(
                              clause_GetLiteral(Clause, j))), NewConstraint);

	if (!list_Empty(NewConstraint))  
	    NewConstraint = sort_ApproxPseudoLinear(NewConstraint,
						    list_Car(NewSuccedent),
						    clause_MaxVar(Clause));
	
	NewClause = clause_Create(NewConstraint, list_Nil(), NewSuccedent, 
				  Flags, Precedence);
	clause_SetSplitLevel(NewClause, 0);
	clause_SetFlag(NewClause,WORKEDOFF);
	clause_SetFromClauseDeletion(NewClause);
	clause_SetParentClauses(NewClause, list_List((POINTER)clause_Number(Clause)));
	clause_AddParentLiteral(NewClause, pli);

	list_Delete(NewConstraint); 
	list_Delete(NewSuccedent); 
	Result    = list_Cons(NewClause, Result);
      }
    }
  }
  return(Result);
}

LIST sort_ApproxMaxDeclClauses(CLAUSE Clause, FLAGSTORE Flags,
			       PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A declaration clause to make special horn clauses
           from, a flag store and a precedence.
  RETURNS: The list of Horn clauses that are pseudo-linear declaration
           clauses generated from maximal declarations in <Clause>
  MEMORY:  Allocates memory for the clauses and the list.
***************************************************************/
{
  LIST    Result, NewConstraint,NewSuccedent;
  CLAUSE  NewClause;
  LITERAL LitK;
  int     i,length,j,lc,pli;

#ifdef CHECK
  if (!clause_IsClause(Clause, Flags, Precedence) || 
      !clause_IsDeclarationClause(Clause)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In sort_ApproxMaxDeclClauses :");
    misc_ErrorReport(" Illegal input.\n");
    misc_FinishErrorReport();
  }
#endif

  Result = list_Nil();
  length = clause_Length(Clause);
    
  for (i = clause_FirstSuccedentLitIndex(Clause); i < length; i++) {
    LitK = clause_GetLiteral(Clause, i);
      
    if (clause_LiteralIsMaximal(LitK) &&
	symbol_IsBaseSort(term_TopSymbol(clause_LiteralSignedAtom(LitK)))) {
      pli           = i;
      NewSuccedent  = list_List(term_Copy(clause_LiteralSignedAtom(LitK)));
      NewConstraint = list_Nil();
      lc            = clause_LastConstraintLitIndex(Clause);

      for (j = clause_FirstLitIndex(); j <= lc; j++)
	if (clause_LitsHaveCommonVar(LitK, clause_GetLiteral(Clause, j)))
	  NewConstraint = list_Cons(term_Copy(clause_LiteralAtom(clause_GetLiteral(Clause, j))), 
				    NewConstraint);

      if (!list_Empty(NewConstraint))  
	NewConstraint = sort_ApproxPseudoLinear(NewConstraint,
						list_Car(NewSuccedent),
						clause_MaxVar(Clause));
	
      NewClause = clause_Create(NewConstraint, list_Nil(), NewSuccedent,
				Flags, Precedence);
      clause_SetSplitLevel(NewClause, 0);
      clause_SetFlag(NewClause,WORKEDOFF);
      clause_SetFromClauseDeletion(NewClause);
      clause_SetParentClauses(NewClause, list_List(Clause));  /* The clause itself is added! */
      clause_AddParentLiteral(NewClause, pli);

      list_Delete(NewConstraint); 
      list_Delete(NewSuccedent); 
      Result    = list_Cons(NewClause, Result);
    }
  }
  return(Result);
}


static LIST sort_ApproxClauses(LIST Clauses, FLAGSTORE Flags,
			       PRECEDENCE Precedence)
/**************************************************************
  INPUT:   A list of top level clauses, a flag store and a
           precedence.
  RETURNS: A list of approximated clauses from <Clauses> which must
           be used for sort deletion.
  EFFECT:  None.
***************************************************************/
{
  LIST   Result,ApproxClauses;
  CLAUSE ActClause;

  Result = list_Nil();

  while (!list_Empty(Clauses)) {
    ActClause     = (CLAUSE)list_Car(Clauses);
    ApproxClauses = sort_ApproxHornClauses(ActClause, Flags, Precedence);
    Result        = list_Nconc(ApproxClauses,Result);
    Clauses       = list_Cdr(Clauses);
  }

  return Result;
}

LIST sort_EliminateSubsumedClauses(LIST Clauses)
/*********************************************************
  INPUT:   A list of clauses.
  RETURNS: <Clauses> without subsumed clauses.
*******************************************************/
{
  LIST RedundantClauses,Iter,Scan;
  BOOL Kept;

  RedundantClauses  = list_Nil();
  for (Scan = Clauses; !list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Kept = TRUE;
    for (Iter = Clauses;!list_Empty(Iter) && Kept; Iter = list_Cdr(Iter))
      if (list_Car(Iter) != list_Car(Scan) &&
	  !list_PointerMember(RedundantClauses,list_Car(Iter)) &&
	  subs_Subsumes(list_Car(Iter),list_Car(Scan),subs_NoException(),subs_NoException())) {
	Kept             = FALSE;
	RedundantClauses = list_Cons(list_Car(Scan),RedundantClauses);
      }
  }
  Clauses = list_NPointerDifference(Clauses,RedundantClauses);
  clause_DeleteClauseList(RedundantClauses);
  return Clauses;
}


SORTTHEORY sort_ApproxStaticSortTheory(LIST Clauses, FLAGSTORE Flags,
				       PRECEDENCE Precedence)
/*********************************************************
  INPUT:   A list of clauses, a flag store and a
           precedence.
  RETURNS: NULL if the approximated sort theory is trivial,
           the approximated sort theory otherwise.
*******************************************************/
{
  LIST       Scan,ApproxClauses;
  CLAUSE     Clause;
  SORTTHEORY Result;

  Result        = sort_TheoryCreate();
  ApproxClauses = sort_ApproxClauses(Clauses, Flags, Precedence);
  ApproxClauses = sort_EliminateSubsumedClauses(ApproxClauses);

  /*fputs("\n\n Approx Sort Theory:", stdout);
    for (Scan = ApproxClauses; !list_Empty(Scan);Scan=list_Cdr(Scan)) {
    fputs("\n\t",stdout); clause_Print(list_Car(Scan));
    }*/
  
  for (Scan = ApproxClauses; !list_Empty(Scan);Scan=list_Cdr(Scan)) {
    Clause = (CLAUSE)list_Car(Scan);
    sort_TheoryInsertClause(Result,Clause,Clause,
			    clause_GetLiteral(Clause,clause_FirstSuccedentLitIndex(Clause)));
  }

  if (sort_SortTheoryIsTrivial(Result,ApproxClauses)) {
    sort_TheoryDelete(Result);
    Result = (SORTTHEORY)NULL;
  }
    
  
  if (flag_GetFlagIntValue(Flags, flag_DOCSST)) {
    fputs("\n\n Approx Sort Theory:", stdout);
    if (Result) {
      puts("\n");
      sort_TheoryPrint(Result);
    }
    else
      fputs(" trivial.", stdout);
  }    
  list_Delete(ApproxClauses);

  return Result;
}


SORTTHEORY sort_ApproxDynamicSortTheory(LIST Clauses)
/*********************************************************
  INPUT:   A list of clauses.
  RETURNS: The approximated dynamic sort theory for <Clauses>
           Only maximal declarations are considered.
*******************************************************/
{
  return (SORTTHEORY)NULL;
}

STR sort_AnalyzeSortStructure(LIST Clauses, FLAGSTORE Flags,
			      PRECEDENCE Precedence)
/*********************************************************
  INPUT:   A list of clauses, a flag store and a
           precedence.
  RETURNS: SORTEQMANY if all positive equations are many
           sorted.
           SORTEQDEC  if all positive equations are sort
	   decreasing.
	   SORTEQNONE otherwise.
	   For the check, the static sort theory is
	   considered.
*******************************************************/
{
  SORTTHEORY Theory;
  LIST       Scan, Cl;
  CLAUSE     Clause,Copy;
  int        i,l;
  TERM       Atom,Left,Right;
  SOJU       SojuLeft,SojuRight;
  BOOL       ManySorted, Decreasing, SubsortTest;

  Theory       = sort_TheoryCreate();
  ManySorted   = TRUE;
  Decreasing   = TRUE;

  for (Scan=Clauses;!list_Empty(Scan);Scan=list_Cdr(Scan)) {
    /* Extract static sort theory */
    Clause = (CLAUSE)list_Car(Scan);
    if (clause_IsPotentialSortTheoryClause(Clause, Flags, Precedence)) {
      Copy = clause_Copy(Clause); 
      symbol_AddProperty(term_TopSymbol(clause_GetLiteralTerm(Copy, 
			 clause_FirstSuccedentLitIndex(Copy))),
			 DECLSORT);
      list_Delete(clause_ParentClauses(Copy));
      clause_SetParentClauses(Copy,list_Nil());
      list_Delete(clause_ParentLiterals(Copy));
      clause_SetParentLiterals(Copy,list_Nil());
      clause_SetNumber(Copy,clause_Number(Clause));
      clause_SetSortConstraint(Copy,FALSE, Flags, Precedence);
      sort_TheoryInsertClause(Theory,Clause,Copy,
			      clause_GetLiteral(Copy,clause_FirstSuccedentLitIndex(Copy)));
    }
  }

  /*putchar('\n');
    sort_TheoryPrint(Theory);
    putchar('\n');*/

  for (Scan=Clauses;!list_Empty(Scan) && Decreasing;Scan=list_Cdr(Scan)) {
    Clause = (CLAUSE)list_Car(Scan);
    l      = clause_Length(Clause);
    for (i=clause_FirstSuccedentLitIndex(Clause);i<l && Decreasing;i++) {
      Atom = clause_GetLiteralTerm(Clause,i);
      if (fol_IsEquality(Atom)) {
	Left      = term_FirstArgument(Atom);
	Right     = term_SecondArgument(Atom);
	SojuLeft  = sort_ComputeSort(Theory, Left, Clause, i, 
					       Flags, Precedence);
	SojuRight = sort_ComputeSort(Theory, Right,Clause, i,
					       Flags, Precedence);
	if (sort_IsTopSort(sort_PairSort(SojuRight)) || sort_IsTopSort(sort_PairSort(SojuLeft))) {
	  /*fputs("\nNon decreasing equation ",stdout); term_PrintPrefix(Atom);
	    fputs(" in clause: ",stdout);clause_Print(Clause);putchar('\n');*/
	  ManySorted = FALSE;
	  Decreasing = FALSE;
	}
	else {
	  if (!sort_Eq(sort_PairSort(SojuRight), sort_PairSort(SojuLeft))) {
	    ManySorted = FALSE;
	    Cl= sort_TheoryIsSubsort(Theory, sort_PairSort(SojuRight), 
				     sort_PairSort(SojuLeft), &SubsortTest);
	    if (Cl && !clause_LiteralIsOrientedEquality(clause_GetLiteral(Clause,i))) {
	      list_Delete(Cl);
	      Cl = sort_TheoryIsSubsort(Theory, sort_PairSort(SojuLeft),
					sort_PairSort(SojuRight),&SubsortTest);
	    }
	    if (Cl)
	      list_Delete(Cl);
	    else {
	      /*fputs("\nNon decreasing equation ",stdout); term_PrintPrefix(Atom);
		fputs(" in clause: ",stdout);clause_Print(Clause);putchar('\n');*/
	      Decreasing = FALSE;
	    }
	  }
	}
	sort_PairDelete(SojuLeft);
	sort_PairDelete(SojuRight);
      }
    }
  }    
  sort_TheoryDelete(Theory);
  if (ManySorted)
    return SORTEQMANY;
  if (Decreasing)
    return SORTEQDECR;

  return SORTEQNONE;
}


