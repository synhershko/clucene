PROJECT(clucene-core)

INCLUDE (CheckIncludeFiles)
INCLUDE (CheckIncludeFileCXX)

#todo: check if _CL_TIME_WITH_SYS_TIME works
CHECK_INCLUDE_FILES ("stdlib.h;stddef.h" _CL_STDC_HEADERS)
CHECK_INCLUDE_FILES ("sys/time.h;time.h" _CL_TIME_WITH_SYS_TIME)

#optional C headers
FOREACH(inc stdlib.h string.h sys/time.h memory.h strings.h sys/types.h inttypes.h stdint.h unistd.h io.h direct.h sys/dir.h sys/ndir.h dirent.h stat.h sys/stat.h stdexcept errno.h fcntl.h windows.h wchar.h wctype.h ctype.h fcntl.h)
	STRING(TOUPPER ${inc} INC)
	STRING(REPLACE . _ INC ${INC})
	STRING(REPLACE / _ INC ${INC})
	CHECK_INCLUDE_FILES (${inc} _CL_HAVE_${INC})
ENDFOREACH(inc stdlib.h string.h sys/time.h memory.h strings.h sys/types.h inttypes.h stdint.h unistd.h io.h direct.h sys/dir.h sys/ndir.h dirent.h stat.h sys/stat.h stdexcept errno.h fcntl.h windows.h wchar.h wctype.h ctype.h fcntl.h)

#compulsary C++ headers
FOREACH(inc algorithm functional map vector list set math.h stdarg.h fcntl.h math.h)
	STRING(TOUPPER ${inc} INC)
	STRING(REPLACE . _ INC ${INC})
	CHECK_INCLUDE_FILE_CXX (${inc} _CL_HAVE_${INC})
	IF ( NOT _CL_HAVE_${INC} )
		MESSAGE ( FATAL_ERROR, "${inc} could not be found" )
	ENDIF ( NOT _CL_HAVE_${INC} )
ENDFOREACH(inc algorithm functional map vector list set math.h stdarg.h stdexcept fcntl.h)

#optional C++ headers
FOREACH(inc hash_map hash_set ext/hash_map ext/hash_set stdarg.h sys/timeb.h wchar.h tchar.h stdexcept)
	STRING(TOUPPER ${inc} INC)
	STRING(REPLACE . _ INC ${INC})
	STRING(REPLACE / _ INC ${INC})
	CHECK_INCLUDE_FILE_CXX (${inc} _CL_HAVE_${INC})
ENDFOREACH(inc hash_map hash_set ext/hash_map ext/hash_set stdarg.h sys/timeb.h wchar.h tchar.h)


INCLUDE (CheckSymbolExists)
CHECK_SYMBOL_EXISTS(float_t math.h _CL_HAVE_FLOAT_T)
#check wchar and intptr


INCLUDE (CheckFunctionExists)

#check for compulsary functions
FOREACH(inc printf wcscpy wcsncpy wcscat wcschr wcsstr wcslen wcscmp wcsncmp wcscspn)
	STRING(TOUPPER ${inc} INC)
	STRING(REPLACE . _ INC ${INC})
	CHECK_FUNCTION_EXISTS (${inc} _CL_HAVE_${INC})
	IF ( NOT _CL_HAVE_${INC} )
		MESSAGE ( FATAL_ERROR, "${inc} could not be found" )
	ENDIF ( NOT _CL_HAVE_${INC} )
ENDFOREACH(inc printf snprintf wcscpy wcsncpy wcscat wcschr wcsstr wcslen wcscmp wcsncmp wcscspn)

#check for optional functions
#todo: wcstoq is bsd equiv of wcstoll, we can use that...
FOREACH(inc mmap snprintf _snprintf wcsupr wcscasecmp wcsicmp wcstoll lltow wcstod strupr strlwr lltoa strtoll tell _filelength)
	STRING(TOUPPER ${inc} INC)
	STRING(REPLACE . _ INC ${INC})
	CHECK_FUNCTION_EXISTS (${inc} _CL_HAVE_${INC})
ENDFOREACH(inc mmap wcsupr wcscasecmp wcsicmp wcstoll lltow wcstod strupr strlwr lltoa strtoll tell _filelength)


#check for ansi capabilities
#INCLUDE(TestForANSIForScope) - not really needed, we keep a clean scope
#INCLUDE(TestForANSIStreamHeaders) - not really needed, we dont use streams
INCLUDE(TestForSTDNamespace)
IF ( NOT CMAKE_NO_STD_NAMESPACE )
	SET(_CL_HAVE_STD 1)
