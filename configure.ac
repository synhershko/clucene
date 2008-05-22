dnl ################################################################################
dnl initialize autoconf and automake
dnl ################################################################################

AC_PREREQ([2.57])
AC_INIT(src/CLucene/StdHeader.h)

dnl the clucene-core package version
VERSION=0.9.20

dnl increment if interfaces have been added, removed or changed
clLIB_current=3

dnl increment if source code has changed
dnl    - set to zero if current is incremented
clLIB_revision=0

dnl increment if interfaces have been added
dnl    - set to zero if interfaces have been removed or changed
clLIB_age=0

CLLIB_VERSION=$clLIB_current:$clLIB_revision:$clLIB_age
AC_SUBST(CLLIB_VERSION)

dnl version info
PACKAGE=clucene-core
PROJECT=CLuceneCore
PACKAGE_BUGREPORT=clucene-developers@lists.sourceforge.net
PACKAGE_NAME=CLuceneCore
PACKAGE_VERSION=$VERSION

AC_CONFIG_SRCDIR([src/CLucene/StdHeader.h])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADER([config.h.tmp])
AX_PREFIX_CONFIG_H([src/CLucene/clucene-config.h],_CL,[config.h.tmp],
	[_ASCII _UCS2 _DEBUG _FILE_OFFSET_BITS _LARGE_FILES uint32_t uint64_t uint8_t int32_t int64_t LUCENE_STATIC_CONSTANT])
AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

dnl Check for existence of a C and C++ compilers.
AC_PROG_CC
AC_PROG_CXX
AC_LANG(C++)

dnl Configure libtool.
AC_PROG_LIBTOOL

dnl #######################################################################
dnl # User choices
dnl #######################################################################
AX_CONFIG_FEATURE_DEFAULT_ENABLED

AC_ARG_ENABLE(debug,
	AC_HELP_STRING([--enable-debug], [build with debugging information]),
	[enable_debug=$enableval], [enable_debug=no])	
if test $enable_debug = yes; then 
	AC_DEFINE(_DEBUG, 1, debuging option)
	if test $ac_cv_prog_cxx_g = yes; then
		CXXFLAGS="-g $CXXFLAGS"
	fi
	echo "enabling debug"
else
	echo disabling debug
fi

AC_ARG_ENABLE(cnddebug,
	AC_HELP_STRING([--enable-cnddebug], [build with asserts]),
	[enable_cnddebug=$enableval], [enable_cnddebug=no])	
if test $enable_cnddebug = yes; then 
	AC_DEFINE(_CND_DEBUG, 1, [Conditional Debugging])
	echo "enabling cnddebug"
else
	echo disabling cnddebug
fi

AC_ARG_ENABLE(ascii,
	AC_HELP_STRING([--enable-ascii], [Forces into ascii mode]),
	[enable_ascii=yes], [enable_ascii=no])
if test $enable_ascii = yes; then
	AC_DEFINE(_ASCII, 1, [Forces into Ascii mode])
fi

AC_ARG_ENABLE(multithreading,
	AC_HELP_STRING([--disable-multithreading], [disable multithreading]),
	[enable_multithreading=no], [enable_multithreading=yes])	
if test $enable_multithreading = yes; then 
	ACX_PTHREAD
	LIBS="$PTHREAD_LIBS $LIBS"
	CXXFLAGS="$PTHREAD_CFLAGS $CXXFLAGS"
	CC="$PTHREAD_CC"
	echo "enabled multithreading"
else
	AC_DEFINE(DISABLE_MULTITHREADING, 1, [Disable multithreading])
	echo "disabled multithreading"
fi
	
AC_ARG_ENABLE(gprof,
	AC_HELP_STRING([--enable-gprof], [turn on profiling support]),
	[enable_gprof=yes], [enable_gprof=no])
if test $enable_gprof = yes; then 
	CXXFLAGS="-pg $CPPFLAGS"
	LDFLAGS="-pg $LDFLAGS"
	echo "enabled gprof"
fi

dnl #######################################################################
dnl # Doxygen Stuff
dnl #######################################################################
AC_CHECK_PROG(DOXYGEN, doxygen, true, false)
if test $DOXYGEN = false; then
	AC_MSG_WARN([*** doxygen not found, docs will not be available])
	enable_doxygen=no
else
  enable_doxygen=yes
	AC_CHECK_PROG(DOT, dot, true, false)
	if test $DOT = false; then
		enable_dot = no;
		AC_MSG_WARN([*** dot not found, graphs will not be available])
	else
		enable_dot = yes;
	fi
fi
AC_SUBST(enable_doxygen)
AC_SUBST(enable_dot)


dnl #######################################################################
dnl # Check headers
dnl #######################################################################

