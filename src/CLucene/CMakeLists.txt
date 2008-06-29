PROJECT(clucene-core)

INCLUDE (CheckIncludeFiles)
INCLUDE (CheckIncludeFileCXX)
INCLUDE (CheckCXXSourceCompiles)
INCLUDE (CheckCXXSourceRuns)
INCLUDE (CheckSymbolExists)
INCLUDE (CheckTypeSize)
INCLUDE (CheckCXXCompilerFlag)

CHECK_INCLUDE_FILES ("sys/time.h;time.h" _CL_TIME_WITH_SYS_TIME)

SET ( OPTIONAL_HEADERS  string.h sys/time.h memory.h sys/types.h
                        stdint.h unistd.h io.h direct.h sys/dir.h sys/ndir.h dirent.h 
                        stat.h sys/stat.h stdexcept errno.h fcntl.h windows.h wchar.h 
                        wctype.h fcntl.h hash_map hash_set ext/hash_map ext/hash_set 
                        sys/timeb.h tchar.h strings.h stdexcept)
SET ( COMPULSARY_HEADERS stdlib.h cstdarg cstddef stddef.h ctype.h algorithm 
						 functional map vector list set math.h fcntl.h limits.h)

#optional C headers
FOREACH(inc ${OPTIONAL_HEADERS})
	STRING(TOUPPER ${inc} INC)
	STRING(REPLACE . _ INC ${INC})
	STRING(REPLACE / _ INC ${INC})
	CHECK_INCLUDE_FILE_CXX (${inc} HAVE_${INC})
	IF ( HAVE_${INC} )
	    SET(_CL_HAVE_${INC} ${INC})
	ENDIF ( HAVE_${INC} )
ENDFOREACH(inc ${OPTIONAL_HEADERS})

#compulsary headers
FOREACH(inc ${COMPULSARY_HEADERS})
	STRING(TOUPPER ${inc} INC)
	STRING(REPLACE . _ INC ${INC})
	CHECK_INCLUDE_FILE_CXX (${inc} HAVE_${INC})
	IF ( HAVE_${INC} )
	    SET(_CL_HAVE_${INC} ${INC})
	ENDIF ( HAVE_${INC} )
	IF ( NOT HAVE_${INC} )
		MESSAGE ( FATAL_ERROR, "${inc} could not be found" )
	ENDIF ( NOT HAVE_${INC} )
ENDFOREACH(inc ${COMPULSARY_HEADERS})

#if defined(_CL_HAVE_HASH_MAP)
	#include <hash_map>
#elif defined(_CL_HAVE_EXT_HASH_MAP)
	#include <ext/hash_map>
#endif


CHECK_CXX_SOURCE_COMPILES("
	#include <math.h>
	int main(){ float_t x=0.0; }" 
	HAVE_FLOAT_T)

IF ( NOT HAVE_SYMBOL_FLOAT_T )
    #try using FLT_EVAL_METHOD
    CHECK_CXX_SOURCE_COMPILES("
	    #define FLT_EVAL_METHOD 2
	    #include <math.h>
	    int main(){ float_t x=0.0; }" 
	    HAVE_SYMBOL_FLOAT_T)
    
    IF ( HAVE_SYMBOL_FLOAT_T )
        SET ( _FLT_EVAL_METHOD 2 )
    ENDIF ( HAVE_SYMBOL_FLOAT_T )

ENDIF ( NOT HAVE_SYMBOL_FLOAT_T )


CHECK_TYPE_SIZE(int8_t      INT8_T)
CHECK_TYPE_SIZE(uint8_t     UINT8_T)
CHECK_TYPE_SIZE(int16_t     INT16_T)
CHECK_TYPE_SIZE(uint16_t    UINT16_T)
CHECK_TYPE_SIZE(int32_t     INT32_T)
CHECK_TYPE_SIZE(uint32_t    UINT32_T)
CHECK_TYPE_SIZE(int64_t     INT64_T)
CHECK_TYPE_SIZE(uint64_t    UINT64_T)

CHECK_TYPE_SIZE(char        SIZEOF_CHAR)
CHECK_TYPE_SIZE(short       SIZEOF_SHORT)
CHECK_TYPE_SIZE(size_t      SIZE_T)

CHECK_TYPE_SIZE(int         SIZEOF_INT)
CHECK_TYPE_SIZE(long        SIZEOF_LONG)

