PROJECT(clucene-core)

INCLUDE (CheckIncludeFiles)
INCLUDE (CheckIncludeFileCXX)
INCLUDE (CheckCXXSourceCompiles)
INCLUDE (CheckCXXSourceRuns)
INCLUDE (CheckCXXCompilerFlag)
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
INCLUDE (DefineOptions)
INCLUDE (DefineMAXPATHValue)

INCLUDE (CheckErrorHandling)
INCLUDE (CheckHashmaps)
INCLUDE (CheckNamespace)
INCLUDE (CheckSnprintf)


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


CHECK_REQUIRED_FUNCTIONS( printf wcscpy wcsncpy wcscat wcschr wcsstr wcslen 
                           wcscmp wcsncmp wcscspn)

#todo: wcstoq is bsd equiv of wcstoll, we can use that...
CHECK_OPTIONAL_FUNCTIONS( mmap wcsupr wcscasecmp wcsicmp wcstoll wprintf MapViewOfFile 
                         lltow wcstod strupr strlwr lltoa strtoll gettimeofday _vsnwprintf usleep)


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
    SET ( FUNCTION_FILESIZE "#define fileSize lucene_filelength" )
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
MUSTDEFINE_VAR("HAVE_TYPE_INT32_T;HAVE_TYPE_UINT32_T;HAVE_TYPE_INT64_T;HAVE_TYPE_UINT64_T;HAVE_TYPE_SIZE_T")


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
#	./document/DateTools.cpp
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
	./index/Payload.cpp
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
file(GLOB_RECURSE HEADERS ${CMAKE_SOURCE_DIR}/src/CLucene/*.h)

DEFINE_OPTIONS(EXTRA_OPTIONS)
ADD_DEFINITIONS(${EXTRA_OPTIONS} -DMAKE_CLUCENE_CORE_LIB)

add_library(clucene-core SHARED 
	${clucene_core_Files} ${HEADERS} ${clucene-core_BINARY_DIR}/clucene-config.h ${clucene-core_BINARY_DIR}/_clucene-config.h
)
add_library(clucene-core-static STATIC 
	${clucene_core_Files} ${HEADERS} ${clucene-core_BINARY_DIR}/clucene-config.h ${clucene-core_BINARY_DIR}/_clucene-config.h
)

SET_TARGET_PROPERTIES(clucene-core PROPERTIES
    VERSION ${CPACK_PACKAGE_VERSION}
    SOVERSION ${CPACK_PACKAGE_SOVERSION}
    DEFINE_SYMBOL "CLUCENE_EXPORT_SYMBOLS"
)

SET_TARGET_PROPERTIES(clucene-core-static PROPERTIES
    VERSION ${CPACK_PACKAGE_VERSION}
    SOVERSION ${CPACK_PACKAGE_SOVERSION}
    DEFINE_SYMBOL ""
)

#install public headers.
foreach(file ${HEADERS})
	get_filename_component(apath ${file} PATH)
	get_filename_component(aname ${file} NAME)
	file(RELATIVE_PATH relpath ${CMAKE_SOURCE_DIR}/src ${apath})

	IF (NOT relpath STREQUAL "demo" )
	    IF ( NOT aname MATCHES "^_.*" )
		    install(FILES ${file} DESTINATION include/${relpath} )
		ENDIF ( NOT aname MATCHES "^_.*" )
	ENDIF (NOT relpath STREQUAL "demo" )
endforeach(file)
#also install CLucene.h
install(FILES ../CLucene.h DESTINATION include/ )
install(FILES ${clucene-core_BINARY_DIR}/clucene-config.h DESTINATION lib/CLucene )

#and install libraries
install(TARGETS clucene-core DESTINATION lib)
install(TARGETS clucene-core-static DESTINATION lib)
