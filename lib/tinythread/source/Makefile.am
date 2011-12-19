####################################################################
#Copyright 2010-2012 Katholieke Universiteit Leuven
# 
#Use of this software is governed by the GNU LGPLv3.0 license
#
#Written by Broes De Cat, Stef De Pooter, Johan Wittocx
#and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
#Celestijnenlaan 200A, B-3001 Leuven, Belgium
####################################################################

AM_CXXFLAGS = @AC_CXXFLAGS@
AM_LDFLAGS = @AC_LDFLAGS@ @LIBS@

lib_LTLIBRARIES = libtinythread-1.0.la
libtinythread_1_0_la_LDFLAGS = -version-info 1:2:0
tinythread_includedir = $(includedir)/tinythread-1.0
libtinythread_1_0_la_SOURCES = tinythread.cpp
				
tinythread_include_HEADERS = fastmutex.h tinythread.h
