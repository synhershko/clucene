#ifndef _SRC_CLUCENE_CLUCENE_CONFIG_H
#define _SRC_CLUCENE_CLUCENE_CONFIG_H 1
 
/* src/CLucene/clucene-config.h. 
*  Generated automatically at end of cmake.
*/

/* CMake will look for these headers: */
#cmakedefine _CL_HAVE_STRING_H  1
#cmakedefine _CL_HAVE_MEMORY_H  1 
#cmakedefine _CL_HAVE_UNISTD_H  1 
#cmakedefine _CL_HAVE_IO_H  1 
#cmakedefine _CL_HAVE_DIRECT_H  1 
#cmakedefine _CL_HAVE_DIRENT_H  1 
#cmakedefine _CL_HAVE_SYS_DIR_H
#cmakedefine _CL_HAVE_SYS_NDIR_H
#cmakedefine _CL_HAVE_ERRNO_H  1 
#cmakedefine _CL_HAVE_WCHAR_H  1 
#cmakedefine _CL_HAVE_WCTYPE_H   
#cmakedefine _CL_HAVE_CTYPE_H  1 
#cmakedefine _CL_HAVE_WINDOWS_H  1 
#cmakedefine _CL_HAVE_SYS_TYPES_H  1 
#cmakedefine _CL_HAVE_DLFCN_H  1 
#cmakedefine _CL_HAVE_FCNTL_H  1 
#cmakedefine _CL_HAVE_EXT_HASH_MAP  1 
#cmakedefine _CL_HAVE_EXT_HASH_SET  1 
#cmakedefine _CL_HAVE_HASH_MAP
#cmakedefine _CL_HAVE_HASH_SET
#cmakedefine _CL_HAVE_NDIR_H
#cmakedefine _CL_HAVE_SYS_STAT_H  1 
#cmakedefine _CL_HAVE_SYS_TIMEB_H  1 
#cmakedefine _CL_HAVE_SYS_TIME_H 1
#cmakedefine _CL_HAVE_TCHAR_H 1


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

#cmakedefine _CL_HAVE_LLTOA
#cmakedefine _CL_HAVE_LLTOW
#cmakedefine _CL_HAVE_PRINTF  1 
#cmakedefine _CL_HAVE_SNPRINTF  1 
#cmakedefine _CL_HAVE_MMAP  1 
#cmakedefine _CL_HAVE_NANOSLEEP  1 
#cmakedefine _CL_HAVE_STRLWR
#cmakedefine _CL_HAVE_STRTOLL
#cmakedefine _CL_HAVE_STRUPR
#cmakedefine _CL_HAVE_GETPAGESIZE

#define CL_NS_HASHING(func) ${CL_NS_HASHING_VALUE}

${SYMBOL_CL_MAX_PATH}
${SYMBOL_O_RANDOM}
${SYMBOL_O_BINARY}
${SYMBOL__S_IREAD}
${SYMBOL__S_IWRITE}

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


// our needed types
#if !@HAVE_INT8_T@
 #define HAVE_INT8_T 1
 #if ${SIZEOF_CHAR}==1 //is char one byte?
  typedef signed char int8_t;
 #else
  #error Could not determine type for int8_t!
 #endif
#endif

#if !@HAVE_UINT8_T@
 #define HAVE_UINT8_T 1
 #if ${SIZEOF_CHAR}==1 //is char one byte?
  typedef unsigned char uint8_t;
 #else
  #error Could not determine type for uint8_t!
 #endif
#endif

#if !@HAVE_INT16_T@
 #define HAVE_INT16_T 1
 #if ${SIZEOF_SHORT}==2 //is short two bytes?
  typedef signed short int16_t;
 #else
  #error Could not determine type for int16_t!
 #endif
#endif

#if !@HAVE_UINT16_T@
 #define HAVE_UINT16_T 1
 #if ${SIZEOF_SHORT}==2 //is short two bytes?
  typedef unsigned short uint16_t;
 #else
  #error Could not determine type for uint16_t!
 #endif
#endif

#if !@HAVE_INT32_T@
 #define HAVE_INT32_T 1
 #if ${SIZEOF_INT}==4 //is int four bytes?
  typedef signed int int32_t;
 #elif ${SIZEOF_LONG}==4 //is long four bytes?
  typedef signed long int32_t;
 #else
  #error Could not determine type for int32_t!
 #endif
