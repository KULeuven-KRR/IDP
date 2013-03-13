/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                 FLAGS OF SPASS                         * */
/* *                                                        * */
/* *  $Module:   FLAGS                                      * */ 
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
/* $Revision: 1.17 $                                        * */
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


/* $RCSfile: flags.c,v $ */

#include <stdio.h>
#include <string.h>

#include "flags.h"
#include "misc.h"
#include "strings.h"

/**************************************************************/
/* Global Declarations                                        */
/**************************************************************/

const int flag_CLEAN     = -5;
const int flag_UNDEFINED = -10;
BOOL  flag_INITIALIZED   = FALSE;


/**************************************************************/
/* File Local Declarations                                    */
/**************************************************************/

/* Define flag properties */
typedef struct {
  int            minimum;
  int            maximum;
  FLAG_TYPE      type;
  FLAG_VALUETYPE vtype;
  const          char *name;
} FLAG_PROPERTY;


static FLAGARRAY     flag_DEFAULTSTORE;
static FLAG_PROPERTY flag_PROPERTIES[flag_MAXFLAG];


/**************************************************************/
/* Functions                                                  */
/**************************************************************/

/**************************************************************
  INPUT:   A flag id, a flag type, a flag name, a default value
           for the flag, and the minimum and maximum values.
  RETURNS: Nothing.
  EFFECT:  Initializes the given flag.
***************************************************************/
static void flag_InitIntern (FLAG_ID Flag, FLAG_TYPE Type, FLAG_VALUETYPE VType,
			     const char *Name, int IntValue, char * StringValue,
			     int Minimum, int Maximum)
{
  FLAG_PROPERTY *property;

  flag_CheckFlagIdInRange(Flag);

  property = &(flag_PROPERTIES[Flag]);

  /* Set the flag type */
  flag_CheckFlagTypeInRange(Type);
  property->type  = Type;
  property->vtype = VType;

  /* Set flag name */
  property->name = Name;

  /* Set flag minimum and maximum */
  property->minimum = Minimum;
  property->maximum = Maximum;

  /* Set flag value */
#ifdef CHECK
  if (IntValue > Minimum && IntValue < Maximum) {
#endif

    flag_DEFAULTSTORE[Flag].intvalue    = IntValue;
    flag_DEFAULTSTORE[Flag].stringvalue = StringValue;

#ifdef CHECK
  }
  else {
    misc_StartErrorReport();
    misc_ErrorReport("\n In flag_InitIntern: Default value out of range.");
    misc_ErrorReport("\n Flag: %s. IntValue: %d.", Name, IntValue);
    misc_FinishErrorReport();
  }
#endif
}

