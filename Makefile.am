SUBDIRS = lib pcsolver
AM_CXXFLAGS = @AC_CXXFLAGS@ -Wall -Wextra\
		-std=c++0x\
		-I$(srcdir)/lib/lua/etc -I$(srcdir)/lib/lua/src\
		-I$(builddir)/lib/lua/src  -I $(srcdir)/lib/\
		-DDATADIR=\"$(datadir)\" -DRCDIR=\"$(localstatedir)\"\
		-DINTERNALLIBARYNAME=\"idp_intern\" -DINTERNALLIBARYLUA=\"/std/idp_intern.lua\"\
		-DINTERNALLIBARYIDP=\"/std/idp_intern.idp\" -DCONFIGFILENAME=\"/idp/.idprc\"
		
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
		luaconnection.hpp luaconnection.cpp\
		internalargument.hpp internalargument.cpp\
		namespace.hpp namespace.cpp\
		parse.yy yyltype.hpp\
		lex.ll\
		clconst.hpp\
		insert.hpp insert.cpp\
		error.hpp error.cpp\
		common.hpp common.cpp\
		interactive.hpp interactive.cpp\
		main.cpp\
		printers/print.hpp printers/print.cpp\
		printers/idpprinter.hpp printers/ecnfprinter.hpp\
		checker.hpp checker.cpp\
		generator.hpp generator.cpp\
		propagate.hpp propagate.cpp\
		ecnf.hpp ecnf.cpp\
		ground.hpp ground.cpp\
		fobdd.hpp fobdd.cpp\
        symmetry.hpp symmetry.cpp\
		monitors/interactiveprintmonitor.hpp monitors/tracemonitor.hpp\
		monitors/luatracemonitor.hpp monitors/luainteractiveprintmonitor.hpp\
		groundtheories/PrintGroundPolicy.hpp\
		groundtheories/GroundPolicy.hpp\
		groundtheories/SolverPolicy.hpp\
		groundtheories/AbstractGroundTheory.hpp\
		commands/allcommands.hpp\
		commands/optimization.hpp\
		commands/commandinterface.hpp\
		commands/printblocks.hpp\
		commands/printdomainatom.hpp\
		commands/printoptions.hpp\
		commands/newstructure.hpp\
		commands/newtheory.hpp\
		commands/modelexpand.hpp\
		commands/newoptions.hpp\
		commands/clonestructure.hpp\
		commands/clonetheory.hpp\
		commands/pushnegations.hpp\
		commands/mergetheories.hpp\
		commands/flatten.hpp\
		commands/idptype.hpp\
		commands/setatomvalue.hpp\
		commands/settablevalue.hpp\
		commands/domainiterator.hpp\
		commands/tableiterator.hpp\
		commands/propagate.hpp\
		commands/query.hpp\
		commands/createtuple.hpp\
		commands/ground.hpp\
		commands/completion.hpp\
		commands/tobdd.hpp\
		commands/clean.hpp\
		commands/help.hpp\
		commands/changevocabulary.hpp\
		commands/estimatecosts.hpp\
		commands/detectsymmetry.hpp\
		commands/derefandincrement.hpp\
		commands/simplify.hpp\
		commands/createrange.hpp

AM_CXXFLAGS+=-I $(top_srcdir)/pcsolver/src/ 

LIBSRCDIR = $(srcdir)/lib
LIBBLDDIR = $(builddir)/lib

# CALLBACK FRAMEWORK
AM_CXXFLAGS += -I $(LIBSRCDIR)/pluscallback1.7/

data_stddir = $(datadir)/std

data_std_DATA = data/idp_intern.idp data/idp_intern.lua data/mx.idp data/theory.idp data/domain.idp

data_localdir = $(localstatedir)/idp
data_local_DATA = data/.idprc

dist_doc_DATA = README $(data_std_DATA) $(data_local_DATA)
