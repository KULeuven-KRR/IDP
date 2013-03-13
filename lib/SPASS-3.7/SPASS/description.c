/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                 PROBLEM DESCRIPTION                    * */
/* *                                                        * */
/* ********************************************************** */
/**************************************************************/

/**************************************************************/
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

#include "description.h"

/**************************************************************/
/* Functions                                                  */
/**************************************************************/

DFGDESCRIPTION    desc_Create(void)
/**************************************************************
  INPUT:   Nothing.
  RETURNS: Newly allocated problem description instance.
***************************************************************/
{
	DFGDESCRIPTION problem;
	problem = (DFGDESCRIPTION)memory_Malloc(sizeof(DFGDESCRIPTION_NODE));
	memset(problem,0,sizeof(DFGDESCRIPTION_NODE));
	problem->status = DFG_UNKNOWNSTATE;		
	return problem;
}
	
void	desc_Delete(DFGDESCRIPTION Description)
/**************************************************************
  INPUT:   Problem description instance.
  RETURNS: Nothing.
  EFFECT:  Frees memory used by the problem description.
***************************************************************/
{
  if (Description->name != NULL)
    string_StringFree(Description->name);
  if (Description->author != NULL)
    string_StringFree(Description->author);
  if (Description->version != NULL)
    string_StringFree(Description->version);
  if (Description->logic != NULL)
    string_StringFree(Description->logic);
  if (Description->description != NULL)
    string_StringFree(Description->description);
  if(Description->date != NULL)
    string_StringFree(Description->date);

  memory_Free(Description,sizeof(DFGDESCRIPTION_NODE));
}

const char* desc_Name(DFGDESCRIPTION Description)
/**************************************************************
  INPUT:   Problem description instance.
  RETURNS: The problem's name from the description section.
***************************************************************/
{
  return Description->name;
}

const char* desc_Author(DFGDESCRIPTION Description)
/**************************************************************
  INPUT:   Problem description instance.
  RETURNS: The problem's author from the description section.
***************************************************************/
{
  return Description->author;
}

const char* desc_Version(DFGDESCRIPTION Description)
/**************************************************************
  INPUT:   Problem description instance.
  RETURNS: The problem's version from the description section.
***************************************************************/
{
  return Description->version;
}

const char* desc_Logic(DFGDESCRIPTION Description)
/**************************************************************
  INPUT:   Problem description instance.
  RETURNS: The problem's logic from the description section.
***************************************************************/
{
  return Description->logic;
}

DFG_STATE desc_Status(DFGDESCRIPTION Description)
/**************************************************************
  INPUT:   Problem description instance.
  RETURNS: The problem's status from the description section.
***************************************************************/
{
  return Description->status;
}

const char* desc_StatusString(DFGDESCRIPTION Description)
/**************************************************************
  INPUT:   Problem description instance.
  RETURNS: The string representation of the problem's status.
***************************************************************/
{
  const char* result = "";

  switch (Description->status) {
  case DFG_SATISFIABLE:
    result = "satisfiable"; break;
  case DFG_UNSATISFIABLE:
    result = "unsatisfiable"; break;
  case DFG_UNKNOWNSTATE:
    result = "unknown"; break;
  default:
    misc_StartErrorReport();
    misc_ErrorReport("\n In desc_StatusString: Invalid status.\n");
    misc_FinishErrorReport();
  }
  return result;
}

const char* desc_Description(DFGDESCRIPTION Description)
/**************************************************************
  INPUT:   Problem description instance.
  RETURNS: The problem's description from the description section.
***************************************************************/
{
  return Description->description;
}

const char* desc_Date(DFGDESCRIPTION Description)
/**************************************************************
  INPUT:   Problem description instance.
  RETURNS: The problem's date from the description section.
***************************************************************/
{
  return Description->date;
}

void desc_FPrintDescription(DFGDESCRIPTION Description, FILE* File)
/**************************************************************
  INPUT:   Problem description instance and a file stream.
  RETURNS: Nothing.
  EFFECT:  The description section from the input file
           is printed to 'File'. You must call the parser first
           before calling this function.
***************************************************************/
{
  fputs("list_of_descriptions.\n  name(", File);
  if (Description->name != NULL)
    fputs(Description->name, File);
  else
    fputs("{* *}", File);
  fputs(").\n  author(", File);
  if (Description->author != NULL)
    fputs(Description->author, File);
  else
    fputs("{* *}", File);
  fputs(").\n", File);
  if (Description->version != NULL) {
    fputs("  version(", File);
    fputs(Description->version, File);
    fputs(").\n", File);
  }
  if (Description->logic != NULL) {
    fputs("  logic(", File);
    fputs(Description->logic, File);
    fputs(").\n", File);
  }
  fputs("  status(", File);
  fputs(desc_StatusString(Description), File);
  fputs(").\n  description(", File);
  if (Description->description != NULL)
    fputs(Description->description, File);
  else
    fputs("{* *}", File);
  fputs(").\n", File);
  if (Description->date != NULL) {
    fputs("  date(", File);
    fputs(Description->date, File);
    fputs(").\n", File);
  }
  fputs("end_of_list.", File);
}