#endif

#if !@HAVE_UINT32_T@
 #define HAVE_UINT32_T 1
 #if ${SIZEOF_INT}==4 //is int four bytes?
  typedef unsigned int uint32_t;
 #elif ${SIZEOF_LONG}==4 //is long four bytes?
  typedef unsigned long uint32_t;
 #else
  #error Could not determine type for uint32_t!
 #endif
#endif

#if !@HAVE_INT64_T@
    #define HAVE_INT64_T 1
    typedef signed ${TYPEOF_INT64} int64_t;
#endif

#if !@HAVE_UINT64_T@
    #define HAVE_UINT64_T 1
    typedef unsigned ${TYPEOF_INT64} uint64_t;
#endif

#define _LUCENE_PRAGMA_WARNINGS

//todo: not sure if this detects correctly? check float.h?
#if !@HAVE_FLOAT_T@
    typedef double float_t;
#endif

//todo not sure if this detects correctly... (should check string.h??)
#if !@HAVE_SIZE_T@
 #ifndef _SIZE_T_DEFINED 
  #ifndef HAVE_SIZE_T
   typedef unsigned int size_t;
   #define HAVE_SIZE_T 1
  #endif
  #define _SIZE_T_DEFINED 1     // kdewin32 define
 #endif
#endif


/* CMake will determine these specifics. Things like bugs, etc */

/* if we can't support the map/set hashing */
#cmakedefine LUCENE_DISABLE_HASHING 1

/* Define if you have POSIX threads libraries and header files. */
#cmakedefine _CL_HAVE_PTHREAD  1 

/* Define if you have Win32 threads libraries and header files. */
#cmakedefine _CL_HAVE_WIN32_THREADS  1 

/* Define what eval method is required for float_t to be defined (for GCC). */
#cmakedefine _FLT_EVAL_METHOD  ${_FLT_EVAL_METHOD} 

/* define if the compiler implements namespaces */
#cmakedefine _CL_HAVE_NAMESPACES   

/* Does not support new float byte<->float conversions */
//todo: not being checked for...
#cmakedefine _CL_HAVE_NO_FLOAT_BYTE

/* Does not support try/catch blocks */
//todo: not checked properly
#cmakedefine _CL_HAVE_NO_FUNCTION_TRY_BLOCKS

/* Define if recursive pthread mutexes are available */
//todo: not being checked for...
#cmakedefine _CL_HAVE_PTHREAD_MUTEX_RECURSIVE  1 

/* Defined if the snprintf overflow test fails */
#cmakedefine _CL_HAVE_SNPRINTF_BUG

/* Defined if the swprintf test fails */
#cmakedefine _CL_HAVE_SNWPRINTF_BUG   

/* How to define a static const in a class */
#define LUCENE_STATIC_CONSTANT(type, assignment)  ${LUCENE_STATIC_CONSTANT_SYNTAX}

/* Define to the necessary symbol if this constant uses a non-standard name on
   your system. */
//todo: not checked
#cmakedefine _CL_PTHREAD_CREATE_JOINABLE

/* Define to 1 if the `S_IS*' macros in <sys/stat.h> do not work properly. */
//todo: not being checked for...
//#cmakedefine _CL_STAT_MACROS_BROKEN

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
//not actually used for anything...
//#cmakedefine _CL_TIME_WITH_SYS_TIME  1 

/* Number of bits in a file offset, on hosts where this is settable. */
//todo: should we only define for hosts where this is applicable?
#define _FILE_OFFSET_BITS 64

/* Define for large files, on AIX-style hosts. */
//todo: should we only define for hosts where this is applicable?
#define _LARGE_FILES

/* Define that we will be using -fvisibility=hidden, and 
 * make public classes visible using __attribute__ ((visibility("default")))
 */
#cmakedefine HAVE_GCCVISIBILITYPATCH 1

/* Versions, etc */

/* Version number of package */
#define _CL_VERSION  "0.9.21-svn" 

/* Name of package */
#define _CL_PACKAGE  "clucene-core" 



/* Configured options (from command line) */

/* Forces into Ascii mode */
#cmakedefine _ASCII

/* Conditional Debugging */
#cmakedefine _CL__CND_DEBUG

/* debuging option */
#cmakedefine _DEBUG

/* Disable multithreading */
#cmakedefine _CL_DISABLE_MULTITHREADING


#endif
