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
/* $Revision: 1.13 $                                        * */
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


/* $RCSfile: flags.h,v $ */

#ifndef _FLAGS_
#define _FLAGS_

#include <limits.h>
#include <stdio.h>

#include "memory.h"
#include "misc.h"

/**************************************************************/
/* Data Structures and Constants                              */
/**************************************************************/

extern const int flag_CLEAN;
extern const int flag_UNDEFINED;

/* Define the legal values for all flags as data types.
  
   All flags have a minimum and a maximum. Legal values are 
   within that range, *excluding* the minimum, maximum value. By using 
   flag_XXXMIN and flag_XXXMAX we have a simple test for a 
   flag's correctness:

     if
       flag value <= flag minimum 
       or flag value >= flag maximum
     then
       the flag has an illegal state.

   Boolean flags have two legal values:
    * flag_XXXOFF ( = 0)
    * flag_XXXON ( = 1)
*/

/* Names for the different programs using */
/* the module and requiring different flags */

typedef enum { flag_SPASS, flag_TPTP2DFG, flag_PGEN, flag_RESCMP, flag_DFG2DFG
} FLAG_USERS;


/* State definitions for boolean flags */
typedef enum { flag_OFF = 0,
	       flag_ON  = 1
} FLAG_BOOLEAN;

/* State definitions for flag_APPLYDEFS */
typedef enum { flag_APPLYDEFSMIN = -1,
               flag_APPLYDEFSOFF = flag_OFF,
               flag_APPLYDEFSMAX = INT_MAX
} FLAG_APPLYDEFSTYPE;

/* State definitions for flag_AUTO */
typedef enum { flag_AUTOMIN = -1,
	       flag_AUTOOFF = flag_OFF,
	       flag_AUTOON  = flag_ON,
	       flag_AUTOMAX
} FLAG_AUTOTYPE;

/* State definitions for flag_BOUNDLOOPS */
typedef enum { flag_BOUNDLOOPSMIN = 0,
	       flag_BOUNDLOOPSMAX = INT_MAX
} FLAG_BOUNDLOOPSTYPE;

/* State definitions for flag_BOUNDMODE */
typedef enum { flag_BOUNDMODEMIN = -1,
	       flag_BOUNDMODEUNLIMITED,
	       flag_BOUNDMODERESTRICTEDBYWEIGHT, 
	       flag_BOUNDMODERESTRICTEDBYDEPTH,
	       flag_BOUNDMODEMAX
} FLAG_BOUNDMODETYPE;

/* State definitions for flag_BOUNDSTART */
typedef enum { flag_BOUNDSTARTMIN = -2,
	       flag_BOUNDSTARTUNLIMITED,
	       flag_BOUNDSTARTMAX = INT_MAX
} FLAG_BOUNDSTARTTYPE;

/* State definitions for flag_CNFFEQREDUCTIONS */
typedef enum { flag_CNFFEQREDUCTIONSMIN = -1,
               flag_CNFFEQREDUCTIONSOFF = flag_OFF,
               flag_CNFFEQREDUCTIONSON  = flag_ON,
               flag_CNFFEQREDUCTIONSMAX
} FLAG_CNFFEQREDUCTIONSTYPE;

/* State definitions for flag_CNFOPTSKOLEM */
typedef enum { flag_CNFOPTSKOLEMMIN = -1,
               flag_CNFOPTSKOLEMOFF = flag_OFF,
               flag_CNFOPTSKOLEMON  = flag_ON,
               flag_CNFOPTSKOLEMMAX
} flag_CNFOPTSKOLEMTYPE;

/* State definitions for flag_CNFPRENAMING */
typedef enum { flag_CNFPRENAMINGMIN = -1,
               flag_CNFPRENAMINGOFF = flag_OFF,
               flag_CNFPRENAMINGON  = flag_ON,
               flag_CNFPRENAMINGMAX
} FLAG_CNFPRENAMINGTYPE;

/* State definitions for flag_CNFPROOFSTEPS */
typedef enum { flag_CNFPROOFSTEPSMIN = 0,
	       flag_CNFPROOFSTEPSMAX = INT_MAX
} FLAG_CNFPROOFSTEPSTYPE;


/* State definitions for flag_CNFSUB */
typedef enum { flag_CNFSUBMIN = -1,
               flag_CNFSUBOFF = flag_OFF,
               flag_CNFSUBON  = flag_ON,
               flag_CNFSUBMAX
} flag_CNFSUBTYPE;


/* State definitions for flag_CNFCON */
typedef enum { flag_CNFCONMIN = -1,
               flag_CNFCONOFF = flag_OFF,
               flag_CNFCONON  = flag_ON,
               flag_CNFCONMAX
} flag_CNFCONTYPE;

/* State definitions for flag_CNFREDTIMELIMIT */
typedef enum { flag_CNFREDTIMELIMITMIN = -2,
	       flag_CNFREDTIMELIMITUNLIMITED,
	       flag_CNFREDTIMELIMITMAX = INT_MAX
} FLAG_CNFREDTIMELIMITTYPE;

/* State definitions for flag_CNFRENAMING */
typedef enum { flag_CNFRENAMINGMIN = -1,
               flag_CNFRENAMINGOFF = flag_OFF,
               flag_CNFRENAMINGOPT,
	       flag_CNFRENAMINGCOMPLEX,
	       flag_CNFRENAMINGQUANTIFIED,
               flag_CNFRENAMINGMAX
} FLAG_CNFRENAMINGTYPE;

/* State definitions for flag_CNFRENMATCH */
typedef enum { flag_CNFRENMATCHMIN = -1,
	       flag_CNFRENMATCHOFF = flag_OFF,
	       flag_CNFRENMATCHON  = flag_ON,
	       flag_CNFRENMATCHMAX 
} FLAG_CNFRENMATCHTYPE;

/* State definitions for flag_CNFSTRSKOLEM */
typedef enum { flag_CNFSTRSKOLEMMIN = -1,
               flag_CNFSTRSKOLEMOFF = flag_OFF,
               flag_CNFSTRSKOLEMON  = flag_ON,
               flag_CNFSTRSKOLEMMAX
} FLAG_CNFSTRSKOLEMTYPE;

/* State definitions for flag_DOCPROOF */
typedef enum { flag_DOCPROOFMIN = -1,
	       flag_DOCPROOFOFF = flag_OFF,
	       flag_DOCPROOFON  = flag_ON,
	       flag_DOCPROOFMAX
} FLAG_DOCPROOFTYPE;

/* State definitions for flag_DOCSPLIT */
typedef enum { flag_DOCSPLITMIN = -1,
	       flag_DOCSPLITOFF = flag_OFF,
	       flag_DOCSPLITON  = flag_ON,
	       flag_DOCSPLITMAX = 3
} FLAG_DOCSPLITTYPE;

/* State definitions for flag_DOCSST */
typedef enum { flag_DOCSSTMIN = -1,
	       flag_DOCSSTOFF = flag_OFF,
	       flag_DOCSSTON  = flag_ON,
	       flag_DOCSSTMAX
} FLAG_DOCSSTTYPE;

/* State definitions for flag_FLOTTER */
typedef enum { flag_FLOTTERMIN = -1,
	       flag_FLOTTEROFF = flag_OFF,
	       flag_FLOTTERON  = flag_ON,
	       flag_FLOTTERMAX
} FLAG_FLOTTERTYPE;

