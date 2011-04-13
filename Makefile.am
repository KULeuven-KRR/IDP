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
		commontypes.hpp\
		parseinfo.hpp\
		vocabulary.hpp vocabulary.cpp\
		structure.hpp structure.cpp\
		term.hpp term.cpp\
		theory.hpp theory.cpp\
		options.cpp options.hpp\
		execute.hpp execute.cpp\
		namespace.hpp namespace.cpp\
		parse.yy\
		lex.ll\
		insert.hpp insert.cpp\
		common.hpp common.cpp\
		ecnf.hpp ecnf.cpp\
		ground.hpp ground.cpp\
		main.cpp\
		error.hpp error.cpp\
		clconst.hpp\
		interactive.hpp interactive.cpp\
		print.hpp print.cpp\
		checker.hpp checker.cpp\
		generator.hpp generator.cpp\
		fobdd.hpp fobdd.cpp\
		propagate.hpp propagate.cpp

AM_CXXFLAGS+=-I $(top_srcdir)/pcsolver/src/ 

LIBSRCDIR = $(srcdir)/lib
LIBBLDDIR = $(builddir)/lib

# CALLBACK FRAMEWORK
AM_CXXFLAGS += -I $(LIBSRCDIR)/pluscallback1.7/

dist_doc_DATA = README

data_stddir = $(datadir)/std
data_std_DATA = data/idp_intern.idp data/idp_intern.lua
