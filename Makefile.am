subdirs=lua/lua-5.1.4
AM_CXXFLAGS = @AC_CXXFLAGS@ -Wall -Wextra -pedantic -I$(srcdir)/lua/lua-5.1.4/etc -I$(srcdir)/lua/lua-5.1.4/src
AM_LDFLAGS = @AC_LDFLAGS@  

gidl_LDADD = -L$(builddir)/lua/lua-5.1.4/src -ldl $(srcdir)/lua/lua-5.1.4/src/liblua.a

YFLAGS = -d -v
LFLAGS = 

bin_PROGRAMS = gidl

BUILT_SOURCES = parse.h

gidl_SOURCES = 
gidl_SOURCES += builtin.cpp 
gidl_SOURCES += insert.cpp  
gidl_SOURCES += namespace.cpp 
gidl_SOURCES += theory.cpp 
gidl_SOURCES += execute.cpp 
gidl_SOURCES += structure.cpp 
gidl_SOURCES += common.cpp 
gidl_SOURCES += ecnf.cpp 
gidl_SOURCES += visitor.cpp 
gidl_SOURCES += ground.cpp   
gidl_SOURCES += main.cpp
gidl_SOURCES += term.cpp
gidl_SOURCES += data.cpp      
gidl_SOURCES += error.cpp
gidl_SOURCES += vocabulary.cpp
gidl_SOURCES += parse.yy
gidl_SOURCES += lex.ll

include_HEADERS = 
include_HEADERS += builtin.hpp  
include_HEADERS += common.hpp  
include_HEADERS += ecnf.hpp   
include_HEADERS += execute.hpp  
include_HEADERS += insert.hpp     
include_HEADERS += options.hpp    
include_HEADERS += structure.hpp  
include_HEADERS += theory.hpp   
include_HEADERS += vocabulary.hpp
include_HEADERS += clconst.hpp  
include_HEADERS += data.hpp    
include_HEADERS += error.hpp  
include_HEADERS += ground.hpp   
include_HEADERS += namespace.hpp  
include_HEADERS += term.hpp       
include_HEADERS += visitor.hpp

#MINISATID PART, HEADERS ARE TAKEN CARE OF BY THE SUB MAKEFILE
gidl_SOURCES += pcsolver/solvers/SATUtils.cpp
gidl_SOURCES += pcsolver/solvers/pcsolver/PCSolver.cpp pcsolver/solvers/pcsolver/ISolver.cpp 
gidl_SOURCES += pcsolver/solvers/idsolver/IDSolver.cpp
gidl_SOURCES += pcsolver/solvers/aggsolver/AggSets.cpp pcsolver/solvers/aggsolver/AggSolver.cpp pcsolver/solvers/aggsolver/Agg.cpp
gidl_SOURCES += pcsolver/solvers/modsolver/ModSolver.cpp pcsolver/solvers/modsolver/SOSolverHier.cpp
gidl_SOURCES += pcsolver/solvers/external/ExternalInterface.cpp pcsolver/solvers/external/ExternalUtils.cpp 
gidl_SOURCES += pcsolver/solvers/utils/Print.cpp pcsolver/solvers/utils/Utils.cpp 
gidl_SOURCES += pcsolver/solvers/parser/Lparseread.cpp
gidl_SOURCES += pcsolver/solvers/Unittests.cpp

include_HEADERS += pcsolver/solvers/utils/Print.hpp pcsolver/solvers/utils/Utils.hpp 
include_HEADERS += pcsolver/solvers/idsolver/IDSolver.hpp
include_HEADERS += pcsolver/solvers/aggsolver/Agg.hpp pcsolver/solvers/aggsolver/AggSolver.hpp pcsolver/solvers/aggsolver/AggSets.hpp pcsolver/solvers/aggsolver/AggSolver.hpp pcsolver/solvers/aggsolver/AggTypes.hpp 
include_HEADERS += pcsolver/solvers/pcsolver/PCSolver.hpp pcsolver/solvers/pcsolver/ISolver.hpp 
include_HEADERS += pcsolver/solvers/modsolver/ModSolver.hpp pcsolver/solvers/modsolver/SOSolverHier.hpp
include_HEADERS += pcsolver/solvers/external/ExternalInterface.hpp pcsolver/solvers/external/ExternalUtils.hpp
include_HEADERS += pcsolver/solvers/parser/Lparseread.hpp
include_HEADERS += pcsolver/solvers/Unittests.hpp pcsolver/solvers/SolverI.hpp
include_HEADERS += pcsolver/solvers/SATSolver.h pcsolver/solvers/SATUtils.h

CXXFLAGS += -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -I $(srcdir)/pcsolver/

#MINISAT 2.07 09z

if USEMINISAT09Z
gidl_SOURCES += pcsolver/solver3/Solver.cpp 
include_HEADERS += pcsolver/solver3/Solver.hpp pcsolver/solver3/SolverTypes.hpp
include_HEADERS += pcsolver/mtlold/Vec.h pcsolver/mtlold/Queue.h pcsolver/mtlold/Alg.h pcsolver/mtlold/Heap.h pcsolver/mtlold/Map.h pcsolver/mtlold/Sort.h
CXXFLAGS+=-DUSEMINISAT09Z
endif

#MINISAT 2.2
#De -I zorgt ervoor dat ze geladen worden tijdens compileren, de volledige url zorgt ervoor dat ze in de distributie worden gestoken

if USEMINISAT22
gidl_SOURCES += pcsolver/solver3minisat22/core/Solver.cc
include_HEADERS += pcsolver/solver3minisat22/core/Solver.h pcsolver/solver3minisat22/core/SolverTypes.h
include_HEADERS += pcsolver/solver3minisat22/mtl/IntTypes.h pcsolver/solver3minisat22/mtl/Alg.h pcsolver/solver3minisat22/mtl/Vec.h pcsolver/solver3minisat22/mtl/Map.h pcsolver/solver3minisat22/mtl/Alloc.h
include_HEADERS += pcsolver/solver3minisat22/mtl/Queue.h pcsolver/solver3minisat22/mtl/Heap.h pcsolver/solver3minisat22/mtl/Sort.h pcsolver/solver3minisat22/mtl/XAlloc.h
include_HEADERS += pcsolver/solver3minisat22/utils/ParseUtils.h pcsolver/solver3minisat22/utils/Options.h
include_HEADERS += pcsolver/solver3minisat22/zconf.h pcsolver/solver3minisat22/zlib.h
CXXFLAGS+=-DUSEMINISAT22
endif

#MINISAT 2.07

if USEMINISAT
gidl_SOURCES += pcsolver/solver3minisat/Solver.C
include_HEADERS += pcsolver/solver3minisat/Solver.h pcsolver/solver3minisat/SolverTypes.h
include_HEADERS += pcsolver/mtlold/Vec.h pcsolver/mtlold/Queue.h pcsolver/mtlold/Alg.h pcsolver/mtlold/Heap.h pcsolver/mtlold/Map.h pcsolver/mtlold/Sort.h
CXXFLAGS+=-DUSEMINISAT
endif

dist_doc_DATA = README