/* State definitions for flag_FPDFGPROOF */
typedef enum { flag_FPDFGPROOFMIN = -1,
               flag_FPDFGPROOFOFF = flag_OFF,
               flag_FPDFGPROOFON  = flag_ON,
               flag_FPDFGPROOFMAX
} FLAG_FPDFGPROOFTYPE;

/* State definitions for flag_FPMODEL */
typedef enum { flag_FPMODELMIN = -1,
	       flag_FPMODELOFF = flag_OFF,
	       flag_FPMODELALLCLAUSES,
	       flag_FPMODELPOTENTIALLYPRODUCTIVECLAUSES,
	       flag_FPMODELMAX
} FLAG_FPMODELTYPE;

/* State definitions for flag_FULLRED */
typedef enum { flag_FULLREDMIN = -1,
               flag_FULLREDOFF = flag_OFF,
               flag_FULLREDON  = flag_ON,
               flag_FULLREDMAX
} FLAG_FULLREDTYPE;


/* State definitions for flag_FUNCWEIGHT */
typedef enum { flag_FUNCWEIGHTMIN = 0,
	       flag_FUNCWEIGHTMAX = INT_MAX
} FLAG_FUNCWEIGHTTYPE;

/* State definitions for flag_IBUR */
typedef enum { flag_BOUNDEDDEPTHUNITRESOLUTIONMIN = -1,
               flag_BOUNDEDDEPTHUNITRESOLUTIONOFF = flag_OFF,
               flag_BOUNDEDDEPTHUNITRESOLUTIONON  = flag_ON,
               flag_BOUNDEDDEPTHUNITRESOLUTIONMAX
} FLAG_IBURTYPE;

/* State definitions for flag_IDEF */
typedef enum { flag_DEFINITIONAPPLICATIONMIN = -1,
               flag_DEFINITIONAPPLICATIONOFF = flag_OFF,
               flag_DEFINITIONAPPLICATIONON  = flag_ON,
               flag_DEFINITIONAPPLICATIONMAX
} FLAG_IDEFTYPE;

/* State definitions for flag_IEMS */
typedef enum { flag_EMPTYSORTMIN = -1,
               flag_EMPTYSORTOFF = flag_OFF,
               flag_EMPTYSORTON  = flag_ON,
               flag_EMPTYSORTMAX
} FLAG_IEMSTYPE;

/* State definitions for flag_IEQF */
typedef enum { flag_EQUALITYFACTORINGMIN = -1,
               flag_EQUALITYFACTORINGOFF = flag_OFF,
               flag_EQUALITYFACTORINGON  = flag_ON,
               flag_EQUALITYFACTORINGMAX
} FLAG_IEQFTYPE;

/* State definitions for flag_IEQR */
typedef enum { flag_EQUALITYRESOLUTIONMIN = -1,
               flag_EQUALITYRESOLUTIONOFF = flag_OFF,
               flag_EQUALITYRESOLUTIONON  = flag_ON,
               flag_EQUALITYRESOLUTIONMAX
} FLAG_IEQRTYPE;

/* State definitions for flag_IERR */
typedef enum { flag_REFLEXIVITYRESOLUTIONMIN = -1,
               flag_REFLEXIVITYRESOLUTIONOFF = flag_OFF,
               flag_REFLEXIVITYRESOLUTIONON  = flag_ON,
               flag_REFLEXIVITYRESOLUTIONMAX
} FLAG_IERRTYPE;

/* State definitions for flag_IMPM */
typedef enum { flag_MERGINGPARAMODULATIONMIN = -1,
               flag_MERGINGPARAMODULATIONOFF = flag_OFF,
               flag_MERGINGPARAMODULATIONON  = flag_ON,
               flag_MERGINGPARAMODULATIONMAX
} FLAG_IMPMTYPE;

/* State definitions for flag_INTERACTIVE */
typedef enum { flag_INTERACTIVEMIN = -1,
	       flag_INTERACTIVEOFF = flag_OFF,
	       flag_INTERACTIVEON  = flag_ON,
	       flag_INTERACTIVEMAX
} FLAG_INTERACTIVETYPE;

/* State definitions for flag_IOFC */
typedef enum { flag_FACTORINGMIN = -1,
	       flag_FACTORINGOFF = flag_OFF,
	       flag_FACTORINGONLYRIGHT,
	       flag_FACTORINGRIGHTANDLEFT,
	       flag_FACTORINGMAX
} FLAG_IOFCTYPE;

/* State definitions for flag_IOHY */
typedef enum { flag_ORDEREDHYPERRESOLUTIONMIN = -1,
               flag_ORDEREDHYPERRESOLUTIONOFF = flag_OFF,
               flag_ORDEREDHYPERRESOLUTIONON  = flag_ON,
               flag_ORDEREDHYPERRESOLUTIONMAX
} FLAG_IOHYTYPE;

/* State definitions for flag_IOPM */
typedef enum { flag_ORDEREDPARAMODULATIONMIN = -1,
               flag_ORDEREDPARAMODULATIONOFF = flag_OFF,
               flag_ORDEREDPARAMODULATIONON  = flag_ON,
               flag_ORDEREDPARAMODULATIONMAX
} FLAG_IOPMTYPE;

/* State definitions for flag_IORE */
typedef enum { flag_ORDEREDRESOLUTIONMIN = -1,
	       flag_ORDEREDRESOLUTIONOFF = flag_OFF,
	       flag_ORDEREDRESOLUTIONNOEQUATIONS,
	       flag_ORDEREDRESOLUTIONWITHEQUATIONS,
	       flag_ORDEREDRESOLUTIONMAX
} FLAG_IORETYPE;

/* State definitions for flag_ISFC */
typedef enum { flag_STANDARDFACTORINGMIN = -1,
	       flag_STANDARDFACTORINGOFF = flag_OFF,
	       flag_STANDARDFACTORINGON  = flag_ON,
	       flag_STANDARDFACTORINGMAX
} FLAG_ISFCTYPE;

/* State definitions for flag_ISHY */
typedef enum { flag_STANDARDHYPERRESOLUTIONMIN = -1,
               flag_STANDARDHYPERRESOLUTIONOFF = flag_OFF,
               flag_STANDARDHYPERRESOLUTIONON  = flag_ON,
               flag_STANDARDHYPERRESOLUTIONMAX
} FLAG_ISHYTYPE;

/* State definitions for flag_ISOR */
typedef enum { flag_SORTRESOLUTIONMIN = -1,
               flag_SORTRESOLUTIONOFF = flag_OFF,
               flag_SORTRESOLUTIONON  = flag_ON,
               flag_SORTRESOLUTIONMAX
} FLAG_ISORTYPE;

/* State definitions for flag_ISPL */
typedef enum { flag_SUPERPOSITIONLEFTMIN = -1,
               flag_SUPERPOSITIONLEFTOFF = flag_OFF,
               flag_SUPERPOSITIONLEFTON  = flag_ON,
               flag_SUPERPOSITIONLEFTMAX
} FLAG_ISPLTYPE;

