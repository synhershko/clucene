PROJECT(clucene-shared)

#define command line options
INCLUDE (DefineOptions)
DEFINE_OPTIONS(EXTRA_OPTIONS)
ADD_DEFINITIONS(${EXTRA_OPTIONS} -DMAKE_CLUCENE_SHARED_LIB)

# include specific modules
set(CMAKE_MODULE_PATH "${clucene-shared_SOURCE_DIR}/cmake")

INCLUDE (CheckIncludeFiles)
INCLUDE (CheckIncludeFileCXX)
INCLUDE (CheckCXXSourceCompiles)
INCLUDE (CheckCXXSourceRuns)
INCLUDE (CheckFunctionExists)

#local macros
INCLUDE (MacroMustDefine)
INCLUDE (MacroChooseType)
INCLUDE (MacroChooseFunction)
INCLUDE (MacroChooseSymbol)
INCLUDE (MacroCheckGccVisibility)

INCLUDE (DefineFloat)
INCLUDE (DefineLongLongSyntax)
INCLUDE (DefineStaticSyntax)
INCLUDE (DefineMAXPATHValue)

INCLUDE (CheckErrorHandling)
INCLUDE (CheckHashmaps)
INCLUDE (CheckNamespace)
INCLUDE (CheckSnprintf)
INCLUDE (CheckPthread)

########################################################################
# test for headers
########################################################################

CHECK_INCLUDE_FILES ("sys/time.h;time.h" _CL_TIME_WITH_SYS_TIME)

CHECK_REQUIRED_HEADERS ( stdlib.h stdarg.h stddef.h ctype.h algorithm  
						 functional map vector list set math.h fcntl.h limits.h)
						 
CHECK_OPTIONAL_HEADERS ( string.h sys/time.h memory.h sys/types.h 
                        stdint.h unistd.h io.h direct.h sys/dir.h sys/ndir.h dirent.h 
                        stat.h sys/stat.h stdexcept errno.h fcntl.h windef.h windows.h wchar.h 
                        wctype.h fcntl.h hash_map hash_set ext/hash_map ext/hash_set 
                        sys/timeb.h tchar.h strings.h stdexcept sys/mman.h )


########################################################################
# test for types
########################################################################
#find int_t types
CHOOSE_TYPE(int8_t      1 signed   "int8_t;char")
CHOOSE_TYPE(uint8_t     1 unsigned "uint8_t;char")
CHOOSE_TYPE(int16_t     2 signed   "int16_t;short")
CHOOSE_TYPE(uint16_t    2 unsigned "uint16_t;short")
CHOOSE_TYPE(int32_t     4 signed   "int32_t;int;long")
CHOOSE_TYPE(uint32_t    4 unsigned "uint32_t;int;long")
CHOOSE_TYPE(int64_t     8 signed   "int64_t;long long;__int64")
CHOOSE_TYPE(uint64_t    8 unsigned "uint64_t;long long;__int64")

CHOOSE_TYPE(size_t      -1 unsigned "size_t;int" )
CHOOSE_TYPE(_timeb      -1 "" "timeb;_timeb" )

#run macro for checking float. (and _FLT_EVAL_METHOD)
DEFINE_FLOAT()

########################################################################
# Test for functions
########################################################################


CHECK_REQUIRED_FUNCTIONS( printf strftime wcscpy wcsncpy wcscat wcschr wcsstr wcslen 
                           wcscmp wcsncmp wcscspn )

#todo: wcstoq is bsd equiv of wcstoll, we can use that...
CHECK_OPTIONAL_FUNCTIONS( mmap wcsupr wcscasecmp wcsicmp wcstoll wprintf MapViewOfFile 
                         lltow wcstod strupr strlwr lltoa strtoll gettimeofday _vsnwprintf )


#make decisions about which functions to use...
CHOOSE_FUNCTION(fileStat "stati64;_stati64;stat;_stat")
CHOOSE_FUNCTION(fileSize "filelengthi64;_filelengthi64;filelength;_filelength")
CHOOSE_FUNCTION(fileSeek "lseeki64;_lseeki64;lseek;_lseek")
CHOOSE_FUNCTION(fileTell "telli64;_telli64;tell;_tell")
CHOOSE_FUNCTION(fileHandleStat "fstati64;_fstati64;fstat;_fstat")

#if fileHandleStat doesn't take a fileStat struct, we should change fileStat.
IF ( NOT _CL_HAVE_FUNCTION_FSTATI64 AND NOT _CL_HAVE_FUNCTION__FSTATI64 )
    #Borland compiler doesn't have a fstati64, so fileStat must be non-i64 as well...
    SET( FUNCTION_FILESTAT 0 )
    CHOOSE_FUNCTION(fileStat "stat;_stat")
ENDIF ( NOT _CL_HAVE_FUNCTION_FSTATI64 AND NOT _CL_HAVE_FUNCTION__FSTATI64 )

