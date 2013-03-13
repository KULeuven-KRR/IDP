## Process this file with automake to produce Makefile.in

## Options we pass to automake
##
## * foreign means we are not a GNU package, so Automake is less
## strict about checking some GNU automake packaging issues.
##
## * subdir-objects means put object files in respective
## subdirectories in build directory.
AUTOMAKE_OPTIONS = foreign subdir-objects

## Tell ac local to pick up the m4 macros from autoconf-macros directory
ACLOCAL_FLAGS=-I m4

AM_CPPFLAGS= \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/SPASS 

BUILT_SOURCES = \
	SPASS/dfgparser.c \
	SPASS/dfgparser.h \
	SPASS/tptpparser.h \
	SPASS/tptpparser.c \
	SPASS/iaparser.c \
	SPASS/iaparser.h

BISON_FLAGS = -d

SPASS/dfgparser.c: SPASS/dfgparser.y
	$(BISON) $(BISON_FLAGS) -p dfg_ -o SPASS/dfgparser.c $(srcdir)/SPASS/dfgparser.y

SPASS/dfgparser.h: SPASS/dfgparser.c

SPASS/tptpparser.c: SPASS/tptpparser.y
	$(BISON) $(BISON_FLAGS) -p tptp_ -o SPASS/tptpparser.c $(srcdir)/SPASS/tptpparser.y

SPASS/tptpparser.h: SPASS/tptpparser.c

SPASS/iaparser.c: SPASS/iaparser.y
	$(BISON) $(BISON_FLAGS) -p ia_ -o SPASS/iaparser.c $(srcdir)/SPASS/iaparser.y

SPASS/iaparser.h: SPASS/iaparser.c

FLEX_FLAGS = -Ce

SPASS/dfgscanner.c: SPASS/dfgscanner.l
	$(FLEX) $(FLEX_FLAGS) -Pdfg_ -oSPASS/dfgscanner.c $(srcdir)/SPASS/dfgscanner.l

SPASS/tptpscanner.c: SPASS/tptpscanner.l
	$(FLEX) $(FLEX_FLAGS) -Ptptp_ -oSPASS/tptpscanner.c $(srcdir)/SPASS/tptpscanner.l

SPASS/iascanner.c: SPASS/iascanner.l
	$(FLEX) $(FLEX_FLAGS) -Pia_ -oSPASS/iascanner.c $(srcdir)/SPASS/iascanner.l


## List of all scripts created by SPASS
## To add a new script, append its name to this variable.
bin_SCRIPTS  = \
        scripts/dfg2otter.pl \
	scripts/FLOTTER


## List of all programs in SPASS
## To add a new program, append its name to this variable.
bin_PROGRAMS = \
        SPASS/SPASS \
        SPASS/dfg2otter \
        SPASS/dfg2tptp \
	SPASS/tptp2dfg \
        SPASS/dfg2ascii \
        SPASS/dfg2dfg 

## Placeholder variable for the large codebase
## that is shared between the different programs.
BASE = \
        SPASS/clause.c \
        SPASS/clause.h \
        SPASS/cmdline.c \
        SPASS/cmdline.h \
        SPASS/context.c \
        SPASS/context.h \
	SPASS/description.c \
        SPASS/description.h \
        SPASS/dfg.h \
        SPASS/dfgparser.c \
        SPASS/dfgparser.h \
        SPASS/dfgscanner.c \
        SPASS/tptp.h \
        SPASS/tptpparser.c \
        SPASS/tptpparser.h \
        SPASS/tptpscanner.c \
	SPASS/eml.c \
	SPASS/eml.h \
        SPASS/flags.c \
        SPASS/flags.h \
        SPASS/foldfg.c \
        SPASS/foldfg.h \
	SPASS/hashmap.c \
	SPASS/hashmap.h \
        SPASS/kbo.c \
        SPASS/kbo.h \
        SPASS/list.c \
        SPASS/list.h \
        SPASS/memory.c \
        SPASS/memory.h \
        SPASS/misc.c \
        SPASS/misc.h \
        SPASS/order.c \
        SPASS/order.h \
        SPASS/rpos.c \
        SPASS/rpos.h \
        SPASS/sharing.c \
        SPASS/sharing.h \
        SPASS/st.c \
        SPASS/st.h \
        SPASS/stack.c \
        SPASS/stack.h \
        SPASS/strings.c \
        SPASS/strings.h \
        SPASS/subst.c \
        SPASS/subst.h \
        SPASS/symbol.c \
        SPASS/symbol.h \
        SPASS/term.c \
        SPASS/term.h \
        SPASS/unify.c \
        SPASS/unify.h \
	SPASS/version.h

