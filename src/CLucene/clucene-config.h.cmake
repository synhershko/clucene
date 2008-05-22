#ifndef _SRC_CLUCENE_CLUCENE_CONFIG_H
#define _SRC_CLUCENE_CLUCENE_CONFIG_H 1
 
/* src/CLucene/clucene-config.h. 
*  Generated automatically at end of cmake.
*/

/* Compulsary headers. cmake will fail if these are not found:
 * Eventually we will take these out of StdHeader to simplify it all a bit.
*/
#define _CL_HAVE_ALGORITHM 1
#define _CL_HAVE_FUNCTIONAL  1
#define _CL_HAVE_MAP  1
#define _CL_HAVE_VECTOR  1
#define _CL_HAVE_LIST  1
#define _CL_HAVE_SET  1
#define _CL_HAVE_MATH_H  1 
#define _CL_HAVE_STDARG_H  1
#define _CL_HAVE_STDEXCEPT 1
#define _CL_HAVE_FCNTL_H  1 

#define _CL_HAVE_WCSCPY  1 
#define _CL_HAVE_WCSNCPY  1 
#define _CL_HAVE_WCSCAT  1 
#define _CL_HAVE_WCSCHR  1 
#define _CL_HAVE_WCSSTR  1 
#define _CL_HAVE_WCSLEN  1 
#define _CL_HAVE_WCSCMP  1 
#define _CL_HAVE_WCSNCMP  1 
#define _CL_HAVE_WCSCSPN  1 

/* CMake will look for these headers: */

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine _CL_STDC_HEADERS  1 

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine _CL_HAVE_STRING_H  1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine _CL_HAVE_STRINGS_H  1 

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine _CL_HAVE_MEMORY_H  1 

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine _CL_HAVE_UNISTD_H  1 

/* Define to 1 if you have the <io.h> header file. */
#cmakedefine _CL_HAVE_IO_H  1 

/* Define to 1 if you have the <direct.h> header file. */
#cmakedefine _CL_HAVE_DIRECT_H  1 

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'. */
#cmakedefine _CL_HAVE_DIRENT_H  1 

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'. */
#cmakedefine _CL_HAVE_SYS_DIR_H

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'. */
#cmakedefine _CL_HAVE_SYS_NDIR_H

/* Define to 1 if you have the <errno.h> header file. */
#cmakedefine _CL_HAVE_ERRNO_H  1 

/* Define to 1 if you have the <wchar.h> header file. */
#cmakedefine _CL_HAVE_WCHAR_H  1 

/* Define to 1 if you have a functioning <wchar.h> header file. */
#cmakedefine _CL_HAVE_WCTYPE_H   

/* Define to 1 if you have the <ctype.h> header file. */
#cmakedefine _CL_HAVE_CTYPE_H  1 

/* Define to 1 if you have the <windows.h> header file. */
#cmakedefine _CL_HAVE_WINDOWS_H  1 

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine _CL_HAVE_SYS_TYPES_H  1 

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine _CL_HAVE_INTTYPES_H  1 


/* CMake will look for these symbols: */

/* Define to 1 if the system has the type `float_t'. */
//todo: HACK
#define _CL_HAVE_FLOAT_T  1 

#ifdef __MINGW32__
typedef float float_t;
#endif


/* Define to 1 if the system has the type `intptr_t'. */
//todo: HACK
#define _CL_HAVE_INTPTR_T  1 

/* Define to 1 if the system has the type `wchar_t'. */
//todo: HACK
#define _CL_HAVE_WCHAR_T  1 



/* CMake will look for these functions: */

/* Define to 1 if you have the `printf' function. */
#cmakedefine _CL_HAVE_PRINTF  1 

/* Define to 1 if you have the `snprintf' function. */
#cmakedefine _CL_HAVE_SNPRINTF  1 
 


/* CMake will determine these module specificics */

/* Define if you have POSIX threads libraries and header files. */
#cmakedefine _CL_HAVE_PTHREAD  1 

/* Define if you have POSIX threads libraries and header files. */
#cmakedefine _CL_HAVE_WIN32_THREADS  1 

