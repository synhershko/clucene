PROJECT(clucene-test)

INCLUDE (DefineOptions)
DEFINE_OPTIONS(EXTRA_OPTIONS EXTRA_LIBS)
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
./queryParser/TestMultiFieldQueryParser.cpp
./analysis/TestAnalysis.cpp
./analysis/TestAnalyzers.cpp
./debug/TestError.cpp
./document/TestDocument.cpp
./document/TestNumberTools.cpp
./store/TestStore.cpp
./search/TestDateFilter.cpp
./search/TestForDuplicates.cpp
./search/TestQueries.cpp
./search/TestSearch.cpp
./search/TestSort.cpp
./search/TestWildcard.cpp
./search/TestTermVector.cpp
./index/TestIndexWriter.cpp
./index/TestIndexModifier.cpp
./index/TestIndexReader.cpp
./index/TestThreading.cpp
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

#link the executable against the releavent clucene-shared library (if we aren't using the object files)
IF ( NOT USE_SHARED_OBJECT_FILES )
    TARGET_LINK_LIBRARIES(cl_test clucene-core clucene-shared ${EXTRA_LIBS})
ENDIF ( NOT USE_SHARED_OBJECT_FILES )

############################
#special tests:
############################

IF ( ENABLE_COMPILE_TESTS )

#define zlib library requirements...
find_package(ZLIB)
IF ( ZLIB_FOUND )
  SET ( EXTRA_LIBS ${EXTRA_LIBS} ${ZLIB_LIBRARY} )
  INCLUDE_DIRECTORIES( ${ZLIB_INCLUDE_DIR} )
ENDIF ( ZLIB_FOUND )
   
SET(test_monolithic_Files ${clucene-core_SOURCE_DIR}/CLucene/CLMonolithic.cpp ./CLMonolithic_Test.cpp)

SET (TESTS_CXX_FLAGS "-DMAKE_CLUCENE_SHARED_LIB -DMAKE_CLUCENE_CORE_LIB")
SET (TESTS_EXE_LINKER_FLAGS "${CMAKE_THREAD_LIBS_INIT}")

ADD_EXECUTABLE(cl_test-ascii EXCLUDE_FROM_ALL ${test_monolithic_Files} )
TARGET_LINK_LIBRARIES(cl_test-ascii "${EXTRA_LIBS}")
SET_TARGET_PROPERTIES(cl_test-ascii PROPERTIES 
    COMPILE_DEFINITIONS "_ASCII"
    COMPILE_FLAGS "${TESTS_CXX_FLAGS}"
    LINK_FLAGS "${TESTS_EXE_LINKER_FLAGS}")
ADD_CUSTOM_TARGET(test-ascii
    COMMENT "Running cl_test-ascii"
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-ascii
    DEPENDS cl_test-ascii)

ADD_EXECUTABLE(cl_test-namespace EXCLUDE_FROM_ALL ${test_monolithic_Files} )
TARGET_LINK_LIBRARIES(cl_test-namespace "${EXTRA_LIBS}")
SET_TARGET_PROPERTIES(cl_test-namespace PROPERTIES 
    COMPILE_DEFINITIONS "DISABLE_NAMESPACE"
    COMPILE_FLAGS "${TESTS_CXX_FLAGS}"
    LINK_FLAGS "${TESTS_EXE_LINKER_FLAGS}")
ADD_CUSTOM_TARGET(test-namespace
    COMMENT "Running cl_test-namespace"
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-namespace
    DEPENDS cl_test-namespace)

ADD_EXECUTABLE(cl_test-mmap EXCLUDE_FROM_ALL ${test_monolithic_Files} )
TARGET_LINK_LIBRARIES(cl_test-mmap "${EXTRA_LIBS}")
SET_TARGET_PROPERTIES(cl_test-mmap PROPERTIES 
    COMPILE_DEFINITIONS "LUCENE_FS_MMAP"
    COMPILE_FLAGS "${TESTS_CXX_FLAGS}"
    LINK_FLAGS "${TESTS_EXE_LINKER_FLAGS}")
ADD_CUSTOM_TARGET(test-mmap
    COMMENT "Running cl_test-mmap"
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-mmap
    DEPENDS cl_test-mmap)

ADD_EXECUTABLE(cl_test-singlethreading EXCLUDE_FROM_ALL ${test_monolithic_Files} )
TARGET_LINK_LIBRARIES(cl_test-singlethreading "${EXTRA_LIBS}")
SET_TARGET_PROPERTIES(cl_test-singlethreading PROPERTIES 
    COMPILE_DEFINITIONS "_CL_DISABLE_MULTITHREADING"
    COMPILE_FLAGS "${TESTS_CXX_FLAGS}"
    LINK_FLAGS "${TESTS_EXE_LINKER_FLAGS}")