IF ( ${SIZEOF_INT}=4 )
	SET (TYPEOF_INT32 "int")
ENDIF( ${SIZEOF_INT}=4 )
IF ( SIZEOF_LONG=4 )
	SET (TYPEOF_INT32 "long")
ENDIF ( SIZEOF_LONG=4 )

CHECK_TYPE_SIZE("long long" SIZEOF_LONGLONG)
CHECK_TYPE_SIZE(__int64     SIZEOF_INT64)
IF ( SIZEOF_INT64 )
	SET (TYPEOF_INT64 __int64)
ENDIF ( SIZEOF_INT64 )
IF (SIZEOF_LONGLONG )
	SET (TYPEOF_INT64 "long long")
ENDIF ( SIZEOF_LONGLONG )
	
CHECK_TYPE_SIZE(uint        UINT)
CHECK_TYPE_SIZE(intptr_t    INTPTR_T)

INCLUDE (CheckFunctionExists)

SET ( COMPULSARY_FUNCTIONS printf wcscpy wcsncpy wcscat wcschr wcsstr wcslen 
                           wcscmp wcsncmp wcscspn )

SET ( OPTIONAL_FUNCTIONS mmap wcsupr wcscasecmp wcsicmp wcstoll wprintf
                         lltow wcstod strupr strlwr lltoa strtoll gettimeofday _vsnwprintf)

#check for compulsary functions
FOREACH(inc ${COMPULSARY_FUNCTIONS})
	STRING(TOUPPER ${inc} INC)
	STRING(REPLACE . _ INC ${INC})
	CHECK_FUNCTION_EXISTS (${inc} _CL_HAVE_${INC})
	IF ( NOT _CL_HAVE_${INC} )
		MESSAGE ( FATAL_ERROR, "${inc} could not be found" )
	ENDIF ( NOT _CL_HAVE_${INC} )
ENDFOREACH(inc ${COMPULSARY_FUNCTIONS})

#check for optional functions
#todo: wcstoq is bsd equiv of wcstoll, we can use that...
FOREACH(inc ${OPTIONAL_FUNCTIONS})
	STRING(TOUPPER ${inc} INC)
	STRING(REPLACE . _ INC ${INC})
	CHECK_FUNCTION_EXISTS (${inc} _CL_HAVE_${INC})
ENDFOREACH(inc ${OPTIONAL_FUNCTIONS})

#macro that sets OUTPUT as the value of oneof options (if _CL_HAVE_OPTION exists)
MACRO(CHOOSE_FUNCTION name options)
    STRING(TOUPPER ${name} NAME)
    FOREACH(option ${options})
        IF ( NOT FUNCTION_${NAME} )
            STRING(TOUPPER ${option} OPTION)
    	    CHECK_FUNCTION_EXISTS (${option} _CL_HAVE_FUNCTION_${OPTION})
            IF ( _CL_HAVE_FUNCTION_${OPTION} )
				IF ( option STREQUAL ${name} )
					#already have it, ignore this...
					SET (FUNCTION_${NAME} "/* undef ${name} ${option} */" )
				ELSE ( option STREQUAL ${name} )
					SET (FUNCTION_${NAME} "#define ${name} ${option}")
				ENDIF ( option STREQUAL ${name} )
    	    ENDIF ( _CL_HAVE_FUNCTION_${OPTION} )
    	ENDIF( NOT FUNCTION_${NAME} )
    ENDFOREACH(option ${options})
    
    IF ( NOT FUNCTION_${NAME} )
        SET (FUNCTION_${NAME} "/* undef ${name} */" )
    ELSE ( NOT FUNCTION_${NAME} )
        SET (HAVE_${NAME} 1)
    ENDIF ( NOT FUNCTION_${NAME} )
ENDMACRO(CHOOSE_FUNCTION)

IF ( HAVE_WINDOWS_H )
	SET (CHOOSE_SYMBOL_INCLUDES "${CHOOSE_SYMBOL_INCLUDES};windows.h")
ENDIF ( HAVE_WINDOWS_H )
IF ( HAVE_LIMITS_H )
	SET (CHOOSE_SYMBOL_INCLUDES "${CHOOSE_SYMBOL_INCLUDES};limits.h")
ENDIF ( HAVE_LIMITS_H )
IF ( HAVE_STAT_H )
	SET (CHOOSE_SYMBOL_INCLUDES "${CHOOSE_SYMBOL_INCLUDES};stat.h")
