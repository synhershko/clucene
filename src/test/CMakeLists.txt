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

file(GLOB_RECURSE test_HEADERS ${CMAKE_SOURCE_DIR}/test/*.h)

SET(test_files ./tests.cpp
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
${test_HEADERS}
)
IF ( USE_SHARED_OBJECT_FILES )
    GET_SHARED_FILES(clucene_shared_Files)
ENDIF ( USE_SHARED_OBJECT_FILES )

#todo: do glob header and include header files for IDE.
ADD_EXECUTABLE(cl_test EXCLUDE_FROM_ALL ${clucene_shared_Files} ${test_files} )
SET_TARGET_PROPERTIES(cl_test PROPERTIES DEFINE_SYMBOL "")

#link the executable against the releavent clucene-shared library (if we aren't using the object files)
IF ( NOT USE_SHARED_OBJECT_FILES )
    IF ( USE_STATIC_SHARED_LIBRARY )
        TARGET_LINK_LIBRARIES(cl_test clucene-core clucene-shared-static)
    ELSE ( USE_STATIC_SHARED_LIBRARY )
        TARGET_LINK_LIBRARIES(cl_test clucene-core clucene-shared)
    ENDIF ( USE_STATIC_SHARED_LIBRARY )
ENDIF ( NOT USE_SHARED_OBJECT_FILES )

############################
#special tests:
############################

IF ( ENABLE_COMPILE_TESTS )
   
SET(test_monolithic_Files ./CLMonolithic_Test.cpp ${clucene-core_SOURCE_DIR}/CLucene/CLMonolithic.cpp)

SET(CMAKE_CXX_FLAGS_DEBUG)
IF ( CMAKE_COMPILER_IS_GNUCC )
    CHECK_CXX_ACCEPTS_FLAG(-Wall GccFlagWall)
    CHECK_CXX_ACCEPTS_FLAG(-pedantic GccFlagPedantic)
ENDIF ( CMAKE_COMPILER_IS_GNUCC )

IF ( GccFlagG )
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
ENDIF ( GccFlagG )
IF ( GccFlagWall )
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wall")
ENDIF ( GccFlagWall )
IF ( GccFlagPedantic )
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pedantic")
ENDIF ( GccFlagPedantic )

ADD_EXECUTABLE(cl_test-pedantic EXCLUDE_FROM_ALL ${test_monolithic_Files})
SET_TARGET_PROPERTIES(cl_test-pedantic PROPERTIES COMPILE_DEFINITIONS "")
ADD_CUSTOM_TARGET(test-pedantic
    COMMENT "Running cl_test-pedantic"
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-pedantic
    DEPENDS cl_test-pedantic)

ADD_EXECUTABLE(cl_test-ascii EXCLUDE_FROM_ALL ${test_monolithic_Files} )
SET_TARGET_PROPERTIES(cl_test-ascii PROPERTIES COMPILE_DEFINITIONS "_ASCII")
ADD_CUSTOM_TARGET(test-ascii
    COMMENT "Running cl_test-ascii"
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-ascii
    DEPENDS cl_test-ascii)

ADD_EXECUTABLE(cl_test-namespace EXCLUDE_FROM_ALL ${test_monolithic_Files} )
SET_TARGET_PROPERTIES(cl_test-namespace PROPERTIES COMPILE_DEFINITIONS "DISABLE_NAMESPACE")
ADD_CUSTOM_TARGET(test-namespace
    COMMENT "Running cl_test-namespace"
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-namespace
    DEPENDS cl_test-namespace)

ADD_EXECUTABLE(cl_test-mmap EXCLUDE_FROM_ALL ${test_monolithic_Files} )
SET_TARGET_PROPERTIES(cl_test-mmap PROPERTIES COMPILE_DEFINITIONS "LUCENE_FS_MMAP")
ADD_CUSTOM_TARGET(test-mmap
    COMMENT "Running cl_test-mmap"
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-mmap
    DEPENDS cl_test-mmap)

ADD_EXECUTABLE(cl_test-singlethreading EXCLUDE_FROM_ALL ${test_monolithic_Files} )
SET_TARGET_PROPERTIES(cl_test-singlethreading PROPERTIES COMPILE_DEFINITIONS "_CL_DISABLE_MULTITHREADING")
ADD_CUSTOM_TARGET(test-singlethreading
    COMMENT "Running cl_test-singlethreading"
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-singlethreading
    DEPENDS cl_test-singlethreading)

ADD_EXECUTABLE(cl_test-refcnt EXCLUDE_FROM_ALL ${test_monolithic_Files} )
SET_TARGET_PROPERTIES(cl_test-refcnt PROPERTIES COMPILE_DEFINITIONS "LUCENE_ENABLE_REFCOUNT")
ADD_CUSTOM_TARGET(test-refcnt
    COMMENT "Running cl_test-refcnt"
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-refcnt
    DEPENDS cl_test-refcnt)
    
ADD_EXECUTABLE(cl_test-platform-charfuncs EXCLUDE_FROM_ALL ${test_monolithic_Files} )
SET_TARGET_PROPERTIES(cl_test-platform-charfuncs PROPERTIES COMPILE_DEFINITIONS "LUCENE_USE_INTERNAL_CHAR_FUNCTIONS=0")
#this one just tests that the compile works... tests will (mostly) fail, since the
#internal functions do not produce the exact results expected.
ADD_CUSTOM_TARGET(test-platform-charfuncs
    COMMENT "Running cl_test-platform-charfuncs"
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-platform-charfuncs
    DEPENDS cl_test-platform-charfuncs)

SET(CMAKE_CXX_FLAGS_DEBUG)

#target for running all tests
ADD_CUSTOM_TARGET(test-all 
    COMMAND ${CMAKE_COMMAND} test-pedantic
    COMMAND ${CMAKE_COMMAND} test-ascii
    COMMAND ${CMAKE_COMMAND} test-namespace
    COMMAND ${CMAKE_COMMAND} test-mmap
    COMMAND ${CMAKE_COMMAND} test-singlethreading
    COMMAND ${CMAKE_COMMAND} test-refcnt
    DEPENDS cl_test-pedantic cl_test-ascii cl_test-namespace cl_test-mmap cl_test-singlethreading cl_test-refcnt cl_test-platform-charfuncs
)

ENDIF ( ENABLE_COMPILE_TESTS )

############################