/* define if the compiler supports ISO C++ standard library */
#cmakedefine _CL_HAVE_STD   














/* Disable multithreading */
#cmakedefine _CL_DISABLE_MULTITHREADING


/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine _CL_HAVE_DLFCN_H  1 


/* Define to 1 if you have the <ext/hash_map> header file. */
#cmakedefine _CL_HAVE_EXT_HASH_MAP  1 

/* Define to 1 if you have the <ext/hash_set> header file. */
#cmakedefine _CL_HAVE_EXT_HASH_SET  1 


/* Define to 1 if you have the <functional> header file. */

/* Does not support new float byte<->float conversions */
#cmakedefine _CL_HAVE_FUNCTIONING_FLOAT_BYTE   

/* Define to 1 if you have the `getpagesize' function. */
#cmakedefine _CL_HAVE_GETPAGESIZE

/* Define to 1 if you have the <hash_map> header file. */
#cmakedefine _CL_HAVE_HASH_MAP

/* Define to 1 if you have the <hash_set> header file. */
#cmakedefine _CL_HAVE_HASH_SET


/* Define to 1 if you have the `lltoa' function. */
#cmakedefine _CL_HAVE_LLTOA

/* Define to 1 if you have the `lltow' function. */
#cmakedefine _CL_HAVE_LLTOW

/* Define to 1 if long double works and has more range or precision than double. */
#cmakedefine _CL_HAVE_LONG_DOUBLE  1 




/* Define to 1 if you have a working `mmap' system call. */
#cmakedefine _CL_HAVE_MMAP  1 

/* define if the compiler implements namespaces */
#cmakedefine _CL_HAVE_NAMESPACES   

/* Define if you have the nanosleep function */
#cmakedefine _CL_HAVE_NANOSLEEP  1 

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
#cmakedefine _CL_HAVE_NDIR_H

/* Does not support new float byte<->float conversions */
#cmakedefine _CL_HAVE_NO_FLOAT_BYTE

/* Does not support try/catch blocks */
#cmakedefine _CL_HAVE_NO_FUNCTION_TRY_BLOCKS


/* Define if recursive pthread mutexes are available */
#cmakedefine _CL_HAVE_PTHREAD_MUTEX_RECURSIVE  1 



/* Defined if the snprintf overflow test fails */
#cmakedefine _CL_HAVE_SNPRINTF_BUG

/* Define to 1 if you have the `snwprintf' function. */
#cmakedefine _CL_HAVE_SNWPRINTF



/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine _CL_HAVE_STDINT_H  1 

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine _CL_HAVE_STDLIB_H  1 

/* define if the compiler supports Standard Template Library */
#cmakedefine _CL_HAVE_STL   


/* Define to 1 if you have the `strlwr' function. */
#cmakedefine _CL_HAVE_STRLWR

/* Define to 1 if you have the `strtoll' function. */
#cmakedefine _CL_HAVE_STRTOLL

/* Define to 1 if you have the `strupr' function. */
#cmakedefine _CL_HAVE_STRUPR

/* Defined if the swprintf test fails */
#cmakedefine _CL_HAVE_SWPRINTF_BUG   

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine _CL_HAVE_SYS_STAT_H  1 

/* Define to 1 if you have the <sys/timeb.h> header file. */
#cmakedefine _CL_HAVE_SYS_TIMEB_H  1 


/* Define to 1 if you have the <tchar.h> header file. */
#cmakedefine _CL_HAVE_TCHAR_H

/* Define to 1 if you have the `tell' function. */
#cmakedefine _CL_HAVE_TELL



/* Define to 1 if you have the `vsnwprintf' function. */
#cmakedefine _CL_HAVE_VSNWPRINTF

/* Define to 1 if you have the `wcscasecmp' function. */
#cmakedefine _CL_HAVE_WCSCASECMP

/* Define to 1 if you have the `wcscat' function. */
#cmakedefine _CL_HAVE_WCSCAT  1 

/* Define to 1 if you have the `wcschr' function. */
#cmakedefine _CL_HAVE_WCSCHR  1 

/* Define to 1 if you have the `wcscmp' function. */
#cmakedefine _CL_HAVE_WCSCMP  1 

