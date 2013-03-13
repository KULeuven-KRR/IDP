/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                 PROBLEM DESCRIPTION                    * */
/* *                                                        * */
/* ********************************************************** */
/**************************************************************/

#ifndef _DESC_
#define _DESC_
/* *  This program is free software; you can redistribute   * */
/* *  it and/or modify it under the terms of the FreeBSD    * */
/* *  Licence.                                              * */
/* *                                                        * */
/* *  This program is distributed in the hope that it will  * */
/* *  be useful, but WITHOUT ANY WARRANTY; without even     * */
/* *  the implied warranty of MERCHANTABILITY or FITNESS    * */
/* *  FOR A PARTICULAR PURPOSE.  See the LICENCE file       * */ 
/* *  for more details.                                     * */

/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include <stdio.h>

#include "memory.h"
#include "strings.h"

/**************************************************************/
/* Structures                                                 */
/**************************************************************/

typedef enum { DFG_SATISFIABLE, DFG_UNSATISFIABLE, DFG_UNKNOWNSTATE } DFG_STATE;
	
/* Used for the texts from description section. */
typedef struct DFGDESCRIPTION_HELP {
  char*     name;
  char*     author;
  char*     version;
  char*     logic;
  DFG_STATE status;
  char*     description;
  char*     date;
} DFGDESCRIPTION_NODE;

typedef DFGDESCRIPTION_NODE* DFGDESCRIPTION;

/**************************************************************/
/* Functions                                                  */
/**************************************************************/

DFGDESCRIPTION    desc_Create(void);
void    	desc_Delete(DFGDESCRIPTION);

/* Functions for accessing description information */
const char* desc_Name(DFGDESCRIPTION);
const char* desc_Author(DFGDESCRIPTION);
const char* desc_Version(DFGDESCRIPTION);
const char* desc_Logic(DFGDESCRIPTION);
const char* desc_StatusString(DFGDESCRIPTION);
DFG_STATE   desc_Status(DFGDESCRIPTION);
const char* desc_Description(DFGDESCRIPTION);
const char* desc_Date(DFGDESCRIPTION);

void desc_FPrintDescription(DFGDESCRIPTION, FILE*);

#endif
