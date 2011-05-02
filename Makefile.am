SUBDIRS = lib pcsolver
AM_CXXFLAGS = @AC_CXXFLAGS@ -Wall -Wextra\
		-I$(srcdir)/lib/lua/etc -I$(srcdir)/lib/lua/src\
		-I$(builddir)/lib/lua/src  -I $(srcdir)/lib/\
		-DDATADIR=\"$(datadir)\" -DRCDIR=\"$(localstatedir)\"
		
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
		parseinfo.hpp parseinfo.cpp\
		vocabulary.hpp vocabulary.cpp\
		structure.hpp structure.cpp\
		term.hpp term.cpp\
		theory.hpp theory.cpp\
		options.cpp options.hpp\
		execute.hpp execute.cpp\
		namespace.hpp namespace.cpp\
		parse.yy\
		lex.ll\
		clconst.hpp\
		insert.hpp insert.cpp\
		error.hpp error.cpp\
		common.hpp common.cpp\
		interactive.hpp interactive.cpp\
		main.cpp\
		print.hpp print.cpp\
		checker.hpp checker.cpp\
		generator.hpp generator.cpp\
		propagate.hpp propagate.cpp\
		ecnf.hpp ecnf.cpp\
		ground.hpp ground.cpp\
		fobdd.hpp fobdd.cpp

AM_CXXFLAGS+=-I $(top_srcdir)/pcsolver/src/ 

LIBSRCDIR = $(srcdir)/lib
LIBBLDDIR = $(builddir)/lib

# CALLBACK FRAMEWORK
AM_CXXFLAGS += -I $(LIBSRCDIR)/pluscallback1.7/

dist_doc_DATA = README

data_stddir = $(datadir)/std
data_std_DATA = data/idp_intern.idp data/idp_intern.lua data/mx.idp data/theory.idp data/domain.idp

data_localdir = $(localstatedir)/idp
data_local_DATA = data/.idprc