/* Define to 1 if you have the `wcscpy' function. */
#cmakedefine _CL_HAVE_WCSCPY  1 

/* Define to 1 if you have the `wcscspn' function. */
#cmakedefine _CL_HAVE_WCSCSPN  1 

/* Define to 1 if you have the `wcsicmp' function. */
#cmakedefine _CL_HAVE_WCSICMP

/* Define to 1 if you have the `wcslen' function. */
#cmakedefine _CL_HAVE_WCSLEN  1 

/* Define to 1 if you have the `wcsncmp' function. */
#cmakedefine _CL_HAVE_WCSNCMP  1 

/* Define to 1 if you have the `wcsncpy' function. */
#cmakedefine _CL_HAVE_WCSNCPY  1 

/* Define to 1 if you have the `wcsstr' function. */
#cmakedefine _CL_HAVE_WCSSTR  1 

/* Define to 1 if you have the `wcstod' function. */
#cmakedefine _CL_HAVE_WCSTOD

/* Define to 1 if you have the `wcstoll' function. */
#cmakedefine _CL_HAVE_WCSTOLL

/* Define to 1 if you have the `wcsupr' function. */
#cmakedefine _CL_HAVE_WCSUPR


/* Define to 1 if you have the `wprintf' function. */
#cmakedefine _CL_HAVE_WPRINTF

/* Define to 1 if you have the `_filelength' function. */
#cmakedefine _CL_HAVE__FILELENGTH

/* How to define a static const in a class */
#cmakedefine LUCENE_STATIC_CONSTANT_SYNTAX  1 

/* Name of package */
#cmakedefine _CL_PACKAGE  "clucene-core" 

/* Define to the address where bug reports for this package should be sent. */
#cmakedefine _CL_PACKAGE_BUGREPORT  "" 

/* Define to the full name of this package. */
#cmakedefine _CL_PACKAGE_NAME  "" 

/* Define to the full name and version of this package. */
#cmakedefine _CL_PACKAGE_STRING  "" 

/* Define to the one symbol short name of this package. */
#cmakedefine _CL_PACKAGE_TARNAME  "" 

/* Define to the version of this package. */
#cmakedefine _CL_PACKAGE_VERSION  "" 

/* Define to the necessary symbol if this constant uses a non-standard name on
   your system. */
#cmakedefine _CL_PTHREAD_CREATE_JOINABLE

/* The size of a `unsigned char', as computed by sizeof. */
#cmakedefine _CL_SIZEOF_UNSIGNED_CHAR

/* The size of a `unsigned int', as computed by sizeof. */
#cmakedefine _CL_SIZEOF_UNSIGNED_INT

/* The size of a `unsigned long', as computed by sizeof. */
#cmakedefine _CL_SIZEOF_UNSIGNED_LONG

/* The size of a `unsigned long long', as computed by sizeof. */
#cmakedefine _CL_SIZEOF_UNSIGNED_LONG_LONG

/* The size of a `unsigned __int64', as computed by sizeof. */
#cmakedefine _CL_SIZEOF_UNSIGNED___INT64

/* Define to 1 if the `S_IS*' macros in <sys/stat.h> do not work properly. */
#cmakedefine _CL_STAT_MACROS_BROKEN

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#cmakedefine _CL_TIME_WITH_SYS_TIME  1 

/* Version number of package */
#cmakedefine _CL_VERSION  "0.9.15" 

/* Forces into Ascii mode */
#cmakedefine _ASCII

/* Conditional Debugging */
#cmakedefine _CL__CND_DEBUG

/* debuging option */
#cmakedefine _DEBUG

/* Number of bits in a file offset, on hosts where this is settable. */
#cmakedefine _FILE_OFFSET_BITS

/* Define for large files, on AIX-style hosts. */
#cmakedefine _LARGE_FILES

/* If not already defined, then define as a datatype of *exactly* 32 bits. */
#cmakedefine uint32_t

/* If not already defined, then define as a datatype of *exactly* 64 bits. */
#cmakedefine uint64_t

/* If not already defined, then define as a datatype of *exactly* 8 bits. */
#cmakedefine uint8_t
 

#endif
