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

gidl_LDADD = $(builddir)/lib/lua/src/.libs/liblua.la
gidl_LDADD += $(builddir)/pcsolver/src/libminisatid-2.0.la


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
		options.cpp options.hpp\
		clconst.hpp\
		interactive.cpp interactive.hpp\
		print.hpp print.cpp\
		checker.cpp checker.hpp\
		generator.cpp generator.hpp\
		fobdd.cpp fobdd.hpp

AM_CXXFLAGS+=-I $(top_srcdir)/pcsolver/src/ 

dist_doc_DATA = README

data_stddir = $(datadir)/std
data_std_DATA = data/idp_intern.idp data/idp_intern.lua