ENDIF ( HAVE_STAT_H )
IF ( HAVE_SYS_STAT_H )
	SET (CHOOSE_SYMBOL_INCLUDES "${CHOOSE_SYMBOL_INCLUDES};sys/stat.h")
ENDIF ( HAVE_SYS_STAT_H )
IF ( HAVE_FCNTL_H )
	SET (CHOOSE_SYMBOL_INCLUDES "${CHOOSE_SYMBOL_INCLUDES};fcntl.h")
ENDIF ( HAVE_FCNTL_H )
MACRO(CHOOSE_SYMBOL name options)
    STRING(TOUPPER ${name} NAME)
    FOREACH(option ${options})
        IF ( NOT SYMBOL_${NAME} )
            STRING(TOUPPER ${option} OPTION)
    	    CHECK_SYMBOL_EXISTS (${option} "${CHOOSE_SYMBOL_INCLUDES}" _CL_HAVE_SYMBOL_${OPTION})
            IF ( _CL_HAVE_SYMBOL_${OPTION} )
            	IF ( option STREQUAL ${name} )
					#already have it, ignore this...
					SET (SYMBOL_${NAME} "/* undef ${name} ${option} */" )
				ELSE ( option STREQUAL ${name} )
					SET (SYMBOL_${NAME} "#define ${name} ${option}")
				ENDIF ( option STREQUAL ${name} )
    	    ENDIF ( _CL_HAVE_SYMBOL_${OPTION} )
    	ENDIF( NOT SYMBOL_${NAME} )
    ENDFOREACH(option ${options})
    
    IF ( NOT SYMBOL_${NAME} )
        SET (SYMBOL_${NAME} "/* undef ${name} */" )
    ELSE ( NOT SYMBOL_${NAME} )
        SET (HAVE_SYMBOL_${NAME} 1)
    ENDIF ( NOT SYMBOL_${NAME} )
ENDMACRO(CHOOSE_SYMBOL)


#Now make decisions about which functions to use...
CHOOSE_FUNCTION(fileStat "stati64;_stati64;stat;_stat")
CHOOSE_FUNCTION(fileSize "filelengthi64;_filelengthi64;filelength;_filelength")
CHOOSE_FUNCTION(fileSeek "lseeki64;_lseeki64;lseek;_lseek")
CHOOSE_FUNCTION(fileTell "telli64;_telli64;tell;_tell")
CHOOSE_FUNCTION(fileHandleStat "fstati64;_fstati64;fstat;_fstat")

#if fileHandleStat doesn't take a fileStat struct, we should change fileStat.
#IF ( NOT _CL_HAVE_FUNCTION_FSTATI64 AND NOT _CL_HAVE_FUNCTION__FSTATI64 )
    #Borland compiler doesn't have a fstati64, so fileStat must be non-i64 as well...
#    SET( FUNCTION_FILESTAT 0 )
#    CHOOSE_FUNCTION(fileStat "stat;_stat")
#ENDIF ( NOT _CL_HAVE_FUNCTION_FSTATI64 AND NOT _CL_HAVE_FUNCTION__FSTATI64 )

CHOOSE_FUNCTION(_realpath "realpath")
CHOOSE_FUNCTION(_rename "rename")
CHOOSE_FUNCTION(_close "_close;close")
CHOOSE_FUNCTION(_read "_read;read")
CHOOSE_FUNCTION(_open "_open;open")
CHOOSE_FUNCTION(_write "_write;write")
CHOOSE_FUNCTION(_unlink "_unlink;unlink")

CHOOSE_FUNCTION(_snprintf "snprintf;_snprintf")
CHOOSE_FUNCTION(_snwprintf "snwprintf;_snwprintf")

IF ( NOT HAVE__REALPATH )
    SET (FUNCTION__REALPATH "#define _realpath(rel,abs) ::_fullpath(abs,rel,CL_MAX_PATH)")
ENDIF ( NOT HAVE__REALPATH )

CHECK_FUNCTION_EXISTS (_mkdir _CL_HAVE_FUNCTION__MKDIR)
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
    SET ( FUNCTION_FILESIZE "#define fileSize lucene_filelength" )
ENDIF ( NOT HAVE__FILELENGTH )