## Sources for dfg2otter
SPASS_dfg2otter_SOURCES = \
        $(BASE) \
        SPASS/dfg2otter.c

## Sources for dfg2tptp
SPASS_dfg2tptp_SOURCES = \
        $(BASE) \
        SPASS/dfg2tptp.c

## Sources for tptp2dfg
SPASS_tptp2dfg_SOURCES = \
        $(BASE) \
        SPASS/tptp2dfg.c

## Sources for dfg2ascii
SPASS_dfg2ascii_SOURCES = \
        $(BASE) \
        SPASS/dfg2ascii.c

## Sources for dfg2dfg
SPASS_dfg2dfg_SOURCES = \
        $(BASE) \
        SPASS/approx.c \
        SPASS/approx.h \
        SPASS/dfg2dfg.c

if COND_WINDOWS
ALARM_C = lib/alarm.c
GETTIMEOFDAY = lib/gettimeofday.c lib/gettimeofday.h
else
ALARM_C = ## SPASS/alarm.c
endif

## Sources for SPASS
SPASS_SPASS_SOURCES = \
        $(BASE) \
        $(ALARM_C) \
	$(GETTIMEOFDAY) \
        SPASS/analyze.c \
        SPASS/analyze.h \
        SPASS/clock.c \
        SPASS/clock.h \
        SPASS/closure.c \
        SPASS/closure.h \
        SPASS/cnf.c \
        SPASS/cnf.h \
        SPASS/component.c \
        SPASS/component.h \
        SPASS/condensing.c \
        SPASS/condensing.h \
        SPASS/defs.c \
        SPASS/defs.h \
        SPASS/doc-proof.c \
        SPASS/doc-proof.h \
        SPASS/graph.c \
        SPASS/graph.h \
        SPASS/hash.c \
        SPASS/hash.h \
        SPASS/hasharray.c \
        SPASS/hasharray.h \
        SPASS/ia.h \
        SPASS/iaparser.c \
        SPASS/iaparser.h \
        SPASS/iascanner.c \
        SPASS/partition.c \
        SPASS/partition.h \
        SPASS/proofcheck.c \
        SPASS/proofcheck.h \
        SPASS/ras.c \
        SPASS/ras.h \
        SPASS/renaming.c \
        SPASS/renaming.h \
        SPASS/resolution.c \
        SPASS/resolution.h \
        SPASS/rules-inf.c \
        SPASS/rules-inf.h \
        SPASS/rules-red.c \
        SPASS/rules-red.h \
        SPASS/rules-sort.c \
        SPASS/rules-sort.h \
        SPASS/rules-split.c \
        SPASS/rules-split.h \
        SPASS/rules-ur.c \
        SPASS/rules-ur.h \
        SPASS/search.c \
        SPASS/search.h \
        SPASS/sort.c \
        SPASS/sort.h \
        SPASS/subsumption.c \
        SPASS/subsumption.h \
        SPASS/table.c \
        SPASS/table.h \
        SPASS/tableau.c \
        SPASS/tableau.h \
        SPASS/terminator.c \
        SPASS/terminator.h \
        SPASS/top.c \
        SPASS/vector.c \
        SPASS/vector.h

if COND_WINDOWS
SPASS_SPASS_LDADD = \
        -L$(top_builddir)/lib/ \
        -lgnu \
	-lwinmm
endif

