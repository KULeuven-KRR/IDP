dnl @synopsis AC_PROG_PERL_VERSION(VERSION, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
dnl
dnl Makes sure that perl supports the version indicated. If true the shell
dnl commands in ACTION-IF-TRUE are executed.  If not the shell commands in
dnl ACTION-IF-FALSE are run.   Note if $PERL is not set (for example by
dnl running AC_CHECK_PROG or AC_PATH_PROG), AC_CHECK_PROG(PERL, perl, perl) will
dnl be run.
dnl
dnl Example:
dnl
dnl   AC_PROG_PERL_VERSION(5.6.0)
dnl
dnl This will check to make sure that the perl you have supports at least
dnl version 5.6.0.
dnl
dnl @version $Id: ac_prog_perl_version.m4,v 1.1 2006/05/08 12:07:07 topic Exp $
dnl @author Dean Povey <povey@wedgetail.com>
dnl
AC_DEFUN([AC_PROG_PERL_VERSION],[dnl
# Make sure we have perl
if test -z "$PERL"; then
AC_PATH_PROG(PERL,perl,perl)
fi

# Check if version of Perl is sufficient
ac_perl_version="$1"

if test "x$PERL" != "x"; then
  AC_MSG_CHECKING(for perl version greater than or equal to $ac_perl_version)
  # NB: It would be nice to log the error if there is one, but we cannot rely
  # on autoconf internals
  $PERL -e "use $ac_perl_version;" > /dev/null 2>&1
  if test $? -ne 0; then
    AC_MSG_RESULT(no);
    $3
  else
    AC_MSG_RESULT(ok);
    $2
  fi
else
  AC_MSG_WARN(could not find perl)
fi
])dnl