void flag_Init(FLAG_USERS User)
/**************************************************************
  INPUT:   The user program.
  RETURNS: Nothing.
  EFFECT:  Sets all default values for known flags of User.
           Initializes the default store at first call.
***************************************************************/
{
  if (!flag_INITIALIZED) {
    int i;
    for (i = (FLAG_ID) 0; i < flag_MAXFLAG; i++)
      (flag_DefaultStore())[i].intvalue = flag_UNDEFINED;  
    flag_INITIALIZED = TRUE;
  }


  switch (User) {
    
  case flag_SPASS:
    /* Autonomous mode */
    flag_InitIntern(flag_AUTO, flag_UNIQUE, flag_INTEGER, "Auto", flag_AUTOON, (char *)NULL,
		    flag_AUTOMIN, flag_AUTOMAX);

    /* Set of Support Mode */
    flag_InitIntern(flag_SOS, flag_UNIQUE, flag_INTEGER, "SOS", flag_SOSOFF, (char *)NULL,
		    flag_SOSMIN, flag_SOSMAX);

    /* If set input is considered from stdin and printed to stdout */
    flag_InitIntern(flag_STDIN, flag_UNIQUE, flag_INTEGER, "Stdin", flag_STDINOFF, (char *)NULL,
		    flag_STDINMIN, flag_STDINMAX);

    /* If set interactive queries are possible */
    flag_InitIntern(flag_INTERACTIVE, flag_UNIQUE, flag_INTEGER, "Interactive", flag_INTERACTIVEOFF, (char *)NULL,
		    flag_INTERACTIVEMIN, flag_INTERACTIVEMAX);

    /* If set only Flotter CNF-translation is performed */
    flag_InitIntern(flag_FLOTTER, flag_UNIQUE, flag_INTEGER, "Flotter", flag_FLOTTEROFF, (char *)NULL,
		    flag_FLOTTERMIN, flag_FLOTTERMAX);

    /* Allowed number of loops, -1 means no restriction */
    flag_InitIntern(flag_LOOPS, flag_UNIQUE, flag_INTEGER, "Loops", flag_LOOPSUNLIMITED, (char *)NULL,
		    flag_LOOPSMIN, flag_LOOPSMAX);

    /* Allowed number of splits, -1 means no restriction */
    flag_InitIntern(flag_SPLITS, flag_UNIQUE, flag_INTEGER, "Splits", flag_SPLITSOFF, (char *)NULL,
		    flag_SPLITSMIN, flag_SPLITSMAX);

    /* Selected splits heuristic, 0 means standard heuristic, 1 means split also ground congecture clauses */
/*     flag_InitIntern(flag_SPLITHEURISTIC, flag_UNIQUE, flag_INTEGER, "SplitHeuristic", flag_SPLITHEURISTICSTANDARD, (char *)NULL, */
/* 		    flag_SPLITHEURISTICMIN, flag_SPLITHEURISTICMAX); */

    /* Number of found instances for a split to be useful, default 2 (at least one extra instance) */
    flag_InitIntern(flag_SPLITMININST, flag_UNIQUE, flag_INTEGER, "SplitMinInst", flag_SPLITMININSTDEFAULT, (char *)NULL,
		    flag_SPLITMININSTMIN, flag_SPLITMININSTMAX);

    /* Decides the level of sort usage: if 0 then no sort information is processed,
       if 1 all negative monadic literals with a variable as its argument are processed,
       if 2 all negative monadic literals are processed */
    flag_InitIntern(flag_SORTS, flag_UNIQUE, flag_INTEGER, "Sorts", flag_SORTSMONADICWITHVARIABLE, (char *)NULL,
		    flag_SORTSMIN, flag_SORTSMAX);

    /* If set, non-constant Skolem terms in the clausal form of the
     * conjecture are replaced by constants */
    flag_InitIntern(flag_CNFQUANTEXCH, flag_TRANSFORM, flag_INTEGER, "CNFQuantExch", flag_CNFQUANTEXCHOFF, (char *)NULL,
		    flag_CNFQUANTEXCHMIN, flag_CNFQUANTEXCHMAX);

    /* ForwardSubsumption output not activated */
    flag_InitIntern(flag_PSUB, flag_PRINTING, flag_INTEGER, "PSub", flag_PSUBOFF, (char *)NULL,
		    flag_PSUBMIN, flag_PSUBMAX);

    /* Maximal memory allocation */
    flag_InitIntern(flag_MEMORY, flag_UNIQUE, flag_INTEGER, "Memory", flag_MEMORYUNLIMITED, (char *)NULL,
		    flag_MEMORYMIN, flag_MEMORYMAX);

    /* Document static soft typing  */
    flag_InitIntern(flag_DOCSST, flag_PRINTING, flag_INTEGER, "DocSST", flag_DOCSSTOFF, (char *)NULL,
		    flag_DOCSSTMIN, flag_DOCSSTMAX);

    /* Rewriting output not activated  */
    flag_InitIntern(flag_PREW, flag_PRINTING, flag_INTEGER, "PRew", flag_PREWOFF, (char *)NULL,
		    flag_PREWMIN, flag_PREWMAX);

    /* Condensing output not activated  */
    flag_InitIntern(flag_PCON, flag_PRINTING, flag_INTEGER, "PCon", flag_PCONOFF, (char *)NULL,
		    flag_PCONMIN, flag_PCONMAX);

    /* Assignment Equation Deletion output not activated  */
    flag_InitIntern(flag_PAED, flag_PRINTING, flag_INTEGER, "PAED", flag_PAEDOFF, (char *)NULL,
		    flag_PAEDMIN, flag_PAEDMAX);

    /* Tautology output not activated  */
    flag_InitIntern(flag_PTAUT, flag_PRINTING, flag_INTEGER, "PTaut", flag_PTAUTOFF, (char *)NULL,
		    flag_PTAUTMIN, flag_PTAUTMAX);

    /* Output of obvious red. not activated  */
    flag_InitIntern(flag_POBV, flag_PRINTING, flag_INTEGER, "PObv", flag_POBVOFF, (char *)NULL,
		    flag_POBVMIN, flag_POBVMAX);

    /* SortSimplification output not activated  */
    flag_InitIntern(flag_PSSI, flag_PRINTING, flag_INTEGER, "PSSi", flag_PSSIOFF, (char *)NULL,
		    flag_PSSIMIN, flag_PSSIMAX);

    /* Static soft typing output not activated  */
    flag_InitIntern(flag_PSST, flag_PRINTING, flag_INTEGER, "PSST", flag_PSSTOFF, (char *)NULL,
		    flag_PSSTMIN, flag_PSSTMAX);

    /* Proof output not activated  */
    flag_InitIntern(flag_DOCPROOF, flag_UNIQUE, flag_INTEGER, "DocProof", flag_DOCPROOFOFF, (char *)NULL,
		    flag_DOCPROOFMIN, flag_DOCPROOFMAX);

    /* Matching Replacement Resolution output not activated  */
    flag_InitIntern(flag_PMRR, flag_PRINTING, flag_INTEGER, "PMRR", flag_PMRROFF, (char *)NULL,
		    flag_PMRRMIN, flag_PMRRMAX);

    /* Unit conflict output not activated  */
    flag_InitIntern(flag_PUNC, flag_PRINTING, flag_INTEGER, "PUnC", flag_PUNCOFF, (char *)NULL,
		    flag_PUNCMIN, flag_PUNCMAX);

    /* Derived clauses output not activated  */
    flag_InitIntern(flag_PDER, flag_PRINTING, flag_INTEGER, "PDer", flag_PDEROFF, (char *)NULL,
		    flag_PDERMIN, flag_PDERMAX);

    /* Given clause output activated  */
    flag_InitIntern(flag_PGIVEN, flag_PRINTING, flag_INTEGER, "PGiven", flag_PGIVENON, (char *)NULL,
		    flag_PGIVENMIN, flag_PGIVENMAX);

    /* If labels are created they are not printed  */
    flag_InitIntern(flag_PLABELS, flag_PRINTING, flag_INTEGER, "PLabels", flag_PLABELSOFF, (char *)NULL,
		    flag_PLABELSMIN, flag_PLABELSMAX);

    /* Kept clauses output not activated  */
    flag_InitIntern(flag_PKEPT, flag_PRINTING, flag_INTEGER, "PKept", flag_PKEPTOFF, (char *)NULL,
		    flag_PKEPTMIN, flag_PKEPTMAX);

    /* Split backtrack emphasizing not activated */
    flag_InitIntern(flag_DOCSPLIT, flag_PRINTING, flag_INTEGER, "DocSplit", flag_DOCSPLITOFF, (char *)NULL,
		    flag_DOCSPLITMIN, flag_DOCSPLITMAX);

    /* Print information about input clauses */
    flag_InitIntern(flag_PPROBLEM, flag_PRINTING, flag_INTEGER, "PProblem", flag_PPROBLEMON, (char *)NULL,
		    flag_PPROBLEMMIN, flag_PPROBLEMMAX);

    /* Print all derived empty clauses */
    flag_InitIntern(flag_PEMPTYCLAUSE, flag_PRINTING, flag_INTEGER, "PEmptyClause", flag_PEMPTYCLAUSEOFF, (char *)NULL,
		    flag_PEMPTYCLAUSEMIN, flag_PEMPTYCLAUSEMAX);

    /* Print statistic about memory, clauses */
    flag_InitIntern(flag_PSTATISTIC, flag_PRINTING, flag_INTEGER, "PStatistic", flag_PSTATISTICON, (char *)NULL,
		    flag_PSTATISTICMIN, flag_PSTATISTICMAX);

    /* Output saturated set of clauses to file, default no */
    flag_InitIntern(flag_FPMODEL, flag_PRINTING, flag_INTEGER, "FPModel", flag_FPMODELOFF, (char *)NULL,
		    flag_FPMODELMIN, flag_FPMODELMAX);

    /* Output proof in DFG format to file, default no */
    flag_InitIntern(flag_FPDFGPROOF, flag_PRINTING, flag_INTEGER, "FPDFGProof", flag_FPDFGPROOFOFF, (char *)NULL,
		    flag_FPDFGPROOFMIN, flag_FPDFGPROOFMAX);

    /* Output the actual values of all SPASS flags */
    flag_InitIntern(flag_PFLAGS, flag_PRINTING, flag_INTEGER, "PFlags", flag_PFLAGSOFF, (char *)NULL,
		    flag_PFLAGSMIN, flag_PFLAGSMAX);

    /* Optimized skolemization output not activated  */
    flag_InitIntern(flag_POPTSKOLEM, flag_PRINTING, flag_INTEGER, "POptSkolem", flag_POPTSKOLEMOFF, (char *)NULL,
		    flag_POPTSKOLEMMIN, flag_POPTSKOLEMMAX);

    /* Strong skolemization output not activated  */
    flag_InitIntern(flag_PSTRSKOLEM, flag_PRINTING, flag_INTEGER, "PStrSkolem", flag_PSTRSKOLEMOFF, (char *)NULL,
		    flag_PSTRSKOLEMMIN, flag_PSTRSKOLEMMAX);

    /* Printing of clauses deleted by bound restriction not activated  */
    flag_InitIntern(flag_PBDC, flag_PRINTING, flag_INTEGER, "PBDC", flag_PBDCOFF, (char *)NULL,
		    flag_PBDCMIN, flag_PBDCMAX);

    /* Printing of bound increase actions  */
    flag_InitIntern(flag_PBINC, flag_PRINTING, flag_INTEGER, "PBInc", flag_PBINCOFF, (char *)NULL,
		    flag_PBINCMIN, flag_PBINCMAX);

    /* Application of definitions  output activated  */
    flag_InitIntern(flag_PAPPLYDEFS, flag_PRINTING, flag_INTEGER, "PApplyDefs", flag_PAPPLYDEFSOFF, (char *)NULL,
		    flag_PAPPLYDEFSMIN, flag_PAPPLYDEFSMAX);

    /* Amount of time (seconds) available to SPASS, -1 means arbitrary */
    flag_InitIntern(flag_TIMELIMIT, flag_UNIQUE, flag_INTEGER, "TimeLimit", flag_TIMELIMITUNLIMITED, (char *)NULL,
		    flag_TIMELIMITMIN, flag_TIMELIMITMAX);
  
    /* Select: 0 -> no selection, 1 -> select if multiple maximal literals, 2 -> always select */
    flag_InitIntern(flag_SELECT, flag_UNIQUE, flag_INTEGER, "Select", flag_SELECTIFSEVERALMAXIMAL, (char *)NULL,
		    flag_SELECTMIN, flag_SELECTMAX);

    /* Activates the inference rule Empty Sort */
    flag_InitIntern(flag_IEMS, flag_INFERENCE, flag_INTEGER, "IEmS", flag_EMPTYSORTOFF, (char *)NULL,
		    flag_EMPTYSORTMIN, flag_EMPTYSORTMAX);

    /* Activates the inference rule Sort Resolution */
    flag_InitIntern(flag_ISOR, flag_INFERENCE, flag_INTEGER, "ISoR", flag_SORTRESOLUTIONOFF, (char *)NULL,
		    flag_SORTRESOLUTIONMIN, flag_SORTRESOLUTIONMAX);

    /* Activates the inference rule Equality Resolution */
    flag_InitIntern(flag_IEQR, flag_INFERENCE, flag_INTEGER, "IEqR", flag_EQUALITYRESOLUTIONOFF, (char *)NULL,
		    flag_EQUALITYRESOLUTIONMIN, flag_EQUALITYRESOLUTIONMAX);

    /* Activates the inference rule Reflexivity Resolution */
    flag_InitIntern(flag_IERR, flag_INFERENCE, flag_INTEGER, "IERR", flag_REFLEXIVITYRESOLUTIONOFF, (char *)NULL,
		    flag_REFLEXIVITYRESOLUTIONMIN, flag_REFLEXIVITYRESOLUTIONMAX);

    /* Activates the inference rule Equality Factoring */
    flag_InitIntern(flag_IEQF, flag_INFERENCE, flag_INTEGER, "IEqF", flag_EQUALITYFACTORINGOFF, (char *)NULL,
		    flag_EQUALITYFACTORINGMIN, flag_EQUALITYFACTORINGMAX);

    /* Activates the inference rule Merging Paramodulation */
    flag_InitIntern(flag_IMPM, flag_INFERENCE, flag_INTEGER, "IMPm", flag_MERGINGPARAMODULATIONOFF, (char *)NULL,
		    flag_MERGINGPARAMODULATIONMIN, flag_MERGINGPARAMODULATIONMAX);

    /* Activates the inference rule Superposition Right */
    flag_InitIntern(flag_ISPR, flag_INFERENCE, flag_INTEGER, "ISpR", flag_SUPERPOSITIONRIGHTOFF, (char *)NULL,
		    flag_SUPERPOSITIONRIGHTMIN, flag_SUPERPOSITIONRIGHTMAX);
  
    /* Inference rule Ordered Paramodulation not active */  
    flag_InitIntern(flag_IOPM, flag_INFERENCE, flag_INTEGER, "IOPm", flag_ORDEREDPARAMODULATIONOFF, (char *)NULL,
		    flag_ORDEREDPARAMODULATIONMIN, flag_ORDEREDPARAMODULATIONMAX);

    /*   Inference rule Paramodulation not active */  
    flag_InitIntern(flag_ISPM, flag_INFERENCE, flag_INTEGER, "ISPm", flag_STANDARDPARAMODULATIONOFF, (char *)NULL,
		    flag_STANDARDPARAMODULATIONMIN, flag_STANDARDPARAMODULATIONMAX);   

    /* Activates the inference rule Superposition Left */
    flag_InitIntern(flag_ISPL, flag_INFERENCE, flag_INTEGER, "ISpL", flag_SUPERPOSITIONLEFTOFF, (char *)NULL,
		    flag_SUPERPOSITIONLEFTMIN, flag_SUPERPOSITIONLEFTMAX);
  
    /* Activates the inference rule Ordered Resolution */
    flag_InitIntern(flag_IORE, flag_INFERENCE, flag_INTEGER, "IORe", flag_ORDEREDRESOLUTIONOFF, (char *)NULL,
		    flag_ORDEREDRESOLUTIONMIN, flag_ORDEREDRESOLUTIONMAX);

    /* Activates the inference rule Standard Resolution */
    flag_InitIntern(flag_ISRE, flag_INFERENCE, flag_INTEGER, "ISRe", flag_STANDARDRESOLUTIONOFF, (char *)NULL,
		    flag_STANDARDRESOLUTIONMIN, flag_STANDARDRESOLUTIONMAX);

    /* Activates the inference rule Standard Hyperresolution */
    flag_InitIntern(flag_ISHY, flag_INFERENCE, flag_INTEGER, "ISHy", flag_STANDARDHYPERRESOLUTIONOFF, (char *)NULL,
		    flag_STANDARDHYPERRESOLUTIONMIN, flag_STANDARDHYPERRESOLUTIONMAX);
  
    /* Activates the inference rule Ordered Hyperresolution */
    flag_InitIntern(flag_IOHY, flag_INFERENCE, flag_INTEGER, "IOHy", flag_ORDEREDHYPERRESOLUTIONOFF, (char *)NULL,
		    flag_ORDEREDHYPERRESOLUTIONMIN, flag_ORDEREDHYPERRESOLUTIONMAX);

    /* Activates the inference rule UR Resolution */
    flag_InitIntern(flag_IURR, flag_INFERENCE, flag_INTEGER, "IURR", flag_UNITRESULTINGRESOLUTIONOFF, (char *)NULL,
		    flag_UNITRESULTINGRESOLUTIONMIN, flag_UNITRESULTINGRESOLUTIONMAX);

    /* Activates the inference rule Ordered Factoring */
    flag_InitIntern(flag_IOFC, flag_INFERENCE, flag_INTEGER, "IOFc", flag_FACTORINGOFF, (char *)NULL,
		    flag_FACTORINGMIN, flag_FACTORINGMAX);

    /* Activates the inference rule Standard Factoring */
    flag_InitIntern(flag_ISFC, flag_INFERENCE, flag_INTEGER, "ISFc", flag_STANDARDFACTORINGOFF, (char *)NULL,
		    flag_STANDARDFACTORINGMIN, flag_STANDARDFACTORINGMAX);

    /* Activates the inference rule Bounded Unit Resolution */
    flag_InitIntern(flag_IBUR, flag_INFERENCE, flag_INTEGER, "IBUR", flag_BOUNDEDDEPTHUNITRESOLUTIONOFF, (char *)NULL,
		    flag_BOUNDEDDEPTHUNITRESOLUTIONMIN, flag_BOUNDEDDEPTHUNITRESOLUTIONMAX);

    /* Activates the inference rule Definition Application */
    flag_InitIntern(flag_IDEF, flag_INFERENCE, flag_INTEGER, "IDEF", flag_DEFINITIONAPPLICATIONOFF, (char *)NULL,
		    flag_DEFINITIONAPPLICATIONMIN, flag_DEFINITIONAPPLICATIONMAX);

    /* Activates the inference rule  Unit Resolution */
    flag_InitIntern(flag_IUNR, flag_INFERENCE, flag_INTEGER, "IUnR", flag_UNITRESOLUTIONOFF, (char *)NULL,
		    flag_UNITRESOLUTIONMIN, flag_UNITRESOLUTIONMAX);
  
    /* Activates Forward Rewriting */
    flag_InitIntern(flag_RFREW, flag_REDUCTION, flag_INTEGER, "RFRew", flag_RFREWOFF, (char *)NULL,
		    flag_RFREWMIN, flag_RFREWMAX);

    /* Activates Backward Rewriting */
    flag_InitIntern(flag_RBREW, flag_REDUCTION, flag_INTEGER, "RBRew", flag_RBREWOFF, (char *)NULL,
		    flag_RBREWMIN, flag_RBREWMAX);

    /* Activates Unit Conflict */
    flag_InitIntern(flag_RUNC, flag_REDUCTION, flag_INTEGER, "RUnC", flag_RUNCOFF, (char *)NULL,
		    flag_RUNCMIN, flag_RUNCMAX);

    /* Activates Terminator */
    flag_InitIntern(flag_RTER, flag_REDUCTION, flag_INTEGER, "RTer", flag_RTEROFF, (char *)NULL,
		    flag_RTERMIN, flag_RTERMAX);

    /* Activates Forward Subsumption */
    flag_InitIntern(flag_RFSUB, flag_REDUCTION, flag_INTEGER, "RFSub", flag_RFSUBOFF, (char *)NULL,
		    flag_RFSUBMIN, flag_RFSUBMAX);

    /* Activates Backward Subsumption */
    flag_InitIntern(flag_RBSUB, flag_REDUCTION, flag_INTEGER, "RBSub", flag_RBSUBOFF, (char *)NULL,
		    flag_RBSUBMIN, flag_RBSUBMAX);

    /* Activates Forward Matching Replacement Resolution */
    flag_InitIntern(flag_RFMRR, flag_REDUCTION, flag_INTEGER, "RFMRR", flag_RFMRROFF, (char *)NULL,
		    flag_RFMRRMIN, flag_RFMRRMAX);

    /* Activates Backward Matching Replacement Resolution */
    flag_InitIntern(flag_RBMRR, flag_REDUCTION, flag_INTEGER, "RBMRR", flag_RBMRROFF, (char *)NULL,
		    flag_RBMRRMIN, flag_RBMRRMAX);

    /* Activates the reduction rule Obvious Reduction */
    flag_InitIntern(flag_ROBV, flag_REDUCTION, flag_INTEGER, "RObv", flag_ROBVOFF, (char *)NULL,
		    flag_ROBVMIN, flag_ROBVMAX);

    /* Activates the reduction rule Tautology */
    flag_InitIntern(flag_RTAUT, flag_REDUCTION, flag_INTEGER, "RTaut", flag_RTAUTOFF, (char *)NULL,
		    flag_RTAUTMIN, flag_RTAUTMAX);

    /* Activates the reduction rule Sort Simplification */
    flag_InitIntern(flag_RSSI, flag_REDUCTION, flag_INTEGER, "RSSi", flag_RSSIOFF, (char *)NULL,
		    flag_RSSIMIN, flag_RSSIMAX);

    /* Activates static soft typing */
    flag_InitIntern(flag_RSST, flag_REDUCTION, flag_INTEGER, "RSST", flag_RSSTOFF, (char *)NULL,
		    flag_RSSTMIN, flag_RSSTMAX);

    /* Activates Assignment Equation Deletion */
    /* If set to 2 it also eliminates equations */
    /* that are redundant only in non-trivial domains */
    flag_InitIntern(flag_RAED, flag_REDUCTION, flag_INTEGER, "RAED", flag_RAEDOFF, (char *)NULL,
		    flag_RAEDMIN, flag_RAEDMAX);

    /* Activates Condensing */
    flag_InitIntern(flag_RCON, flag_REDUCTION, flag_INTEGER, "RCon", flag_RCONOFF, (char *)NULL,
		    flag_RCONMIN, flag_RCONMAX);

    /* Activates reduction of input clauses */
    flag_InitIntern(flag_RINPUT, flag_UNIQUE, flag_INTEGER, "RInput", flag_RINPUTON, (char *)NULL,
		    flag_RINPUTMIN, flag_RINPUTMAX);

    /* Activates application of definitions */
    flag_InitIntern(flag_APPLYDEFS, flag_UNIQUE, flag_INTEGER, "ApplyDefs", flag_APPLYDEFSOFF, (char *)NULL,
		    flag_APPLYDEFSMIN, flag_APPLYDEFSMAX);

    /* If true usable and worked off are completely interreduced; otherwise only worked off */
    flag_InitIntern(flag_FULLRED, flag_UNIQUE, flag_INTEGER, "FullRed", flag_FULLREDON, (char *)NULL,
		    flag_FULLREDMIN, flag_FULLREDMAX);

    /* Activates unit saturation of input clauses */
    flag_InitIntern(flag_SATINPUT, flag_UNIQUE, flag_INTEGER, "SatInput", flag_SATINPUTOFF, (char *)NULL,
		    flag_SATINPUTMIN, flag_SATINPUTMAX);

    /* Ratio between weight and depth selection of clauses from usable */
    flag_InitIntern(flag_WDRATIO, flag_UNIQUE, flag_INTEGER, "WDRatio", 5, (char *)NULL,
		    flag_WDRATIOMIN, flag_WDRATIOMAX);

    /* Factor to divide the weight of conjecture clauses to prefer them for selection */
    flag_InitIntern(flag_PREFCON, flag_UNIQUE, flag_INTEGER, "PrefCon", flag_PREFCONUNCHANGED,  (char *)NULL,
		    flag_PREFCONMIN, flag_PREFCONMAX);

    /* Weight of a function symbol; weight of clause is used to select given */
    flag_InitIntern(flag_FUNCWEIGHT, flag_UNIQUE, flag_INTEGER, "FuncWeight", 1, (char *)NULL,
		    flag_FUNCWEIGHTMIN, flag_FUNCWEIGHTMAX);

    /* Weight of a variable symbol; weight of clause is used to select given */
    flag_InitIntern(flag_VARWEIGHT, flag_UNIQUE, flag_INTEGER, "VarWeight", 1, (char *)NULL,
		    flag_VARWEIGHTMIN, flag_VARWEIGHTMAX);

    /* Prefer the selection of clauses with many variable occurrences */
    flag_InitIntern(flag_PREFVAR, flag_UNIQUE, flag_INTEGER, "PrefVar", flag_PREFVAROFF, (char *)NULL,
		    flag_PREFVARMIN, flag_PREFVARMAX);

    /* The type of bound: 0 (no bound) 1 (by clause weight) 2 (by clause term depth) */
    flag_InitIntern(flag_BOUNDMODE, flag_UNIQUE, flag_INTEGER, "BoundMode", flag_BOUNDMODEUNLIMITED, (char *)NULL,
		    flag_BOUNDMODEMIN, flag_BOUNDMODEMAX);

    /* The initial bound value, where -1 means no restriction */
    flag_InitIntern(flag_BOUNDSTART, flag_UNIQUE, flag_INTEGER, "BoundStart", flag_BOUNDSTARTUNLIMITED, (char *)NULL,
		    flag_BOUNDSTARTMIN, flag_BOUNDSTARTMAX);

    /* The number of bound saturation loops */
    flag_InitIntern(flag_BOUNDLOOPS, flag_UNIQUE, flag_INTEGER, "BoundLoops", 1, (char *)NULL,
		    flag_BOUNDLOOPSMIN, flag_BOUNDLOOPSMAX);

    /* Flags for selecting the ordering to use */
    flag_InitIntern(flag_ORD, flag_UNIQUE, flag_INTEGER, "Ordering", flag_ORDKBO, (char *)NULL,
		    flag_ORDMIN, flag_ORDMAX);

    /* CNF flag, if set optimized skolemization is performed */
    flag_InitIntern(flag_CNFOPTSKOLEM, flag_UNIQUE, flag_INTEGER, "CNFOptSkolem", flag_CNFOPTSKOLEMON, (char *)NULL,
		    flag_CNFOPTSKOLEMMIN, flag_CNFOPTSKOLEMMAX);

    /* CNF flag, restricts the number of optimized skolemization proof steps */
    flag_InitIntern(flag_CNFPROOFSTEPS, flag_UNIQUE, flag_INTEGER, "CNFProofSteps", 100, (char *)NULL,
		    flag_CNFPROOFSTEPSMIN, flag_CNFPROOFSTEPSMAX);

    /* CNF flag, if set renaming is performed */
    flag_InitIntern(flag_CNFRENAMING, flag_UNIQUE, flag_INTEGER, "CNFRenaming", flag_CNFRENAMINGOPT, (char *)NULL,
		    flag_CNFRENAMINGMIN, flag_CNFRENAMINGMAX);

    /* CNF flag, if set condensing on generated clauses is performed */
    flag_InitIntern(flag_CNFCON, flag_UNIQUE, flag_INTEGER, "CNFCon", flag_CNFCONON, (char *)NULL,
		    flag_CNFCONMIN, flag_CNFCONMAX);

    /* CNF flag, if set subsumtion on generated clauses is performed */
    flag_InitIntern(flag_CNFSUB, flag_UNIQUE, flag_INTEGER, "CNFSub", flag_CNFSUBON, (char *)NULL,
		    flag_CNFSUBMIN, flag_CNFSUBMAX);
  
    /* Amount of time (seconds) available to CNF reductions, -1 means arbitrary */
    flag_InitIntern(flag_CNFREDTIMELIMIT, flag_UNIQUE, flag_INTEGER, "CNFRedTimeLimit", flag_CNFREDTIMELIMITUNLIMITED, (char *)NULL,
		    flag_CNFREDTIMELIMITMIN, flag_CNFREDTIMELIMITMAX);

    /* CNF flag, if set renaming is printed */
    flag_InitIntern(flag_CNFPRENAMING, flag_UNIQUE, flag_INTEGER, "CNFPRenaming", flag_CNFPRENAMINGOFF, (char *)NULL,
		    flag_CNFPRENAMINGMIN, flag_CNFPRENAMINGMAX);

    /* CNF flag, if set renaming variant subformulae are replaced by the same
     * literal */
    flag_InitIntern(flag_CNFRENMATCH, flag_TRANSFORM, flag_INTEGER, "CNFRenMatch", flag_CNFRENMATCHON, (char *)NULL,
		    flag_CNFRENMATCHMIN, flag_CNFRENMATCHMAX);

    /* CNF flag, if set strong skolemization is performed */
    flag_InitIntern(flag_CNFSTRSKOLEM, flag_UNIQUE, flag_INTEGER, "CNFStrSkolem", flag_CNFSTRSKOLEMON, (char *)NULL,
		    flag_CNFSTRSKOLEMMIN, flag_CNFSTRSKOLEMMAX);

    /* CNF flag, if set reductions on equality literals are performed */
    flag_InitIntern(flag_CNFFEQREDUCTIONS, flag_UNIQUE, flag_INTEGER, "CNFFEqR", flag_CNFFEQREDUCTIONSON, (char *)NULL,
		    flag_CNFFEQREDUCTIONSMIN, flag_CNFFEQREDUCTIONSMAX);

    /* dfg2otter flag, if set input options for otter are generated */
    flag_InitIntern(flag_TDFG2OTTEROPTIONS, flag_UNIQUE, flag_INTEGER, "TDfg2OtterOptions", flag_TDFG2OTTEROPTIONSOFF, (char *)NULL,
		    flag_TDFG2OTTEROPTIONSMIN, flag_TDFG2OTTEROPTIONSMAX);

    /* Special flag to indicate syntax of extended modal logic is used and
     * translation is required, set during parsing */
    flag_InitIntern(flag_EML, flag_EMLSPECIAL, flag_INTEGER, "EML", flag_EMLOFF, (char *)NULL,
		    flag_EMLMIN, flag_EMLMAX);

    /* Autonomous mode, as yet not functional */
    flag_InitIntern(flag_EMLAUTO, flag_EMLSPECIAL, flag_INTEGER, "EMLAuto", flag_EMLAUTOON, (char *)NULL,
		    flag_EMLAUTOMIN, flag_EMLAUTOMAX);

    /* If set, propositional formulae are converted to relational formulae
     * before they are translated to FOL */
    flag_InitIntern(flag_EML2REL, flag_EMLSPECIAL, flag_INTEGER, "EML2Rel", flag_EML2RELOFF, (char *)NULL,
		    flag_EML2RELMIN, flag_EML2RELMAX);

    /* Special EML flag, determines the translation method used, the
     * default is the relational translation method */
    flag_InitIntern(flag_EMLTR, flag_EMLSPECIAL, flag_INTEGER, "EMLTranslation", flag_EMLTRREL, (char *)NULL,
		    flag_EMLTRMIN, flag_EMLTRMAX);

    /* Determines which background theory is assumed */
    flag_InitIntern(flag_EMLTHEORY, flag_EMLSPECIAL, flag_INTEGER, "EMLTheory", flag_EMLTHEORYK, (char *)NULL,
		    flag_EMLTHEORYMIN, flag_EMLTHEORYMAX);

    /* If set, translates  tr(dia(phi),s) = nde(s) /\ exists x tr(phi,[s x])  --
     * nde / quantifier expression,
     * otherwise  tr(dia(phi),s) = exists x nde(s) /\ tr(phi,[s x])  --
     * quantifier / nde expression.  
     * Only meaningful when the flag for the functional or semi functional
     * translation method is set. */
    flag_InitIntern(flag_EMLFUNCNDEQ, flag_EMLSPECIAL, flag_INTEGER, "EMLFuncNdeQ", flag_ON, (char *)NULL,
		    flag_EMLFUNCNDEQMIN, flag_EMLFUNCNDEQMAX);

    /* If set the functional translation into fluted logic is used */
    flag_InitIntern(flag_EMLFNARY, flag_EMLSPECIAL, flag_INTEGER, "EMLFuncNary", flag_EMLFNARYOFF, (char *)NULL,
		    flag_EMLFNARYMIN, flag_EMLFNARYMAX);

    /* If set sorts for terms are used */
    flag_InitIntern(flag_EMLFFSORTS, flag_EMLSPECIAL, flag_INTEGER, "EMLFFSorts", flag_EMLFFSORTSOFF, (char *)NULL,
		    flag_EMLFFSORTSMIN, flag_EMLFFSORTSMAX);

    /* If set try to eliminate composition */
    flag_InitIntern(flag_EMLELIMCOMP, flag_EMLSPECIAL, flag_INTEGER, "EMLElimComp", flag_EMLELIMCOMPOFF, (char *)NULL,
		    flag_EMLELIMCOMPMIN, flag_EMLELIMCOMPMAX);

    /* If set the translation to FOL is printed */
    flag_InitIntern(flag_EMLPTRANS, flag_EMLSPECIAL, flag_INTEGER, "EMLPTrans", flag_EMLPTRANSOFF, (char *)NULL,
		    flag_EMLPTRANSMIN, flag_EMLPTRANSMAX);

    /* Deletion of input file not activated  */
    flag_InitIntern(flag_GLOBALRIF, flag_UNIQUE, flag_INTEGER, "rf", flag_GLOBALRIFOFF, (char *)NULL,
		    flag_GLOBALRIFMIN, flag_GLOBALRIFMAX);

    flag_InitIntern(flag_TPTP, flag_UNIQUE, flag_INTEGER, "TPTP", flag_TPTPOFF, (char *)NULL,
		    flag_TPTPMIN, flag_TPTPMAX);
    return;

  case flag_TPTP2DFG:
    flag_InitIntern(flag_INCLUDE, flag_UNIQUE, flag_INTEGER, "include", flag_INCLUDEOFF, (char *)NULL,
		    flag_INCLUDEMIN, flag_INCLUDEMAX);
    return;

  case flag_PGEN:
    flag_InitIntern(flag_PGENT, flag_UNIQUE, flag_INTEGER, "t", flag_PGENTON, (char *)NULL,
		    flag_PGENTMIN, flag_PGENTMAX);
    flag_InitIntern(flag_PGEND, flag_UNIQUE, flag_STRING, "d", flag_PGENDOFF, (char *)NULL,
		    flag_PGENDMIN, flag_PGENDMAX);
    flag_InitIntern(flag_PGENS, flag_UNIQUE, flag_STRING, "s", flag_PGENSOFF, (char *)NULL,
		    flag_PGENSMIN, flag_PGENSMAX);
    flag_InitIntern(flag_PGENQ, flag_UNIQUE, flag_INTEGER, "q", flag_PGENQOFF, (char *)NULL,
		    flag_PGENQMIN, flag_PGENQMAX);
    flag_InitIntern(flag_PGENR, flag_UNIQUE, flag_STRING, "r", flag_PGENROFF, (char *)NULL,
		    flag_PGENRMIN, flag_PGENRMAX);
    flag_InitIntern(flag_PGENC, flag_UNIQUE, flag_INTEGER, "c", flag_PGENCOFF, (char *)NULL,
		    flag_PGENCMIN, flag_PGENCMAX); 
    flag_InitIntern(flag_PGENJ, flag_UNIQUE, flag_INTEGER, "j", flag_PGENJOFF, (char *)NULL,
		    flag_PGENJMIN, flag_PGENJMAX);
    flag_InitIntern(flag_PGENN, flag_UNIQUE, flag_STRING, "n", flag_PGENNOFF, (char *)NULL,
		    flag_PGENNMIN, flag_PGENNMAX);
    flag_InitIntern(flag_PGENXVCG, flag_UNIQUE, flag_INTEGER, "xvcg", flag_PGENXVCGOFF, (char *)NULL,
		    flag_PGENXVCGMIN, flag_PGENXVCGMAX);
    flag_InitIntern(flag_PGENVINCI, flag_UNIQUE, flag_INTEGER, "vinci", flag_PGENVINCIOFF, (char *)NULL,
		    flag_PGENVINCIMIN, flag_PGENVINCIMAX);
    return;

  case flag_RESCMP:
    flag_InitIntern(flag_RESCMPV, flag_UNIQUE, flag_INTEGER, "v", flag_RESCMPVON, (char *)NULL,
		    flag_RESCMPVMIN, flag_RESCMPVMAX);
    return;

  case flag_DFG2DFG:
    flag_InitIntern(flag_DFG2DFGMONADIC, flag_UNIQUE, flag_INTEGER, "v", flag_DFG2DFGMONADICOFF, (char *)NULL,
		    flag_DFG2DFGMONADICMIN, flag_DFG2DFGMONADICMAX);
    flag_InitIntern(flag_DFG2DFGSHALLOW, flag_UNIQUE, flag_INTEGER, "shallow", flag_DFG2DFGSHALLOWOFF, (char *)NULL,
		    flag_DFG2DFGSHALLOWMIN, flag_DFG2DFGSHALLOWMAX);
    flag_InitIntern(flag_DFG2DFGHORN, flag_UNIQUE, flag_INTEGER, "horn", flag_DFG2DFGHORNOFF, (char *)NULL,
		    flag_DFG2DFGHORNMIN, flag_DFG2DFGHORNMAX);
    flag_InitIntern(flag_DFG2DFGLINEAR, flag_UNIQUE, flag_INTEGER, "linear", flag_DFG2DFGLINEAROFF, (char *)NULL,
		    flag_DFG2DFGLINEARMIN, flag_DFG2DFGLINEARMAX);
    return;
    
  }
}


