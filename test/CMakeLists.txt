PROJECT(clucene-test)

INCLUDE_DIRECTORIES( ${clucene-test_SOURCE_DIR} )

OPTION(ENABLE_ASCII_MODE
  "enable ascii support"
  OFF)
IF(ENABLE_ASCII_MODE)
	ADD_DEFINITIONS(-D_ASCII)
ELSE(ENABLE_ASCII_MODE)
	ADD_DEFINITIONS(-D_UNICODE)
ENDIF(ENABLE_ASCII_MODE)

SOURCE_GROUP("unit_testing" ./*.cpp)
SOURCE_GROUP("analysis" ./analysis/*)
SOURCE_GROUP("debug" ./debug/*)
SOURCE_GROUP("document" ./document/*)
SOURCE_GROUP("index" ./index/*)
SOURCE_GROUP("queryParser" ./queryParser/*)
SOURCE_GROUP("search" ./search/*)
SOURCE_GROUP("store" ./store/*)
SOURCE_GROUP("util" ./util/*)

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
)
TARGET_LINK_LIBRARIES(cl_test clucene-core-static)
