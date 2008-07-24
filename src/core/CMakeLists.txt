PROJECT(clucene-core)

INCLUDE (DefineOptions)
DEFINE_OPTIONS(EXTRA_OPTIONS)
ADD_DEFINITIONS(${EXTRA_OPTIONS} -DMAKE_CLUCENE_CORE_LIB)

#add the files to our groups and core
SOURCE_GROUP("analysis" ./CLucene/analysis/*)
#seems to be a bug in cmake, can't put these in analysis\\standard folder :(
SOURCE_GROUP("analysis-standard" ./CLucene/analysis/standard/*)
SOURCE_GROUP("document" ./CLucene/document/*)
SOURCE_GROUP("index" ./CLucene/index/*)
SOURCE_GROUP("queryParser" ./CLucene/queryParser/*)
SOURCE_GROUP("search" ./CLucene/search/*)
SOURCE_GROUP("store" ./CLucene/store/*)
SOURCE_GROUP("util" ./CLucene/util/*)

SET(clucene_core_Files
	./CLucene/StdHeader.cpp
	./CLucene/util/ThreadLocal.cpp
	./CLucene/util/Reader.cpp
	./CLucene/util/Equators.cpp
	./CLucene/util/FastCharStream.cpp
	./CLucene/util/fileinputstream.cpp
	./CLucene/util/MD5Digester.cpp
	./CLucene/util/StringIntern.cpp
	./CLucene/util/BitSet.cpp
	./CLucene/queryParser/Lexer.cpp
	./CLucene/queryParser/MultiFieldQueryParser.cpp
	./CLucene/queryParser/QueryParser.cpp
	./CLucene/queryParser/QueryParserBase.cpp
	./CLucene/queryParser/QueryToken.cpp
	./CLucene/queryParser/TokenList.cpp
	./CLucene/analysis/standard/StandardAnalyzer.cpp
	./CLucene/analysis/standard/StandardFilter.cpp
	./CLucene/analysis/standard/StandardTokenizer.cpp
	./CLucene/analysis/Analyzers.cpp
	./CLucene/analysis/AnalysisHeader.cpp
	./CLucene/store/MMapInput.cpp
	./CLucene/store/TransactionalRAMDirectory.cpp
	./CLucene/store/IndexInput.cpp
	./CLucene/store/Lock.cpp
	./CLucene/store/LockFactory.cpp
	./CLucene/store/IndexOutput.cpp
	./CLucene/store/Directory.cpp
	./CLucene/store/FSDirectory.cpp
	./CLucene/store/RAMDirectory.cpp
	./CLucene/document/Document.cpp
	./CLucene/document/DateField.cpp
	./CLucene/document/DateTools.cpp
	./CLucene/document/Field.cpp
	./CLucene/index/IndexFileNames.cpp
	./CLucene/index/SegmentMergeInfo.cpp
	./CLucene/index/SegmentInfos.cpp
	./CLucene/index/SegmentTermDocs.cpp
	./CLucene/index/FieldsWriter.cpp
	./CLucene/index/TermInfosWriter.cpp
	./CLucene/index/Term.cpp
	./CLucene/index/Terms.cpp
	./CLucene/index/DocumentWriter.cpp
	./CLucene/index/SegmentTermVector.cpp
	./CLucene/index/TermVectorReader.cpp
	./CLucene/index/FieldInfos.cpp
	./CLucene/index/CompoundFile.cpp
	./CLucene/index/SegmentReader.cpp
	./CLucene/index/TermVectorWriter.cpp
	./CLucene/index/IndexReader.cpp
	./CLucene/index/SegmentTermPositions.cpp
	./CLucene/index/SegmentMerger.cpp
	./CLucene/index/IndexWriter.cpp
	./CLucene/index/MultiReader.cpp
	./CLucene/index/Payload.cpp
	./CLucene/index/SegmentTermEnum.cpp
	./CLucene/index/TermInfo.cpp
	./CLucene/index/IndexModifier.cpp
	./CLucene/index/SegmentMergeQueue.cpp
	./CLucene/index/FieldsReader.cpp
	./CLucene/index/TermInfosReader.cpp
	./CLucene/search/Compare.cpp
	./CLucene/search/Scorer.cpp
	./CLucene/search/ScorerDocQueue.cpp
	./CLucene/search/PhraseScorer.cpp
	./CLucene/search/SloppyPhraseScorer.cpp
	./CLucene/search/DisjunctionSumScorer.cpp
	./CLucene/search/ConjunctionScorer.cpp
	./CLucene/search/PhraseQuery.cpp
	./CLucene/search/PrefixQuery.cpp
	./CLucene/search/ExactPhraseScorer.cpp
	./CLucene/search/TermScorer.cpp
	./CLucene/search/Similarity.cpp
	./CLucene/search/BooleanScorer.cpp
	./CLucene/search/BooleanScorer2.cpp
	./CLucene/search/HitQueue.cpp
	./CLucene/search/FieldCacheImpl.cpp
	./CLucene/search/ChainedFilter.cpp
	./CLucene/search/RangeFilter.cpp
	./CLucene/search/CachingWrapperFilter.cpp
	./CLucene/search/QueryFilter.cpp
	./CLucene/search/TermQuery.cpp
	./CLucene/search/FuzzyQuery.cpp
	./CLucene/search/SearchHeader.cpp
	./CLucene/search/RangeQuery.cpp
	./CLucene/search/IndexSearcher.cpp
	./CLucene/search/Sort.cpp
	./CLucene/search/PhrasePositions.cpp
	./CLucene/search/FieldDocSortedHitQueue.cpp
	./CLucene/search/WildcardTermEnum.cpp
	./CLucene/search/MultiSearcher.cpp
	./CLucene/search/Hits.cpp
	./CLucene/search/MultiTermQuery.cpp
	./CLucene/search/FilteredTermEnum.cpp
	./CLucene/search/FieldSortedHitQueue.cpp
	./CLucene/search/WildcardQuery.cpp
	./CLucene/search/Explanation.cpp
	./CLucene/search/BooleanQuery.cpp
	./CLucene/search/FieldCache.cpp
	./CLucene/search/DateFilter.cpp
)

GET_SHARED_FILES(clucene_shared_Files)

#find our headers
file(GLOB_RECURSE HEADERS ${clucene-core_SOURCE_DIR}/*.h)


#create the libraries
INCLUDE_DIRECTORIES( ${clucene_SOURCE_DIR}/src/core )

add_library(clucene-core SHARED
	${clucene_core_Files} ${clucene_shared_Files} ${HEADERS}
)

add_library(clucene-core-static STATIC
	${clucene_core_Files} ${clucene_shared_Files} ${HEADERS}
)

#set properties on the libraries
SET_TARGET_PROPERTIES(clucene-core PROPERTIES
    VERSION ${CPACK_PACKAGE_VERSION}
    SOVERSION ${CPACK_PACKAGE_SOVERSION}
    DEFINE_SYMBOL "CLUCENE_EXPORT_SYMBOLS"
    COMPILE_DEFINITIONS_DEBUG _DEBUG
)

SET_TARGET_PROPERTIES(clucene-core-static PROPERTIES
    VERSION ${CPACK_PACKAGE_VERSION}
    SOVERSION ${CPACK_PACKAGE_SOVERSION}
    DEFINE_SYMBOL ""
    COMPILE_DEFINITIONS_DEBUG _DEBUG
    OUTPUT_NAME "clucene-core"
)

#install public headers.
FOREACH(file ${HEADERS})
	get_filename_component(apath ${file} PATH)
	get_filename_component(aname ${file} NAME)
	file(RELATIVE_PATH relpath ${CMAKE_SOURCE_DIR}/src/core ${apath})
ENDFOREACH(file)
#also install CLucene.h
install(FILES ${clucene-shared_BINARY_DIR}/clucene-config.h DESTINATION lib/CLucene )

install(FILES ${HEADERS} DESTINATION include/)

#and install libraries
install(TARGETS clucene-core DESTINATION lib)
install(TARGETS clucene-core-static DESTINATION lib)
