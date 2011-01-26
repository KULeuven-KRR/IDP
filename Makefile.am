SUBDIRS = lib pcsolver
AM_CXXFLAGS = @AC_CXXFLAGS@ -Wall -Wextra\
		-I$(srcdir)/lib/lua/etc -I$(srcdir)/lib/lua/src\
		-I$(builddir)/lib/lua/src  -I $(srcdir)/lib/\
		-DDATADIR=\"$(datadir)\"
		
if USEINTERACTIVE
AM_CXXFLAGS += -DUSEINTERACTIVE
endif
		
AM_LDFLAGS = @AC_LDFLAGS@

YFLAGS = -d -v
LFLAGS = 

bin_PROGRAMS = gidl

gidl_LDADD = -L$(builddir)/lib/lua/src/.libs -llua

BUILT_SOURCES = parse.h

gidl_SOURCES = \
		builtin.hpp builtin.cpp\
		insert.hpp insert.cpp\
		namespace.hpp namespace.cpp\
		theory.hpp theory.cpp\
		execute.hpp execute.cpp\
		structure.hpp structure.cpp\
		common.hpp common.cpp\
		ecnf.hpp ecnf.cpp\
		visitor.hpp visitor.cpp\
		ground.hpp ground.cpp\
		main.cpp\
		term.hpp term.cpp\
		data.hpp data.cpp\
		error.hpp error.cpp\
		vocabulary.hpp vocabulary.cpp\
		parse.yy\
		lex.ll\
		options.hpp\
		clconst.hpp\
		interactive.cpp interactive.hpp\
		print.hpp print.cpp

#MINISATID PART, HEADERS ARE TAKEN CARE OF BY THE SUB MAKEFILE
gidl_SOURCES += \
		pcsolver/src/satsolver/SATSolver.cpp pcsolver/src/satsolver/SATUtils.cpp\
		pcsolver/src/theorysolvers/PCSolver.cpp pcsolver/src/theorysolvers/SOSolver.cpp\
		pcsolver/src/modules/DPLLTmodule.cpp pcsolver/src/modules/IDSolver.cpp pcsolver/src/modules/AggSolver.cpp pcsolver/src/modules/ModSolver.cpp\
		pcsolver/src/modules/aggsolver/AggTransform.cpp pcsolver/src/modules/aggsolver/AggUtils.cpp pcsolver/src/modules/aggsolver/AggPrint.cpp\
		pcsolver/src/modules/aggsolver/FullyWatched.cpp\
		pcsolver/src/modules/aggsolver/GenPartiallyWatched.cpp\
		pcsolver/src/modules/aggsolver/AggProp.cpp\
		pcsolver/src/external/ExternalInterface.cpp pcsolver/src/external/ExternalUtils.cpp\
		pcsolver/src/utils/Print.cpp pcsolver/src/utils/Utils.cpp pcsolver/src/utils/PrintMessage.cpp \
		pcsolver/src/parser/ResourceManager.cpp pcsolver/src/parser/Lparseread.cpp pcsolver/src/parser/PBread.cpp pcsolver/src/parser/ParseOptions.cpp\
		pcsolver/src/Unittests.cpp\
		pcsolver/src/satsolver/SATSolver.hpp pcsolver/src/satsolver/SATUtils.hpp\
		pcsolver/src/modules/DPLLTmodule.hpp pcsolver/src/modules/IDSolver.hpp pcsolver/src/modules/AggSolver.hpp pcsolver/src/modules/ModSolver.hpp\
		pcsolver/src/modules/aggsolver/AggTransform.hpp pcsolver/src/modules/aggsolver/AggUtils.hpp\
		pcsolver/src/modules/aggsolver/AggPrint.hpp pcsolver/src/modules/aggsolver/FullyWatched.hpp\
		pcsolver/src/modules/aggsolver/PartiallyWatched.hpp\
		pcsolver/src/modules/aggsolver/AggProp.hpp\
		pcsolver/src/theorysolvers/LogicSolver.hpp pcsolver/src/theorysolvers/PCSolver.hpp pcsolver/src/theorysolvers/SOSolver.hpp\
		pcsolver/src/external/ExternalInterface.hpp pcsolver/src/external/ExternalUtils.hpp\
		pcsolver/src/utils/Print.hpp pcsolver/src/utils/Utils.hpp pcsolver/src/utils/PrintMessage.hpp pcsolver/src/utils/IntTypes.h\
		pcsolver/src/parser/ResourceManager.hpp pcsolver/src/parser/Lparseread.hpp pcsolver/src/parser/PBread.hpp\
		pcsolver/src/parser/ParseOptions.hpp\
		pcsolver/src/tclap/CmdLine.h\
		pcsolver/src/external/Translator.hpp pcsolver/src/external/Translator.cpp\
		pcsolver/src/Unittests.hpp

