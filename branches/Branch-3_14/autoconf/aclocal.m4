# Search path for a program which passes the given test.
# Ulrich Drepper <drepper@cygnus.com>, 1996.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.

# Define a conditional.

AC_DEFUN(AM_CONDITIONAL,
[AC_SUBST($1_TRUE)
AC_SUBST($1_FALSE)
if $2; then
  $1_TRUE=
  $1_FALSE='#'
else
  $1_TRUE='#'
  $1_FALSE=
fi])

# serial 1

dnl AM_PATH_PROG_WITH_TEST(VARIABLE, PROG-TO-CHECK-FOR,
dnl   TEST-PERFORMED-ON-FOUND_PROGRAM [, VALUE-IF-NOT-FOUND [, PATH]])
AC_DEFUN(AM_PATH_PROG_WITH_TEST,
[# Extract the first word of "$2", so it can be a program name with args.
set dummy $2; ac_word=[$]2
AC_MSG_CHECKING([for $ac_word])
AC_CACHE_VAL(ac_cv_path_$1,
[case "[$]$1" in
  /*)
  ac_cv_path_$1="[$]$1" # Let the user override the test with a path.
  ;;
  *)
  IFS="${IFS=   }"; ac_save_ifs="$IFS"; IFS="${IFS}:"
  for ac_dir in ifelse([$5], , $PATH, [$5]); do
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/$ac_word; then
      if [$3]; then
        ac_cv_path_$1="$ac_dir/$ac_word"
        break
      fi
    fi
  done
  IFS="$ac_save_ifs"
dnl If no 4th arg is given, leave the cache variable unset,
dnl so AC_PATH_PROGS will keep looking.
ifelse([$4], , , [  test -z "[$]ac_cv_path_$1" && ac_cv_path_$1="$4"
])dnl
  ;;
esac])dnl
$1="$ac_cv_path_$1"
if test -n "[$]$1"; then
  AC_MSG_RESULT([$]$1)
else
  AC_MSG_RESULT(no)
fi
AC_SUBST($1)dnl
])

# Check whether LC_MESSAGES is available in <locale.h>.
# Ulrich Drepper <drepper@cygnus.com>, 1995.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.

# serial 1

AC_DEFUN(AM_LC_MESSAGES,
  [if test $ac_cv_header_locale_h = yes; then
    AC_CACHE_CHECK([for LC_MESSAGES], am_cv_val_LC_MESSAGES,
      [AC_TRY_LINK([#include <locale.h>], [return LC_MESSAGES],
       am_cv_val_LC_MESSAGES=yes, am_cv_val_LC_MESSAGES=no)])
    if test $am_cv_val_LC_MESSAGES = yes; then
      AC_DEFINE(HAVE_LC_MESSAGES)
    fi
  fi])

# Macro to add for using GNU gettext.
# Ulrich Drepper <drepper@cygnus.com>, 1995.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.
# NOTE: This version has been hacked to reverse the obnoxious 'yes' default.

# serial 5

AC_DEFUN(AM_WITH_NLS,
  [AC_MSG_CHECKING([whether NLS is requested])
    dnl Default is disable NLS
    AC_ARG_ENABLE(nls,
      [AC_HELP_STRING([--enable-nls], [enable Native Language Support])],
      USE_NLS=$enableval, USE_NLS=no)
    AC_MSG_RESULT($USE_NLS)
    AC_SUBST(USE_NLS)

    USE_INCLUDED_LIBINTL=no

    dnl If we use NLS figure out what method
    if test "$USE_NLS" = "yes"; then
      AC_DEFINE(ENABLE_NLS)
      AC_MSG_CHECKING([whether included gettext is requested])
      AC_ARG_WITH(included-gettext,
        [  --with-included-gettext use the GNU gettext library included here],
        nls_cv_force_use_gnu_gettext=$withval,
        nls_cv_force_use_gnu_gettext=no)
      AC_MSG_RESULT($nls_cv_force_use_gnu_gettext)

      nls_cv_use_gnu_gettext="$nls_cv_force_use_gnu_gettext"
      if test "$nls_cv_force_use_gnu_gettext" != "yes"; then
        dnl User does not insist on using GNU NLS library.  Figure out what
        dnl to use.  If gettext or catgets are available (in this order) we
        dnl use this.  Else we have to fall back to GNU NLS library.
        dnl catgets is only used if permitted by option --with-catgets.
        nls_cv_header_intl=
        nls_cv_header_libgt=
        CATOBJEXT=NONE

        AC_CHECK_HEADER(libintl.h,
          [AC_CACHE_CHECK([for gettext in libc], gt_cv_func_gettext_libc,
            [AC_TRY_LINK([#include <libintl.h>], [return (int) gettext ("")],
               gt_cv_func_gettext_libc=yes, gt_cv_func_gettext_libc=no)])

           if test "$gt_cv_func_gettext_libc" != "yes"; then
             AC_CHECK_LIB(intl, bindtextdomain,
               [AC_CACHE_CHECK([for gettext in libintl],
                 gt_cv_func_gettext_libintl,
                 [AC_CHECK_LIB(intl, gettext,
                  gt_cv_func_gettext_libintl=yes,
                  gt_cv_func_gettext_libintl=no)],
                 gt_cv_func_gettext_libintl=no)])
           fi

           if test "$gt_cv_func_gettext_libc" = "yes" \
              || test "$gt_cv_func_gettext_libintl" = "yes"; then
              AC_DEFINE(HAVE_GETTEXT)
              AM_PATH_PROG_WITH_TEST(MSGFMT, msgfmt,
                [test -z "`$ac_dir/$ac_word -h 2>&1 | grep 'dv '`"], no)dnl
              if test "$MSGFMT" != "no"; then
                AC_CHECK_FUNCS(dcgettext)
                AC_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)
                AM_PATH_PROG_WITH_TEST(XGETTEXT, xgettext,
                  [test -z "`$ac_dir/$ac_word -h 2>&1 | grep '(HELP)'`"], :)
                AC_TRY_LINK(, [extern int _nl_msg_cat_cntr;
                               return _nl_msg_cat_cntr],
                  [CATOBJEXT=.gmo
                   DATADIRNAME=share],
                  [CATOBJEXT=.mo
                   DATADIRNAME=lib])
                INSTOBJEXT=.mo
              fi
            fi
        ])

        if test "$CATOBJEXT" = "NONE"; then
          AC_MSG_CHECKING([whether catgets can be used])
          AC_ARG_WITH(catgets,
            [  --with-catgets          use catgets functions if available],
            nls_cv_use_catgets=$withval, nls_cv_use_catgets=no)
          AC_MSG_RESULT($nls_cv_use_catgets)

          if test "$nls_cv_use_catgets" = "yes"; then
            dnl No gettext in C library.  Try catgets next.
            AC_CHECK_LIB(i, main)
            AC_CHECK_FUNC(catgets,
              [AC_DEFINE(HAVE_CATGETS)
               INTLOBJS="\$(CATOBJS)"
               AC_PATH_PROG(GENCAT, gencat, no)dnl
               if test "$GENCAT" != "no"; then
                 AC_PATH_PROG(GMSGFMT, gmsgfmt, no)
                 if test "$GMSGFMT" = "no"; then
                   AM_PATH_PROG_WITH_TEST(GMSGFMT, msgfmt,
                    [test -z "`$ac_dir/$ac_word -h 2>&1 | grep 'dv '`"], no)
                 fi
                 AM_PATH_PROG_WITH_TEST(XGETTEXT, xgettext,
                   [test -z "`$ac_dir/$ac_word -h 2>&1 | grep '(HELP)'`"], :)
                 USE_INCLUDED_LIBINTL=yes
                 CATOBJEXT=.cat
                 INSTOBJEXT=.cat
                 DATADIRNAME=lib
                 INTLDEPS='$(topdir)/src/intl/libintl.a'
                 INTLINCLUDE='-I$(topdir)/src/intl'
                 INTLLIBS=$INTLDEPS
                 LIBS=`echo $LIBS | sed -e 's/-lintl//'`
                 nls_cv_header_intl=$srcdir/include/libintl.h
                 nls_cv_header_libgt=$srcdir/src/intl/libgettext.h
               fi])
          fi
        fi

        if test "$CATOBJEXT" = "NONE"; then
          dnl Neither gettext nor catgets in included in the C library.
          dnl Fall back on GNU gettext library.
          nls_cv_use_gnu_gettext=yes
        fi
      fi

      if test "$nls_cv_use_gnu_gettext" = "yes"; then
        dnl Mark actions used to generate GNU NLS library.
        INTLOBJS="\$(GETTOBJS)"
        AM_PATH_PROG_WITH_TEST(MSGFMT, msgfmt,
          [test -z "`$ac_dir/$ac_word -h 2>&1 | grep 'dv '`"], msgfmt)
        AC_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)
        AM_PATH_PROG_WITH_TEST(XGETTEXT, xgettext,
          [test -z "`$ac_dir/$ac_word -h 2>&1 | grep '(HELP)'`"], :)
        AC_SUBST(MSGFMT)
        USE_INCLUDED_LIBINTL=yes
        CATOBJEXT=.gmo
        INSTOBJEXT=.mo
        DATADIRNAME=share
        INTLDEPS='$(topdir)/src/intl/libintl.a'
        INTLINCLUDE='-I$(topdir)/src/intl'
        INTLLIBS=$INTLDEPS
        LIBS=`echo $LIBS | sed -e 's/-lintl//'`
        nls_cv_header_intl=$srcdir/include/libintl.h
        nls_cv_header_libgt=$srcdir/src/intl/libgettext.h
      fi

      dnl Test whether we really found GNU xgettext.
      if test "$XGETTEXT" != ":"; then
        dnl If it is no GNU xgettext we define it as : so that the
        dnl Makefiles still can work.
        if $XGETTEXT --omit-header /dev/null 2> /dev/null; then
          : ;
        else
          AC_MSG_RESULT(
            [found xgettext program is not GNU xgettext; ignore it])
          XGETTEXT=":"
        fi
      fi

      # We need to process the po/ directory.
      POSUB=po
    else
      DATADIRNAME=share
      nls_cv_header_intl=$srcdir/include/libintl.h
      nls_cv_header_libgt=$srcdir/src/intl/libgettext.h
    fi
    AC_LINK_FILES($nls_cv_header_libgt, $nls_cv_header_intl)
    AC_OUTPUT_COMMANDS(
     [case "$CONFIG_FILES" in *po/Makefile.in*)
        sed -e "/POTFILES =/r src/po/POTFILES" src/po/Makefile.in > src/po/Makefile
      esac])


    # If this is used in GNU gettext we have to set USE_NLS to `yes'
    # because some of the sources are only built for this goal.
    if test "$PACKAGE" = "gettext" ; then
      USE_NLS=yes
      USE_INCLUDED_LIBINTL=yes
    fi

    dnl These rules are solely for the distribution goal.  While doing this
    dnl we only have to keep exactly one list of the available catalogs
    dnl in configure.in.
    for lang in $ALL_LINGUAS; do
      GMOFILES="$GMOFILES $lang.gmo"
      POFILES="$POFILES $lang.po"
    done

    dnl Make all variables we use known to autoconf.
    AC_SUBST(USE_INCLUDED_LIBINTL)
    AC_SUBST(CATALOGS)
    AC_SUBST(CATOBJEXT)
    AC_SUBST(DATADIRNAME)
    AC_SUBST(GMOFILES)
    AC_SUBST(INSTOBJEXT)
    AC_SUBST(INTLDEPS)
    AC_SUBST(INTLLIBS)
    AC_SUBST(INTLINCLUDE)
    AC_SUBST(INTLOBJS)
    AC_SUBST(POFILES)
    AC_SUBST(POSUB)
  ])

AC_DEFUN(AM_GNU_GETTEXT,
  [AC_REQUIRE([AC_PROG_MAKE_SET])dnl
   AC_REQUIRE([AC_PROG_CC])dnl
   AC_REQUIRE([AC_PROG_RANLIB])dnl
   AC_REQUIRE([AC_ISC_POSIX])dnl
   AC_REQUIRE([AC_HEADER_STDC])dnl
   AC_REQUIRE([AC_C_CONST])dnl
   AC_REQUIRE([AC_C_INLINE])dnl
   AC_REQUIRE([AC_TYPE_OFF_T])dnl
   AC_REQUIRE([AC_TYPE_SIZE_T])dnl
   AC_REQUIRE([AC_FUNC_ALLOCA])dnl
   AC_REQUIRE([AC_FUNC_MMAP])dnl

   AC_CHECK_HEADERS([argz.h limits.h locale.h nl_types.h malloc.h string.h \
unistd.h sys/param.h])
   AC_CHECK_FUNCS([getcwd munmap putenv setenv setlocale strchr strcasecmp \
strdup __argz_count __argz_stringify __argz_next])

   if test "${ac_cv_func_stpcpy+set}" != "set"; then
     AC_CHECK_FUNCS(stpcpy)
   fi
   if test "${ac_cv_func_stpcpy}" = "yes"; then
     AC_DEFINE(HAVE_STPCPY)
   fi

   AM_LC_MESSAGES
   AM_WITH_NLS

   if test "x$CATOBJEXT" != "x"; then
     if test "x$ALL_LINGUAS" = "x"; then
       LINGUAS=
     else
       AC_MSG_CHECKING(for catalogs to be installed)
       NEW_LINGUAS=
       for lang in ${LINGUAS=$ALL_LINGUAS}; do
         case "$ALL_LINGUAS" in
          *$lang*) NEW_LINGUAS="$NEW_LINGUAS $lang" ;;
         esac
       done
       LINGUAS=$NEW_LINGUAS
       AC_MSG_RESULT($LINGUAS)
     fi

     dnl Construct list of names of catalog files to be constructed.
     if test -n "$LINGUAS"; then
       for lang in $LINGUAS; do CATALOGS="$CATALOGS $lang$CATOBJEXT"; done
     fi
   fi

   dnl The reference to <locale.h> in the installed <libintl.h> file
   dnl must be resolved because we cannot expect the users of this
   dnl to define HAVE_LOCALE_H.
   if test $ac_cv_header_locale_h = yes; then
     INCLUDE_LOCALE_H="#include <locale.h>"
   else
     INCLUDE_LOCALE_H="\
/* The system does not provide the header <locale.h>.  Take care yourself.  */"
   fi
   AC_SUBST(INCLUDE_LOCALE_H)

   dnl Determine which catalog format we have (if any is needed)
   dnl For now we know about two different formats:
   dnl   Linux libc-5 and the normal X/Open format
   test -d $srcdir/src/intl || mkdir $srcdir/src/intl
   if test "$CATOBJEXT" = ".cat"; then
     AC_CHECK_HEADER(linux/version.h, msgformat=linux, msgformat=xopen)

     dnl Transform the SED scripts while copying because some dumb SEDs
     dnl cannot handle comments.
     sed -e '/^#/d' $srcdir/src/intl/$msgformat-msg.sed > $srcdir/src/intl/po2msg.sed
   fi
   dnl po2tbl.sed is always needed.
   sed -e '/^#.*[^\\]$/d' -e '/^#$/d' \
     $srcdir/src/intl/po2tbl.sed.in > $srcdir/src/intl/po2tbl.sed

   dnl In the src/intl/Makefile.in we have a special dependency which makes
   dnl only sense for gettext.  We comment this out for non-gettext
   dnl packages.
   if test "$PACKAGE" = "gettext"; then
     GT_NO="#NO#"
     GT_YES=
   else
     GT_NO=
     GT_YES="#YES#"
   fi
   AC_SUBST(GT_NO)
   AC_SUBST(GT_YES)


   dnl *** For now the libtool support in src/intl/Makefile is not for real.
   l=
   AC_SUBST(l)

   dnl Generate list of files to be processed by xgettext which will
   dnl be included in src/po/Makefile.
   test -d src/po || mkdir src/po
   if test "x$srcdir" != "x."; then
     if test "x`echo $srcdir | sed 's@/.*@@'`" = "x"; then
       posrcprefix="$srcdir/"
     else
       posrcprefix="../$srcdir/"
     fi
   else
     posrcprefix="../"
   fi
   rm -f src/po/POTFILES
   sed -e "/^#/d" -e "/^\$/d" -e "s,.*, $posrcprefix& \\\\," -e "\$s/\(.*\) \\\\/\1/" \
        < $srcdir/src/po/POTFILES.in > src/po/POTFILES
  ])

AC_DEFUN(AC_TYPE_SOCKETLEN_T,
[ dnl check for socklen_t (in Unix98)
  AC_MSG_CHECKING(for socklen_t)
  AC_TRY_COMPILE([
  #include <sys/types.h>
  #include <sys/socket.h>
  socklen_t x;
  ],[],[AC_MSG_RESULT(yes)],[
  AC_TRY_COMPILE([
  #include <sys/types.h>
  #include <sys/socket.h>
  int accept (int, struct sockaddr *, size_t *);
  ],[],[
  AC_MSG_RESULT(size_t)
  AC_DEFINE(socklen_t,size_t)], [
  AC_MSG_RESULT(int)
  AC_DEFINE(socklen_t,int)])])
])

# pkg.m4 - Macros to locate and utilise pkg-config.            -*- Autoconf -*-
# 
# Copyright © 2004 Scott James Remnant <scott@netsplit.com>.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# As a special exception to the GNU General Public License, if you
# distribute this file as part of a program that contains a
# configuration script generated by Autoconf, you may include it under
# the same distribution terms that you use for the rest of that program.

# PKG_PROG_PKG_CONFIG([MIN-VERSION])
# ----------------------------------
AC_DEFUN([PKG_PROG_PKG_CONFIG],
[m4_pattern_forbid([^_?PKG_[A-Z_]+$])
m4_pattern_allow([^PKG_CONFIG(_PATH)?$])
AC_ARG_VAR([PKG_CONFIG], [path to pkg-config utility])dnl
if test "x$ac_cv_env_PKG_CONFIG_set" != "xset"; then
	AC_PATH_TOOL([PKG_CONFIG], [pkg-config])
fi
if test -n "$PKG_CONFIG"; then
	_pkg_min_version=m4_default([$1], [0.9.0])
	AC_MSG_CHECKING([pkg-config is at least version $_pkg_min_version])
	if $PKG_CONFIG --atleast-pkgconfig-version $_pkg_min_version; then
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
		PKG_CONFIG=""
	fi
		
fi[]dnl
])# PKG_PROG_PKG_CONFIG

# PKG_CHECK_EXISTS(MODULES, [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# Check to see whether a particular set of modules exists.  Similar
# to PKG_CHECK_MODULES(), but does not set variables or print errors.
#
#
# Similar to PKG_CHECK_MODULES, make sure that the first instance of
# this or PKG_CHECK_MODULES is called, or make sure to call
# PKG_CHECK_EXISTS manually
# --------------------------------------------------------------
AC_DEFUN([PKG_CHECK_EXISTS],
[AC_REQUIRE([PKG_PROG_PKG_CONFIG])dnl
if test -n "$PKG_CONFIG" && \
    AC_RUN_LOG([$PKG_CONFIG --exists --print-errors "$1"]); then
  m4_ifval([$2], [$2], [:])
m4_ifvaln([$3], [else
  $3])dnl
fi])


# _PKG_CONFIG([VARIABLE], [COMMAND], [MODULES])
# ---------------------------------------------
m4_define([_PKG_CONFIG],
[if test -n "$PKG_CONFIG"; then
    if test -n "$$1"; then
        pkg_cv_[]$1="$$1"
    else
        PKG_CHECK_EXISTS([$3],
                         [pkg_cv_[]$1=`$PKG_CONFIG --[]$2 "$3" 2>/dev/null`],
			 [pkg_failed=yes])
    fi
else
	pkg_failed=untried
fi[]dnl
])# _PKG_CONFIG

# _PKG_SHORT_ERRORS_SUPPORTED
# -----------------------------
AC_DEFUN([_PKG_SHORT_ERRORS_SUPPORTED],
[AC_REQUIRE([PKG_PROG_PKG_CONFIG])
if $PKG_CONFIG --atleast-pkgconfig-version 0.20; then
        _pkg_short_errors_supported=yes
else
        _pkg_short_errors_supported=no
fi[]dnl
])# _PKG_SHORT_ERRORS_SUPPORTED


# PKG_CHECK_MODULES(VARIABLE-PREFIX, MODULES, [ACTION-IF-FOUND],
# [ACTION-IF-NOT-FOUND])
#
#
# Note that if there is a possibility the first call to
# PKG_CHECK_MODULES might not happen, you should be sure to include an
# explicit call to PKG_PROG_PKG_CONFIG in your configure.ac
#
#
# --------------------------------------------------------------
AC_DEFUN([PKG_CHECK_MODULES],
[AC_REQUIRE([PKG_PROG_PKG_CONFIG])dnl
AC_ARG_VAR([$1][_CFLAGS], [C compiler flags for $1, overriding pkg-config])dnl
AC_ARG_VAR([$1][_LIBS], [linker flags for $1, overriding pkg-config])dnl

pkg_failed=no
AC_MSG_CHECKING([for $1])

_PKG_CONFIG([$1][_CFLAGS], [cflags-only-I], [$2])
_PKG_CONFIG([$1][_LIBS], [libs], [$2])

m4_define([_PKG_TEXT], [Alternatively, you may set the environment variables $1[]_CFLAGS
and $1[]_LIBS to avoid the need to call pkg-config.
See the pkg-config man page for more details.])

if test $pkg_failed = yes; then
        _PKG_SHORT_ERRORS_SUPPORTED
        if test $_pkg_short_errors_supported = yes; then
	        $1[]_PKG_ERRORS=`$PKG_CONFIG --short-errors --errors-to-stdout --print-errors "$2"`
        else 
	        $1[]_PKG_ERRORS=`$PKG_CONFIG --errors-to-stdout --print-errors "$2"`
        fi
	# Put the nasty error message in config.log where it belongs
	echo "$$1[]_PKG_ERRORS" >&AS_MESSAGE_LOG_FD

	ifelse([$4], , [AC_MSG_ERROR(dnl
[Package requirements ($2) were not met:

$$1_PKG_ERRORS

Consider adjusting the PKG_CONFIG_PATH environment variable if you
installed software in a non-standard prefix.

_PKG_TEXT
])],
		[$4])
elif test $pkg_failed = untried; then
	ifelse([$4], , [AC_MSG_FAILURE(dnl
[The pkg-config script could not be found or is too old.  Make sure it
is in your PATH or set the PKG_CONFIG environment variable to the full
path to pkg-config.

_PKG_TEXT

To get pkg-config, see <http://www.freedesktop.org/software/pkgconfig>.])],
		[$4])
else
	$1[]_CFLAGS=$pkg_cv_[]$1[]_CFLAGS
	$1[]_LIBS=$pkg_cv_[]$1[]_LIBS
        AC_MSG_RESULT([yes])
	ifelse([$3], , :, [$3])
fi[]dnl
])# PKG_CHECK_MODULES


dnl @synopsis AX_FUNC_WHICH_GETHOSTBYNAME_R
dnl
dnl Determines which historical variant of the gethostbyname_r() call
dnl (taking three, five, or six arguments) is available on the system
dnl and defines one of the following macros accordingly:
dnl
dnl   HAVE_FUNC_GETHOSTBYNAME_R_6
dnl   HAVE_FUNC_GETHOSTBYNAME_R_5
dnl   HAVE_FUNC_GETHOSTBYNAME_R_3
dnl
dnl If used in conjunction with gethostname.c, the API demonstrated in
dnl test.c can be used regardless of which gethostbyname_r() is
dnl available. These example files can be found at
dnl http://www.csn.ul.ie/~caolan/publink/gethostbyname_r
dnl
dnl based on David Arnold's autoconf suggestion in the threads faq
dnl
dnl Originally named "AC_caolan_FUNC_WHICH_GETHOSTBYNAME_R". Rewritten
dnl for Autoconf 2.5x by Daniel Richard G.
dnl
dnl @category InstalledPackages
dnl @author Caolan McNamara <caolan@skynet.ie>
dnl @author Daniel Richard G. <skunk@iskunk.org>
dnl @version 2005-01-21
dnl @license GPLWithACException

AC_DEFUN([AX_FUNC_WHICH_GETHOSTBYNAME_R], [

    AC_LANG_PUSH(C)
    AC_MSG_CHECKING([how many arguments gethostbyname_r() takes])

    AC_CACHE_VAL(ac_cv_func_which_gethostbyname_r, [

################################################################

ac_cv_func_which_gethostbyname_r=unknown

#
# ONE ARGUMENT (sanity check)
#

# This should fail, as there is no variant of gethostbyname_r() that takes
# a single argument. If it actually compiles, then we can assume that
# netdb.h is not declaring the function, and the compiler is thereby
# assuming an implicit prototype. In which case, we're out of luck.
#
AC_COMPILE_IFELSE(
    AC_LANG_PROGRAM(
	[[#include <netdb.h>]],
	[[
	    char *name = "www.gnu.org";
	    (void)gethostbyname_r(name) /* ; */
	]]),
    ac_cv_func_which_gethostbyname_r=no)

#
# SIX ARGUMENTS
# (e.g. Linux)
#

if test "$ac_cv_func_which_gethostbyname_r" = "unknown"; then

AC_COMPILE_IFELSE(
    AC_LANG_PROGRAM(
	[[#include <netdb.h>]],
	[[
	    char *name = "www.gnu.org";
	    struct hostent ret, *retp;
	    char buf@<:@1024@:>@;
	    int buflen = 1024;
	    int my_h_errno;
	    (void)gethostbyname_r(name, &ret, buf, buflen, &retp, &my_h_errno) /* ; */
	]]),
    ac_cv_func_which_gethostbyname_r=six)

fi

#
# FIVE ARGUMENTS
# (e.g. Solaris)
#

if test "$ac_cv_func_which_gethostbyname_r" = "unknown"; then

AC_COMPILE_IFELSE(
    AC_LANG_PROGRAM(
	[[#include <netdb.h>]],
	[[
	    char *name = "www.gnu.org";
	    struct hostent ret;
	    char buf@<:@1024@:>@;
	    int buflen = 1024;
	    int my_h_errno;
	    (void)gethostbyname_r(name, &ret, buf, buflen, &my_h_errno) /* ; */
	]]),
    ac_cv_func_which_gethostbyname_r=five)

fi

#
# THREE ARGUMENTS
# (e.g. AIX, HP-UX, Tru64)
#

if test "$ac_cv_func_which_gethostbyname_r" = "unknown"; then

AC_COMPILE_IFELSE(
    AC_LANG_PROGRAM(
	[[#include <netdb.h>]],
	[[
	    char *name = "www.gnu.org";
	    struct hostent ret;
	    struct hostent_data data;
	    (void)gethostbyname_r(name, &ret, &data) /* ; */
	]]),
    ac_cv_func_which_gethostbyname_r=three)

fi

################################################################

]) dnl end AC_CACHE_VAL

case "$ac_cv_func_which_gethostbyname_r" in
    three)
    AC_MSG_RESULT([three])
    AC_DEFINE(HAVE_FUNC_GETHOSTBYNAME_R_3)
    ;;

    five)
    AC_MSG_RESULT([five])
    AC_DEFINE(HAVE_FUNC_GETHOSTBYNAME_R_5)
    ;;

    six)
    AC_MSG_RESULT([six])
    AC_DEFINE(HAVE_FUNC_GETHOSTBYNAME_R_6)
    ;;

    no)
    AC_MSG_RESULT([cannot find function declaration in netdb.h])
    ;;

    unknown)
    AC_MSG_RESULT([can't tell])
    ;;

    *)
    AC_MSG_ERROR([internal error])
    ;;
esac

AC_LANG_POP(C)

]) dnl end AC_DEFUN