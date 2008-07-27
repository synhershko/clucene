PROJECT (clucene)

SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "9")
SET(CPACK_PACKAGE_VERSION_REVISION "23")
SET(CPACK_PACKAGE_VERSION_PATCH "0")

SET(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_REVISION}.${CPACK_PACKAGE_VERSION_PATCH}")
SET(CPACK_PACKAGE_SOVERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_REVISION}")

CMAKE_MINIMUM_REQUIRED(VERSION 2.4.2 FATAL_ERROR)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

if(WIN32)
 set(CMAKE_DEBUG_POSTFIX "d")
endif(WIN32)

# include specific modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

#define options...

Include (CLuceneDocs)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ELSE(NOT CMAKE_BUILD_TYPE)
    MESSAGE( "Compiling as ${CMAKE_BUILD_TYPE}" )
ENDIF(NOT CMAKE_BUILD_TYPE)

OPTION(ENABLE_DEBUG
  "enable debug support"
  OFF)
OPTION(ENABLE_MMAP
  "enable mmap support (experimental)"
  OFF)
OPTION(DISABLE_MULTITHREADING
  "disable multithreading - remove all locking code"
  OFF)
OPTION(ENABLE_ASCII_MODE
  "enable ascii support"
  OFF)
OPTION(LUCENE_USE_INTERNAL_CHAR_FUNCTIONS
  "use internal character functions. required to run tests correctly"
  ON)
OPTION(ENABLE_GPROF
  "turn on gprof profiling support"
  OFF)
OPTION(ENABLE_PACKAGING
  "create build scripts for creating clucene packages"
  OFF)

SET ( ENABLE_COMPILE_TESTS_VALUE ON )
IF ( MSVC_IDE )
	#this is annoying...
	SET ( ENABLE_COMPILE_TESTS_VALUE OFF )
ENDIF( MSVC_IDE )

OPTION(ENABLE_COMPILE_TESTS
  "enable various projects that test alternative build switches"
  ${ENABLE_COMPILE_TESTS_VALUE})

OPTION(TESTS_USE_SHARED_LIBRARY
  "Link tests against the shared library"
  ON)

#check flags...
INCLUDE (TestCXXAcceptsFlag)
IF ( CMAKE_COMPILER_IS_GNUCC )
    CHECK_CXX_ACCEPTS_FLAG(-g GccFlagG)
    IF ( GccFlagG )
        SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    ENDIF ( GccFlagG )
ENDIF ( CMAKE_COMPILER_IS_GNUCC  )

IF(ENABLE_GPROF)
    CHECK_CXX_ACCEPTS_FLAG(-pg GccFlagPg)
        
    IF ( GccFlagPg )
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
        #todo: add flag to linker also...
    ENDIF ( GccFlagPg )
ENDIF(ENABLE_GPROF)

IF (ENABLE_PACKAGING)
    INCLUDE(CreateClucenePackages)
    CREATE_CLUCENE_PACKAGES()
ENDIF ( ENABLE_PACKAGING)


#Single output directory for building all executables and libraries.
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE PATH "Executable Output Directory" FORCE)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE PATH "Library Output Directory" FORCE)

#add tests
#todo: why does 'make test' not work???
ENABLE_TESTING()
ADD_TEST(SimpleTest ${EXECUTABLE_OUTPUT_PATH}/cl_test )

#use single output directory
INCLUDE_DIRECTORIES( ${clucene_SOURCE_DIR}/src/shared )
INCLUDE_DIRECTORIES( ${clucene_BINARY_DIR}/src/shared )
INCLUDE_DIRECTORIES( ${clucene_SOURCE_DIR}/src/core )

#include the projects
ADD_SUBDIRECTORY (src/shared)
ADD_SUBDIRECTORY (src/core)
ADD_SUBDIRECTORY (src/test)
ADD_SUBDIRECTORY (src/demo EXCLUDE_FROM_ALL)