CHOOSE_FUNCTION(_realpath "realpath")
CHOOSE_FUNCTION(_rename "rename")
CHOOSE_FUNCTION(_close "_close;close")
CHOOSE_FUNCTION(_read "_read;read")
CHOOSE_FUNCTION(_open "_open;open")
CHOOSE_FUNCTION(_write "_write;write")
CHOOSE_FUNCTION(_unlink "_unlink;unlink")
CHOOSE_FUNCTION(_ftime "_ftime;ftime")
CHOOSE_FUNCTION (_mkdir _mkdir)
CHOOSE_FUNCTION (SLEEPFUNCTION "usleep;Sleep;_sleep")


CHOOSE_FUNCTION(_snprintf "snprintf;_snprintf")
CHOOSE_FUNCTION(_snwprintf "snwprintf;_snwprintf")

IF ( NOT HAVE_FUNCTION__REALPATH )
    SET (FUNCTION__REALPATH "#define _realpath(rel,abs) ::_fullpath(abs,rel,CL_MAX_PATH)")
ENDIF ( NOT HAVE_FUNCTION__REALPATH )

IF ( NOT _CL_HAVE_FUNCTION__MKDIR )
    SET (FUNCTION__MKDIR "#define _mkdir(x) mkdir(x,0777)")
ENDIF ( NOT _CL_HAVE_FUNCTION__MKDIR )

IF ( NOT HAVE__TELL )
    #ftell (and probably soon ftell64) are POSIX standard functions, but tell and
    #tell64 are not, so we define fileTell in terms of fileSeek.
    SET ( FUNCTION_TELL "#define fileTell(fhandle) fileSeek(fhandle, 0, SEEK_CUR)" )
ENDIF ( NOT HAVE__TELL )

#now that int64_t is defined, we can define this...
IF ( NOT HAVE__FILELENGTH )
    SET ( FUNCTION_FILESIZE "#define fileSize CL_NS(util)::Misc::filelength" )
ENDIF ( NOT HAVE__FILELENGTH )


########################################################################
# test for symbols
########################################################################
CHOOSE_SYMBOL (_O_RANDOM "_O_RANDOM;O_RANDOM")
CHOOSE_SYMBOL (_O_BINARY "_O_BINARY;O_BINARY")
CHOOSE_SYMBOL (_S_IREAD  "_S_IREAD;S_IREAD")
CHOOSE_SYMBOL (_S_IWRITE "_S_IWRITE;S_IWRITE")


#define defaults
IF ( NOT HAVE_SYMBOL__O_RANDOM )
	SET (SYMBOL__O_RANDOM "#define _O_RANDOM 0")
ENDIF ( NOT HAVE_SYMBOL__O_RANDOM )

IF ( NOT HAVE_SYMBOL__O_BINARY )
	SET (SYMBOL__O_BINARY "#define _O_BINARY 0")
ENDIF ( NOT HAVE_SYMBOL__O_BINARY)

IF ( NOT HAVE_SYMBOL__S_IREAD )
	SET (SYMBOL__S_IREAD "#define _S_IREAD 0333")
ENDIF ( NOT HAVE_SYMBOL__S_IREAD )

IF ( NOT HAVE_SYMBOL__S_IWRITE )
	SET (SYMBOL__S_IWRITE "#define _S_IWRITE 0333")
ENDIF ( NOT HAVE_SYMBOL__S_IWRITE )

#try and figure out the actual value of what _CL_MAX_PATH is
DEFINE_MAXPATH_VALUE (SYMBOL_CL_MAX_PATH)


########################################################################
# test for tchar replacments
########################################################################
IF ( ENABLE_ASCII_MODE )
    CHOOSE_TYPE(TCHAR   1 "" "TCHAR;char" SYMBOL_TCHAR )
ELSE ( ENABLE_ASCII_MODE )
    CHOOSE_TYPE(TCHAR   -1 "" "TCHAR;wchar_t;unsigned short" SYMBOL_TCHAR)
ENDIF ( ENABLE_ASCII_MODE )
IF ( NOT SYMBOL_TCHAR STREQUAL "TCHAR" )
    SET( SYMBOL_TCHAR "#define TCHAR ${SYMBOL_TCHAR}" )
ELSE ( NOT SYMBOL_TCHAR STREQUAL "TCHAR" )
    SET( SYMBOL_TCHAR "/* #undef TCHAR */" )
ENDIF ( NOT SYMBOL_TCHAR STREQUAL "TCHAR" )

CHOOSE_SYMBOL (_T "_T" SYMBOL__T)
IF ( NOT HAVE_SYMBOL__T )
    IF ( ENABLE_ASCII_MODE )
        SET (SYMBOL__T "#define _T(x) x")
    ELSE ( ENABLE_ASCII_MODE )
        SET (SYMBOL__T "#define _T(x) L ## x")
    ENDIF ( ENABLE_ASCII_MODE )
