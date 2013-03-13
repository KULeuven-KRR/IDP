dnl Process this file with autoconf to produce a configure script.

AC_REVISION($Revision: 1.4 $)
AC_INIT
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([m4])
AC_CONFIG_SRCDIR([SPASS/analyze.c])
AM_INIT_AUTOMAKE([SPASS], [3.7])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB
AM_PROG_CC_C_O

if test -n "$GCC"; then
  CFLAGS="-O3 -DSPASS_SIGNALS=1 -D__inline__="
else
  CFLAGS="-D__inline__="
fi
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# don't use yacc, use bison instead, since we need to have two parsers
# in a single executable.
AC_CHECK_PROGS([BISON], [bison])

# don't use lex, use flex instead, since we need to have two scanners
# in a single executable.
AC_CHECK_PROGS([FLEX], [flex])

# Find an appropriate tar for use in "dist" targets.  A "best guess"
# is good enough -- if we can't find GNU tar, we don't really care.
AC_CHECK_PROGS([TAR], [gnutar gtar tar])

AC_ARG_ENABLE(gui,
[  --enable-gui            Build SPASS desktop gui.])

AM_CONDITIONAL([COND_GUI], [test "$enable_gui" = yes])

AC_ARG_ENABLE(windows,
[  --enable-windows        Build SPASS for windows.])

# turn off cygwin DLL for SPASS builds on windows
if test "$enable_windows" = yes ; then
   CFLAGS="$(CFLAGS) -mno-cygwin"
fi

AM_CONDITIONAL([COND_WINDOWS], [test "$enable_windows" = yes])

# If GUI needs to be built check Qt support
if test "$enable_gui" = yes; then
   AC_CHECK_PROGS([QMAKE],[qmake])
fi

# Minimum required perl version for development: 5.0004
AC_PROG_PERL_VERSION([5.0004])

# Search for texi2html
AC_PATH_PROG(TEXI2HTML, texi2html)
if test -z "$TEXI2HTML"; then
  AC_MSG_WARN([texi2html not found])
fi

# Search for pod2man
AC_PATH_PROG(POD2MAN, pod2man)
if test -z "$POD2MAN"; then
  AC_MSG_WARN([pod2man not found])
fi

dnl Checks for libraries.
AC_CHECK_LIB(m, log10)

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h malloc.h mmsystem.h unistd.h windows.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

dnl Checks for library functions.
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([gettimeofday time strtol strtoul])

AC_CONFIG_HEADERS([SPASS/config.h])
AC_CONFIG_FILES([Makefile \
scripts/dfg2otter.pl \
scripts/texi2pod.pl])
AC_CONFIG_COMMANDS([dfg2otter],[[chmod +x scripts/dfg2otter.pl]],[[]])
AC_CONFIG_COMMANDS([texi2pod],[[chmod +x scripts/texi2pod.pl]],[[]])
AC_OUTPUT
