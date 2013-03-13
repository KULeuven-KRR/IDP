/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                MISCELLANEOUS                           * */
/* *                                                        * */
/* *  $Module:   MISC                                       * */ 
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


/* $RCSfile: misc.c,v $ */

/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include "misc.h"
#include "strings.h" /* Connot be mived to misc.h as long as there is no separate type.h, e.g. for pointers */

/**************************************************************/
/* Functions                                                  */
/**************************************************************/

void misc_StartErrorReporting(const char* file, int line)  
{ 
  fflush(stdout);
  fprintf(misc_ERROROUT,"\n\tError in file %s at line %d\n",file, line);
}

void misc_FinishErrorReport(void) 
{ 
  fputs("\n Please report this error via email to spass@mpi-sb.mpg.de including\n the SPASS version, input problem, options, operating system.\n",misc_ERROROUT);
  misc_DumpCore();
}

void misc_ErrorReport(const char * Format, ...)
{
  va_list args;
  va_start(args,Format);
  vfprintf(misc_ERROROUT,Format,args);
  va_end(args);
}

void misc_UserWarning(const char * Format, ...)
{
  va_list args;
  va_start(args,Format);
  vfprintf(misc_ERROROUT,Format,args);
  va_end(args);
}

void misc_UserErrorReport(const char * Format, ...)
{
  va_list args;
  va_start(args,Format);
  vfprintf(misc_USERERROROUT,Format,args);
  va_end(args);
}


void misc_PrintChar(NAT Number, char Character)
/**************************************************************
  INPUT:   A positive number and a character.
  RETURNS: Nothing.
  EFFECT:  Prints as many times the same character as the number 
           demands.
***************************************************************/
{
  NAT Counter;
  for (Counter = 1; Counter <= Number; Counter++)
    putchar(Character);
}


void misc_DumpCore(void)
/**************************************************************
  INPUT:   Nothing.
  RETURNS: Nothing.
  EFFECT:  Flushes the streams then dumps a core.
***************************************************************/
{
  fputs("\n\n", misc_ERROROUT);
  fflush(misc_ERROROUT);
  fflush(stdout);
  fflush(stderr);
  abort();
}

void misc_DumpCoreOut(const char* String)
/**************************************************************
  INPUT:   A  string.
  RETURNS: Nothing.
  EFFECT:  Prints <String> and then dumps a core.
***************************************************************/
{
  fprintf(stderr, "\n %s \n", String);
  misc_DumpCore();
}



int misc_ReturnValue(void)
{
  return 0;
}


int misc_Max(int a, int b)
{
  if (a > b)
    return a;
  else
    return b;
}

FILE* misc_OpenFile(const char* Name, const char* Mode)
/**************************************************************
  INPUT:   The name of a file and a string containing the mode
           for opening the file (see fopen(3)).
           Examples for Mode are "r" for reading and "w" for writing.
  RETURNS: The FILE pointer, if the file was successfully opened.
  EFFECT:  If it wasn't possible to open the file with the
           requested mode, an error message is printed and the
           program exits.
***************************************************************/
{
  FILE* File;

  File = fopen(Name,Mode);

  if (File == (FILE*)NULL) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n\tError in opening file %s for %s !\n\n", Name, 
			 (Mode[0] == 'r' ? "reading" :
			  (Mode[0] == 'w' ? "writing" : "i/o operations")));
    misc_FinishUserErrorReport();
  }  

  return File;
}

FILE* misc_OpenFileExt(const char* Name, const char* Mode, 
                       char* SearchPaths,
                       char ** const DiscoveredName)
/**************************************************************
  INPUT:   The name of a file, a string containing the mode
           for opening the file (see fopen(3)),
           a string containg a colon/semicolon separated list of directories
           to look for the file, and a pointer to char pointer
           to hold the fullpath name of the file 
           that was opened in the end (can be null).          
           Examples for Mode are "r" for reading and "w" for writing.

  RETURNS: The FILE pointer, if the file was successfully opened.
           if <DiscoveredName> is not null, the location it points 
           to will contain a newly allocated string 
           containg the full path name of the opened file.

  EFFECT:  The file is first looked for in the current directory.
           If not found, the individual directories from SearchPaths
           are tried consecutively to look for the file, 
           until a first file is found that can be opened.
           
           If it wasn't possible to open any of the files with the
           requested mode, an error message is printed and the
           program exits.
***************************************************************/
{
  FILE* File;

  File = fopen(Name,Mode);

  if (File == (FILE*)NULL) {
    const char * const Delims = ":;";    
    LIST DirNames, Scan;

    DirNames = string_Split(SearchPaths,Delims);
    
    for(Scan = DirNames; !list_Empty(Scan); Scan = list_Cdr(Scan)) {
      char* DirName;
      char* WholeName;

      DirName = list_Car(Scan);      
      WholeName = string_Conc(DirName,Name);
      File = fopen(WholeName,Mode);
      
      if (File != (FILE*)NULL) {
        if (DiscoveredName)
          *DiscoveredName = WholeName;
        else
          string_StringFree(WholeName);
        
        list_DeleteWithElement(DirNames,(void (*)(POINTER))string_StringFree);
        return File;
      }
      
      string_StringFree(WholeName);
    }
    list_DeleteWithElement(DirNames,(void (*)(POINTER))string_StringFree);
  } else {
    if (DiscoveredName)
      *DiscoveredName = string_StringCopy(Name);
  }

  if (File == (FILE*)NULL) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n\tError in opening file %s for %s !\n\n", Name, 
			 (Mode[0] == 'r' ? "reading" :
			  (Mode[0] == 'w' ? "writing" : "i/o operations")));
    misc_FinishUserErrorReport();
  }  

  return File;
}
  
FILE* misc_OpenFileEnv(const char* Name, const char* Mode,
                       const char* Environ,
                       char ** const DiscoveredName)
/**************************************************************
  INPUT:   The name of a file, a string containing the mode
           for opening the file (see fopen(3)),
           a string containg a name of environment variable
           and a pointer to char pointer
           to hold the fullpath name of the file 
           that was opened in the end (can be null).         
           Examples for Mode are "r" for reading and "w" for writing.

  RETURNS: The FILE pointer, if the file was successfully opened.
           if <DiscoveredName> is not null, the location it points 
           to will contain a newly allocated string 
           containg the full path name of the opened file.

  EFFECT:  Fetches a value from environment variable Environ
           to be used as SearchPaths in a call to misc_OpenFileExt
           (see also there).
***************************************************************/
{
  char dummy[] = "\0";
  
  char* paths;
  paths = getenv(Environ);
  
  return misc_OpenFileExt(Name,Mode,paths ? paths : dummy,DiscoveredName);
}


void misc_CloseFile(FILE* File, const char* Name)
/**************************************************************
  INPUT:   A FILE and its name.
  RETURNS: Nothing.
  EFFECT:  Closes the file. If an error occurs, a message is
           printed and the program exits.
***************************************************************/
{
  int Result;

  Result = fclose(File);

  if (Result != 0) {
    misc_StartUserErrorReport();
    misc_UserErrorReport("\n\tError in closing file %s !\n\n", Name);
    misc_FinishUserErrorReport();
  }  
}

