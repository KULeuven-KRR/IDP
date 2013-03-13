/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                     ALARM                              * */
/* *                                                        * */
/* *  $Module:   ALARM                                      * */ 
/* *                                                        * */
/* *  Copyright (C) 2006                                    * */
/* *  MPI fuer Informatik                                   * */
/* *                                                        * */
/* *  This program is free software; you can redistribute   * */
/* *  it and/or modify it under the terms of the GNU        * */
/* *  General Public License as published by the Free       * */
/* *  Software Foundation; either version 2 of the License, * */
/* *  or (at your option) any later version.                * */
/* *                                                        * */
/* *  This program is distributed in the hope that it will  * */
/* *  be useful, but WITHOUT ANY WARRANTY; without even     * */
/* *  the implied warranty of MERCHANTABILITY or FITNESS    * */
/* *  FOR A PARTICULAR PURPOSE.  See the GNU General Public * */
/* *  License for more details.                             * */
/* *                                                        * */
/* *  You should have received a copy of the GNU General    * */
/* *  Public License along with this program; if not, write * */
/* *  to the Free Software Foundation, Inc., 59 Temple      * */
/* *  Place, Suite 330, Boston, MA  02111-1307  USA         * */
/* *                                                        * */
/* *                                                        * */
/* $Revision: 1.1 $                                        * */
/* $State: Exp $                                            * */
/* $Date: 2007/07/13 08:38:25 $                             * */
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


/* $RCSfile: alarm.c,v $ */

#include "alarm.h"

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>

/**************************************************************/
/* Globals                                                    */
/**************************************************************/

static  UINT     WindowsTimerResolution;

static int alarm_TIMEOUT;  /* Set to 1 if the SPASS time polling detects out of time, we
		      do not use BOOL because of Windows incompatability*/

/**************************************************************/
/* Functions                                                  */
/**************************************************************/

static void alarm_OutOfTime(void)
/**************************************************************
  INPUT:   The global alarm_TIMEOUT variable.
  RETURNS: None.
  EFFECT:  Exits the program unless alarm_TIMEOUT is TRUE, meaning
           that the SPASS mainloop has already detected the timeout.
***************************************************************/
{
  if (alarm_TIMEOUT != 1) {
    printf("\nSPASS %s ", vrs_VERSION);
    puts("\nSPASS beiseite: Ran out of time. SPASS was killed.\n");
    exit(EXIT_FAILURE);
  }
}

static
void CALLBACK alarm_Callback(UINT WindowsTimerID, UINT Message, 
			     DWORD UserData, DWORD Data1, 
			     DWORD Data2)
/*********************************************************
  INPUT:   A windows timer ID, a message, a user data dword, 
           and two additional dwords of data. The input 
           can be safely ignored in SPASS' case.
  EFFECT:  Invokes the SPASS out of time handler under 
           Windows.
  RETURNS: None.
**********************************************************/
{ 
    alarm_OutOfTime();

    if (timeEndPeriod(WindowsTimerResolution) 
	!= TIMERR_NOERROR)
       exit(EXIT_FAILURE);

    exit(EXIT_SUCCESS);
} 

static
void alarm_SetAlarmCallback(UINT MilliSecondInterval)
/*********************************************************
  INPUT:   The interval before the callback is initiated 
           in milliseconds.
  EFFECT:  Set up the timer under Windows to call a 
           callback function to invoke SPASS' out of time 
           handler.
  RETURNS: None.
**********************************************************/
{ 
  const UINT TargetResolution = 1;
  TIMECAPS TimeCaps;

  if (timeGetDevCaps(&TimeCaps, sizeof(TIMECAPS)) 
      == TIMERR_NOERROR) 
  { 
    WindowsTimerResolution = min(max(TimeCaps.wPeriodMin, 
				     TargetResolution), 
				 TimeCaps.wPeriodMax);

    if (timeBeginPeriod(WindowsTimerResolution) 
	== TIMERR_NOERROR)
    {

      if (timeSetEvent(
		   MilliSecondInterval,
		   WindowsTimerResolution,
		   alarm_Callback,
		   0,
		   TIME_ONESHOT
		   ) != 0)
	{
	  /* everything setup fine */
	  return;
	}
    }
  }

  exit(EXIT_FAILURE);
} 

void alarm_Init(void)
/*********************************************************
  INPUT:   None.
  EFFECT:  Initializes the alarm module.
  RETURNS: None.
  MEMORY:  None.
**********************************************************/
{
  alarm_TIMEOUT = 0;
}

void alarm_Off(void)
/*********************************************************
  INPUT:   None.
  EFFECT:  Turns off the alarm module.
  RETURNS: None.
  MEMORY:  None.
**********************************************************/
{
  alarm_TIMEOUT = 1;
}

void alarm_SetAlarm(int Seconds)
/**************************************************************
  INPUT:   A duration in seconds.
  RETURNS: None.
  EFFECT:  Starts a alarm that will triger after duration seconds.
***************************************************************/
{
  const unsigned int MillisecondsInASecond = 1000;
  alarm_SetAlarmCallback(Seconds * MillisecondsInASecond);
}