/* State definitions for flag_ISPM */
typedef enum { flag_STANDARDPARAMODULATIONMIN = -1,
               flag_STANDARDPARAMODULATIONOFF = flag_OFF,
               flag_STANDARDPARAMODULATIONON  = flag_ON,
               flag_STANDARDPARAMODULATIONMAX
} FLAG_ISPMTYPE;

/* State definitions for flag_ISPR */
typedef enum { flag_SUPERPOSITIONRIGHTMIN = -1,
               flag_SUPERPOSITIONRIGHTOFF = flag_OFF,
               flag_SUPERPOSITIONRIGHTON  = flag_ON,
               flag_SUPERPOSITIONRIGHTMAX
} FLAG_ISPRTYPE;

/* State definitions for flag_ISRE */
typedef enum { flag_STANDARDRESOLUTIONMIN = -1,
	       flag_STANDARDRESOLUTIONOFF = flag_OFF,
	       flag_STANDARDRESOLUTIONNOEQUATIONS,
	       flag_STANDARDRESOLUTIONWITHEQUATIONS,
	       flag_STANDARDRESOLUTIONMAX
} FLAG_ISRETYPE;

/* State definitions for flag_IUNR */
typedef enum { flag_UNITRESOLUTIONMIN = -1,
               flag_UNITRESOLUTIONOFF = flag_OFF,
               flag_UNITRESOLUTIONON  = flag_ON,
               flag_UNITRESOLUTIONMAX
} FLAG_IUNRTYPE;

/* State definitions for flag_IURR */
typedef enum { flag_UNITRESULTINGRESOLUTIONMIN = -1,
               flag_UNITRESULTINGRESOLUTIONOFF = flag_OFF,
               flag_UNITRESULTINGRESOLUTIONON  = flag_ON,
               flag_UNITRESULTINGRESOLUTIONMAX
} FLAG_IURRTYPE;

/* State definitions for flag_LOOPS */
typedef enum { flag_LOOPSMIN = -2,
	       flag_LOOPSUNLIMITED,
	       flag_LOOPSMAX = INT_MAX
} FLAG_LOOPSTYPE;

/* State definitions for flag_MEMORY */
typedef enum { flag_MEMORYMIN = -2,
	       flag_MEMORYUNLIMITED,
	       flag_MEMORYMAX = INT_MAX
} FLAG_MEMORYTYPE;

/* State definitions for flag_ORD */
typedef enum { flag_ORDMIN = -1,
	       flag_ORDKBO, 
	       flag_ORDRPOS,
	       flag_ORDMAX
} FLAG_ORDTYPE;

/* State definitions for flag_PAPPLYDEFS */
typedef enum { flag_PAPPLYDEFSMIN = -1,
               flag_PAPPLYDEFSOFF = flag_OFF,
               flag_PAPPLYDEFSON  = flag_ON,
               flag_PAPPLYDEFSMAX
} FLAG_PAPPLYDEFSTYPE;

/* State definitions for flag_PBDC */
typedef enum { flag_PBDCMIN = -1,
               flag_PBDCOFF = flag_OFF,
               flag_PBDCON  = flag_ON,
               flag_PBDCMAX
} FLAG_PBDCTYPE;

/* State definitions for flag_PBINC */
typedef enum { flag_PBINCMIN = -1,
               flag_PBINCOFF = flag_OFF,
               flag_PBINCON  = flag_ON,
               flag_PBINCMAX
} FLAG_PBINCTYPE;

/* State definitions for flag_PMRR */
typedef enum { flag_PMRRMIN = -1,
	       flag_PMRROFF = flag_OFF,
	       flag_PMRRON  = flag_ON,
	       flag_PMRRMAX
} FLAG_PMRRTYPE;

/* State definitions for flag_PCON */
typedef enum { flag_PCONMIN = -1,
	       flag_PCONOFF = flag_OFF,
	       flag_PCONON  = flag_ON,
	       flag_PCONMAX
} FLAG_PCONTYPE;

/* State definitions for flag_PDER */
typedef enum { flag_PDERMIN = -1,
	       flag_PDEROFF = flag_OFF,
	       flag_PDERON  = flag_ON,
	       flag_PDERMAX
} FLAG_PDERTYPE;

/* State definitions for flag_PEMPTYCLAUSE */
typedef enum { flag_PEMPTYCLAUSEMIN = -1,
	       flag_PEMPTYCLAUSEOFF = flag_OFF,
	       flag_PEMPTYCLAUSEON  = flag_ON,
	       flag_PEMPTYCLAUSEMAX
} FLAG_PEMPTYCLAUSETYPE;

/* State definitions for flag_PFLAGS */
typedef enum { flag_PFLAGSMIN = -1,
               flag_PFLAGSOFF = flag_OFF,
               flag_PFLAGSON  = flag_ON,
               flag_PFLAGSMAX
} FLAG_PFLAGSTYPE;

/* State definitions for flag_PGIVEN */
typedef enum { flag_PGIVENMIN = -1,
	       flag_PGIVENOFF = flag_OFF,
	       flag_PGIVENON  = flag_ON,
	       flag_PGIVENMAX
} FLAG_PGIVENTYPE;

/* State definitions for flag_PKEPT */
typedef enum { flag_PKEPTMIN = -1,
	       flag_PKEPTOFF = flag_OFF,
	       flag_PKEPTON  = flag_ON,
	       flag_PKEPTMAX
} FLAG_PKEPTTYPE;

/* State definitions for flag_PLABELS */
typedef enum { flag_PLABELSMIN = -1,
	       flag_PLABELSOFF = flag_OFF,
	       flag_PLABELSON  = flag_ON,
	       flag_PLABELSMAX
} FLAG_PLABELSTYPE;

/* State definitions for flag_POBV */
typedef enum { flag_POBVMIN = -1,
	       flag_POBVOFF = flag_OFF,
	       flag_POBVON  = flag_ON,
	       flag_POBVMAX
} FLAG_POBVTYPE;

/* State definitions for flag_POPTSKOLEM */
typedef enum { flag_POPTSKOLEMMIN = -1,
               flag_POPTSKOLEMOFF = flag_OFF,
               flag_POPTSKOLEMON  = flag_ON,
               flag_POPTSKOLEMMAX
} FLAG_POPTSKOLEMTYPE;

/* State definitions for flag_PPROBLEM */
typedef enum { flag_PPROBLEMMIN = -1,
	       flag_PPROBLEMOFF = flag_OFF,
	       flag_PPROBLEMON  = flag_ON,
	       flag_PPROBLEMMAX
} FLAG_PPROBLEMTYPE;

/* State definitions for flag_PREFCON */
typedef enum { flag_PREFCONMIN = 0,
	       flag_PREFCONUNCHANGED,
	       flag_PREFCONMAX = INT_MAX
} FLAG_PREFCONTYPE;

/* State definitions for flag_PREFVAR */
typedef enum { flag_PREFVARMIN = -1,
               flag_PREFVAROFF = flag_OFF,
               flag_PREFVARON  = flag_ON,
               flag_PREFVARMAX
} FLAG_PREFVARTYPE;

/* State definitions for flag_PREW */
typedef enum { flag_PREWMIN = -1,
	       flag_PREWOFF = flag_OFF,
	       flag_PREWON  = flag_ON,
	       flag_PREWDET = 2,
	       flag_PREWMAX
} FLAG_PREWTYPE;


