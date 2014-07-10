#  -*- Autoconf -*-

AC_PREREQ([2.63])
AC_INIT([utxx], [1.1], [BUG-REPORT-ADDRESS])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign -Wall -Wno-portability])
AC_CONFIG_HEADERS(config.h)
AX_PREFIX_CONFIG_H(include/utxx/config.h)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

AC_SUBST([utxxdir], [$libdir])

# Default prefix
AC_PREFIX_DEFAULT(${top_srcdir})
AC_SUBST([default_prefix], [${abs_srcdir}/install])
test "$prefix" = "NONE" && prefix="$default_prefix"
test "$sysconfdir" = "\${prefix}/etc" && sysconfdir='../etc'
test "$scriptdir" = "" && scriptdir='../lib'

# Setting Ix86 conditional for x86 platform
AC_CANONICAL_HOST
AM_CONDITIONAL([Ix86], [case x$host_cpu in xi?86) test -n a;; *) test;; esac])

# Options

AC_ARG_VAR([ERLC_FLAGS], [general flags to prepend to ERLC_FLAGS])

CXXFLAGS="-MMD -D_REENTRANT -Wno-unused-local-typedefs -DBOOST_SYSTEM_NO_DEPRECATED"
CPPFLAGS=
LDFLAGS=

AC_ARG_ENABLE(debug,
     AC_HELP_STRING([--enable-debug],[enable debug [default=yes]]),
     [ if test "x$enable_debug" = "xyes" -o -z "x$enable_debug"; then
           # CXXFLAGS="${CXXFLAGS% } -ggdb -O0 -Wall -fno-default-inline -fno-inline -fno-inline-functions"
           CXXFLAGS+=" -g -O0 -Wall"
       fi
     ],
)

AC_ARG_ENABLE(optimize,
     AC_HELP_STRING([--enable-optimize],[enable code optimization [default=no]]),
     [ if test "x$enable_optimize" = "xyes" ; then
           CXXFLAGS+=" -O3 -g"
       fi
     ],
)

AC_ARG_ENABLE(warnings,
     AC_HELP_STRING([--enable-warnings],[enable Wall compiling [default=yes]]),
     [ if test "x$enable_warnings" = "xyes" -o -z "$enable_warnings"; then
           CXXFLAGS+=" -Wall -Werror"
       fi
     ],
)

AC_LANG_PUSH(C++)
AC_ARG_ENABLE(cxx11,
    AC_HELP_STRING([--disable-cxx11],[disable c++11 standard [default=no]]),
    [ if test "x${enable_cxx11}" != "xno" -a -z "${disable_cxx11}"; then
         AX_CXX_COMPILE_STDCXX_11([noext], [optional])
      fi
    ],
    [
        AX_CXX_COMPILE_STDCXX_11([noext], [optional])
    ]
)
AC_LANG_POP([C++])

# libtool
LT_INIT([disable-static])

# Checks for programs.
AC_PROG_CXX
AC_PROG_SED
#AX_PROG_XSLTPROC
AC_PATH_PROG(PYTHON, python2 python2.7 python2.6 python2.5 python)
AM_PATH_PYTHON
AX_PYTHON_MODULE(lxml.etree, fatal)

AC_CHECK_PROG(lc1_size, getconf, $(getconf LEVEL1_DCACHE_LINESIZE))
if test -n $lc1_size; then
    # Note: we use AC_DEFINE_UNQUOTED to put a shell variable in config.h
    AC_DEFINE_UNQUOTED(CL_SIZE, [${lc1_size}], [Level1 cacheline size])
fi

# Make sure Python package python-lxml is installed

# Checks for libraries.

AC_CHECK_LIB(pthread, pthread_create)

AC_CHECK_FUNCS(clock_gettime, [], [
  AC_CHECK_LIB(rt, clock_gettime, [
    RTLIB="-lrt"
    AC_SUBST([RTLIB])
  ])
])

AX_BOOST_BASE([1.49.0], [], [AC_MSG_ERROR("BOOST library version >= 1.49.0 not found!")])
AX_BOOST_SYSTEM
AX_BOOST_DATE_TIME
AX_BOOST_FILESYSTEM
AX_BOOST_UNIT_TEST_FRAMEWORK

# workaround unexpected extra dependency on boost_system library (at least in 1.49)
LDFLAGS+=" $BOOST_LDFLAGS $BOOST_SYSTEM_LIB"
AX_BOOST_THREAD
AX_BOOST_CHRONO
LDFLAGS=

if test x"$ac_boost_path" != x; then
    BOOST_ROOT="$ac_boost_path"
    AC_SUBST([BOOST_ROOT])
else
    ac_boost_path="/usr"
    BOOST_ROOT="$ac_boost_path"
    BOOST_LIBDIR="$ac_boost_path/lib"