ENDIF ( NOT CMAKE_NO_STD_NAMESPACE )


#check for pthreads
find_package(Threads REQUIRED)
IF ( CMAKE_USE_PTHREADS_INIT )
 SET ( _CL_HAVE_PTHREAD 1 )
ENDIF (CMAKE_USE_PTHREADS_INIT)
IF ( CMAKE_USE_WIN32_THREADS_INIT )
 SET ( _CL_HAVE_WIN32_THREADS 1 )
ENDIF ( CMAKE_USE_WIN32_THREADS_INIT )
IF ( CMAKE_USE_SPROC_INIT OR CMAKE_HP_PTHREADS_INIT )
 MESSAGE(FATAL_ERROR "Threads library not implemented")
ENDIF( CMAKE_USE_SPROC_INIT OR CMAKE_HP_PTHREADS_INIT )


#make msvc 8 not complain... needs to be fixed.
IF( MSVC80 )
 ADD_DEFINITIONS( -D_CRT_SECURE_NO_DEPRECATE )
ENDIF ( MSVC80 )



#now write out our configuration....
CONFIGURE_FILE(${clucene-core_SOURCE_DIR}/clucene-config.h.cmake ${clucene-core_BINARY_DIR}/clucene-config.h)

#add the files to our groups and core
SOURCE_GROUP("analysis\\standard" REGULAR_EXPRESSION ./analysis/standard/*)
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

SET(clucene_core_Files
	./queryParser/Lexer.cpp
	./queryParser/MultiFieldQueryParser.cpp
	./queryParser/QueryParser.cpp
	./queryParser/QueryToken.cpp
	./queryParser/TokenList.cpp
	./queryParser/QueryParserBase.cpp
	./analysis/standard/StandardAnalyzer.cpp
	./analysis/standard/StandardFilter.cpp
	./analysis/standard/StandardTokenizer.cpp
	./analysis/Analyzers.cpp
	./analysis/AnalysisHeader.cpp
	./debug/memtracking.cpp
	./debug/condition.cpp
	./debug/error.cpp
	./store/MMapInput.cpp
	./store/TransactionalRAMDirectory.cpp
	./store/IndexInput.cpp
	./store/Lock.cpp
	./store/IndexOutput.cpp
	./store/FSDirectory.cpp
	./store/RAMDirectory.cpp
	./search/PhraseScorer.cpp
	./search/SloppyPhraseScorer.cpp
	./search/ConjunctionScorer.cpp
	./search/PhraseQuery.cpp
	./search/PrefixQuery.cpp
	./search/ExactPhraseScorer.cpp
	./search/TermScorer.cpp
	./search/Similarity.cpp
	./search/BooleanScorer.cpp
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
	./config/repl_tcslwr.cpp
	./config/repl_tcstoll.cpp
	./config/repl_tcscasecmp.cpp
	./config/repl_tprintf.cpp
	./config/repl_lltot.cpp
	./config/repl_tcstod.cpp
	./config/utf8.cpp
	./config/threads.cpp
	./config/gunichartables.cpp
	./document/Document.cpp
	./document/DateField.cpp
	./document/Field.cpp
	./index/SegmentMergeInfo.cpp
	./index/SegmentInfos.cpp
	./index/SegmentTermDocs.cpp
	./index/FieldsWriter.cpp
	./index/TermInfosWriter.cpp
	./index/Term.cpp
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
	./util/ThreadLocal.cpp
	./StdHeader.cpp
)
IF( MSVC OR MINGW )
	add_library(clucene-core STATIC 
		${clucene_core_Files}
	)
ELSE( MSVC OR MINGW )
	add_library(clucene-core SHARED 
		${clucene_core_Files}
	)
ENDIF( MSVC OR MINGW )

file(GLOB_RECURSE HEADERS ${CMAKE_SOURCE_DIR}/src/*.h)

foreach(file ${HEADERS})
	get_filename_component(apath ${file} PATH)
	file(RELATIVE_PATH relpath ${CMAKE_SOURCE_DIR}/src ${apath})
	if (relpath STREQUAL "demo")
	else (relpath STREQUAL "demo")
		install(FILES ${file} DESTINATION include/${relpath} )
	endif (relpath STREQUAL "demo")
endforeach(file)

install(TARGETS clucene-core DESTINATION lib)
