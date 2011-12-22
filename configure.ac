# Copyright 2007-2011 Katholieke Universiteit Leuven
# Use of this software is governed by the GNU LGPLv3.0 license
# Written by the KRR research group, K.U.Leuven, Departement
# Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
AC_INIT([gidl],[m4_esyscmd([./m4/version.m4])],[krr@cs.kuleuven.be])
AM_SILENT_RULES([yes])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

#this is not an ideal solution, but it overrides the default cxxflags, so those set later are not invalid
if test -z $CXXFLAGS; then
    CXXFLAGS=" "
fi
AC_PROG_CXX
AC_DECL_YYTEXT
#AC_PROG_LEX
AC_DISABLE_SHARED
AC_PROG_LIBTOOL
AC_PROG_YACC
AC_LANG([C++])

LIBTOOL="$LIBTOOL --silent"

AM_INIT_AUTOMAKE([tar-pax foreign subdir-objects -Wall silent-rules])

#check for the necessary headers
AC_HEADER_STDC
AC_CHECK_HEADERS([\
	algorithm clogno cstdio cstdlib cstring ctime exception float.h \
	fstream inttypes.h iostream limits limits.h list map queue \
	set sstream stack stdexcept stdint.h stdio.h stdlib.h string \
	tr1/memory vector])
AC_TYPE_SIZE_T

# distribute additional compiler and linker flags
AC_SUBST([AC_CXXFLAGS])
AC_SUBST([AC_LDFLAGS])
AC_SUBST([LIBS])

GIDL_VERSION=m4_esyscmd([./m4/version.m4 | tr -d '\n'])
AC_SUBST([GIDL_VERSION])

AC_CXXFLAGS=" "
AC_LDFLAGS=" "

AC_CXXFLAGS+="-Wall -Wextra -pedantic -pthread "

AC_ARG_ENABLE([build],
[  --enable-build    Turn on debugging (debug), profiling (profile), release version(release), static release version (static) or code cover building (codecover)],
[case "${enableval}" in
  debug)   AC_CXXFLAGS+="-O0 -ggdb -D DEBUG " 		AC_LDFLAGS+="-ggdb " ;;
  profile) AC_CXXFLAGS+="-O3 -pg -ggdb -D NDEBUG " 	AC_LDFLAGS+="-ggdb " ;;
  release) AC_CXXFLAGS+="-O3 -D NDEBUG " 			AC_LDFLAGS+="" ;;
  static)  AC_CXXFLAGS+="-O3 -D NDEBUG " 			AC_LDFLAGS+="-static " ;;
  codecover) AC_CXXFLAGS+="-O0 -fprofile-arcs -ftest-coverage -ggdb -D DEBUG " AC_LDFLAGS+="-ggdb " ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-build]) ;;
esac],[		AC_CXXFLAGS+="-DNDEBUG -O3 " AC_LDFLAGS+=" "])

AC_ARG_ENABLE([gmp],
[  --enable-gmp    Turn on support for arbitrary size integer values],
[case "${enableval}" in
	no) AC_CXXFLAGS+=" -D NO_GMP " ;;
	yes) AC_CXXFLAGS+=" -D GMP " AC_LDFLAGS+=" -lgmp -lgmpxx ";;
	*) AC_MSG_ERROR([bad value ${enableval} for --enable-gmp]) ;;
esac],[ AC_CXXFLAGS+=" -D NO_GMP " ])

AC_ARG_ENABLE([sat],
[  --enable-sat    Use minisat 2.0b(old), minisat 09z(hack) or minisat 2.2(new)],
[case "${enableval}" in
  old)   oldm=true
		 hackm=false
  		 newm=false
  		 ;;
  hack)  oldm=false
		 hackm=true
  		 newm=false
  		;;
  new)   oldm=false
		 hackm=false
  		 newm=true
		;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-sat]) ;;
esac],[newm=true])

AM_CONDITIONAL([USEMINISAT],[test x$oldm = xtrue])
AM_CONDITIONAL([USEMINISAT09Z],[test x$hackm = xtrue])
AM_CONDITIONAL([USEMINISAT22],[test x$newm = xtrue])


AC_ARG_ENABLE([bits],
[  --enable-bits    Turn on 32/64 bit compiling],
[case "${enableval}" in
	32) AC_CXXFLAGS+=" -m32 " ;;
	64) AC_CXXFLAGS+=" -m64 ";;
	*) AC_MSG_ERROR([bad value ${enableval} for --enable-bits]) ;;
esac],[])

AC_ARG_ENABLE([cp],
[  --enable-cp    Include constraint programming support [yes/no] ],
[enablegecode=$enableval],[enablegecode=no])

AC_ARG_ENABLE([cpdir],
[  --enable-cpdir    Specify directory in which gecode is installed (libs in dir/lib, headers in dir/include/) ],
[gecodedir=$enableval],[gecodedir=""])

if test "x$gecodedir" != "x"; then
	hasgecodedir="yes"
	enablegecode="yes"
else
	hasgecodedir=no
fi

# Check for gecode
if test "x$enablegecode" = "xyes" -a "x$hasgecodedir" = "xno"; then
	#what function to check for? AC_CHECK_LIB([gecodekernel], [space], [], [enablegecode=no], [])
	AC_CHECK_HEADERS([gecode/kernel.hh gecode/driver.hh gecode/int.hh gecode/minimodel.hh], [], [enablegecode=no])
	if test "x$enablegecode" = "xno"; then
		AC_MSG_WARN([gecode headers could not be found, disabling cp support])
	else
	    AC_CXXFLAGS+=" -D GECODESUPPORT "
	fi
fi

if test "x$enablegecode" = "xyes" -a "x$hasgecodedir" = "xyes"; then
	echo "Expecting gecode directory to be correct, not verifying"
	AC_CXXFLAGS+=" -D GECODESUPPORT "
fi

AC_ARG_ENABLE([tests],
[  --enable-tests    Also compile in unit-tests (if you have an appropriate google-test library)],
[case "${enableval}" in
	yes) enabletests=yes ;;
	no) enabletests=no ;;
	*) AC_MSG_ERROR([bad value ${enableval} for --enable-tests]) ;;
esac],[ tests=yes ])

AC_ARG_WITH(
  [interactive],
  [AC_HELP_STRING([--with-interactive], [Compile with support for interactive usage [default=yes]])],
  [interactive=$withval],
  [interactive=yes]
)

AM_CONDITIONAL([BUILDTESTS],[test x$enabletests = xyes])
AM_CONDITIONAL([USEINTERACTIVE],[test x$interactive = xyes])

AC_CONFIG_SUBDIRS([lib/lua lib/minisatid])
AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile lib/Makefile src/parser/Makefile lib/tinythread/Makefile lib/tinythread/source/Makefile lib/linenoise/Makefile])

AC_OUTPUT