/* State definitions for flag_PAED */
typedef enum { flag_PAEDMIN = -1,
	       flag_PAEDOFF = flag_OFF,
	       flag_PAEDON  = flag_ON,
	       flag_PAEDMAX
} FLAG_PAEDTYPE;

/* State definitions for flag_PSSI */
typedef enum { flag_PSSIMIN = -1,
	       flag_PSSIOFF = flag_OFF,
	       flag_PSSION  = flag_ON,
	       flag_PSSIMAX
} FLAG_PSSITYPE;

/* State definitions for flag_PSST */
typedef enum { flag_PSSTMIN = -1,
	       flag_PSSTOFF = flag_OFF,
	       flag_PSSTON  = flag_ON,
	       flag_PSSTMAX
} FLAG_PSSTTYPE;

/* State definitions for flag_PSTATISTIC */
typedef enum { flag_PSTATISTICMIN = -1,
               flag_PSTATISTICOFF = flag_OFF,
               flag_PSTATISTICON  = flag_ON,
               flag_PSTATISTICMAX
} FLAG_PSTATISTICTYPE;

/* State definitions for flag_PSTRSKOLEM */
typedef enum { flag_PSTRSKOLEMMIN = -1,
               flag_PSTRSKOLEMOFF = flag_OFF,
               flag_PSTRSKOLEMON  = flag_ON,
               flag_PSTRSKOLEMMAX
} FLAG_PSTRSKOLEMTYPE;

/* State definitions for flag_PSUB */
typedef enum { flag_PSUBMIN = -1,
	       flag_PSUBOFF = flag_OFF,
	       flag_PSUBON  = flag_ON,
	       flag_PSUBMAX
} FLAG_PSUBTYPE;

/* State definitions for flag_PTAUT */
typedef enum { flag_PTAUTMIN = -1,
	       flag_PTAUTOFF = flag_OFF,
	       flag_PTAUTON  = flag_ON,
	       flag_PTAUTMAX
} FLAG_PTAUTTYPE;

/* State definitions for flag_PUNC */
typedef enum { flag_PUNCMIN = -1,
	       flag_PUNCOFF = flag_OFF,
	       flag_PUNCON  = flag_ON,
	       flag_PUNCMAX
} FLAG_PUNCTYPE;

/* State definitions for flag_RBMRR */
typedef enum { flag_RBMRRMIN = -1,
               flag_RBMRROFF = flag_OFF,
               flag_RBMRRON  = flag_ON,
               flag_RBMRRMAX
} FLAG_RBMRRTYPE;

/* State definitions for flag_RBREW */
typedef enum { flag_RBREWMIN = -1,
               flag_RBREWOFF = flag_OFF,
               flag_RBREWON  = flag_ON,
               flag_RBREWCRW = 2,       /* activate Back Contextual Rewriting */
               flag_RBREWRCRW = 3,      /* activate Back Subterm Contextual Rewriting */ 
               flag_RBREWRNEGCRW = 4,   /* also Back Subterm Contextual Literal Elimination  */
               flag_RBREWMAX
} FLAG_RBREWTYPE;


/* State definitions for flag_RBSUB */
typedef enum { flag_RBSUBMIN = -1,
               flag_RBSUBOFF = flag_OFF,
               flag_RBSUBON  = flag_ON,
               flag_RBSUBMAX
} FLAG_RBSUBTYPE;

/* State definitions for flag_RCON */
typedef enum { flag_RCONMIN = -1,
               flag_RCONOFF = flag_OFF,
               flag_RCONON  = flag_ON,
               flag_RCONMAX
} FLAG_RCONTYPE;

/* State definitions for flag_RFMRR */
typedef enum { flag_RFMRRMIN = -1,
               flag_RFMRROFF = flag_OFF,
               flag_RFMRRON  = flag_ON,
               flag_RFMRRMAX
} FLAG_RFMRRTYPE;

/* State definitions for flag_RFREW */
typedef enum { flag_RFREWMIN = -1,
               flag_RFREWOFF = flag_OFF,
               flag_RFREWON  = flag_ON,
               flag_RFREWCRW     = 2,     /* activate Contextual Rewriting */
               flag_RFREWRCRW    = 3,     /* activate Subterm Contextual Rewriting */     
               flag_RFREWRNEGCRW = 4,     /* also Subterm Contextual Literal Elimination  */
               flag_RFREWMAX
} FLAG_RFREWTYPE;


/* State definitions for flag_RFSUB */
typedef enum { flag_RFSUBMIN = -1,
               flag_RFSUBOFF = flag_OFF,
               flag_RFSUBON  = flag_ON,
               flag_RFSUBMAX
} FLAG_RFSUBTYPE;

/* State definitions for flag_RINPUT */
typedef enum { flag_RINPUTMIN = -1,
               flag_RINPUTOFF = flag_OFF,
               flag_RINPUTON  = flag_ON,
               flag_RINPUTMAX
} FLAG_RINPUTTYPE;

/* State definitions for flag_ROBV */
typedef enum { flag_ROBVMIN = -1,
               flag_ROBVOFF = flag_OFF,
               flag_ROBVON  = flag_ON,
               flag_ROBVMAX
} FLAG_ROBVTYPE;

/* State definitions for flag_RAED */
typedef enum { flag_RAEDMIN = -1,
	       flag_RAEDOFF = flag_OFF,
	       flag_RAEDSOUND,
	       flag_RAEDPOTUNSOUND,
	       flag_RAEDMAX
} FLAG_RAEDTYPE;

/* State definitions for flag_RSSI */
typedef enum { flag_RSSIMIN = -1,
               flag_RSSIOFF = flag_OFF,
               flag_RSSION  = flag_ON,
               flag_RSSIMAX
} FLAG_RSSITYPE;

/* State definitions for flag_RSST */
typedef enum { flag_RSSTMIN = -1,
               flag_RSSTOFF = flag_OFF,
               flag_RSSTON  = flag_ON,
               flag_RSSTMAX
} FLAG_RSSTTYPE;

/* State definitions for flag_RTAUT */
typedef enum { flag_RTAUTMIN = -1,
	       flag_RTAUTOFF = flag_OFF,
	       flag_RTAUTSYNTACTIC,
	       flag_RTAUTSEMANTIC,
	       flag_RTAUTMAX
} FLAG_RTAUTTYPE;

/* State definitions for flag_RTER */
typedef enum { flag_RTERMIN = -1,
	       flag_RTEROFF = flag_OFF,
	       flag_RTERMAX = INT_MAX
} FLAG_RTERTYPE;

/* State definitions for flag_RUNC */
typedef enum { flag_RUNCMIN = -1,
               flag_RUNCOFF = flag_OFF,
               flag_RUNCON  = flag_ON,
               flag_RUNCMAX
} FLAG_RUNCTYPE;

/* State definitions for flag_SATINPUT */
typedef enum { flag_SATINPUTMIN = -1,
               flag_SATINPUTOFF = flag_OFF,
               flag_SATINPUTON  = flag_ON,
               flag_SATINPUTMAX
} FLAG_SATINPUTTYPE;

/* State definitions for flag_SELECT */
typedef enum { flag_SELECTMIN = -1,
	       flag_SELECTOFF = flag_OFF,
	       flag_SELECTIFSEVERALMAXIMAL,
	       flag_SELECTALWAYS,
	       flag_SELECTFROMLIST,
	       flag_SELECTMAX
} FLAG_SELECTTYPE;