AM_CXXFLAGS+=-I $(top_srcdir)/pcsolver/src -I $(top_srcdir)/pcsolver/tclap/include/ #-std=gnu++0x 
AM_LDFLAGS+=-pthread  -L$(top_builddir)/pcsolver/perftools/ -ltcmalloc_minimal # -lprofiler

#ESSENTIAL FOR INTTYPES USAGE 
AM_CXXFLAGS+= -D __STDC_FORMAT_MACROS -D __STDC_LIMIT_MACROS

gidl_LDADD+=-L$(top_builddir)/pcsolver/src/pbsolver -lpbsolver-1.0

#PBSOLVER
gidl_SOURCES+=pbsolver/PbSolver.h
AM_CXXFLAGS+=-I$(top_srcdir)/pcsolver/src/pbsolver/ -I$(top_srcdir)/pcsolver/src/pbsolver/pbbase/h/ -I$(top_srcdir)/pcsolver/src/pbsolver/ADTs/


#MINISAT 2.07 09z
if USEMINISAT09Z
gidl_SOURCES +=\
		pcsolver/src/solver3/Solver.cpp\
		pcsolver/src/solver3/Solver.hpp pcsolver/src/solver3/SolverTypes.hpp\
		pcsolver/src/mtlold/Vec.h pcsolver/src/mtlold/Queue.h pcsolver/src/mtlold/Alg.h pcsolver/src/mtlold/Heap.h pcsolver/src/mtlold/Map.h pcsolver/src/mtlold/Sort.h
AM_CXXFLAGS+= -D USEMINISAT09Z
endif


#MINISAT 2.2
if USEMINISAT22
gidl_SOURCES +=\
		pcsolver/src/solver3minisat22/core/Solver.cc\
		pcsolver/src/solver3minisat22/core/Solver.h pcsolver/src/solver3minisat22/core/SolverTypes.h \
		pcsolver/src/solver3minisat22/mtl/IntTypes.h pcsolver/src/solver3minisat22/mtl/Alg.h pcsolver/src/solver3minisat22/mtl/Vec.h\
		pcsolver/src/solver3minisat22/mtl/Map.h pcsolver/src/solver3minisat22/mtl/Alloc.h\
		pcsolver/src/solver3minisat22/mtl/Queue.h pcsolver/src/solver3minisat22/mtl/Heap.h\
		pcsolver/src/solver3minisat22/mtl/Sort.h pcsolver/src/solver3minisat22/mtl/XAlloc.h\
		pcsolver/src/solver3minisat22/utils/ParseUtils.h pcsolver/src/solver3minisat22/utils/Options.h\
		pcsolver/src/solver3minisat22/zconf.h pcsolver/src/solver3minisat22/zlib.h
AM_CXXFLAGS+= -D USEMINISAT22 -I$(srcdir)/pcsolver/src/solver3minisat22/
#necessary if not used by minisatid itself! AM_CXXFLAGS+= -D __STDC_FORMAT_MACROS -D __STDC_LIMIT_MACROS 
endif

#MINISAT 2.07
if USEMINISAT
gidl_SOURCES +=\
		pcsolver/src/solver3minisat/Solver.C\
		pcsolver/src/solver3minisat/Solver.h pcsolver/src/solver3minisat/SolverTypes.h\
		pcsolver/src/mtlold/Vec.h pcsolver/src/mtlold/Queue.h pcsolver/src/mtlold/Alg.h pcsolver/src/mtlold/Heap.h pcsolver/src/mtlold/Map.h mtlold/Sort.h
AM_CXXFLAGS+= -D USEMINISAT
endif


dist_doc_DATA = README

data_stddir = $(datadir)/std
data_std_DATA = data/idp_intern.idp data/idp_intern.lua