FLAGSTORE flag_DefaultStore(void)
/**************************************************************
  INPUT:   None.
  RETURNS: Default flag store.
***************************************************************/
{
  return flag_DEFAULTSTORE;
}


void flag_Print(FLAGSTORE Store)
/**************************************************************
  INPUT:   A FlagStore.
  RETURNS: Nothing.
  EFFECT:  Prints the values of all flags to stdout.
***************************************************************/
{
  flag_FPrint(stdout, Store);
}



void flag_FPrintFlag(FILE* File, FLAGSTORE Store, FLAG_ID Flag)
/**************************************************************
  INPUT:   A File to print to, a FlagStore, and a Flag.
  RETURNS: Nothing.
  EFFECT:  Prints the value of the Flag in the FlagStore to 
           File.
***************************************************************/
{
  flag_FPrintFlagWithValue(File, Store, Flag, flag_GetFlagIntValue(Store, Flag),  flag_GetFlagStringValue(Store, Flag));
}

void flag_FPrintFlagWithValue(FILE* File, FLAGSTORE Store, FLAG_ID Flag, int IntValue, char* StringValue)
/**************************************************************
  INPUT:   A File to print to, a FlagStore, and a Flag and 
           an int and string value for the flag.
  RETURNS: Nothing.
  EFFECT:  Prints the value of the Flag in the FlagStore to 
           File.
***************************************************************/
{
  if (flag_IsOfValueType(Flag,flag_STRING))
    fprintf(File,"\nset_flag(%s,%s).", flag_Name(Flag), StringValue);
  if (flag_IsOfValueType(Flag,flag_INTEGER))
    fprintf(File,"\nset_flag(%s,%d).", flag_Name(Flag), IntValue); 
}

