# Compiler
GPP=g++
GPPFLAGS=-O3 -D NDEBUG -Wall -Wextra -Wno-unused-parameter -ffloat-store -pedantic -Wno-variadic-macros #-D GMP

# Lexer
FLEX=flex
FLEXFLAGS=

# Parser generator
YACC=bison
YACCFLAGS=--defines

ifdef V

  ifeq (TEST,$(findstring TEST,$(V)))
    GPPFLAGS=-ggdb -Wall -pedantic
    YACCFLAGS=-d -v --verbose
  endif

  ifeq (TRAIL,$(findstring TRAIL,$(V)))
    GPPFLAGS=-ggdb -D APPROXTRAIL -Wall -pedantic
    YACCFLAGS=-d -v --verbose 
  endif
  
endif

# filenames
# ############################################################
PHDRS		= parse.tab.hpp #pcsolver/solvers/ecnf.y.hpp 
CHDRS		= $(PHDRS) $(wildcard *.hpp) $(wildcard pcsolver/solver3/*.hpp) $(wildcard pcsolver/mtl/*.hpp) $(wildcard pcsolver/solvers/*.hpp) $(wildcard pcsolver/solvers/aggs/*.hpp)
PSRCS		= parse.tab.cpp lex.yy.cpp #pcsolver/solvers/ecnf.y.cpp pcsolver/solvers/ecnf.l.cpp pcsolver/solvers/ecnf.y.cpp 
CSRCS		= $(PSRCS) $(wildcard *.cpp) $(wildcard pcsolver/solvers/[!M]*.cpp) $(wildcard pcsolver/solvers/M[!a]*.cpp) $(wildcard pcsolver/solvers/aggs/*.cpp) $(wildcard pcsolver/solver3/*.cpp) $(wildcard pcsolver/mtl/*.cpp)
CFLAGS	= -Ipcsolver/mtl -Ipcsolver/solvers -Ipcsolver/solvers/aggs -Ipcsolver/solver3 -I.
LFLAGS	= #-lz -lgmpxx -lgmp
EXEC		= gidl

# stuff to make
# ############################################################

all: $(EXEC)
	
#CSRCS	?= $(wildcard *.cpp)
#CHDRS	?= $(wildcard *.hpp)

COBJS	?= $(addsuffix .o, $(basename $(CSRCS)))
#PCOBJS	= $(addsuffix p,  $(COBJS))
#DCOBJS	= $(addsuffix d,  $(COBJS))
#RCOBJS	= $(addsuffix r,  $(COBJS))
#CCCOBJS	= $(addsuffix cc,  $(COBJS))

EXEC		?= $(notdir $(shell pwd))
#LIB		?= $(EXEC)

CFLAGS	?= $(GPPFLAGS)

#CFLAGS	?= -Wall
#LFLAGS	?= -Wall

#COPTIMIZE ?= -O3

#.PHONY : s cc p d r rs lib libd clean 

#s:	$(EXEC)
#p:	$(EXEC)_profile
#d:	$(EXEC)_debug
#r:	$(EXEC)_release
#cc: $(EXEC)_codecover
#rs:	$(EXEC)_static
#lib:	lib$(LIB).a
#libd:	lib$(LIB)d.a

## Compile options
%.o:		CFLAGS +=$(COPTIMIZE) -ggdb -D DEBUG
#%.op:	CFLAGS +=$(COPTIMIZE) -pg -ggdb -D NDEBUG
#%.od:	CFLAGS +=-O0 -ggdb -D DEBUG # -D INVARIANTS
#%.or:	CFLAGS +=$(COPTIMIZE) -D NDEBUG
#%.occ:	CFLAGS +=-O0 -fprofile-arcs -ftest-coverage -ggdb -D DEBUG # -D INVARIANTS

## Link options
$(EXEC):					LFLAGS := -ggdb $(LFLAGS)
#$(EXEC)_codecover:	LFLAGS := -ggdb -lgcov $(LFLAGS)
#$(EXEC)_profile:		LFLAGS := -ggdb -pg $(LFLAGS)
#$(EXEC)_debug:		LFLAGS := -ggdb $(LFLAGS)
#$(EXEC)_release:		LFLAGS := $(LFLAGS)
#$(EXEC)_static:		LFLAGS := --static $(LFLAGS)

## Dependencies
$(EXEC):			$(COBJS)
#$(EXEC)_profile:	$(PCOBJS)
#$(EXEC)_debug:	$(DCOBJS)
#$(EXEC)_release:	$(RCOBJS)
#$(EXEC)_static:	$(RCOBJS)
#$(EXEC)_codecover:$(CCCOBJS)

#lib$(LIB).a:	$(filter-out Main.or, $(RCOBJS))
#lib$(LIB)d.a:	$(filter-out Main.od, $(DCOBJS))

## Library rule
#lib$(LIB).a lib$(LIB)d.a:
#	@echo Library: "$@ ( $^ )"
#	@rm -f $@
#	@ar cq $@ $^

## Build rule
%.o %.op %.od %.or %.occ: %.cpp
	@echo Compiling: "$@ ( $< )"
	@$(GPP) $(CFLAGS) -c -o $@ $<
	
%.tab.cpp: %.yy
	@echo Compiling: "$@ ( $< )"
	@bison $(YACCFLAGS)  --output=$@ $<
	
%.yy.cpp: %.ll
	@echo Compiling: "$@ ( $< )"
	@flex $(FLEXFLAGS) -o$@ $<
	
%.y.cpp: %.y
	@echo Compiling: "$@ ( $< )"
	@bison -p ecnf --defines --output=$@ $<
	
%.l.cpp: %.l
	@echo Compiling: "$@ ( $< )"
	@flex -P ecnf -o$@ $<

## Linking rules for all executables
$(EXEC): # $(EXEC)_codecover $(EXEC)_profile $(EXEC)_debug $(EXEC)_release $(EXEC)_static:
	@echo Linking: "$@" #( $^ )"
	@$(GPP) $^ $(GPPFLAGS) $(LFLAGS) -o $@
	
#clean:
#	-rm -f $(LEX).yy.* $(DATALEX).yy.* $(OUTPUT) $(GRAMMAR).tab.cc $(GRAMMAR).output $(GRAMMAR).tab.hh $(OBJECTS)

## Clean rule
clean:
	@rm -f $(EXEC) $(EXEC)_codecover $(EXEC)_profile $(EXEC)_debug $(EXEC)_release $(EXEC)_static \
		$(PSRCS) $(PHDRS) $(COBJS) $(CCCOBJS) $(PCOBJS) $(DCOBJS) $(RCOBJS) *.core depend.mk lib$(LIB).a lib$(LIB)d.a
	  	  

## Make dependencies
#depend.mk: $(CSRCS) $(CHDRS)
#	@echo Making dependencies ...
#	@$(GPP) $(CFLAGS) -MM $(CSRCS) > depend.mk
#	@cp depend.mk depend.mk.tmp
#	@sed "s/o:/occ:/" depend.mk.tmp >> depend.mk
#	@sed "s/o:/op:/" depend.mk.tmp >> depend.mk
#	@sed "s/o:/od:/" depend.mk.tmp >> depend.mk
#	@sed "s/o:/or:/" depend.mk.tmp >> depend.mk
#	@rm depend.mk.tmp

#-include depend.mk

#@echo "headers $(CHDRS)"
#@echo "sources $(CSRCS)"
#@echo "objects $(COBJS)"
