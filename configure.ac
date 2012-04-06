#  -*- Autoconf -*-

AC_PREREQ([2.63])
AC_INIT([util], [0.1], [BUG-REPORT-ADDRESS])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign -Wall -Wno-portability])

AC_SUBST([utildir], [$libdir/util])

# Default prefix
AC_PREFIX_DEFAULT(`pwd`)
AC_SUBST([default_prefix], [${PWD}/install])
test "$prefix" = "NONE" && prefix="$default_prefix"
test "$sysconfdir" = "\${prefix}/etc" && sysconfdir='../etc'
test "$scriptdir" = "" && scriptdir='../lib'

# Options

AC_ARG_VAR([ERLC_FLAGS], [general flags to prepend to ERLC_FLAGS])

CXXFLAGS="${CXXFLAGS% } -MMD"

AC_ARG_ENABLE(debug,
     AC_HELP_STRING([--enable-debug],[enable debug [[default=yes]]]),
     [ if test "x$enable_debug" = "xyes" -o -z "x$enable_debug"; then
           # CXXFLAGS="${CXXFLAGS% } -ggdb -O0 -Wall -fno-default-inline -fno-inline -fno-inline-functions"
           CXXFLAGS="${CXXFLAGS% } -g -O0 -Wall"
       fi
     ],
)

AC_ARG_ENABLE(optimize,
     AC_HELP_STRING([--enable-optimize],[enable code optimization [[default=no]]]),
     [ if test "x$enable_optimize" = "xyes" ; then
           CXXFLAGS="${CXXFLAGS% } -O3 -g"
       fi
     ],
)

AC_ARG_ENABLE(warnings,
     AC_HELP_STRING([--enable-warnings],[enable Wall compiling [[default=yes]]]),
     [ if test "x$enable_warnings" = "xyes" -o -z "$enable_warnings"; then
           CXXFLAGS="${CXXFLAGS% } -Wall -Werror"
       fi
     ],
)

# libtool
LT_INIT([disable-static])

# Checks for programs.
AC_PROG_CXX
AC_PROG_SED
AX_PROG_XSLTPROC

dnl AC_CONFIG_SRCDIR([src/common.cpp])
AC_CONFIG_HEADERS([config.h])

# Checks for libraries.

BOOST_REQUIRE([1.49.0], [AC_MSG_ERROR("BOOST library version >= 1.49.0 not found!")])
BOOST_FIND_HEADER([boost/property_tree/ptree.hpp])
BOOST_SYSTEM
BOOST_THREADS
BOOST_TEST

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday socket])

dnl ------------------------------------------------------------------
dnl Output.
dnl ------------------------------------------------------------------

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([test/Makefile])
AC_CONFIG_FILES([util-${PACKAGE_VERSION}.pc:util.pc.in])

AC_OUTPUT

echo
echo "========================================================================"
echo "   CXXFLAGS: ${CXXFLAGS}" $BOOST_CPPFLAGS
echo "      BOOST: $BOOST_ROOT"
echo "------------------------------------------------------------------------"
echo "             Configuration completed successfully!                      "
echo "========================================================================"
echo