CHOOSE_SYMBOL(CL_MAX_PATH "PATH_MAX;MAX_PATH;_MAX_PATH;_POSIX_PATH_MAX")
CHOOSE_SYMBOL (O_RANDOM O_RANDOM)
CHOOSE_SYMBOL (O_BINARY O_BINARY)
CHOOSE_SYMBOL (_S_IREAD "_S_IREAD;S_IREAD")
CHOOSE_SYMBOL (_S_IWRITE "_S_IWRITE;S_IWRITE")

#todo make a thing at the end that checks for all un-determined values...
IF ( NOT HAVE_SYMBOL_CL_MAX_PATH )
	MESSAGE ( FATAL_ERROR "Couldn't determine CL_MAX_PATH value" )
ENDIF ( NOT HAVE_SYMBOL_CL_MAX_PATH )

IF ( NOT HAVE_SYMBOL_O_RANDOM )
	SET (SYMBOL_O_RANDOM "#define O_RANDOM 0")
ENDIF ( NOT HAVE_SYMBOL_O_RANDOM )
IF ( NOT HAVE_SYMBOL_O_BINARY )
	SET (SYMBOL_O_BINARY "#define O_BINARY 0")
ENDIF ( NOT HAVE_SYMBOL_O_BINARY)
IF ( NOT HAVE_SYMBOL__S_IREAD )
	SET (SYMBOL__S_IREAD "#define _S_IREAD 0333")
ENDIF ( NOT HAVE_SYMBOL__S_IREAD )
IF ( NOT HAVE_SYMBOL__S_IWRITE )
	SET (SYMBOL__S_IWRITE "#define _S_IWRITE 0333")
ENDIF ( NOT HAVE_SYMBOL__S_IWRITE )


#check which syntax of static const to use
CHECK_CXX_SOURCE_COMPILES("class x{public: static const int SCI=55; }; int main(){ x a; if ( a.SCI!=55 ) throw \"err\"; }" LUCENE_STATIC_CONSTANT_SYNTAX)
IF ( LUCENE_STATIC_CONSTANT_SYNTAX )
    SET ( LUCENE_STATIC_CONSTANT_SYNTAX "static const type assignment")
ELSE ( LUCENE_STATIC_CONSTANT_SYNTAX )
    SET ( LUCENE_STATIC_CONSTANT_SYNTAX "enum { assignment }")
ENDIF ( LUCENE_STATIC_CONSTANT_SYNTAX )


#check how to define a long (and longlong number)
CHECK_CXX_SOURCE_COMPILES("int main(){ int x = (int)(1234LL); }" _CL_ILONGLONG_LL)
IF ( _CL_ILONGLONG_LL )
    SET(_CL_ILONGLONG_VALUE "x ## LL")
ELSE ( _CL_ILONGLONG_LL )
    CHECK_CXX_SOURCE_COMPILES("int main(){ int x = (int)(1234i64); }" _CL_ILONGLONG_i64)
    IF ( _CL_ILONGLONG_i64 )
        SET(_CL_ILONGLONG_VALUE "x ## i64")
    ELSE ( _CL_ILONGLONG_i64 )
        MESSAGE( FATAL_ERROR "_CL_ILONGLONG_VALUE could not be determined" )
    ENDIF ( _CL_ILONGLONG_i64 )
ENDIF ( _CL_ILONGLONG_LL )

#Check if namespaces work in the compiler
CHECK_CXX_SOURCE_COMPILES("
	namespace Outer { namespace Inner { int i = 0; }}
	int main(){ using namespace Outer::Inner; return i; }" 
	_CL_HAVE_NAMESPACES)

#todo:check if our float<>byte tests work...


