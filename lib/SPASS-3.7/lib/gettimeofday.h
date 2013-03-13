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


/* $RCSfile: gettimeofday.h,v $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef SPASS_GETTIME_H
#define SPASS_GETTIME_H

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#ifdef HAVE_MMSYSTEM_H
#include <mmsystem.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>


/**************************************************************/
/* Functions                                                  */
/**************************************************************/

#ifndef HAVE_GETTIMEOFDAY
int gettimeofday(struct timeval * tp, void * tzp);
#endif

#endif /* SPASS_GETTIME_H */
