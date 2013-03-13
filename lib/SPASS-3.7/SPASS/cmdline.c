 /**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                COMMAND LINE PARSER                     * */
/* *                                                        * */
/* *  $Module:   CMDLNE                                     * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1997, 1998, 1999, 2000            * */
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
/* $Revision: 1.5 $                                         * */
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
/* $RCSfile: cmdline.c,v $ */


/***************************************************************/



#include "cmdline.h"



/**************************************************************/
/* Local variables  and types                                 */
/**************************************************************/


const char* cmdlne_InputFile;
const char* cmdlne_OutputFile;

static LIST cmdlne_ArgumentsList;

/**************************************************************/
/* Functions                                                  */
/**************************************************************/


void cmdlne_Init()
/**************************************************************
  INPUT:   NONE
  RETURNS: Nothing.
  EFFECT:  Initializes command line module 
 ***************************************************************/
{
     cmdlne_InputFile = (char*)NULL;
     cmdlne_OutputFile = (char*)NULL;
     cmdlne_ArgumentsList = NULL;
}

void cmdlne_PrintSPASSNames()
/**************************************************************
  INPUT:   None 
  RETURNS: Nothing
  EFFECT:  Prints all options in three rows
***************************************************************/
{
  int i,j;
  
  for (i=0; i < flag_GetMaxFlag(); i=i+4) { 
    for (j =0; j <=3; j++) 
      if (i+j < flag_GetMaxFlag() && !flag_IsUndefined(i+j)) {
	printf("%-18s ", flag_Name(i+j)); 
      } 
    putchar('\n');
  }
}

void cmdlne_FreePair(LIST Pair)
/***************************************************************
  INPUT:   a Pair
  RETURNS: Nothing.
  EFFECT:  Free the Pair and its elements.
********************* ******************************************/

{
  string_StringFree(list_PairFirst(Pair));
  string_StringFree(list_PairSecond(Pair));

  list_PairFree(Pair);

}

void cmdlne_Free()
/****************************************************************
  INPUT:   NONE
  RETURNS: Nothing.
  EFFECT:  Free memory of module
  ***************************************************************/
{
  list_DeleteWithElement(cmdlne_ArgumentsList, (void (*) (POINTER)) cmdlne_FreePair);	 
}

const char* cmdlne_GetInputFile()
/****************************************************************
  INPUT:   None
  RETURNS: Pointer to the name of input file
 ****************************************************************/
{
      return cmdlne_InputFile;
}

const char* cmdlne_GetOutputFile()
/****************************************************************
  INPUT:   None
  RETURNS: Pointer to the name of output file
*****************************************************************/
{

      return cmdlne_OutputFile;
}


BOOL cmdlne_SetFlags(FLAGSTORE flagstore)
/****************************************************************
  INPUT:   a FLAGSTORE
  RETURNS: TRUE if all arguments of <cmdlne_ArgumentsList> are 
           valid arguments and the respective flags of <flagstore>
	   could be set and FALSE otherwise.
  EFFECT:  Set flags of <flagstore> according to arguments stored in
  	   <cmdlne_ArgumentsList>
*****************************************************************/
{
	int id, tk;
	LIST Scan;	
	BOOL found;


	for(Scan=cmdlne_ArgumentsList; !list_Empty(Scan); Scan = list_Cdr(Scan)) 
	{
		found = FALSE;
		
		for (id=0; id < flag_GetMaxFlag() && !found; id++)
		{
		  if (!flag_IsUndefined(id) && string_Equal(flag_Name(id), 
					       (char*) list_PairFirst(list_Car(Scan))) )
	   		{
			  if (flag_IsOfValueType(id, flag_INTEGER)) {
			    if(!string_StringIsInteger(list_PairSecond(list_Car(Scan)))){
			        misc_StartUserErrorReport();
			        misc_UserErrorReport("\nError: Argument of option %s must be an integer.\n\n", flag_Name(id));
				misc_FinishUserErrorReport();
				return FALSE;
			    }
				tk = atoi((char*) list_PairSecond(list_Car(Scan)));
				flag_SetFlagIntValue(flagstore, id, tk);
			  }
			  else {
			    flag_SetFlagStringValue(flagstore, id, string_StringCopy((char*)list_PairSecond(list_Car(Scan))));
			  }
			  found = TRUE;
           		}
	   
		}
		
		if(!found) {
		  misc_StartUserErrorReport();
		  misc_UserErrorReport("\n Unrecognized option %s\n\n", (char*) list_PairFirst(list_Car(Scan)));
		  misc_FinishUserErrorReport();
		  return FALSE;
		}
		
	}
	return TRUE;
}