ADD_CUSTOM_TARGET(test-singlethreading
    COMMENT "Running cl_test-singlethreading"
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-singlethreading
    DEPENDS cl_test-singlethreading)

ADD_EXECUTABLE(cl_test-refcnt EXCLUDE_FROM_ALL ${test_monolithic_Files} )
TARGET_LINK_LIBRARIES(cl_test-refcnt "${EXTRA_LIBS}")
SET_TARGET_PROPERTIES(cl_test-refcnt PROPERTIES 
    COMPILE_DEFINITIONS "LUCENE_ENABLE_REFCOUNT"
    COMPILE_FLAGS "${TESTS_CXX_FLAGS}"
    LINK_FLAGS "${TESTS_EXE_LINKER_FLAGS}")
ADD_CUSTOM_TARGET(test-refcnt
    COMMENT "Running cl_test-refcnt"
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-refcnt
    DEPENDS cl_test-refcnt)
    
ADD_EXECUTABLE(cl_test-platform-charfuncs EXCLUDE_FROM_ALL ${test_monolithic_Files} )
TARGET_LINK_LIBRARIES(cl_test-platform-charfuncs "${EXTRA_LIBS}")
SET_TARGET_PROPERTIES(cl_test-platform-charfuncs PROPERTIES 
    COMPILE_DEFINITIONS "LUCENE_USE_INTERNAL_CHAR_FUNCTIONS=0"
    COMPILE_FLAGS "${TESTS_CXX_FLAGS}"
    LINK_FLAGS "${TESTS_EXE_LINKER_FLAGS}")
#this one just tests that the compile works... tests will (mostly) fail, since the
#internal functions do not produce the exact results expected.
ADD_CUSTOM_TARGET(test-platform-charfuncs
    COMMENT "Running cl_test-platform-charfuncs"
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-platform-charfuncs
    DEPENDS cl_test-platform-charfuncs)

#this one last so we only have pedantic for test-pedantic
ADD_EXECUTABLE(cl_test-pedantic EXCLUDE_FROM_ALL ${test_monolithic_Files})
TARGET_LINK_LIBRARIES(cl_test-pedantic "${EXTRA_LIBS}")
SET_TARGET_PROPERTIES(cl_test-pedantic PROPERTIES
    COMPILE_DEFINITIONS ""
    COMPILE_FLAGS "${TESTS_CXX_FLAGS} -std=gnu++98 -pedantic -Wnon-virtual-dtor -Wundef -Wcast-align -Wchar-subscripts -W -Wformat-security -Wextra -Wall -Wno-long-long -Wno-sign-compare"
    LINK_FLAGS "${TESTS_EXE_LINKER_FLAGS} -Wall")
ADD_CUSTOM_TARGET(test-pedantic
    COMMENT "Running cl_test-pedantic"
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-pedantic
    DEPENDS cl_test-pedantic)


CHECK_CXX_ACCEPTS_FLAG("-fprofile-arcs -ftest-coverage" GccFlagGcov)
IF ( GccFlagGcov )
	ADD_EXECUTABLE(cl_test-gcov EXCLUDE_FROM_ALL ${test_monolithic_Files} )
  TARGET_LINK_LIBRARIES(cl_test-gcov "${EXTRA_LIBS}")
	SET_TARGET_PROPERTIES(cl_test-gcov PROPERTIES
	    COMPILE_FLAGS "${TESTS_CXX_FLAGS} -fprofile-arcs -ftest-coverage"
	    LINK_FLAGS "${TESTS_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
	ADD_CUSTOM_TARGET(test-gcov
	    COMMENT "Running cl_test-gcov"
	    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-gcov
      COMMAND lcov --directory CMakeFiles/cl_test-gcov.dir/__/core/CLucene -c -o clucene-coverage.info
      COMMAND lcov --remove clucene-coverage.info "\"/usr/*\"" -o clucene-coverage.clean
      COMMAND genhtml -o ${CMAKE_BINARY_DIR}/clucene-coverage clucene-coverage.clean
	    DEPENDS cl_test-gcov)
ENDIF ( GccFlagGcov )



#target for running all tests
ADD_CUSTOM_TARGET(test-all 
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-pedantic
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-ascii
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-namespace
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-mmap
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-singlethreading
    COMMAND ${EXECUTABLE_OUTPUT_PATH}/cl_test-refcnt
    DEPENDS cl_test-pedantic cl_test-ascii cl_test-namespace cl_test-mmap cl_test-singlethreading cl_test-refcnt cl_test-platform-charfuncs
)

ENDIF ( ENABLE_COMPILE_TESTS )

############################