scripts/FLOTTER: SPASS/SPASS
	$(RM) scripts/FLOTTER
	$(LN_S) SPASS/SPASS scripts/FLOTTER

if COND_WINDOWS
# We don't set the values for the prefix on Windows, since we use a 
# flat file hierarchy there.
else
QMAKE_DOC_PARAMETERS="DEFINES += HELP_PREFIX=\\\"\\\\\\\"$(datadir)/spass/\\\\\\\"\\\""
QMAKE_BIN_PARAMETERS="DEFINES += SPASS_BIN_PREFIX=\\\"\\\\\\\"$(bindir)/\\\\\\\"\\\"" 
endif

if COND_GUI
QT_GENERATED=gui/Makefile.qmake
BUILT_SOURCES += $(QT_GENERATED)

QMAKE_BUILT_PROGRAM=SPASSDesktop
QMAKE_PROJECT=$(QMAKE_BUILT_PROGRAM).pro

gui/Makefile.qmake: gui/$(QMAKE_PROJECT)
	(mkdir -p gui && cd gui && $(QMAKE) $(QMAKE_DOC_PARAMETERS) $(QMAKE_BIN_PARAMETERS) ../$(srcdir)/gui/$(QMAKE_PROJECT) -o Makefile.qmake)

all-local: 
	$(MAKE) -C gui -f Makefile.qmake $(AM_MAKEFLAGS) all

gui-clean-local: 
	$(MAKE) -C gui -f Makefile.qmake $(AM_MAKEFLAGS) clean

distclean-local: 
	$(RM) $(QT_GENERATED)
	$(RM) gui/$(QMAKE_BUILT_PROGRAM)$(EXEEXT)

# Assuming the package knows how to install itself
install-data-local: 
	$(MAKE) -C gui -f Makefile.qmake $(AM_MAKEFLAGS) install

install-exec-local: 
	$(INSTALL_PROGRAM) gui/$(QMAKE_BUILT_PROGRAM)$(EXEEXT) $(DESTDIR)$(bindir)/$(QMAKE_BUILT_PROGRAM)$(EXEEXT) 

uninstall-local: 
	$(MAKE) -C gui -f Makefile.qmake $(AM_MAKEFLAGS) uninstall
else
gui-clean-local:

endif

# Distribute desktop source files from here.
##SPASSDESKTOP_FILES = \
##        gui/AboutListener.cpp \
##        gui/AboutListener.h \
##        gui/CopyOutputListener.cpp \
##        gui/CopyOutputListener.h \
##        gui/ExitListener.cpp \
##        gui/ExitListener.h \
##        gui/FileListener.cpp \
##        gui/FileListener.h \
##        gui/FontChangedListener.cpp \
##        gui/FontChangedListener.h \
##        gui/HelpListener.cpp \
##        gui/HelpListener.h \
##        gui/KillApplicationListener.cpp \
##        gui/KillApplicationListener.h \
##        gui/Listener.cpp \
##        gui/Listener.h \
##        gui/NewFileListener.cpp \
##        gui/NewFileListener.h \
##        gui/OpenFileListener.cpp \
##        gui/OpenFileListener.h \
##        gui/PDFHelpListener.cpp \
##        gui/PDFHelpListener.h \
##        gui/RunApplicationListener.cpp \
##        gui/RunApplicationListener.h \
##        gui/SPASSAboutBox.cpp \
##        gui/SPASSAboutBox.h \
##        gui/SPASSActions.cpp \
##        gui/SPASSActions.h \
##        gui/SPASSApplications.cpp \
##        gui/SPASSApplications.h \
##        gui/SPASSDesktop.cpp \
##        gui/SPASSDesktop.pro \
##        gui/SPASSDesktopIcon.ico \
##        gui/SPASSDesktopIcon.rc \
##        gui/SPASSDesktopMainWidget.cpp \
##        gui/SPASSDesktopMainWidget.h \
##        gui/SPASSDesktopWidget.cpp \
##        gui/SPASSDesktopWidget.h \
##        gui/SPASSDialogs.cpp \
##        gui/SPASSDialogs.h \
##        gui/SPASSFiles.cpp \
##        gui/SPASSFiles.h \
##	gui/SPASSIcons.qrc \
##        gui/SPASSPreferences.cpp \
##        gui/SPASSPreferences.h \
##        gui/SPASSSplitter.cpp \
##        gui/SPASSSplitter.h \
##        gui/SPASSStatusBar.cpp \
##        gui/SPASSStatusBar.h \
##        gui/SPASSTextEdit.cpp \
##        gui/SPASSTextEdit.h \
##        gui/SPASSToolOptions.cpp \
##        gui/SPASSToolOptions.h \
##        gui/SPASSdefines.h \
##        gui/SaveAsFileListener.cpp \
##        gui/SaveAsFileListener.h \
##        gui/SaveFileListener.cpp \
##        gui/SaveFileListener.h \
##	gui/copy.svg \
##	gui/cut.svg \
##	gui/folder.svg \
##	gui/font.svg \
##	gui/new.svg \
##	gui/off.svg \
##	gui/paste.svg \
##	gui/run.svg \
##	gui/run_asci.svg \
##	gui/run_dfg.svg \
##	gui/save.svg \
##	gui/stop.svg \
##	gui/to_edit.svg \
##	gui/spasslogo.gif
##
## I've split TEXINFOS into two variables,
## as otherwise makeinfo complains about
## being run on single *.texi files where
## the node-tree structure isn't right.