/* State definitions for flag_SORTS */
typedef enum { flag_SORTSMIN = -1,
	       flag_SORTSOFF = flag_OFF,
	       flag_SORTSMONADICWITHVARIABLE,
	       flag_SORTSMONADICALL,
	       flag_SORTSMAX
} FLAG_SORTSTYPE;

/* State definitions for flag_CNFQUANTEXCH */
typedef enum { flag_CNFQUANTEXCHMIN = -1,
	       flag_CNFQUANTEXCHOFF = flag_OFF,
	       flag_CNFQUANTEXCHON  = flag_ON,
	       flag_CNFQUANTEXCHMAX 
} FLAG_CNFQUANTEXCHTYPE;

/* State definitions for flag_SOS */
typedef enum { flag_SOSMIN = -1,
	       flag_SOSOFF = flag_OFF,
	       flag_SOSON  = flag_ON,
	       flag_SOSMAX
} FLAG_SOSTYPE;

/* State definitions for flag_SPLITS */
typedef enum { flag_SPLITSMIN = -2,
	       flag_SPLITSUNLIMITED,
	       flag_SPLITSOFF = flag_OFF,
	       flag_SPLITSMAX = INT_MAX
} FLAG_SPLITSTYPE;

/* State definitions for flag_SPLITHEURISTIC */
/* typedef enum { flag_SPLITHEURISTICMIN = -1, */
/* 	       flag_SPLITHEURISTICSTANDARD, */
/* 	       flag_SPLITHEURISTICGROUND, */
/* 	       flag_SPLITHEURISTICALWAYS, */
/* 	       flag_SPLITHEURISTICMAX */
/* } FLAG_SPLITHEURISTICTYPE; */

/* State definitions for flag_SPLITMININST */
typedef enum { flag_SPLITMININSTMIN = 1,
	       flag_SPLITMININSTDEFAULT,
	       flag_SPLITMININSTMAX = INT_MAX
} FLAG_SPLITMININSTTYPE;

/* State definitions for flag_STDIN */
typedef enum { flag_STDINMIN = -1,
	       flag_STDINOFF = flag_OFF,
	       flag_STDINON  = flag_ON,
	       flag_STDINMAX
} FLAG_STDINTYPE;

/* State definitions for flag_TDFG2OTTEROPTIONS */
typedef enum { flag_TDFG2OTTEROPTIONSMIN = -1,
	       flag_TDFG2OTTEROPTIONSOFF = flag_OFF,
	       flag_TDFG2OTTEROPTIONSPROOFCHECK,
	       flag_TDFG2OTTEROPTIONSAUTO,
	       flag_TDFG2OTTEROPTIONSAUTO2,
	       flag_TDFG2OTTEROPTIONSMAX
} FLAG_TDFG2OTTEROPTIONSTYPE;

/* State definitions for flag_TIMELIMIT */
typedef enum { flag_TIMELIMITMIN = -2,
	       flag_TIMELIMITUNLIMITED,
	       flag_TIMELIMITMAX = INT_MAX
} FLAG_TIMELIMITTYPE;

/* State definitions for flag_VARWEIGHT */
typedef enum { flag_VARWEIGHTMIN = 0,
	       flag_VARWEIGHTMAX = INT_MAX
} FLAG_VARWEIGHTTYPE;

/* State definitions for flag_WDRATIO */
typedef enum { flag_WDRATIOMIN = 0,
	       flag_WDRATIOMAX = INT_MAX
} FLAG_WDRATIOTYPE;

/* State definitions for flag_EML */
typedef enum { flag_EMLMIN = -1,
	       flag_EMLOFF = flag_OFF,
	       flag_EMLON  = flag_ON,
	       flag_EMLMAX 
} FLAG_EMLTYPE;

/* State definitions for flag_EMLAUTO */
typedef enum { flag_EMLAUTOMIN = -1,
	       flag_EMLAUTOOFF = flag_OFF,
	       flag_EMLAUTOON  = flag_ON,
	       flag_EMLAUTOMAX 
} FLAG_EMLAUTOTYPE;

/* State definitions for flag_EML2REL */
typedef enum { flag_EML2RELMIN = -1,
	       flag_EML2RELOFF = flag_OFF,
	       flag_EML2RELON  = flag_ON,
	       flag_EML2RELMAX 
} FLAG_EML2RELTYPE;

/* State definitions for flag_EMLTR */
typedef enum { flag_EMLTRMIN = -1,
	       flag_EMLTRREL,
	       flag_EMLTRFUNC,
	       flag_EMLTROPTFUNC,
	       flag_EMLTRSEMIFUNC,
	       flag_EMLTRMAX 
} FLAG_EMLTRTYPE;

/* State definitions for flag_EMLTHEORY */
typedef enum { flag_EMLTHEORYMIN = -1,
	       flag_EMLTHEORYK,
	       flag_EMLTHEORYKD,
	       flag_EMLTHEORYKT, 
	       flag_EMLTHEORYKB,
	       flag_EMLTHEORYK4,
	       flag_EMLTHEORYK5,
	       flag_EMLTHEORYS4,
	       flag_EMLTHEORYS5,
	       flag_EMLTHEORYMAX 
} FLAG_EMLTHEORYTYPE;

/* State definitions for flag_EMLFUNCNDEQ */
typedef enum { flag_EMLFUNCNDEQMIN = -1,
	       flag_EMLFUNCNDEQOFF = flag_OFF,
	       flag_EMLFUNCNDEQON  = flag_ON,
	       flag_EMLFUNCNDEQMAX 
} FLAG_EMLFUNCNDEQTYPE;

/* State definitions for flag_EMLFNARY */
typedef enum { flag_EMLFNARYMIN = -1,
	       flag_EMLFNARYOFF = flag_OFF,
	       flag_EMLFNARYON  = flag_ON,
	       flag_EMLFNARYMAX 
} FLAG_EMLFNARYTYPE;

/* State definitions for flag_EMLFFSORTS */
typedef enum { flag_EMLFFSORTSMIN = -1,
	       flag_EMLFFSORTSOFF = flag_OFF,
	       flag_EMLFFSORTSON  = flag_ON,
	       flag_EMLFFSORTSMAX 
} FLAG_EMLFFSORTSTYPE;

/* State definitions for flag_EMLELIMCOMP */
typedef enum { flag_EMLELIMCOMPMIN = -1,
	       flag_EMLELIMCOMPOFF = flag_OFF,
	       flag_EMLELIMCOMPON  = flag_ON,
	       flag_EMLELIMCOMPMAX 
} FLAG_EMLELIMCOMPTYPE;

/* State definitions for flag_EMLPTRANS */
typedef enum { flag_EMLPTRANSMIN = -1,
	       flag_EMLPTRANSOFF = flag_OFF,
	       flag_EMLPTRANSON  = flag_ON,
	       flag_EMLPTRANSMAX 
} FLAG_EMLPTRANSTYPE;


/* State definitions for flag_GLOBALRIF */
typedef enum { flag_GLOBALRIFMIN = -1,
               flag_GLOBALRIFOFF = flag_OFF,
               flag_GLOBALRIFON  = flag_ON,
               flag_GLOBALRIFMAX
} flag_GLOBALRIFTYPE;

