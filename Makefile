# Compiler
GPP=g++
GPPFLAGS=-O3 -D NDEBUG -Wall -pedantic

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
LEX=lex
GRAMMAR=parse
SOURCES=vocabulary.o structure.o term.o theory.o namespace.o insert.o error.o common.o execute.o builtin.o visitor.o ground.o ecnf.o
MAIN=main
OBJECTS=$(GRAMMAR).tab.o $(LEX).yy.o $(SOURCES) $(MAIN).o
OUTPUT=gidl

# stuff to make
# ############################################################

all: gidl

gidl: $(OBJECTS)
	$(GPP) $(GPPFLAGS) -o $(OUTPUT) $(OBJECTS)

clean:
	-rm -f $(LEX).yy.* $(DATALEX).yy.* $(OUTPUT) $(GRAMMAR).tab.cpp $(GRAMMAR).output \
		$(GRAMMAR).tab.hpp $(OBJECTS)

# separate rule for this, because of ".yy.c" instead of ".yy.cc" --> see (*)
%.o:	%.cpp
	$(GPP) $(GPPFLAGS) -c -o $@ $<

%.tab.cpp:%.yy
	$(YACC) $(YACCFLAGS) --output=$@ $<

%.yy.cpp:	%.ll
	$(FLEX) $(FLEXFLAGS) -o$@ $<