info_TEXINFOS = \
	doc/texinfo/script.texi

info_SPASS_TEXINFOS = \
	doc/texinfo/SPASS.texi \
	doc/texinfo/dfg2otter.pl.texi \
	doc/texinfo/dfg2dfg.texi \
	doc/texinfo/dfg2ascii.texi \
	doc/texinfo/dfg2tptp.texi \
	doc/texinfo/tptp2dfg.texi \
	doc/texinfo/dfg2otter.texi \
	doc/texinfo/spassfaq.texi

htmldir = $(datadir)/spass/html/

html_DATA = \
	doc/html/script.html \
	doc/html/script_1.html \
	doc/html/script_2.html \
	doc/html/script_3.html \
	doc/html/script_4.html \
	doc/html/script_5.html \
	doc/html/script_6.html \
	doc/html/script_7.html \
	doc/html/script_abt.html \
	doc/html/script_ovr.html \
	doc/html/LICENCE.html 

BUILT_SOURCES += $(html_DATA)

pdfdir = $(datadir)/spass/pdf/

pdf_DATA = \
	doc/handbook/handbook-spass.pdf \
	doc/syntax/spass-input-syntax.pdf \
	doc/tutorial/tutorial.pdf

man_MANS = \
	doc/man/SPASS.1 \
	doc/man/dfg2otter.pl.1 \
	doc/man/dfg2dfg.1 \
	doc/man/dfg2ascii.1 \
	doc/man/dfg2otter.1 \
	doc/man/dfg2tptp.1 \
	doc/man/tptp2dfg.1

TEXI2POD = scripts/texi2pod.pl
POD2MAN_FLAGS =  -c SPASS

doc/man/SPASS.1:  doc/texinfo/SPASS.texi
	(mkdir -p doc && \
	mkdir -p doc/man && \
	$(TEXI2POD) $(srcdir)/doc/texinfo/SPASS.texi > doc/man/SPASS.pod && \
	$(POD2MAN) $(POD2MAN_FLAGS) doc/man/SPASS.pod doc/man/SPASS.1 )

doc/man/dfg2ascii.1:  doc/texinfo/dfg2ascii.texi
	(mkdir -p doc && \
	mkdir -p doc/man && \
	$(TEXI2POD) $(srcdir)/doc/texinfo/dfg2ascii.texi > doc/man/dfg2ascii.pod && \
	$(POD2MAN) $(POD2MAN_FLAGS) doc/man/dfg2ascii.pod doc/man/dfg2ascii.1 )

