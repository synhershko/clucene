#ifndef _SRC_CLUCENE_INTERNAL_CLUCENE_CONFIG_H
#define _SRC_CLUCENE_INTERNAL_CLUCENE_CONFIG_H 1
 
/* src/CLucene/_clucene-config.h. 
*  Generated automatically at end of cmake.
*  These are internal definitions, and this file does not need to be distributed
*/

/* CMake will look for these functions: */
#cmakedefine _CL_HAVE__VSNWPRINTF
#cmakedefine _CL_HAVE__SNWPRINTF
#cmakedefine _CL_HAVE_WCSCASECMP
#cmakedefine _CL_HAVE_WCSCAT  1 
#cmakedefine _CL_HAVE_WCSCHR  1 
#cmakedefine _CL_HAVE_WCSCMP  1 
#cmakedefine _CL_HAVE_WCSCPY  1 
#cmakedefine _CL_HAVE_WCSCSPN  1 
#cmakedefine _CL_HAVE_WCSICMP
#cmakedefine _CL_HAVE_WCSLEN  1 
#cmakedefine _CL_HAVE_WCSNCMP  1 
#cmakedefine _CL_HAVE_WCSNCPY  1 
#cmakedefine _CL_HAVE_WCSSTR  1 
#cmakedefine _CL_HAVE_WCSTOD
#cmakedefine _CL_HAVE_WCSTOLL
#cmakedefine _CL_HAVE_WCSUPR
#cmakedefine _CL_HAVE_GETTIMEOFDAY
#cmakedefine _CL_HAVE_MAPVIEWOFFILE
#cmakedefine _CL_HAVE_USLEEP

#cmakedefine _CL_HAVE_LLTOA
#cmakedefine _CL_HAVE_LLTOW
#cmakedefine _CL_HAVE_PRINTF  1 
#cmakedefine _CL_HAVE_SNPRINTF  1 
#cmakedefine _CL_HAVE_MMAP  1 
#cmakedefine _CL_HAVE_STRLWR
#cmakedefine _CL_HAVE_STRTOLL
#cmakedefine _CL_HAVE_STRUPR
#cmakedefine _CL_HAVE_GETPAGESIZE

${SYMBOL_CL_MAX_PATH}
${SYMBOL__O_RANDOM}
${SYMBOL__O_BINARY}
${SYMBOL__S_IREAD}
${SYMBOL__S_IWRITE}
${TYPE__TIMEB}

#define _ILONG(x) x ## L
#define _ILONGLONG(x) ${_CL_ILONGLONG_VALUE}

${FUNCTION_FILESTAT}
${FUNCTION_FILESIZE}
${FUNCTION_FILESEEK}
${FUNCTION_FILETELL}
${FUNCTION_FILEHANDLESTAT}
${FUNCTION__REALPATH}
${FUNCTION__RENAME}
${FUNCTION__CLOSE}
${FUNCTION__READ}
${FUNCTION__OPEN}
${FUNCTION__WRITE}
${FUNCTION__SNPRINTF}
${FUNCTION__MKDIR}
${FUNCTION__UNLINK}
${FUNCTION__FTIME}

//todo: this is not checked, i think
#define _LUCENE_PRAGMA_WARNINGS


/* CMake will determine these specifics. Things like bugs, etc */

/* Does not support new float byte<->float conversions */
//todo: not being checked for...
#cmakedefine _CL_HAVE_NO_FLOAT_BYTE

/* Does not support try/catch blocks */
//todo: not checked properly
#cmakedefine _CL_HAVE_NO_FUNCTION_TRY_BLOCKS

/* Define if recursive pthread mutexes are available */
//todo: not being checked for...
#cmakedefine _CL_HAVE_PTHREAD_MUTEX_RECURSIVE  1 

/* Number of bits in a file offset, on hosts where this is settable. */
//todo: should we only define for hosts where this is applicable?
#define _FILE_OFFSET_BITS 64

/* Define for large files, on AIX-style hosts. */
//todo: should we only define for hosts where this is applicable?
#define _LARGE_FILES


/* Compiler oddities */

//not sure why, but cygwin reports _mkdir, but doesn't actually work...
//TODO: make this work properly (this bit shouldn't be necessary)
#ifdef __CYGWIN__
    #define _mkdir(x) mkdir(x,0777)
    #define _open open
    #define _read read
    #define _write write
    #define _close close
    #define _unlink unlink
    #define _ftime ftime
#endif

#endif