/* State definitions for flag_TPTP */
typedef enum { flag_TPTPMIN = -1,
               flag_TPTPOFF = flag_OFF,
               flag_TPTPON  = flag_ON,
               flag_TPTPMAX
} flag_TPTPTYPE;


/* State definitions for flag_INCLUDE */
typedef enum { flag_INCLUDEMIN = -1,
               flag_INCLUDEOFF = flag_OFF,
               flag_INCLUDEON  = flag_ON,
               flag_INCLUDEMAX
} flag_INCLUDETYPE;

/* State definitions for flag_PGENT */
typedef enum { flag_PGENTMIN = -2,
               flag_PGENTON = 3,
               flag_PGENTMAX = INT_MAX
} flag_PGENTTYPE;

/* State definitions for flag_PGEND */
typedef enum { flag_PGENDMIN = -1,
               flag_PGENDOFF = flag_OFF,
               flag_PGENDON  = flag_ON,
               flag_PGENDMAX
} flag_PGENDTYPE;

/* State definitions for flag_PGENS */
typedef enum { flag_PGENSMIN = -1,
               flag_PGENSOFF = flag_OFF,
               flag_PGENSON  = flag_ON,
               flag_PGENSMAX
} flag_PGENSTYPE;

/* State definitions for flag_PGENQ */
typedef enum { flag_PGENQMIN = -1,
               flag_PGENQOFF = flag_OFF,
               flag_PGENQON  = flag_ON,
               flag_PGENQMAX
} flag_PGENQTYPE;

/* State definitions for flag_PGENJ */
typedef enum { flag_PGENJMIN = -1,
               flag_PGENJOFF = flag_OFF,
               flag_PGENJON  = flag_ON,
               flag_PGENJMAX
} flag_PGENJTYPE;

/* State definitions for flag_PGENN */
typedef enum { flag_PGENNMIN = -1,
               flag_PGENNOFF = flag_OFF,
               flag_PGENNON  = flag_ON,
               flag_PGENNMAX
} flag_PGENNTYPE;

/* State definitions for flag_PGENC */
typedef enum { flag_PGENCMIN = -1,
               flag_PGENCOFF = flag_OFF,
               flag_PGENCON  = flag_ON,
               flag_PGENCMAX
} flag_PGENCTYPE;

/* State definitions for flag_PGENR */
typedef enum { flag_PGENRMIN = -1,
               flag_PGENROFF = flag_OFF,
               flag_PGENRON  = flag_ON,
               flag_PGENRMAX
} flag_PGENRTYPE;

/* State definitions for flag_PGENXVCG */
typedef enum { flag_PGENXVCGMIN = -1,
               flag_PGENXVCGOFF = flag_OFF,
               flag_PGENXVCGON  = flag_ON,
               flag_PGENXVCGMAX
} flag_PGENXVCGTYPE;

/* State definitions for flag_PGENVINCI */
typedef enum { flag_PGENVINCIMIN = -1,
               flag_PGENVINCIOFF = flag_OFF,
               flag_PGENVINCION  = flag_ON,
               flag_PGENVINCIMAX
} flag_PGENVINCITYPE;

/* State definitions for flag_RESCMPV */
typedef enum { flag_RESCMPVMIN = -1,
               flag_RESCMPVOFF = flag_OFF,
               flag_RESCMPVON  = flag_ON,
               flag_RESCMPVMAX
} flag_RESCMPVTYPE;

/* State definitions for flag_DFG2DFGSHALLOW */
typedef enum { flag_DFG2DFGSHALLOWMIN = -1,
               flag_DFG2DFGSHALLOWOFF = flag_OFF,
               flag_DFG2DFGSHALLOW1,
	       flag_DFG2DFGSHALLOW2,
	       flag_DFG2DFGSHALLOW3,
               flag_DFG2DFGSHALLOWMAX
} flag_DFG2DFGSHALLOWTYPE;

/* State definitions for flag_DFG2DFGMONADIC */
typedef enum { flag_DFG2DFGMONADICMIN = -1,
               flag_DFG2DFGMONADICOFF = flag_OFF,
	       flag_DFG2DFGMONADICTERM,
               flag_DFG2DFGMONADICPROJECT,
               flag_DFG2DFGMONADICMAX
} flag_DFG2DFGMONADICTYPE;

/* State definitions for flag_DFG2DFGLINEAR */
typedef enum { flag_DFG2DFGLINEARMIN = -1,
               flag_DFG2DFGLINEAROFF = flag_OFF,
               flag_DFG2DFGLINEARON  = flag_ON,
               flag_DFG2DFGLINEARMAX
} flag_DFG2DFGLINEARTYPE;

/* State definitions for flag_DFG2DFGHORN */
typedef enum { flag_DFG2DFGHORNMIN = -1,
               flag_DFG2DFGHORNOFF = flag_OFF,
               flag_DFG2DFGHORNON  = flag_ON,
               flag_DFG2DFGHORNMAX
} flag_DFG2DFGHORNTYPE;

/* Define all flags */

typedef enum { flag_AUTO, flag_STDIN, flag_INTERACTIVE, flag_FLOTTER,
	       flag_SOS,

               flag_SPLITS,     flag_SPLITMININST,
	       flag_MEMORY,     flag_TIMELIMIT,
	       flag_DOCSST,     flag_DOCPROOF,
	       flag_DOCSPLIT,   flag_LOOPS,      flag_PSUB,
	       flag_PREW,       flag_PCON,
	       flag_PTAUT,      flag_POBV,       flag_PSSI,
	       flag_PSST,       flag_PMRR,       flag_PUNC,
	       flag_PAED,

	       flag_PDER,       flag_PGIVEN,     flag_PLABELS,
	       flag_PKEPT,      flag_PPROBLEM,   flag_PEMPTYCLAUSE,
	       flag_PSTATISTIC, flag_FPMODEL,    flag_FPDFGPROOF,
	       flag_PFLAGS,     flag_POPTSKOLEM, flag_PSTRSKOLEM,
	       flag_PBDC,       flag_PBINC,
	       flag_PAPPLYDEFS,

	       flag_SELECT,     flag_RINPUT,     flag_SORTS,
	       flag_SATINPUT,   flag_WDRATIO,    flag_PREFCON,
	       flag_FULLRED,
	       flag_FUNCWEIGHT, flag_VARWEIGHT,  flag_PREFVAR,
	       flag_BOUNDMODE,  flag_BOUNDSTART, 
	       flag_BOUNDLOOPS, flag_APPLYDEFS,

	       flag_ORD,

	       flag_CNFQUANTEXCH,	
	       flag_CNFOPTSKOLEM, flag_CNFSTRSKOLEM, flag_CNFPROOFSTEPS,
	       flag_CNFSUB,       flag_CNFCON,       flag_CNFREDTIMELIMIT,
	       flag_CNFRENAMING,  flag_CNFRENMATCH,  flag_CNFPRENAMING, 
	       flag_CNFFEQREDUCTIONS,
	       
	       
	       flag_IEMS,       flag_ISOR,
	       flag_IEQR,       flag_IERR,
	       flag_IEQF,       flag_IMPM,       flag_ISPR,
	       flag_IOPM,       flag_ISPM,
	       flag_ISPL,       flag_IORE,       flag_ISRE,
	       flag_ISHY,       flag_IOHY,       flag_IURR,
	       flag_IOFC,       flag_ISFC,
	       flag_IUNR,       flag_IBUR,       flag_IDEF,

	       flag_RFREW,      flag_RBREW,
	       flag_RFMRR,      flag_RBMRR,
	       flag_ROBV,       flag_RUNC,       flag_RTER,
	       flag_RTAUT,      flag_RSST,       flag_RSSI,
	       flag_RFSUB,      flag_RBSUB,      flag_RAED,
	       flag_RCON,       
	       
	       flag_TDFG2OTTEROPTIONS,

	       flag_EML,	flag_EMLAUTO,    flag_EMLTR,
	       flag_EML2REL,    flag_EMLTHEORY,  flag_EMLFUNCNDEQ,
               flag_EMLFNARY,   flag_EMLFFSORTS, flag_EMLELIMCOMP,
	       flag_EMLPTRANS,

	       flag_TPTP,       flag_GLOBALRIF,  

	       flag_INCLUDE,                                     /* tptp2dfg flag */

	       flag_PGENT, flag_PGEND, flag_PGENS, flag_PGENQ,
	       flag_PGENJ, flag_PGENN, flag_PGENR, flag_PGENC,  
	       flag_PGENXVCG,  flag_PGENVINCI,                    /*  pgen flags */

	       flag_RESCMPV,                                      /*  rescmp flags */

	       flag_DFG2DFGHORN, flag_DFG2DFGLINEAR, flag_DFG2DFGMONADIC, flag_DFG2DFGSHALLOW,  /*  dfg2dfg flags */

	       flag_MAXFLAG } FLAG_ID;     /* flag_MAXFLAG is a final Dummy */


