PROJECT (clucene)

#Rules for version:
#MAJOR and MINOR versions are purely political
#REVISION version MUST be revised if the headers or compatibility change
#PATCH should be 0 unless a patch is made that doesn't affect the public signature (i.e. clients don't need to re-compile).

SET(CLUCENE_VERSION_MAJOR "0")
SET(CLUCENE_VERSION_MINOR "9")
SET(CLUCENE_VERSION_REVISION "23")
SET(CLUCENE_VERSION_PATCH "0")
SET(CLUCENE_INT_VERSION 92300)

SET(CLUCENE_VERSION "${CLUCENE_VERSION_MAJOR}.${CLUCENE_VERSION_MINOR}.${CLUCENE_VERSION_REVISION}.${CLUCENE_VERSION_PATCH}")
SET(CLUCENE_SOVERSION "${CLUCENE_VERSION_MAJOR}.${CLUCENE_VERSION_MINOR}.${CLUCENE_VERSION_REVISION}")

CMAKE_MINIMUM_REQUIRED(VERSION 2.4.2 FATAL_ERROR)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

#set various platform specific global options
if(WIN32)
 set(CMAKE_DEBUG_POSTFIX "d")
endif(WIN32)


# include specific modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

#define options...

Include (CLuceneDocs)
Include (FindThreads)

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
OPTION(ENABLE_DMALLOC
  "enable dmalloc memory leak checker"
  OFF)
OPTION(ENABLE_ASCII_MODE
  "enable ascii support"
  OFF)
OPTION(LUCENE_USE_INTERNAL_CHAR_FUNCTIONS
  "use internal character functions. required to run tests correctly"
  ON)
OPTION(ENABLE_PACKAGING
  "create build scripts for creating clucene packages"
  OFF)
OPTION(BUILD_STATIC_LIBRARIES
  "create targets for building static libraries"
  OFF)
OPTION(BUILD_CONTRIBS
  "create targets for building the clucene-contribs"
  OFF)
OPTION(BUILD_CONTRIBS_LIB
  "create targets for building the clucene-contribs-lib"
  OFF)
SET(LUCENE_SYS_INCLUDES "" CACHE PATH
      "location for non-system independent files. defaults to CMAKE_INSTALL_PREFIX. see INSTALL documentation for further information."
      )
#install path options
SET(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
SET(LIB_DESTINATION "lib${LIB_SUFFIX}")


SET ( ENABLE_COMPILE_TESTS_VALUE ON )
IF ( MSVC_IDE )
	#this is annoying...
	SET ( ENABLE_COMPILE_TESTS_VALUE OFF )
ENDIF( MSVC_IDE )

OPTION(ENABLE_COMPILE_TESTS
  "enable various projects that test alternative build switches"
  ${ENABLE_COMPILE_TESTS_VALUE})

#check flags...
INCLUDE (TestCXXAcceptsFlag)
IF ( CMAKE_COMPILER_IS_GNUCC )
    CHECK_CXX_ACCEPTS_FLAG(-pg GccFlagPg)
    IF ( GccFlagPg )
        OPTION(ENABLE_GPROF
          "turn on gprof profiling support"
          OFF)

        IF ( ENABLE_GPROF )
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
            SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
            SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
            SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -pg")
        ENDIF ( ENABLE_GPROF )
    ENDIF ( GccFlagPg )
    
    IF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC"  )
    ENDIF("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")

ENDIF(CMAKE_COMPILER_IS_GNUCC) 


#Single output directory for building all executables and libraries.
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE PATH "Executable Output Directory" FORCE)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE PATH "Library Output Directory" FORCE)

#add tests
ENABLE_TESTING()
ADD_TEST(SimpleTest ${EXECUTABLE_OUTPUT_PATH}/cl_test )

#use single output directory
INCLUDE_DIRECTORIES( ${clucene_SOURCE_DIR}/src/shared )
INCLUDE_DIRECTORIES( ${clucene_BINARY_DIR}/src/shared )
INCLUDE_DIRECTORIES( ${clucene_SOURCE_DIR}/src/core )

#include the projects
ADD_SUBDIRECTORY (src/ext)
ADD_SUBDIRECTORY (src/shared)
ADD_SUBDIRECTORY (src/core)
ADD_SUBDIRECTORY (src/test)
ADD_SUBDIRECTORY (src/demo EXCLUDE_FROM_ALL)
IF ( BUILD_CONTRIBS )
  ADD_SUBDIRECTORY (src/contribs EXCLUDE_FROM_ALL)
  SET(BUILD_CONTRIBS_LIB 1)
ENDIF ( BUILD_CONTRIBS )
IF ( BUILD_CONTRIBS_LIB )
  ADD_SUBDIRECTORY (src/contribs-lib )
ENDIF ( BUILD_CONTRIBS_LIB )

#add uninstall command
CONFIGURE_FILE(
  "${CMAKE_MODULE_PATH}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
  
ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

#this must go last...
IF (ENABLE_PACKAGING)
  INCLUDE(CreateClucenePackages)
ENDIF ( ENABLE_PACKAGING)