ELSE ( NOT HAVE_SYMBOL__T )
    SET( SYMBOL__T "/* #undef _T */" )
ENDIF ( NOT HAVE_SYMBOL__T )


########################################################################
# test for compiler capabilities.
########################################################################

#check for pthreads
find_package(Threads REQUIRED)
IF ( CMAKE_USE_WIN32_THREADS_INIT )
    SET ( _CL_HAVE_WIN32_THREADS 1 )
ENDIF ( CMAKE_USE_WIN32_THREADS_INIT )
IF ( CMAKE_USE_PTHREADS_INIT )
    SET ( _CL_HAVE_PTHREAD 1 )
ENDIF (CMAKE_USE_PTHREADS_INIT)
IF ( CMAKE_USE_SPROC_INIT OR CMAKE_HP_PTHREADS_INIT )
    MESSAGE(FATAL_ERROR "Threads library not implemented")
ENDIF( CMAKE_USE_SPROC_INIT OR CMAKE_HP_PTHREADS_INIT )

#define if we have pthreads with recusrive capabilities
CHECK_PTHREAD_RECURSIVE(_CL_HAVE_PTHREAD _CL_HAVE_PTHREAD_MUTEX_RECURSIVE)

#see if we can hide all symbols by default...
MACRO_CHECK_GCC_VISIBILITY(_CL_HAVE_GCCVISIBILITYPATCH)

#Check that we can handle try/catch
CHECK_HAVE_FUNCTION_TRY_BLOCKS (_CL_HAVE_FUNCTION_TRY_BLOCKS)

#check how to use hashmaps
CHECK_HASH_MAPS (CL_NS_HASHING_VALUE LUCENE_DISABLE_HASHING)

#check that we have namespace support
CHECK_NAMESPACE (_CL_HAVE_NAMESPACES)

#check if snprintf functions are buggy
CHECK_SNPRINTF()

#define how we are going to define 64bit numbers
DEFINE_LONGLONG_SYNTAX()

#define how to define a static const (or fallback to enum)
DEFINE_STATIC_SYNTAX()

#check that all these definitions are set, or fail...
MUSTDEFINE_VAR("HAVE_TYPE_INT8_T;HAVE_TYPE_UINT8_T;HAVE_TYPE_INT16_T;HAVE_TYPE_UINT16_T;HAVE_TYPE_UINT16_T;" )
MUSTDEFINE_VAR("HAVE_TYPE_INT32_T;HAVE_TYPE_UINT32_T;HAVE_TYPE_INT64_T;HAVE_TYPE_UINT64_T;HAVE_TYPE_SIZE_T;")


#now write out our configuration....
CONFIGURE_FILE(${clucene-shared_SOURCE_DIR}/CLucene/clucene-config.h.cmake ${clucene-shared_BINARY_DIR}/CLucene/clucene-config.h)
CONFIGURE_FILE(${clucene-shared_SOURCE_DIR}/CLucene/_clucene-config.h.cmake ${clucene-shared_BINARY_DIR}/CLucene/_clucene-config.h)

#add the files to our groups
SOURCE_GROUP("config" ./CLucene/config/*)
SOURCE_GROUP("debug" ./CLucene/debug/*)
SOURCE_GROUP("util" ./CLucene/util/*)

SET(clucene_shared_Files
	./CLucene/SharedHeader.cpp
	./CLucene/config/gunichartables.cpp
	./CLucene/config/repl_tcslwr.cpp
	./CLucene/config/repl_tcstoll.cpp
	./CLucene/config/repl_tcscasecmp.cpp
	./CLucene/config/repl_tprintf.cpp
	./CLucene/config/repl_lltot.cpp
	./CLucene/config/repl_tcstod.cpp
	./CLucene/config/utf8.cpp
	./CLucene/config/threads.cpp
	./CLucene/debug/condition.cpp
	./CLucene/debug/error.cpp
	./CLucene/util/StringBuffer.cpp
	./CLucene/util/Misc.cpp
	./CLucene/util/dirent.cpp
)
file(GLOB_RECURSE HEADERS ${clucene-shared_SOURCE_DIR}/*.h)

add_library(clucene-shared-static STATIC
	${clucene_shared_Files} ${HEADERS}
	#${clucene-shared_BINARY_DIR}/clucene-config.h ${clucene-shared_BINARY_DIR}/_clucene-config.h
)

SET_TARGET_PROPERTIES(clucene-shared-static PROPERTIES
    LINKER_LANGUAGE CXX
    VERSION ${CPACK_PACKAGE_VERSION}
    SOVERSION ${CPACK_PACKAGE_SOVERSION}
    DEFINE_SYMBOL ""
    COMPILE_DEFINITIONS_DEBUG _DEBUG
)