doc/man/dfg2dfg.1:  doc/texinfo/dfg2dfg.texi
	(mkdir -p doc && \
	mkdir -p doc/man && \
	$(TEXI2POD) $(srcdir)/doc/texinfo/dfg2dfg.texi > doc/man/dfg2dfg.pod && \
	$(POD2MAN) $(POD2MAN_FLAGS) doc/man/dfg2dfg.pod doc/man/dfg2dfg.1 )

doc/man/dfg2otter.1:  doc/texinfo/dfg2otter.texi
	(mkdir -p doc && \
	mkdir -p doc/man && \
	$(TEXI2POD) $(srcdir)/doc/texinfo/dfg2otter.texi > doc/man/dfg2otter.pod && \
	$(POD2MAN) $(POD2MAN_FLAGS) doc/man/dfg2otter.pod doc/man/dfg2otter.1 )

doc/man/dfg2otter.pl.1:  doc/texinfo/dfg2otter.pl.texi
	(mkdir -p doc && \
	mkdir -p doc/man && \
	$(TEXI2POD) $(srcdir)/doc/texinfo/dfg2otter.pl.texi > doc/man/dfg2otter.pl.pod && \
	$(POD2MAN) $(POD2MAN_FLAGS) doc/man/dfg2otter.pl.pod doc/man/dfg2otter.pl.1 )

doc/man/dfg2tptp.1:  doc/texinfo/dfg2tptp.texi
	(mkdir -p doc && \
	mkdir -p doc/man && \
	$(TEXI2POD) $(srcdir)/doc/texinfo/dfg2tptp.texi > doc/man/dfg2tptp.pod && \
	$(POD2MAN) $(POD2MAN_FLAGS) doc/man/dfg2tptp.pod doc/man/dfg2tptp.1 )

doc/man/tptp2dfg.1:  doc/texinfo/tptp2dfg.texi
	(mkdir -p doc && \
	mkdir -p doc/man && \
	$(TEXI2POD) $(srcdir)/doc/texinfo/tptp2dfg.texi > doc/man/tptp2dfg.pod && \
	$(POD2MAN) $(POD2MAN_FLAGS) doc/man/tptp2dfg.pod doc/man/tptp2dfg.1 )

man-clean-local:
	$(RM) doc/man/SPASS.pod
	$(RM) doc/man/dfg2ascii.pod 
	$(RM) doc/man/dfg2dfg.pod 
	$(RM) doc/man/dfg2tptp.pod 
	$(RM) doc/man/tptp2dfg.pod 
	$(RM) doc/man/dfg2otter.pod 
	$(RM) doc/man/dfg2otter.pl.pod 
	if test $(top_builddir) != $(srcdir); then \
		$(RM) $(man_MANS); \
	fi

TEXI2HTML_FLAGS=--split=chapter --no-sec-nav --output doc/html/

## Convert texi files to HTML
html-local: $(info_TEXINFOS) $(info_SPASS_TEXINFOS)
	$(TEXI2HTML) $(TEXI2HTML_FLAGS) $(srcdir)/doc/texinfo/script.texi

clean-local: gui-clean-local man-clean-local
	if test $(top_builddir) != $(srcdir); then \
		$(RM) $(html_DATA); \
	fi

EXAMPLES = \
	examples/problem54.dfg 

EXTRA_DIST = \
	$(EXAMPLES) \
	$(info_SPASS_TEXINFOS) \
	$(man_MANS) \
	$(html_DATA) \
	$(pdf_DATA) \
##	$(SPASSDESKTOP_FILES)
        SPASS/dfgparser.y \
        SPASS/dfgscanner.l \
        SPASS/tptpparser.y \
        SPASS/tptpscanner.l \
        SPASS/iaparser.y \
        SPASS/iascanner.l \
	README \
	LICENCE \
	INSTALL \
	VERSIONHISTORY \
	m4/ac_prog_perl_version.m4 \
	scripts/dfg2otter.pl.in \
	scripts/texi2pod.pl.in
