PROJECT(clucene-test)

INCLUDE (DefineOptions)
DEFINE_OPTIONS(EXTRA_OPTIONS)
ADD_DEFINITIONS(${EXTRA_OPTIONS})

INCLUDE_DIRECTORIES( ${clucene-test_SOURCE_DIR} )

SOURCE_GROUP("unit_testing" ./*.cpp)
SOURCE_GROUP("analysis" ./analysis/*)
SOURCE_GROUP("debug" ./debug/*)
SOURCE_GROUP("document" ./document/*)
SOURCE_GROUP("index" ./index/*)
SOURCE_GROUP("queryParser" ./queryParser/*)
SOURCE_GROUP("search" ./search/*)
SOURCE_GROUP("store" ./store/*)
SOURCE_GROUP("util" ./util/*)

file(GLOB_RECURSE HEADERS ${CMAKE_SOURCE_DIR}/test/*.h)

ADD_EXECUTABLE(cl_test 
./tests.cpp
./CuTest.cpp
./testall.cpp
./queryParser/TestQueryParser.cpp
./analysis/TestAnalysis.cpp
./analysis/TestAnalyzers.cpp
./debug/TestError.cpp
./document/TestDocument.cpp
./store/TestStore.cpp
./search/TestDateFilter.cpp
./search/TestForDuplicates.cpp
./search/TestQueries.cpp
./search/TestSearch.cpp
./search/TestSort.cpp
./search/TestWildcard.cpp
./search/TestTermVector.cpp
./index/TestIndexWriter.cpp
./index/TestUtf8.cpp
./index/TestHighFreqTerms.cpp
./index/TestReuters.cpp
./util/TestPriorityQueue.cpp
./util/English.cpp
${HEADERS}
)


SET_TARGET_PROPERTIES(cl_test PROPERTIES
#    VERSION "0.9.16a"
#    SOVERSION "0.9"
DEFINE_SYMBOL ""
)
TARGET_LINK_LIBRARIES(cl_test clucene-core-static)