BOOL cmdlne_SetArgument(const char* token, const char* value) 
/**************************************************************
  INPUT:   two pointers
  RETURNS: TRUE if the argument <token> has not been added to the
           list of arguments before and FALSE otherwise.
  EFFECT:  Adds argument <token> with value <value> to list of 
  	   arguments (<cmdlne_ArgumentsList>)
  CAUTION: <value> has to be a character pointer pointing to
  	   a string representing an integer
***************************************************************/
{

	LIST Pair;
	LIST Scan;

	
	/* Check if Argument has already been defined */
	for(Scan=cmdlne_ArgumentsList; !list_Empty(Scan); Scan = list_Cdr(Scan)) {
		if(string_Equal(list_PairFirst(list_Car(Scan)),token)) {
			misc_StartUserErrorReport();
    			misc_UserErrorReport("\n Option %s is multiply defined.\n\n", token);
    			misc_FinishUserErrorReport();
    			return FALSE;
		}
	}
	
	   	
	
	/* Add <token> to argument list with value <value>*/
	Pair = list_PairCreate(string_StringCopy(token), string_StringCopy(value));
	cmdlne_ArgumentsList = list_Cons(Pair, cmdlne_ArgumentsList);
	
	return TRUE;
}


BOOL cmdlne_HandleShortArg(TOKEN token)
/******************************************************************
  INPUT:   FLAGSTORE, a TOKEN
  RETURNS: TRUE if a flag with name <token> exists in <flagstore>.
  	   FALSE otherwise
  EFFECT:  Adds argument <token> with default value to list of arguments
*******************************************************************/
{
   return cmdlne_SetArgument(token, cmdlne_DEFAULTARGVALUE);
}


BOOL cmdlne_HandleArg(TOKEN token1, TOKEN token2)
/*****************************************************************
  INPUT:   FLAGSTORE, two TOKEN's
  RETURNS: TRUE if a flag with name <token1> exists in <flagstore>
	   FALSE otherwise
  EFFECT:  Adds argument with name <token1> and value <token2>
  	   to list of agrguments.
  CAUTION: cmdlne_HandleArg assumes that <token2> is the
	   representation of an integer
******************************************************************/
{
   return cmdlne_SetArgument(token1, token2);
}


BOOL cmdlne_isArgSym(const char c)
/*****************************************************************
  INPUT:   a char constant 
  RETURNS: TRUE if <c> is a character symbol allowed to occur in 
           a string argument of a commandline option 
  ****************************************************************/
{

  if(isalnum(c)|| c == '.' || c == '-' || c == '+'  || c == '/' || c == '\\' || c == '_') {
    return TRUE;
  }
  else
    return FALSE;

}

