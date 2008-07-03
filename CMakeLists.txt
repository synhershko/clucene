PROJECT (clucene)

CMAKE_MINIMUM_REQUIRED(VERSION 2.4.2 FATAL_ERROR)

# include specific modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

#define options...

Include (Documentation)

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
  "enable ascii support")

#Single output directory for building all executables and libraries.
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE PATH "Executable Output Directory" FORCE)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE PATH "Library Output Directory" FORCE)

#add tests
ENABLE_TESTING()
ADD_TEST(SimpleTest ${EXECUTABLE_OUTPUT_PATH}/cl_test )

# To build the documention, you will have to enable it
# and then do the equivalent of "make DoxygenDoc".
IF(BUILD_DOCUMENTATION)
    IF(DOT)
        SET(HAVE_DOT YES)
    ELSE(DOT)
        SET(HAVE_DOT NO)
    ENDIF(DOT)
    # This processes our Doxyfile.cmake and substitutes paths to generate a final Doxyfile
    CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/Doxyfile.cmake ${PROJECT_BINARY_DIR}/doc/doxyfile )
    CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/doc/helpheader.htm.cmake ${PROJECT_BINARY_DIR}/doc/helpheader.htm )
    CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/doc/helpfooter.htm.cmake ${PROJECT_BINARY_DIR}/doc/helpfooter.htm )
    CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/doc/doxygen.css.cmake ${PROJECT_BINARY_DIR}/doc/html/doxygen.css )
    CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/doc/clucene.jpg ${PROJECT_BINARY_DIR}/doc/clucene.jpg COPYONLY )

    # This creates a new target to build documentation.
    # It runs ${DOXYGEN} which is the full path and executable to
    # Doxygen on your system, set by the FindDoxygen.cmake module
    # (called by FindDocumentation.cmake).
    # It runs the final generated Doxyfile against it.
    # The DOT_PATH is substituted into the Doxyfile.
    ADD_CUSTOM_TARGET(DoxygenDoc ${DOXYGEN}
        ${PROJECT_BINARY_DIR}/doc/doxyfile
        COMMAND(tar -czf doc/clucene-core-doc.tar.gz ${PROJECT_BINARY_DIR}/doc/html/)
    )
ENDIF(BUILD_DOCUMENTATION)


#use single output directory
INCLUDE_DIRECTORIES( ${clucene_SOURCE_DIR}/src )
INCLUDE_DIRECTORIES( ${clucene_BINARY_DIR}/src )

#include the projects
SUBDIRS (src/CLucene test src/demo)
