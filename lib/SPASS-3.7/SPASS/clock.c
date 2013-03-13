/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                     CLOCK                              * */
/* *                                                        * */
/* *  $Module:   CLOCK                                      * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1999, 2000, 2001                  * */
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


/* $RCSfile: clock.c,v $ */

#include "clock.h"

/**************************************************************/
/* Global Variables                                           */
/**************************************************************/

float      clock_Akku[clock_TYPESIZE];
CLOCK_TMS  clock_Counters[clock_TYPESIZE];

/**************************************************************/
/* Functions                                                  */
/**************************************************************/

void clock_Init(void)
/*********************************************************
  INPUT:   None.
  EFFECT:  Initializes the clock Module.
  RETURNS: None.
  MEMORY:  None.
**********************************************************/
{
  int i;

  for (i=0;i<clock_TYPESIZE;i++) {
    clock_InitCounter(i);
  }
}



void clock_InitCounter(CLOCK_CLOCKS ClockCounter)
/*********************************************************
  INPUT:   A clock counter.
  EFFECT:  The clock counter <ClockCounter> is initialized.
  RETURNS: None.
  MEMORY:  None.
**********************************************************/
{
  clock_Akku[ClockCounter] = 0;
}


void clock_StartCounter(CLOCK_CLOCKS ClockCounter)
/*********************************************************
  INPUT:   A clock counter.
  EFFECT:  The clock counter <ClockCounter> is started.
  RETURNS: None.
  MEMORY:  None.
**********************************************************/
{
#ifndef CLOCK_NO_TIMING
  gettimeofday(&(clock_Counters[ClockCounter]), NULL);
#endif /* CLOCK_NO_TIMING */
}


void clock_StopPassedTime(CLOCK_CLOCKS ClockCounter) 
/*********************************************************
  INPUT:   A clock counter.
  EFFECT:  Stores the number of seconds passed since given
           counter was started in the according
	   accumulator.
  RETURNS: None.
  MEMORY:  None.
**********************************************************/
{
#ifndef CLOCK_NO_TIMING
  CLOCK_TMS    newtime;
  gettimeofday(&newtime, NULL);
  clock_Akku[ClockCounter] = clock_GetSeconds(ClockCounter);
#endif
}


void clock_StopAddPassedTime(CLOCK_CLOCKS ClockCounter) 
/*********************************************************
  INPUT:   A clock counter.
  EFFECT:  Adds the number of seconds passed since given
           counter was started to the according
	   accumulator.
  RETURNS: None.
  MEMORY:  None.
**********************************************************/
{
#ifndef CLOCK_NO_TIMING
  CLOCK_TMS    newtime;
  gettimeofday(&newtime, NULL);
  clock_Akku[ClockCounter] += clock_GetSeconds(ClockCounter);
#endif
}

float clock_GetAkku(CLOCK_CLOCKS ClockCounter)
/*********************************************************
  INPUT:   A clock counter.
  EFFECT:  None.
  RETURNS: The akkumulated number of seconds stored  by the 
           counter as a float.
**********************************************************/
{
  return clock_Akku[ClockCounter];
}


float clock_GetSeconds(CLOCK_CLOCKS ClockCounter)
/*********************************************************
  INPUT:   A clock counter.
  EFFECT:  Computes the number of seconds spent by the
           counter.
  RETURNS: The number of seconds spent by the counter as
           a float.
  MEMORY:  None.
**********************************************************/
{
#ifndef CLOCK_NO_TIMING
  CLOCK_TMS    newtime;
  time_t       seconds_passed;
  long         microseconds_passed;

  gettimeofday(&newtime, NULL);

  seconds_passed = newtime.tv_sec - clock_Counters[ClockCounter].tv_sec;
  microseconds_passed = newtime.tv_usec - clock_Counters[ClockCounter].tv_usec;

  return ((float) seconds_passed
	  + (microseconds_passed /(float)1000000));

#else /* CLOCK_NO_TIMING */
  return 0;
#endif /* ! CLOCK_NO_TIMING */
}



void clock_PrintTime(CLOCK_CLOCKS ClockCounter)
/*********************************************************
  INPUT:   A clock counter.
  EFFECT:  The time is printed in format hh:mm:ss.dd to stdout
  RETURNS: None.
  MEMORY:  None.
**********************************************************/
{
#ifndef CLOCK_NO_TIMING
  NAT   hours, minutes;
  float seconds;

  seconds   = clock_Akku[ClockCounter];
  hours     = (NAT)seconds/3600;
  seconds  -= hours*3600;
  minutes   = (NAT)seconds/60;
  seconds  -= (minutes*60);
  if (seconds >= 10.0)
    printf("%u:%02u:%2.2f",hours,minutes,seconds);
  else
    printf("%u:%02u:0%2.2f",hours,minutes,seconds);
#else
  fputs(" No Timing on this machine. ",stdout);
#endif
}
