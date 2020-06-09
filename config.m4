# CODYlib		-*- mode:autoconf -*-
# Copyright (C) 2019-2020 Nathan Sidwell, nathan@acm.org
# License: Apache v2.0

AC_DEFUN([CODY_TOOL_BIN],
[AC_MSG_CHECKING([tool binaries])
AC_ARG_WITH([toolbin],
AS_HELP_STRING([--with-toolbin=DIR],[tool bin directory]),
if test "$withval" = "yes" ; then
  AC_MSG_ERROR([tool bin location not specified])
elif test "$withval" = "no" ; then
  :
elif ! test -d "${withval%/bin}/bin" ; then
  AC_MSG_ERROR([tool bin not present])
else
  toolbin=${withval%/bin}/bin
fi)
AC_MSG_RESULT($toolbin)
if test "$toolbin" ; then
  PATH="$toolbin:$PATH"
fi
AC_SUBST(toolbin)])

AC_DEFUN([CODY_TOOL_INC],
[AC_MSG_CHECKING([tool include])
AC_ARG_WITH([toolinc],
AS_HELP_STRING([--with-toolinc=DIR],[tool include directory]),
if test "$withval" = "yes" ; then
  AC_MSG_ERROR([tool include location not specified])
elif test "$withval" = "no" ; then
  if test "${toolbin}" ; then
    toolinc="${toolbin%/bin}/include"
    if ! test -d "$toolinc" ; then
      # was not found
      toolinc=
    fi
  fi
elif ! test -d "${withval}" ; then
  AC_MSG_ERROR([tool include not present])
else
  toolinc=${withval}
fi)
AC_MSG_RESULT($toolinc)
if test "$toolinc" ; then
  CXX+=" -I $toolinc"
fi])

AC_DEFUN([CODY_TOOL_LIB],
[AC_MSG_CHECKING([tool libraries])
AC_ARG_WITH([toollib],
AS_HELP_STRING([--with-toollib=DIR],[tool library directory]),
if test "$withval" = "yes" ; then
  AC_MSG_ERROR([tool library location not specified])
elif test "$withval" = "no" ; then
  if test "${toolbin}" ; then
    toollib="${toolbin%/bin}/lib"
    if os=$(CXX -print-multi-os-directory 2>/dev/null) ; then
      toollib+="/${os}"
    fi
    if ! test -d "$toollib" ; then
      # was not found
      toollib=
    fi
  fi
elif ! test -d "${withval}" ; then
  AC_MSG_ERROR([tool library not present])
else
  toollib=${withval}
fi)
AC_MSG_RESULT($toollib)
if test "$toollib" ; then
  LDFLAGS+="-L $toollib"
fi])

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