BOOL cmdlne_LexScan(const char* string)
/*****************************************************************
  INPUT:   FLAGSTORE, a string containing program parameter data 
  RETURNS: TRUE iff <string> contains a valid option and returns 
           FALSE if <string> is not a valid option
  EFFECT:  Scan the cmdlne arguments 
  ****************************************************************/  
{
  int cp,tp1,tp2,tp3;
  char c;
  char token1[cmdlne_MAXTOKENLENGTH];
  char token2[cmdlne_MAXTOKENLENGTH];
  tp1 = tp2 = tp3 = 0;

  if (string[0] == '-')
    {
      cp = 1;
      c = string[cp];
      if (isalpha(c))
	{
	  while(isalpha(c))
	    {
	      token1[tp1] = c;
	      tp1++;
	      cp++;
	      c = string[cp];	
	    }
		
	   if (c == '\0')
	    {
		      token1[tp1] = '\0';
		      if(cmdlne_HandleShortArg(token1))
		      {
			return TRUE;
		      }
		      else
		      {
			misc_StartUserErrorReport();
			misc_UserErrorReport("\nUnrecognized option: -%s\n\n",token1);
			misc_FinishUserErrorReport();
			return FALSE;
		      }
					

	    }
	  else if (c == '=')
	    {
	      /*For long type of arguments*/
			
	      cp++;
	      c = string[cp];
	      token1[tp1] = '\0';
	      
   
	      if (cmdlne_isArgSym(c))
		{
		  token2[tp2]= c;
		  tp2++;
		  cp++;
		  c=string[cp];
		  
		  while (cmdlne_isArgSym(c))
		  {
		      token2[tp2]=c;
		      tp2++;
		      cp++;
		      c = string[cp];
		  }
		  if(c == '\0')
		  {
		        token2[tp2] = '\0';
		        if(cmdlne_HandleArg(token1, token2))
			{
			  return TRUE;
			}
		        else 
		        {
			  misc_StartUserErrorReport();
			  misc_UserErrorReport("\nUnrecognized option: -%s=%s\n\n",token1, token2);
			  misc_FinishUserErrorReport();
			  return FALSE;
		        }
		   }
		   else
		   {
		        misc_StartUserErrorReport();
		        misc_UserErrorReport("\nError: Invalid argument of option %s.\n\n", token1);
			misc_FinishUserErrorReport();
		        return FALSE;
		   }
		    
		  }
	      else 
		{
		  misc_StartUserErrorReport();
		  misc_UserErrorReport("\n Invalid argument of option %s.\n\n", token1);
		  misc_FinishUserErrorReport();
		  return FALSE;
		}
	    }
	  else
	    {
	        while (!c == '\0')
		    {
		      token1[tp1]=c;
		      tp1++;
		      cp++;
		      c = string[cp];
		    }

	       token1[tp1] = '\0';
	       printf("\nUnrecognized option: -%s\n\n",token1);
	       return FALSE;
	    }

	}
      else
	{
            while (!c == '\0')
		 {
		   token1[tp1]=c;
		   tp1++;
		   cp++;
		   c = string[cp];
		 }

	    token1[tp1] = '\0';

	    printf("\nUnrecognized option: -%s\n\n",token1);
	    return FALSE;
	}

 
    }
  else 
    {
	if(cmdlne_InputFile == (char*)NULL) 
	 {
	      cmdlne_InputFile = string;
	      /*printf("\nInput File: %s", cmdlne_InputFile);*/
	      return TRUE;
	 }
        else if(cmdlne_OutputFile == (char*)NULL)
	 {
	      cmdlne_OutputFile = string;
	      /*printf("\nOutput File: %s", cmdlne_OutputFile);*/
	      return TRUE;
	 }
   }

  /* This function should never execute this code */
  misc_StartUserErrorReport();
  misc_UserErrorReport("\n Parse error in cmdlne_LexScan.\n\n");
  misc_FinishUserErrorReport();


  return FALSE;
}


BOOL cmdlne_Read(int argc, const char *argv[])
/***************************************************************
  INPUT:   number of commandline argument, command
  	   line arguments  
  RETURNS: TRUE if options are in correct syntax and no option
           is defined multiple times. 
  EFFECT:  The options, specified by the commandline arguments,
           and their arguments are stored in <cmdlne_ArgumentsList>.
           The respective flags can be set with cmdline_SetFlags.
***************************************************************/
{

  int iterator;
  
  iterator = 1;
  
  while(iterator < argc)
    {
      if(!cmdlne_LexScan(argv[iterator])) 
      {
	return FALSE;
      }
      iterator++;
    }

  return TRUE;
}
