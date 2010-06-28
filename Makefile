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

COBJS	?= $(addsuffix .o, $(basename $(CSRCS)))

EXEC		?= $(notdir $(shell pwd))

CFLAGS	?= $(GPPFLAGS)

## Compile options
%.o:		CFLAGS +=$(COPTIMIZE) -ggdb -D DEBUG

## Link options
$(EXEC):					LFLAGS := -ggdb $(LFLAGS)

## Dependencies
$(EXEC):			$(COBJS)

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

## Clean rule
clean:
	@rm -f $(EXEC) $(EXEC)_codecover $(EXEC)_profile $(EXEC)_debug $(EXEC)_release $(EXEC)_static \
		$(PSRCS) $(PHDRS) $(COBJS) $(CCCOBJS) $(PCOBJS) $(DCOBJS) $(RCOBJS) *.core depend.mk lib$(LIB).a lib$(LIB)d.a
