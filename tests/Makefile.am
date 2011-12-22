####################################################################
#Copyright 2010-2012 Katholieke Universiteit Leuven
# 
#Use of this software is governed by the GNU LGPLv3.0 license
#
#Written by Broes De Cat, Stef De Pooter, Johan Wittocx
#and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
#Celestijnenlaan 200A, B-3001 Leuven, Belgium
####################################################################

libsrcdir = $(top_srcdir)/lib
libbuilddir = $(top_builddir)/lib

AM_CXXFLAGS = @AC_CXXFLAGS@ -Wall -Wextra -std=c++0x\
		-I $(libsrcdir)/loki/include\
		-I $(top_srcdir)/src\
		-I $(libsrcdir)/tinythread/source\
		-I $(libsrcdir)/pcsolver/src/\
		-I $(libsrcdir)/pluscallback1.7/\
		-I $(libsrcdir)/linenoise/include\
		-I $(libsrcdir)/lua/etc -I$(libsrcdir)/lua/src -I $(libbuilddir)/lua/src
				
if USEINTERACTIVE
AM_CXXFLAGS += -DUSEINTERACTIVE
endif

if BUILDTESTS
bin_PROGRAMS = alltests
endif 

alltests_SOURCES = \
		testsuite.cpp\
		mxtests.cpp\
		generatortests.cpp\
		bddtests.cpp\
		grounderfactorytests.cpp\
		definitiontests.cpp\
		propagationtests.cpp\
		parsertests.cpp\
		transformationtests.cpp\
		cppinterface.hpp cppinterface.cpp\
		testingtools.hpp testingtools.cpp\
		TestUtils.hpp TestUtils.cpp

alltests_LDADD = $(top_builddir)/src/.libs/libgidl-2.0.la
alltests_LDFLAGS = -lgtest -pthread
AM_CXXFLAGS += -DTESTDIR=\"$(abs_srcdir)/\"

TESTS = alltests
