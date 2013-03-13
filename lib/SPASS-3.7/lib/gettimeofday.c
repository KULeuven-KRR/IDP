/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *                     GETTIMEOFDAY                       * */
/* *                                                        * */
/* *  $Module:   GETTIMEOFDAY                               * */ 
/* *                                                        * */
/* *  Copyright (C) 2008                                    * */
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
/* $Revision: 1.2 $                                        * */
/* $State: Exp $                                            * */
/* $Date: 2008/04/14 14:35:06 $                             * */
/* $Author: topic $                                       * */
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


/* $RCSfile: gettimeofday.c,v $ */

#include "gettimeofday.h"

/**************************************************************/
/* Functions                                                  */
/**************************************************************/

int gettimeofday(struct timeval * tp, void * tzp)
/**************************************************************
  INPUT:   A timeval to return the time in, and an unused time zone pointer.
  RETURNS: 0.
  EFFECT:  Returns the system time.
***************************************************************/
{
    DWORD milliseconds;

    milliseconds = timeGetTime();

    tp->tv_sec = milliseconds / 1000;
    tp->tv_usec = (milliseconds % 1000) * 1000;

    return 0;
}