/* Define different flag types */
typedef enum { flag_INFERENCE,
	       flag_PRINTING,
	       flag_REDUCTION,
	       flag_TRANSFORM,   /* CNF transformation, quantifier exchange, etc */
	       flag_EMLSPECIAL,
	       flag_UNIQUE,      /* miscellaneous flags */
	       flag_MAXTYPE
} FLAG_TYPE;

/* Define different flag value types */
typedef enum { flag_INTEGER,
	       flag_STRING,
	       flag_VALUEMAXTYPE
} FLAG_VALUETYPE;


/* Define the flag data type */
typedef struct flag {int intvalue; char* stringvalue;} FLAG;

/* Define the internal representation of a flag store */
typedef FLAG FLAGARRAY[flag_MAXFLAG];

/* Define the flag store */
typedef FLAG *FLAGSTORE;

/**************************************************************/
/* Functions                                                  */
/**************************************************************/

void              flag_Init(FLAG_USERS);
void              flag_InitFlotterFlags(FLAGSTORE, FLAGSTORE);
void              flag_InitFlotterSubproofFlags(FLAGSTORE, FLAGSTORE);
FLAGSTORE         flag_DefaultStore(void);
void              flag_Print(FLAGSTORE);
void              flag_FPrint(FILE*, FLAGSTORE);
void              flag_FPrintFlag(FILE*, FLAGSTORE, FLAG_ID);
void              flag_FPrintFlagWithValue(FILE*, FLAGSTORE, FLAG_ID, int, char *);
BOOL              flag_Lookup(const char*);
FLAG_ID           flag_Id(const char*);
const char*       flag_Name(FLAG_ID);
int               flag_Minimum(FLAG_ID);
int               flag_Maximum(FLAG_ID);
int               flag_GetFlagIntValue(FLAGSTORE, FLAG_ID);
char *            flag_GetFlagStringValue(FLAGSTORE, FLAG_ID);
void              flag_SetFlagIntValue(FLAGSTORE, FLAG_ID, int);
void              flag_SetFlagStringValue(FLAGSTORE, FLAG_ID, char *);
FLAG_TYPE         flag_Type(FLAG_ID);
BOOL              flag_IsOfType(FLAG_ID, FLAG_TYPE);
FLAG_VALUETYPE    flag_ValueType(FLAG_ID);
BOOL              flag_IsOfValueType(FLAG_ID, FLAG_VALUETYPE);
void              flag_ClearInferenceRules(FLAGSTORE);
void              flag_ClearReductionRules(FLAGSTORE);
void              flag_ClearPrinting(FLAGSTORE);
void              flag_SetReductionsToDefaults(FLAGSTORE);
void              flag_CheckStore(FLAGSTORE);
void              flag_DeleteStore(FLAGSTORE);
void              flag_PrintEmlTranslationMethod(int);
void              flag_CheckEmlTranslationFlags(FLAGSTORE, int);
unsigned int      flag_GetMaxFlag();     

/**************************************************************/
/* Macros                                                     */
/**************************************************************/

static __inline__ void flag_CheckFlagIdInRange(FLAG_ID FlagId)
  /* prints an error report if a FLAG_ID is not valid */
{
#ifdef CHECK
  if (FlagId >= flag_MAXFLAG) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In flag_CheckFlagIdInRange: Range of flags exceeded.");
    misc_FinishErrorReport();
  }
#endif
}

static __inline__ void flag_CheckFlagValueInRange(FLAG_ID FlagId, int Value)
  /* prints an error report if a flag's value is out of range */
{
  flag_CheckFlagIdInRange(FlagId);
 
  if (Value <= flag_Minimum(FlagId)) {
     misc_StartUserErrorReport();
     misc_UserErrorReport("\n Error: Flag value %d is too small for flag %s.\n", Value, flag_Name(FlagId));
     misc_FinishUserErrorReport();
  }
  else
    if (Value >= flag_Maximum(FlagId)) {
      misc_StartUserErrorReport();
      misc_UserErrorReport("\n Error: Flag value %d is too large for flag %s.\n", Value, flag_Name(FlagId));
      misc_FinishUserErrorReport();
    }
}

static __inline__ void flag_CheckFlagTypeInRange(FLAG_TYPE Type)
  /* prints an error report if a flag's type is out of range */
{
#ifdef CHECK
  if (Type >= flag_MAXTYPE) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In flag_CheckFlagTypeInRange: Range of types exceeded.");
    misc_FinishErrorReport();
  }
#endif
}

static __inline__ void flag_CheckFlagValueTypeInRange(FLAG_VALUETYPE VType)
  /* prints an error report if a flag's type is out of range */
{
#ifdef CHECK
  if (VType >= flag_VALUEMAXTYPE) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In flag_CheckFlagValueTypeInRange: Range of value types exceeded.");
    misc_FinishErrorReport();
  }
#endif
}

static __inline__ BOOL flag_StoreIsDefaultStore(FLAGSTORE Store)
     /* returns TRUE if a flag store is the default store, FALSE otherwise */
{
  return (BOOL) (Store == flag_DefaultStore());
}

static __inline__ BOOL flag_ValueIsClean(FLAGSTORE Store, FLAG_ID FlagId)
{
#ifdef CHECK
  flag_CheckFlagIdInRange(FlagId);
  return (BOOL) ((Store[FlagId]).intvalue == flag_CLEAN);
#else
  return (BOOL) (flag_GetFlagIntValue(Store, FlagId) == flag_CLEAN);
#endif
}