fi
if test x"$ac_boost_lib_path" != x; then
    BOOST_LIBDIR="$ac_boost_lib_path"
else
    libsubdirs="lib"
    ax_arch=`uname -m`
    if test $ax_arch = x86_64 -o $ax_arch = ppc64 -o $ax_arch = s390x -o $ax_arch = sparc64; then
        libsubdirs="lib64 lib"
    fi
    for ac_boost_path_tmp in $libsubdirs; do
        if test -d "$ac_boost_path"/"$ac_boost_path_tmp" ; then
            BOOST_LIBDIR="$ac_boost_path/$ac_boost_path_tmp"
            break
        fi
    done
fi

AC_SUBST([BOOST_ROOT])
AC_SUBST([BOOST_LIBDIR])

if test -n "$ac_boost_path"; then
    # Checks for new (since 1.48) boost timer header and library
    CPPFLAGS_SAVED=$CPPFLAGS
    LDFLAGS_SAVED=$LDFLAGS
    CPPFLAGS="$BOOST_CPPFLAGS"
    LDFLAGS="$BOOST_LDFLAGS"
    AC_LANG_PUSH(C++)
    AC_CHECK_HEADERS([boost/timer/timer.hpp], [
        BOOST_TIMER_LIB="-lboost_timer $BOOST_CHRONO_LIB"
        AC_SUBST(BOOST_TIMER_LIB)
        AC_DEFINE([HAVE_BOOST_TIMER_TIMER_HPP], [1],
                  [Define to 1 if boost/time/timer.hpp is available])
    ])
    AC_LANG_POP([C++])
    CPPFLAGS=$CPPFLAGS_SAVED
    LDFLAGS=$LDFLAGS_SAVED
fi

dnl Check for Thrift installation
AC_ARG_WITH([thrift],
	AC_HELP_STRING([--with-thrift=PATH],
	    [PATH to Thrift installation (optional)]),
	with_thrift=${withval%/},
	with_thrift="")

if test -z "${with_thrift}"; then
    AC_CHECK_HEADER(thrift/Thrift.h, [
        with_thrift=${ac_cv_header_thrift_Thrift_h%/*/*/*}
    ])
else
    AC_DEFINE([HAVE_THRIFT_H], [1], [Define to 1 if Thrift library is available])
fi

AC_ARG_WITH([thrift-libdir],
	AC_HELP_STRING([--with-thrift-libdir=PATH],
	    [PATH to Thrift library directory installation (optional)]),
	with_thrift_libdir=${withval%/},
	with_thrift_libdir="${with_thrift}/lib")

if test -n "${with_thrift}"; then
    if test ! -d "${with_thrift}" -o ! -f "$with_thrift/bin/thrift"; then
        AC_MSG_ERROR([no Thrift installation is found in ${with_thrift}!])
    else
    if test ! -f "$with_thrift_libdir/libthrift.so"; then
        AC_MSG_ERROR([no Thrift library is found!])
    else
        THRIFT_DIR="$with_thrift"
        THRIFT_PROG="$with_thrift/bin/thrift"
        THRIFT_LIBDIR="$with_thrift_libdir"
        THRIFT_LIBS="-lthrift"
        THRIFT_LDFLAGS="-L$with_thrift_libdir"
        THRIFT_CPPFLAGS="-I$with_thrift/include"
        AC_SUBST([THRIFT_PROG])
        AC_SUBST([THRIFT_DIR])
        AC_SUBST([THRIFT_LDFLAGS])
        AC_SUBST([THRIFT_LIBDIR])
        AC_SUBST([THRIFT_LIBS])
        AC_SUBST([THRIFT_CPPFLAGS])
    fi
    fi
fi

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
#AC_TYPE_UINT32_T
#AC_TYPE_UINT64_T

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday socket])

CXXFLAGS+=" $BOOST_CPPFLAGS"
AC_SUBST([CPPFLAGS])
AC_SUBST([LDFLAGS])

dnl ------------------------------------------------------------------
dnl Output.
dnl ------------------------------------------------------------------

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([test/Makefile])
AC_CONFIG_FILES([test.sh], [chmod +x test.sh])
AC_CONFIG_FILES([utxx.pc:utxx.pc.in])

AC_OUTPUT

echo
echo "========================================================================"
echo "   CXXFLAGS: ${CXXFLAGS}"
echo "    LDFLAGS: ${LDFLAGS}"
echo "      BOOST: (version: $(awk '/^#.*BOOST_LIB_VERSION/{print $3}' \
                    ${BOOST_ROOT}/include/boost/version.hpp | xargs)) ${BOOST_ROOT}"
echo "     THRIFT: ${THRIFT_DIR}"
echo "------------------------------------------------------------------------"
echo "             Configuration completed successfully!                      "
echo "========================================================================"
echo