void flag_FPrint(FILE* File, FLAGSTORE Store)
/**************************************************************
  INPUT:   A File to print to, and a FlagStore.
  RETURNS: Nothing.
  EFFECT:  Prints the values of all flags to File.
***************************************************************/
{
  FLAG_ID  i;
  char name[60];
  
  fputs("list_of_settings(SPASS).{*", File);

  for (i = (FLAG_ID) 0; i < flag_MAXFLAG; i+= (FLAG_ID) 3) {
    if (!flag_IsUndefined(i)) {
      sprintf(name,"set_flag(%s,%d).", flag_Name(i), flag_GetFlagIntValue(Store, i));
      fprintf(File,"\n %-30s",name);
      if (i+1 < flag_MAXFLAG) {
	sprintf(name,"set_flag(%s,%d).", flag_Name(i+ (FLAG_ID) 1), flag_GetFlagIntValue(Store, i+ (FLAG_ID) 1));
	fprintf(File," %-30s",name);
	if (i+2 < flag_MAXFLAG) {
	  sprintf(name," set_flag(%s,%d).", flag_Name(i+ (FLAG_ID) 2), flag_GetFlagIntValue(Store, i+ (FLAG_ID) 2));
	  fprintf(File," %-30s",name);
	}
      }
    }
  }
  fputs("*}\nend_of_list.\n", File);
}


