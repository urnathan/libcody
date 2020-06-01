# CODYlib		-*- mode:autoconf -*-
# Copyright (C) 2019-2020 Nathan Sidwell, nathan@acm.org
# License: LGPL v3.0 or later

# CODY_SUBPROJ(NAME,[FOUND=],[NOT-FOUND=])
AC_DEFUN([CODY_SUBPROJ],
[AC_MSG_CHECKING([subproject $1])
if test -d ${srcdir}/$1; then
  if test -h ${srcdir}/$1; then
    AC_MSG_RESULT([by reference])
  else
    AC_MSG_RESULT([by value])
  fi
  $2
else
  AC_MSG_RESULT([not present])
  $3
fi])

AC_DEFUN([CODY_REQUIRED_SUBPROJ],
[CODY_SUBPROJ([$1],,[AC_MSG_ERROR([$1 project is required, install by value (copy) or reference (symlink)])])])

# CODY_SUBPROJ(NAME,[FOUND=],[NOT-FOUND=])
AC_DEFUN([CODY_OPTIONAL_SUBPROJ],
[AC_MSG_CHECKING([subproject $1])
if test -d ${srcdir}/$1; then
  SUBPROJS+=" $1"
  if test -h ${srcdir}/$1; then
    AC_MSG_RESULT([by reference])
  else
    AC_MSG_RESULT([by value])
  fi
  $2
else
  AC_MSG_RESULT([not present])
  $3
fi])

AC_DEFUN([CODY_TOOLS],
[AC_MSG_CHECKING([tools])
AC_ARG_WITH([tools],
AS_HELP_STRING([--with-tools=DIR],[tool directory]),
if test "$withval" = "yes" ; then
  AC_MSG_ERROR([tool location not specified])
elif test "$withval" = "no" ; then
  :
elif ! test -d "${withval%/bin}/bin" ; then
  AC_MSG_ERROR([tools not present])
else
  tools=${withval%/bin}/bin
fi)
AC_MSG_RESULT($tools)
PATH=$tools${tools+:}$PATH
AC_SUBST(tools)])

AC_DEFUN([CODY_CXX_COMPILER],
[AC_ARG_WITH([compiler],
AS_HELP_STRING([--with-compiler=NAME],[which compiler to use]),
AC_MSG_CHECKING([C++ compiler])
if test "$withval" = "yes" ; then
  AC_MSG_ERROR([NAME not specified])
elif test "$withval" = "no" ; then
  AC_MSG_ERROR([Gonna need a c++ compiler, dofus!])
else
  CXX="${withval}"
  AC_MSG_RESULT([$CXX])
fi)])

AC_DEFUN([CODY_CXX_11],
[AC_MSG_CHECKING([whether $CXX supports C++11])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
[#if __cplusplus <= 201103
#error "C++11 is required"
#endif
]])],
[AC_MSG_RESULT([yes])],
[CXX+=" -std=c++11"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
[#if __cplusplus <= 201103
#error "C++11 is required"
#endif
]])],
AC_MSG_RESULT([adding -std=c++11]),
AC_MSG_RESULT([no])
AC_MSG_ERROR([C++11 is required])]))])

AC_DEFUN([CODY_BUGURL],
[AC_MSG_CHECKING([bugurl])
AC_ARG_WITH(bugurl,
AS_HELP_STRING([--with-bugurl=URL],[where to report bugs]),
if test "$withval" = "yes" ; then
  AC_MSG_ERROR([URL not specified])
elif test "$withval" = "no" ; then
  BUGURL=""
else
  BUGURL="${withval}"
fi,BUGURL="${PACKAGE_BUGREPORT}")
AC_MSG_RESULT($BUGURL)
AC_DEFINE_UNQUOTED(BUGURL,"$BUGURL",[Bug reporting location])])

AC_DEFUN([CODY_ENABLE_CHECKING],
[AC_ARG_ENABLE(checking,
AS_HELP_STRING([--enable-checking],
[enable run-time checking]),,
[enable_checking="yes"])
case "$enable_checking" in
  ("yes") cody_checking=yes ;;
  ("no") cody_checking= ;;
  (*) AC_MSG_ERROR([unknown check "$enable_check"]) ;;
esac
AC_MSG_CHECKING([checking])
AC_MSG_RESULT([${cody_checking:-no}])
if test "$cody_checking" = yes ; then
  AC_DEFINE([CODY_CHECKING], [1], [Enable checking])
fi])
