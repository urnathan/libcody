# CODYlib  -*- mode:autoconf -*-
# Copyright (C) 2020 Nathan Sidwell, nathan@acm.org
# License: Apache v2.0

AC_INIT([codylib],[0.0],[github.com/urnathan/libcody])
AC_CONFIG_SRCDIR(README.md)
m4_include(config.m4)

AC_CONFIG_AUX_DIR(build-aux)
AC_SUBST(PACKAGE_VERSION)

AC_CANONICAL_HOST

CODY_TOOL_BIN
CODY_CXX_COMPILER
AC_LANG(C++)
AC_PROG_CXX
CODY_CXX_11
CODY_TOOL_LIB
CODY_TOOL_INC

CODY_BUGURL
CODY_ENABLE_CHECKING

AC_CONFIG_HEADER(config.h)
AC_CHECK_TOOL([AR],[ar])
AC_CHECK_TOOL([DOXYGEN],[doxygen],[: NOTdoxygen])
AC_CHECK_TOOL([ALOY],[aloy],[: NOTaloy])
AH_VERBATIM([_GNU_SOURCE],[#define _GNU_SOURCE 1])
AH_VERBATIM([_FORTIFY_SOURCE],[#undef _FORTIFY_SOURCE])

for generated in config.h.in configure ; do
  if test $srcdir/configure.ac -nt $srcdir/$generated ; then
    touch $srcdir/$generated
  fi
done

CONFIG_FILES="Makefile gdbinit dox.cfg"
for make in Makesub tests/Makesub ; do
  test -f ${srcdir}/$make.in && CONFIG_FILES+=" $make"
done
AC_CONFIG_FILES([$CONFIG_FILES])

AC_SUBST(configure_args,[$ac_configure_args])
AC_SUBST(CONFIG_FILES)

AC_OUTPUT