BOOL flag_Lookup(const char* String)
/**************************************************************
  INPUT:   A string <String>.
  RETURNS: TRUE iff <String> is the string of a known flag.
***************************************************************/
{
  return (flag_Id(String) != -1);
}


FLAG_ID flag_Id(const char* String)
/**************************************************************
  INPUT:   A string <String>.
  RETURNS: The identification of the flag <String> if it exists
           -1 otherwise.
***************************************************************/
{
  FLAG_ID i;

  for (i = (FLAG_ID) 0; i < flag_MAXFLAG; i++) {
    const char * Name = flag_Name(i);     
    if (Name && /* Not all the slots need to be initialized */
        string_Equal(Name, String)) 
      return i;
  }

  return (FLAG_ID) -1;
}


const char* flag_Name(FLAG_ID Flag)
/**************************************************************
  INPUT:   A FlagId.
  RETURNS: The name of the flag <Flag>.
  EFFECT:  Looks up the name of the flag <Flag> and returns it,
           if it exists.
***************************************************************/
{
  flag_CheckFlagIdInRange(Flag);
 
  return(flag_PROPERTIES[Flag].name);
}


int flag_Minimum(FLAG_ID Flag)
/**************************************************************
  INPUT:   A FlagId.
  RETURNS: The first integer below the minimal legal value 
           of the flag.
***************************************************************/
{
  flag_CheckFlagIdInRange(Flag);
 
  return flag_PROPERTIES[Flag].minimum;
}


