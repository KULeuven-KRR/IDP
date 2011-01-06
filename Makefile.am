SUBDIRS = lib
AM_CXXFLAGS = @AC_CXXFLAGS@ -Wall -Wextra -pedantic -I$(srcdir)/lib/lua/etc -I$(srcdir)/lib/lua/src -I$(builddir)/lib/lua/src
AM_LDFLAGS = @AC_LDFLAGS@  

YFLAGS = -d -v
LFLAGS = 

bin_PROGRAMS = gidl

gidl_LDADD = -L$(builddir)/lib/lua/src/.libs/ -llua-5.1

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
		clconst.hpp

#MINISATID PART, HEADERS ARE TAKEN CARE OF BY THE SUB MAKEFILE
gidl_SOURCES += \
		pcsolver/solvers/SATUtils.cpp\
		pcsolver/solvers/pcsolver/PCSolver.cpp pcsolver/solvers/pcsolver/ISolver.cpp\
		pcsolver/solvers/idsolver/IDSolver.cpp\
		pcsolver/solvers/aggsolver/AggSets.cpp pcsolver/solvers/aggsolver/AggSolver.cpp pcsolver/solvers/aggsolver/Agg.cpp\
		pcsolver/solvers/modsolver/ModSolver.cpp pcsolver/solvers/modsolver/SOSolverHier.cpp\
		pcsolver/solvers/external/ExternalInterface.cpp pcsolver/solvers/external/ExternalUtils.cpp\
		pcsolver/solvers/utils/Print.cpp pcsolver/solvers/utils/Utils.cpp\
		pcsolver/solvers/parser/Lparseread.cpp\
		pcsolver/solvers/Unittests.cpp\
		pcsolver/solvers/utils/Print.hpp pcsolver/solvers/utils/Utils.hpp\
		pcsolver/solvers/idsolver/IDSolver.hpp\
		pcsolver/solvers/aggsolver/Agg.hpp pcsolver/solvers/aggsolver/AggSolver.hpp pcsolver/solvers/aggsolver/AggSets.hpp pcsolver/solvers/aggsolver/AggSolver.hpp pcsolver/solvers/aggsolver/AggTypes.hpp\
		pcsolver/solvers/pcsolver/PCSolver.hpp pcsolver/solvers/pcsolver/ISolver.hpp\
		pcsolver/solvers/modsolver/ModSolver.hpp pcsolver/solvers/modsolver/SOSolverHier.hpp\
		pcsolver/solvers/external/ExternalInterface.hpp pcsolver/solvers/external/ExternalUtils.hpp\
		pcsolver/solvers/parser/Lparseread.hpp\
		pcsolver/solvers/Unittests.hpp pcsolver/solvers/SolverI.hpp\
		pcsolver/solvers/SATSolver.h pcsolver/solvers/SATUtils.h

CXXFLAGS += -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -I $(srcdir)/pcsolver/

#MINISAT 2.07 09z

if USEMINISAT09Z
gidl_SOURCES += \
		pcsolver/solver3/Solver.cpp\
		pcsolver/solver3/Solver.hpp pcsolver/solver3/SolverTypes.hpp\
		pcsolver/mtlold/Vec.h pcsolver/mtlold/Queue.h pcsolver/mtlold/Alg.h pcsolver/mtlold/Heap.h pcsolver/mtlold/Map.h pcsolver/mtlold/Sort.h
CXXFLAGS+=-DUSEMINISAT09Z
endif

#MINISAT 2.2
#De -I zorgt ervoor dat ze geladen worden tijdens compileren, de volledige url zorgt ervoor dat ze in de distributie worden gestoken

if USEMINISAT22
gidl_SOURCES += \
		pcsolver/solver3minisat22/core/Solver.cc\
		pcsolver/solver3minisat22/core/Solver.h pcsolver/solver3minisat22/core/SolverTypes.h\
		pcsolver/solver3minisat22/mtl/IntTypes.h pcsolver/solver3minisat22/mtl/Alg.h pcsolver/solver3minisat22/mtl/Vec.h pcsolver/solver3minisat22/mtl/Map.h pcsolver/solver3minisat22/mtl/Alloc.h\
		pcsolver/solver3minisat22/mtl/Queue.h pcsolver/solver3minisat22/mtl/Heap.h pcsolver/solver3minisat22/mtl/Sort.h pcsolver/solver3minisat22/mtl/XAlloc.h\
		pcsolver/solver3minisat22/utils/ParseUtils.h pcsolver/solver3minisat22/utils/Options.h\
		pcsolver/solver3minisat22/zconf.h pcsolver/solver3minisat22/zlib.h
CXXFLAGS+=-DUSEMINISAT22
endif

#MINISAT 2.07

if USEMINISAT
gidl_SOURCES += \
		pcsolver/solver3minisat/Solver.C\
		pcsolver/solver3minisat/Solver.h pcsolver/solver3minisat/SolverTypes.h\
		pcsolver/mtlold/Vec.h pcsolver/mtlold/Queue.h pcsolver/mtlold/Alg.h pcsolver/mtlold/Heap.h pcsolver/mtlold/Map.h pcsolver/mtlold/Sort.h
CXXFLAGS+=-DUSEMINISAT
endif

dist_doc_DATA = README
