SUBDIRS = pcsolver

AM_CXXFLAGS = @AC_CXXFLAGS@ -Wall -Wextra -pedantic -I $(srcdir)/pcsolver/
AM_LDFLAGS = @AC_LDFLAGS@  

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

CXXFLAGS += -D USEMINISAT

dist_doc_DATA = README