int flag_Maximum(FLAG_ID Flag)
/**************************************************************
  INPUT:   A FlagId.
  RETURNS: The first integer above the maximal legal value 
           of the flag.
***************************************************************/
{
  flag_CheckFlagIdInRange(Flag);
 
  return flag_PROPERTIES[Flag].maximum;
}


FLAG_TYPE flag_Type(FLAG_ID Flag)
/**************************************************************
  INPUT:   A FlagId.
  RETURNS: The flag type.
***************************************************************/
{
  flag_CheckFlagIdInRange(Flag);

  return flag_PROPERTIES[Flag].type;  
}

FLAG_VALUETYPE flag_ValueType(FLAG_ID Flag)
/**************************************************************
  INPUT:   A FlagId.
  RETURNS: The flag type.
***************************************************************/
{
  flag_CheckFlagIdInRange(Flag);

  return flag_PROPERTIES[Flag].vtype;  
}

BOOL flag_IsOfType(FLAG_ID Flag, FLAG_TYPE Type)
/**************************************************************
  INPUT:   A FlagId and a flag type.
  RETURNS: TRUE is the flag is of given type,
           FALSE otherwise.
***************************************************************/
{
  flag_CheckFlagIdInRange(Flag);
  flag_CheckFlagTypeInRange(Type);

  return (BOOL) (flag_Type(Flag) == Type);
}