#check that our snprintf works correctly...
IF ( _CL_HAVE_FUNCTION_SNPRINTF )
CHECK_CXX_SOURCE_RUNS("
	#include <stdio.h>
	int main(){
		char ovbuf[7];
		int i;
		for (i=0; i<7; i++) ovbuf[i]='x';
		snprintf(ovbuf, 4,\"foo%s\", \"bar\");
		if (ovbuf[5]!='x') return 1;
		snprintf(ovbuf, 4,\"foo%d\", 666);
		if (ovbuf[5]!='x') return 1;
		return 0;
	}" _CL_HAVE_NO_SNPRINTF_BUG) 
	IF ( NOT _CL_HAVE_NO_SNPRINTF_BUG )
		SET ( _CL_HAVE_SNPRINTF_BUG 1 )
		MESSAGE ( FATAL_ERROR "snprintf has a bug, and we don't have a replacement!" )
	ENDIF ( NOT _CL_HAVE_NO_SNPRINTF_BUG )
ENDIF ( _CL_HAVE_FUNCTION_SNPRINTF )

#check that our swnprintf works correctly...
IF ( _CL_HAVE_FUNCTION_SNWPRINTF )
CHECK_CXX_SOURCE_RUNS("
	#include <stdio.h>
	#include <wchar.h>
	
	int main(void)
	{
    	wchar_t buf[5];
    	snwprintf(buf,5,L\"%s\",L\"foo\");
    	if ( wcslen(buf) != 3 )
    		return 1;
    	return 0;
	}" _CL_HAVE_NO_SNWPRINTF_BUG) 
	IF ( NOT _CL_HAVE_NO_SNWPRINTF_BUG )
		SET ( _CL_HAVE_SNWPRINTF_BUG 1 )
	ENDIF ( NOT _CL_HAVE_NO_SNWPRINTF_BUG )
ENDIF ( _CL_HAVE_FUNCTION_SNWPRINTF )


#check for try/catch blocks
CHECK_CXX_SOURCE_RUNS("
	void foo() try{} catch( ... ){}
	int main(){ foo(); }" _CL_HAVE_FUNCTION_TRY_BLOCKS)
IF ( NOT _CL_HAVE_FUNCTION_TRY_BLOCKS )
	SET ( _CL_HAVE_NO_FUNCTION_TRY_BLOCKS 1 )
ENDIF ( NOT _CL_HAVE_FUNCTION_TRY_BLOCKS )

IF ( _CL_HAVE_EXT_HASH_MAP OR D_CL_HAVE_HASH_MAP )
    #find hashing namespace...
    MACRO(HASHMAP_TEST namespace)
    	IF ( NOT CL_NS_HASHING_VALUE )
            IF ( _CL_HAVE_EXT_HASH_MAP )
                SET(CMAKE_REQUIRED_DEFINITIONS "-D_CL_HAVE_EXT_HASH_MAP=1")
            ENDIF ( _CL_HAVE_EXT_HASH_MAP )
            IF ( _CL_HAVE_HASH_MAP )
                SET(CMAKE_REQUIRED_DEFINITIONS "-D_CL_HAVE_HASH_MAP=1")
            ENDIF ( _CL_HAVE_HASH_MAP )
            
            STRING(TOUPPER ${namespace} NAMESPACE)
            STRING(REPLACE / _ NAMESPACE ${NAMESPACE})
            	    
            CHECK_CXX_SOURCE_COMPILES("
    #if defined(_CL_HAVE_HASH_MAP)
    	#include <hash_map>
    #elif defined(_CL_HAVE_EXT_HASH_MAP)
    	#include <ext/hash_map>
    #endif
    int main() {  
        ${namespace}::hash_map<int,char> a; 
        return 0; 
    }
            " _CL_HAVE_${NAMESPACE}_HASHMAP)
            
            IF ( _CL_HAVE_${NAMESPACE}_HASHMAP )
                SET (CL_NS_HASHING_VALUE "${namespace}::func")
            ENDIF ( _CL_HAVE_${NAMESPACE}_HASHMAP )
        
        ENDIF ( NOT CL_NS_HASHING_VALUE )
    ENDMACRO(HASHMAP_TEST)
    
    HASHMAP_TEST (std)
    HASHMAP_TEST (stdext)
    HASHMAP_TEST (__gnu_cxx)

    IF ( NOT CL_NS_HASHING_VALUE )
        MESSAGE ( STATUS "CL_NS_HASHING not determined, disabling hasing (not a big problem)")
        SET(LUCENE_DISABLE_HASHING 1)
    ENDIF ( NOT CL_NS_HASHING_VALUE )

ENDIF ( _CL_HAVE_EXT_HASH_MAP OR D_CL_HAVE_HASH_MAP )

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

#see if we can hide all symbols by default...
IF (CMAKE_COMPILER_IS_GNUCXX)
    CHECK_CXX_COMPILER_FLAG(-fvisibility=hidden _CL_HAVE_GCCVISIBILITYPATCH)
    IF (_CL_HAVE_GCCVISIBILITYPATCH)
        ADD_DEFINITIONS(-fvisibility=hidden)
    ENDIF (_CL_HAVE_GCCVISIBILITYPATCH)
ENDIF (CMAKE_COMPILER_IS_GNUCXX)


#turn these args into true/false valid for if @asdf@
SET ( TRUEFALSE "HAVE_O_RANDOM;HAVE_O_BINARY;HAVE__S_IREAD;HAVE__S_IWRITE;HAVE_FLOAT_T" )
SET ( TRUEFALSE "${TRUEFALSE};HAVE_INT8_T;HAVE_UINT8_T;HAVE_INT16_T;HAVE_UINT16_T;HAVE_UINT16_T;" )
SET ( TRUEFALSE "${TRUEFALSE};HAVE_INT32_T;HAVE_UINT32_T;HAVE_INT64_T;HAVE_UINT64_T;HAVE_SIZE_T")
foreach(ARG ${TRUEFALSE})
    IF ( ${ARG} )
        SET ( ${ARG} "1" )
    ELSE ( ${ARG} )
        SET ( ${ARG} "0" )
    ENDIF ( ${ARG} )
endforeach ( ARG )

#check that all these definitions are set, or fail...
SET ( MUSTDEFINE "" )
foreach(ARG ${MUSTDEFINE})
	IF ( NOT ${ARG} )
		MESSAGE( FATAL_ERROR, "The symbol '${ARG}' was not defined." )
	ENDIF ( NOT ${ARG} )
endforeach(ARG)

#now write out our configuration....
CONFIGURE_FILE(${clucene-core_SOURCE_DIR}/clucene-config.h.cmake ${clucene-core_BINARY_DIR}/clucene-config.h)
CONFIGURE_FILE(${clucene-core_SOURCE_DIR}/_clucene-config.h.cmake ${clucene-core_BINARY_DIR}/_clucene-config.h)

#add the files to our groups and core
SOURCE_GROUP("analysis\\standard" ./analysis/standard/*)
SOURCE_GROUP("analysis" ./analysis/*)
SOURCE_GROUP("config" ./config/*)
SOURCE_GROUP("debug" ./debug/*)
SOURCE_GROUP("document" ./document/*)
SOURCE_GROUP("index" ./index/*)
SOURCE_GROUP("queryParser" ./queryParser/*)
SOURCE_GROUP("search" ./search/*)
SOURCE_GROUP("store" ./store/*)
SOURCE_GROUP("util" ./util/*)
SOURCE_GROUP("" FILES ./StdHeader.cpp)

file(GLOB_RECURSE clucene_core_Headers ${CMAKE_SOURCE_DIR}/src/[A-Za-z]?*.h)

SET(clucene_core_Files
	./StdHeader.cpp
	./util/ThreadLocal.cpp
	./config/gunichartables.cpp
	./config/repl_tcslwr.cpp
	./config/repl_tcstoll.cpp
	./config/repl_tcscasecmp.cpp
	./config/repl_tprintf.cpp
	./config/repl_lltot.cpp
	./config/repl_tcstod.cpp
	./config/utf8.cpp
	./config/threads.cpp
	./debug/condition.cpp
	./debug/error.cpp
	./util/StringBuffer.cpp
	./util/Reader.cpp
	./util/Equators.cpp
	./util/FastCharStream.cpp
	./util/fileinputstream.cpp
	./util/MD5Digester.cpp
	./util/dirent.cpp
	./util/StringIntern.cpp
	./util/BitSet.cpp
	./util/Misc.cpp
	./queryParser/Lexer.cpp
	./queryParser/MultiFieldQueryParser.cpp
	./queryParser/QueryParser.cpp
	./queryParser/QueryParserBase.cpp
	./queryParser/QueryToken.cpp
	./queryParser/TokenList.cpp
	./analysis/standard/StandardAnalyzer.cpp
	./analysis/standard/StandardFilter.cpp
	./analysis/standard/StandardTokenizer.cpp
	./analysis/Analyzers.cpp
	./analysis/AnalysisHeader.cpp
	./store/MMapInput.cpp
	./store/TransactionalRAMDirectory.cpp
	./store/IndexInput.cpp
	./store/Lock.cpp
	./store/LockFactory.cpp
	./store/IndexOutput.cpp
	./store/Directory.cpp
	./store/FSDirectory.cpp
	./store/RAMDirectory.cpp
	./document/Document.cpp
	./document/DateField.cpp
	./document/DateTools.cpp
	./document/Field.cpp
	./index/IndexFileNames.cpp
	./index/SegmentMergeInfo.cpp
	./index/SegmentInfos.cpp
	./index/SegmentTermDocs.cpp
	./index/FieldsWriter.cpp
	./index/TermInfosWriter.cpp
	./index/Term.cpp
	./index/Terms.cpp
	./index/DocumentWriter.cpp
	./index/SegmentTermVector.cpp
	./index/TermVectorReader.cpp
	./index/FieldInfos.cpp
	./index/CompoundFile.cpp
	./index/SegmentReader.cpp
	./index/TermVectorWriter.cpp
	./index/IndexReader.cpp
	./index/SegmentTermPositions.cpp
	./index/SegmentMerger.cpp
	./index/IndexWriter.cpp
	./index/MultiReader.cpp
	./index/SegmentTermEnum.cpp
	./index/TermInfo.cpp
	./index/IndexModifier.cpp
	./index/SegmentMergeQueue.cpp
	./index/FieldsReader.cpp
	./index/TermInfosReader.cpp
	./search/Compare.cpp
	./search/Scorer.cpp
	./search/ScorerDocQueue.cpp
	./search/PhraseScorer.cpp
	./search/SloppyPhraseScorer.cpp
	./search/DisjunctionSumScorer.cpp
	./search/ConjunctionScorer.cpp
	./search/PhraseQuery.cpp
	./search/PrefixQuery.cpp
	./search/ExactPhraseScorer.cpp
	./search/TermScorer.cpp
	./search/Similarity.cpp
	./search/BooleanScorer.cpp
	./search/BooleanScorer2.cpp
	./search/HitQueue.cpp
	./search/FieldCacheImpl.cpp
	./search/ChainedFilter.cpp
	./search/RangeFilter.cpp
	./search/CachingWrapperFilter.cpp
	./search/QueryFilter.cpp
	./search/TermQuery.cpp
	./search/FuzzyQuery.cpp
	./search/SearchHeader.cpp
	./search/RangeQuery.cpp
	./search/IndexSearcher.cpp
	./search/Sort.cpp
	./search/PhrasePositions.cpp
	./search/FieldDocSortedHitQueue.cpp
	./search/WildcardTermEnum.cpp
	./search/MultiSearcher.cpp
	./search/Hits.cpp
	./search/MultiTermQuery.cpp
	./search/FilteredTermEnum.cpp
	./search/FieldSortedHitQueue.cpp
	./search/WildcardQuery.cpp
	./search/Explanation.cpp
	./search/BooleanQuery.cpp
	./search/FieldCache.cpp
	./search/DateFilter.cpp
)



OPTION(ENABLE_ASCII_MODE
  "enable ascii support"
  OFF)
IF(ENABLE_ASCII_MODE)
	ADD_DEFINITIONS(-D_ASCII)
ELSE(ENABLE_ASCII_MODE)
	ADD_DEFINITIONS(-D_UNICODE)
ENDIF(ENABLE_ASCII_MODE)

ADD_DEFINITIONS(-DMAKE_CLUCENE_CORE_LIB)

#todo: this isn't correct...
IF (MSVC8)
ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
ENDIF ( MSVC8)

add_library(clucene-core SHARED 
	${clucene_core_Files}
)
add_library(clucene-core-static STATIC 
	${clucene_core_Files}
)



SET_TARGET_PROPERTIES(clucene-core PROPERTIES
#    VERSION "0.9.16a"
#    SOVERSION "0.9"
DEFINE_SYMBOL "CLUCENE_EXPORT_SYMBOLS"
)

SET_TARGET_PROPERTIES(clucene-core-static PROPERTIES
#    VERSION "0.9.16a"
#    SOVERSION "0.9"
DEFINE_SYMBOL ""
)

#set_target_properties(clucene-core
#    PROPERTIES
#    VERSION ${STRIGI_VERSION}
#    SOVERSION ${STRIGI_VERSION_MAJOR}
#    DEFINE_SYMBOL MAKE_STREAMS_LIB
#)

file(GLOB_RECURSE HEADERS ${CMAKE_SOURCE_DIR}/src/[A-Za-z]?*.h)

foreach(file ${HEADERS})
	get_filename_component(apath ${file} PATH)
	file(RELATIVE_PATH relpath ${CMAKE_SOURCE_DIR}/src ${apath})
	if (relpath STREQUAL "demo")
	else (relpath STREQUAL "demo")
		install(FILES ${file} DESTINATION include/${relpath} )
	endif (relpath STREQUAL "demo")
endforeach(file)

install(TARGETS clucene-core DESTINATION lib)
install(TARGETS clucene-core-static DESTINATION lib)