dnl Check for headers
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_STAT
AC_HEADER_TIME

dnl check for stl
AC_CXX_HAVE_STL

dnl check for functioning wctype.h
AC_CXX_HAVE_WCTYPE_H

AC_CHECK_HEADERS([algorithm functional map vector list set],,[AC_MSG_ERROR([A required C++ header file is not found.])])

dnl compulsary headers
AC_CHECK_HEADERS([math.h],,[AC_MSG_ERROR([A required C header file is not found.])])

dnl optional headers
AC_CHECK_HEADERS([hash_map hash_set ext/hash_map ext/hash_set stdarg.h sys/timeb.h wchar.h tchar.h errno.h fcntl.h ctype.h inttypes.h sys/types.h])

dnl seems as though c++ headers aren't being checked properly...
AC_DEFINE([HAVE_STDEXCEPT],[],[x])


dnl #######################################################################
dnl # Check for compiler features
dnl #######################################################################

dnl Check for large file support
AC_SYS_LARGEFILE

dnl Check for strange function behaviours
AC_FUNC_STRTOD

dnl check for namespace availability
AC_CXX_NAMESPACES

dnl check for try/catch availability
MDL_CXX_FUNCTION_TRY_BLOCKS

dnl check for std namespace
AC_CXX_HAVE_STD

dnl check for mmap
dnl todo: check that it functions...
case "$host" in
	*cygwin*)
		AC_CHECK_FUNCS(mmap)
	;;
	*)
	AC_FUNC_MMAP
	;;
esac

dnl check for presence of certain optional methods
if test $enable_ascii = yes; then
	AC_CHECK_FUNCS(strupr strlwr lltoa strtoll)
else
	AC_CHECK_FUNCS(wcscpy wcsupr wcsncpy wcscasecmp wcsicmp  wcscat wcschr wcstoll lltow wcstod wcsstr wcslen wcscmp wcsncmp wcscspn)
	dnl todo: wcstoq is bsd equiv of wcstoll, we can use that...
fi
dnl always check for file functions
AC_CHECK_FUNCS(tell _filelength)

dnl check for presence of certain compulsary methods
AC_CHECK_FUNCS(printf snprintf,,[AC_MSG_ERROR([A required function is not found.])])

dnl check for broken string functions
dps_swprintf_works
dps_snprintf_oflow

dnl Test for byte<->float conversions
dps_float_byte

dnl Define a valid LUCENE_STATIC_CONSTANT macro
dps_static_const

dnl only try using internal functions if the bugs are not detected
if test $dps_cv_snprintf_bug -eq 0; then
 if test $dps_cv_swprintf_works -eq 0; then
  AC_CHECK_FUNCS(wprintf snwprintf vsnwprintf)
 fi
fi

dnl #######################################################################
dnl # Check for types
dnl #######################################################################

dnl check for functioning long double type
dnl AC_C_LONG_DOUBLE - not necessary - we dont use long doubles currently

AC_DEFINE_INTEGER_BITS(uint64_t, unsigned long long, unsigned __int64, unsigned long)
if test "$uint64_t" = no; then
  AC_MSG_ERROR([unable to continue without an unsigned 64-bit datatype])
fi

AC_DEFINE_INTEGER_BITS(uint32_t, unsigned int, unsigned long)
if test "$uint32_t" = no; then
  AC_MSG_ERROR([unable to continue without an unsigned 32-bit datatype])
fi

AC_DEFINE_INTEGER_BITS(uint8_t, unsigned char)
if test "$uint8_t" = no; then
  AC_MSG_ERROR([unable to continue without an unsigned 8-bit datatype])
fi

dnl check for certain types
AC_CHECK_TYPES([float_t],,,[#include <math.h>])

AC_CHECK_TYPES([wchar_t, intptr_t])

dnl #######################################################################
dnl # Other stuff
dnl #######################################################################

dnl Check for nanosleep library
AC_SEARCH_LIBS([nanosleep], [rt],
  [AC_DEFINE([HAVE_NANOSLEEP], [1],
    [Define if you have the nanosleep function])])

dnl check for endian TODO:
dnl AC_C_BIGENDIAN(AC_DEFINE([IS_BIG_ENDIAN],[],[Define if platform is big endian]),
dnl  AC_DEFINE([IS_LITTLE_ENDIAN],[],[Define if platform is little endian]))



dnl #######################################################################
dnl # Finish off
dnl #######################################################################

AC_CONFIG_FILES([Makefile src/Makefile src/demo/Makefile 
                 doc/Makefile win32/Makefile
                 test/Makefile test/data/Makefile])

AC_OUTPUT([Doxyfile doc/helpheader.htm])

echo "configured $PROJECT version $VERSION"