BOOL flag_IsOfValueType(FLAG_ID Flag, FLAG_VALUETYPE VType)
/**************************************************************
  INPUT:   A FlagId and a flag type.
  RETURNS: TRUE is the flag is of given type,
           FALSE otherwise.
***************************************************************/
{
  flag_CheckFlagIdInRange(Flag);
  flag_CheckFlagValueTypeInRange(VType);

  return (BOOL) (flag_ValueType(Flag) == VType);
}

int flag_GetFlagIntValue(FLAGSTORE Store, FLAG_ID FlagId)
/**************************************************************
  INPUT:   A FlagStore, and a FlagId.
  RETURNS: The flag value.
***************************************************************/
{
  int IntValue;

  flag_CheckFlagIdInRange(FlagId);

  IntValue = Store[FlagId].intvalue;
#ifdef CHECK
  if (IntValue == flag_CLEAN || IntValue == flag_UNDEFINED) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In flag_GetFlagIntValue:");
    misc_ErrorReport(" Attempt to read undefined flag value.");
    misc_FinishErrorReport();
  }
#endif

  return IntValue;
}

char* flag_GetFlagStringValue(FLAGSTORE Store, FLAG_ID FlagId)
/**************************************************************
  INPUT:   A FlagStore, and a FlagId.
  RETURNS: The flag value.
***************************************************************/
{
  char * StringValue;
  int    IntValue;  

  flag_CheckFlagIdInRange(FlagId);

  StringValue = Store[FlagId].stringvalue;
  IntValue    = Store[FlagId].intvalue;

#ifdef CHECK
  if (IntValue == flag_CLEAN || IntValue == flag_UNDEFINED || !flag_IsOfValueType(FlagId,flag_STRING)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In flag_GetFlagStringValue:");
    misc_ErrorReport(" Attempt to read undefined flag value.");
    misc_FinishErrorReport();
  }
#endif

  return StringValue;
}

void flag_SetFlagIntValue(FLAGSTORE Store, FLAG_ID FlagId, int Value)
/**************************************************************
  INPUT:   A FlagStore, a FlagId and a new value.
  RETURNS: Nothing.
***************************************************************/
{
#ifdef CHECK
  if (flag_StoreIsDefaultStore(Store)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In flag_SetFlagIntValue:");
    misc_ErrorReport(" Attempt to modify default flag value.");
    misc_FinishErrorReport();
  }
#endif

  flag_CheckFlagIdInRange(FlagId);

  flag_CheckFlagValueInRange(FlagId, Value);

  Store[FlagId].intvalue = Value;
}

void flag_SetFlagStringValue(FLAGSTORE Store, FLAG_ID FlagId, char* Value)
/**************************************************************
  INPUT:   A FlagStore, a FlagId and a new value.
  RETURNS: Nothing.
***************************************************************/
{
#ifdef CHECK
  if (flag_StoreIsDefaultStore(Store)) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In flag_SetFlagStringValue:");
    misc_ErrorReport(" Attempt to modify default flag value.");
    misc_FinishErrorReport();
  }
#endif

  flag_CheckFlagIdInRange(FlagId);


  Store[FlagId].stringvalue = Value;
}

void flag_ClearInferenceRules(FLAGSTORE Store)
/**************************************************************
  INPUT:   A FlagStore.
  RETURNS: Nothing.
  EFFECT:  Turns all inference rules off.
***************************************************************/
{
  FLAG_ID i;

  for (i = (FLAG_ID) 0; i < flag_MAXFLAG; i++) {
    if (!flag_IsUndefined(i) && flag_IsInference(i))
      flag_SetFlagIntValue(Store, i, flag_OFF);
  }
}


void flag_ClearReductionRules(FLAGSTORE Store)
/**************************************************************
  INPUT:   A FlagStore.
  RETURNS: Nothing.
  EFFECT:  Turns all reduction rules off.
***************************************************************/
{
  FLAG_ID i;

  for (i = (FLAG_ID) 0; i < flag_MAXFLAG; i++) {
    if (!flag_IsUndefined(i) && flag_IsReduction(i)) {
        flag_SetFlagIntValue(Store, i, flag_OFF);
    }
  }
}


void flag_ClearPrinting(FLAGSTORE Store)
/**************************************************************
  INPUT:   A FlagStore.
  RETURNS: Nothing.
  EFFECT:  Turns all printing off.
***************************************************************/
{

  FLAG_ID i;

  for (i = (FLAG_ID) 0; i < flag_MAXFLAG; i++) {
    if (!flag_IsUndefined(i) && flag_IsPrinting(i))
      flag_SetFlagIntValue(Store, i, flag_OFF);
  }
}


void flag_SetReductionsToDefaults(FLAGSTORE Store)
/**************************************************************
  INPUT:   A FlagStore.
  RETURNS: Nothing.
  EFFECT:  Sets all reduction rules to defaults.
***************************************************************/
{

  FLAG_ID i;

  for (i = (FLAG_ID) 0; i < flag_MAXFLAG; i++) {
    if (!flag_IsUndefined(i) && flag_IsReduction(i))
      flag_SetFlagToDefault(Store, i);
  }
}


void flag_InitFlotterSubproofFlags(FLAGSTORE Source, FLAGSTORE Target)
/**************************************************************
  INPUT:   Two flag stores.
  RETURNS: Nothing.
  EFFECT:  Initializes the flag store <Target> to the values required by a
           Flotter subproof. The other flag store is needed to take over
           some flags, e.g. DOCPROOF.
***************************************************************/
{
  /* Deactivate printing */
  flag_ClearPrinting(Target);

  /* Deactivate inference rules */
  flag_ClearInferenceRules(Target);

  /* Set reductions to default values */
  flag_SetReductionsToDefaults(Target);

  flag_SetFlagToDefault(Target, flag_CNFFEQREDUCTIONS);
  flag_SetFlagToDefault(Target, flag_RINPUT);
 
  /* Copy flag_DOCPROOF and flag_CNFPROOFSTEPS */
  flag_TransferFlag(Source, Target, flag_DOCPROOF);  
  flag_TransferFlag(Source, Target, flag_CNFPROOFSTEPS);

  /* Activate BoundedDepthUnitResolution */
  flag_SetFlagIntValue(Target, flag_IBUR, flag_BOUNDEDDEPTHUNITRESOLUTIONON);

  /* Activate KBO */
  flag_SetFlagIntValue(Target, flag_ORD, flag_ORDKBO);

  /* Transfer Weights for Terms */
  flag_TransferFlag(Source, Target, flag_FUNCWEIGHT);  
  flag_TransferFlag(Source, Target, flag_VARWEIGHT);

  /* Transfer Selection Strategy, not needed for depth bounded */
  /* unit resolution (see above) but for other potentially useful inference rules */
  flag_TransferFlag(Source, Target, flag_SELECT);  
}