static __inline__ BOOL flag_IsUndefined(FLAG_ID FlagId)
{
#ifdef CHECK
  flag_CheckFlagIdInRange(FlagId);
#endif
  return (BOOL) ((flag_DefaultStore())[FlagId].intvalue == flag_UNDEFINED);
}

static __inline__ void flag_CleanStore(FLAGSTORE Store)
{
  int i;
  for (i = 0; i < flag_MAXFLAG; i++) {
    Store[i].intvalue    = flag_CLEAN;
    Store[i].stringvalue = (char *)NULL;
  }
}


static __inline__ FLAGSTORE flag_CreateStore(void)
     /* creates a fresh, clean FLAGSTORE */
{
  FLAGSTORE store;

  store = (FLAGSTORE)memory_Malloc(sizeof(FLAGARRAY));
  flag_CleanStore(store);
  
  return store;
}



static __inline__ void flag_InitStoreByDefaults(FLAGSTORE Store)
{
  FLAG_ID i;
  for (i = (FLAG_ID) 0; i < flag_MAXFLAG; i++) {
    if (!flag_IsUndefined(i))
      flag_SetFlagIntValue(Store, i, flag_GetFlagIntValue(flag_DefaultStore(),i));  
  }
}


static __inline__ void flag_SetFlagToDefault(FLAGSTORE Store, FLAG_ID Flag)
{
  flag_SetFlagIntValue(Store, Flag, flag_GetFlagIntValue(flag_DefaultStore(), Flag));
}


static __inline__ void flag_TransferFlag(FLAGSTORE Source, FLAGSTORE Destination, FLAG_ID FlagId)
{
  flag_SetFlagIntValue(Destination, FlagId, flag_GetFlagIntValue(Source, FlagId));
}


static __inline__ void flag_TransferAllFlags(FLAGSTORE Source, FLAGSTORE Destination)
{
  FLAG_ID i;
  for (i = (FLAG_ID) 0; i < flag_MAXFLAG; i++)
    Destination[i].intvalue = Source[i].intvalue;
}


static __inline__ void flag_TransferSetFlags(FLAGSTORE Source, FLAGSTORE Destination)
{
  FLAG_ID i;
  for (i = (FLAG_ID) 0; i < flag_MAXFLAG; i++)
    if (!flag_ValueIsClean(Source,i) && !flag_IsUndefined(i))
      flag_TransferFlag(Source, Destination, i);
}

static __inline__ BOOL flag_IsInference(FLAG_ID Flag) 
/**************************************************************
  INPUT:   A FlagId.
  RETURNS: TRUE is the flag is an inference flag,
           FALSE otherwise.
***************************************************************/
{
  flag_CheckFlagIdInRange(Flag);

  return flag_IsOfType(Flag, flag_INFERENCE);
}


static __inline__ BOOL flag_IsReduction(FLAG_ID Flag) 
/**************************************************************
  INPUT:   A FlagId.
  RETURNS: TRUE is the flag is a reduction flag,
           FALSE otherwise.
***************************************************************/
{
  flag_CheckFlagIdInRange(Flag);

  return flag_IsOfType(Flag, flag_REDUCTION);
}


static __inline__ BOOL flag_IsPrinting(FLAG_ID Flag) 
/**************************************************************
  INPUT:   A FlagId.
  RETURNS: TRUE is the flag is a printing flag,
           FALSE otherwise.
***************************************************************/
{
  flag_CheckFlagIdInRange(Flag);

  return flag_IsOfType(Flag, flag_PRINTING);
}


static __inline__ BOOL flag_IsUnique(FLAG_ID Flag) 
/**************************************************************
  INPUT:   A FlagId.
  RETURNS: TRUE is the flag is an unique flag,
           FALSE otherwise.
***************************************************************/
{
  flag_CheckFlagIdInRange(Flag);

  return flag_IsOfType(Flag, flag_UNIQUE);
}

static __inline__ BOOL flag_IsTransforming(FLAG_ID Flag) 
/**************************************************************
  INPUT:   A FlagId.
  RETURNS: TRUE is the flag is an unique flag,
           FALSE otherwise.
***************************************************************/
{
  flag_CheckFlagIdInRange(Flag);

  return flag_IsOfType(Flag, flag_TRANSFORM);
}

static __inline__ BOOL flag_IsEmlSpecial(FLAG_ID Flag) 
/**************************************************************
  INPUT:   A FlagId.
  RETURNS: TRUE is the flag is a reduction flag,
           FALSE otherwise.
***************************************************************/
{
  flag_CheckFlagIdInRange(Flag);

  return flag_IsOfType(Flag, flag_EMLSPECIAL);
}

static __inline__ void flag_PrintReductionRules(FLAGSTORE Store)
/**************************************************************
  INPUT:   A FlagStore.
  RETURNS: Nothing.
  EFFECT:  Prints the values of all reduction flags to stdout.
***************************************************************/
{
  FLAG_ID i;
  fputs("\n Reductions: ", stdout);

  for (i = (FLAG_ID) 0; i < flag_MAXFLAG; i++) {
    if (!flag_IsUndefined(i) && flag_IsReduction(i) && flag_GetFlagIntValue(Store, i))
      printf("%s=%d ",flag_Name(i), flag_GetFlagIntValue(Store, i));
  }
}

static __inline__ void flag_PrintInferenceRules(FLAGSTORE Store)
/**************************************************************
  INPUT:   A FlagStore.
  RETURNS: Nothing.
  EFFECT:  Prints the values of all inference flags to stdout.
***************************************************************/
{
  FLAG_ID i;
  fputs("\n Inferences: ", stdout);

  for (i = (FLAG_ID) 0; i < flag_MAXFLAG; i++) {
    if (!flag_IsUndefined(i) && flag_IsInference(i) && flag_GetFlagIntValue(Store, i))
      printf("%s=%d ",flag_Name(i), flag_GetFlagIntValue(Store,i));
  }
}

static __inline__ void flag_PrintTransformationFlags(FLAGSTORE Store)
/**************************************************************
  INPUT:   A FlagStore.
  RETURNS: Nothing.
  EFFECT:  Prints the values of all inference flags to stdout.
***************************************************************/
{
  FLAG_ID i;
  fputs("\n Transform : ", stdout);

  for (i = (FLAG_ID) 0; i < flag_MAXFLAG; i++) {
    if (!flag_IsUndefined(i) && flag_IsTransforming(i) && flag_GetFlagIntValue(Store, i))
      printf("%s=%d ",flag_Name(i), flag_GetFlagIntValue(Store,i));
  }
} 

static __inline__ void flag_PrintEmlTranslationFlags(FLAGSTORE Store)
/**************************************************************
  INPUT:   A FlagStore.
  RETURNS: Nothing.
  EFFECT:  Prints the values of all inference flags to stdout.
***************************************************************/
{
  FLAG_ID i;
  fputs("\n EML flags : ", stdout);

  for (i = (FLAG_ID) 0; i < flag_MAXFLAG; i++) {
    if (!flag_IsUndefined(i) && flag_IsEmlSpecial(i) && flag_GetFlagIntValue(Store, i))
      printf("%s=%d ",flag_Name(i), flag_GetFlagIntValue(Store,i));
  }
} 

#endif