void flag_InitFlotterFlags(FLAGSTORE Source, FLAGSTORE Target)
/**************************************************************
  INPUT:   Two flag stores.
  RETURNS: Nothing.
  EFFECT:  Initalizes the flag store <Target> to the values required by
           Flotter. The other flag store is needed to set
           some flags, e.g. DOCPROOF.
***************************************************************/
{
  flag_InitFlotterSubproofFlags(Source, Target);

  /* Set ordering to default value */
  flag_SetFlagToDefault(Target, flag_ORD);

  /* Set weighting flags to default values */
  flag_SetFlagToDefault(Target, flag_FUNCWEIGHT);
  flag_SetFlagToDefault(Target, flag_VARWEIGHT);

  /* Copy given values to diverse flags */
  flag_TransferFlag(Source, Target, flag_CNFRENAMING);
  flag_TransferFlag(Source, Target, flag_CNFRENMATCH);
  flag_TransferFlag(Source, Target, flag_CNFOPTSKOLEM);
  flag_TransferFlag(Source, Target, flag_CNFSTRSKOLEM);
  flag_TransferFlag(Source, Target, flag_CNFSUB);
  flag_TransferFlag(Source, Target, flag_CNFCON);
  flag_TransferFlag(Source, Target, flag_CNFREDTIMELIMIT);
  flag_TransferFlag(Source, Target, flag_CNFQUANTEXCH);
  flag_TransferFlag(Source, Target, flag_PAPPLYDEFS);
  flag_TransferFlag(Source, Target, flag_CNFFEQREDUCTIONS);
  flag_TransferFlag(Source, Target, flag_PBDC);
  flag_TransferFlag(Source, Target, flag_PBINC);
  flag_TransferFlag(Source, Target, flag_FLOTTER);
  flag_TransferFlag(Source, Target, flag_CNFPRENAMING);
  flag_TransferFlag(Source, Target, flag_POPTSKOLEM);
  flag_TransferFlag(Source, Target, flag_PSTRSKOLEM);
  flag_TransferFlag(Source, Target, flag_INTERACTIVE);
}


void flag_CheckStore(FLAGSTORE Store) 
/**************************************************************
  INPUT:   A flag store.
  RETURNS: TRUE is the flag store is in a valid state,
           FALSE otherwise.
***************************************************************/
{
  FLAG_ID i;
  int  intvalue;

  /* check all flags */
  for (i = (FLAG_ID) 0; i < flag_MAXFLAG; i ++) {
    /* Get flag value first. We can't use flag_GetFlagIntValue() since it
       prints an error message and exits, if a flag is clean. A flag can
       be clean, only reading it is an error (for most functions).
    */

    intvalue = Store[i].intvalue;
    if (!flag_IsUndefined(i) && intvalue != flag_CLEAN) {
      flag_CheckFlagValueInRange(i,intvalue);
    }
  }
}

void flag_DeleteStore(FLAGSTORE Store)
/**************************************************************
  INPUT:   A flag store.
  EFFECT:  Deletes Store and all string values.
***************************************************************/
{
  FLAG_ID i;

#ifdef CHECK
  /* Check if the flag store is a valid state */
  flag_CheckStore(Store);
#endif

  for (i = (FLAG_ID) 0; i < flag_MAXFLAG; i++) {
    if (!flag_IsUndefined(i) && flag_IsOfValueType(i, flag_STRING) &&
	flag_GetFlagStringValue(Store,i) != (char*)NULL)
      string_StringFree(flag_GetFlagStringValue(Store,i));
  }
  
  memory_Free(Store,sizeof(FLAGARRAY));
}



unsigned int flag_GetMaxFlag()
/**************************************************************
  INPUT:   Nothing.
  RETURNS: maximal flag value
  EFFECT:  None.
***************************************************************/
{
	return flag_MAXFLAG;
}

void flag_PrintEmlTranslationMethod(int flag)
/**************************************************************
  INPUT:   A FlagStore.
  RETURNS: Nothing.
  EFFECT:  Prints the values of all inference flags to stdout.
***************************************************************/
{
  if (flag == flag_EMLTRREL) 
    printf("relational translation method");
  else if (flag == flag_EMLTRFUNC)
    printf("functional translation method");
  else if (flag == flag_EMLTROPTFUNC)
    printf("optimised functional translation method");
  else if (flag == flag_EMLTRSEMIFUNC)
    printf("semi-functional translation method");
  else 
    printf("unknown translation method");
} 

void flag_CheckEmlTranslationFlags(FLAGSTORE Store, int flag)
/**************************************************************
  INPUT:   A FlagStore.
  RETURNS: Nothing.
  EFFECT:  Print warnings for non-sensical flag combinations.
***************************************************************/
{
  if ((flag != flag_EMLTRFUNC && flag != flag_EMLTROPTFUNC)) {
    if (flag_GetFlagIntValue(Store, flag_EMLFUNCNDEQ)) {
      printf("\nWarning: Flag %s has no effect together with ",
        flag_Name(flag_EMLFUNCNDEQ));
      flag_PrintEmlTranslationMethod(flag);
    }
    if (flag_GetFlagIntValue(Store, flag_EMLFFSORTS)) {
      printf("\nWarning: Flag %s has no effect together with ",
        flag_Name(flag_EMLFFSORTS));
      flag_PrintEmlTranslationMethod(flag);
    }
    if (flag_GetFlagIntValue(Store, flag_EMLFFSORTS)) {
      printf("\nWarning: Flag %s has no effect together with ",
        flag_Name(flag_EMLFFSORTS));
      flag_PrintEmlTranslationMethod(flag);
    }
  }
  /* Current implementation of optimised and strong 
   * Skolemisation works a copy of the clauses, thus 
   * setting flag_CNFQUANTEXCH would have no effect */
  if (flag_GetFlagIntValue(Store, flag_CNFQUANTEXCH) && flag_GetFlagIntValue(Store, flag_CNFOPTSKOLEM)) {      
    printf("\nWarning: Flag %s is incompatible with flag %s; ",
	  flag_Name(flag_CNFOPTSKOLEM), flag_Name(flag_CNFQUANTEXCH));
    printf("\n    always use %s together with both %s and %s disabled \n",
	  flag_Name(flag_CNFQUANTEXCH), flag_Name(flag_CNFOPTSKOLEM), flag_Name(flag_CNFSTRSKOLEM));
    flag_SetFlagIntValue(Store, flag_CNFOPTSKOLEM, flag_CNFOPTSKOLEMOFF);
    /* Alternatively, make the prover abort at this point,
     * or fix the dependency of cnf_Init of flag_CNFSTRSKOLEM */
  }
  if (flag_GetFlagIntValue(Store, flag_CNFQUANTEXCH) && flag_GetFlagIntValue(Store, flag_CNFSTRSKOLEM)) {      
    printf("\nWarning: Flag %s is incompatible with flag %s; ",
	  flag_Name(flag_CNFSTRSKOLEM), flag_Name(flag_CNFQUANTEXCH));
    printf("\n    always use %s together with both %s and %s disabled \n",
	  flag_Name(flag_CNFQUANTEXCH), flag_Name(flag_CNFOPTSKOLEM), flag_Name(flag_CNFSTRSKOLEM));
    flag_SetFlagIntValue(Store, flag_CNFSTRSKOLEM, flag_CNFSTRSKOLEMOFF);
    /* Not setting this will cause a segmentation fault!
     * Alternatively, make the prover abort at this point,
     * or fix the dependency of cnf_Init of flag_CNFSTRSKOLEM */
  }
}

