PROJECT (clucene)

#create package command

#Rules for version:
#MAJOR and MINOR versions are purely political
#REVISION version MUST be revised if the headers or compatibility change
#PATCH should be 0 unless a patch is made that doesn't affect the public signature (i.e. clients don't need to re-compile).
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "9")
SET(CPACK_PACKAGE_VERSION_REVISION "21")
SET(CPACK_PACKAGE_VERSION_PATCH "0")

SET(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_REVISION}.${CPACK_PACKAGE_VERSION_PATCH}")
SET(CPACK_PACKAGE_SOVERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_REVISION}")

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CLucene")
SET(CPACK_PACKAGE_VENDOR "Ben van Klinken")

SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CLucene - a C++ search engine, ported from the popular Apache Lucene")

SET(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")

SET(CPACK_GENERATOR "TGZ")
SET(CPACK_PACKAGE_FILE_NAME "clucene-core-${CPACK_PACKAGE_VERSION}")

IF(WIN32 AND NOT UNIX)
	SET(CPACK_SOURCE_GENERATOR "ZIP")
ELSE(WIN32 AND NOT UNIX)
	SET(CPACK_SOURCE_GENERATOR "TBZ2;TGZ")
ENDIF(WIN32 AND NOT UNIX)
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "clucene-core-src-${CPACK_PACKAGE_VERSION}")

#don't include the current binary dir.
get_filename_component(clucene_BINARY_DIR_name ${clucene_BINARY_DIR} NAME)
#SET(CPACK_SOURCE_INSTALL_CMAKE_PROJECTS "/home/ben/workspaces/lucene232/gcc;clucene;ALL;/")
#SET(CPACK_SOURCE_IGNORE_FILES "/CVS/;/\\\\.svn/;\\\\.swp$;\\\\.#;/#")
SET(CPACK_SOURCE_IGNORE_FILES "/\\\\.svn/;\\\\.swp$;\\\\.#;/#;.*~;.*\\\\.tmp;/${clucene_BINARY_DIR_name}/")

#IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  #SET(CPACK_GENERATOR "${CPACK_GENERATOR};NSIS")
  #SET(CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/Utilities/Release\\\\InstallIcon.bmp")
  #SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\MyExecutable.exe")
  #SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} My Famous Project")
  #SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.my-project-home-page.org")
  #SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.my-personal-home-page.com")
  #SET(CPACK_NSIS_CONTACT "me@my-personal-home-page.com")
  #SET(CPACK_NSIS_MODIFY_PATH ON)
#ELSE(WIN32 AND NOT UNIX)
#  SET(CPACK_STRIP_FILES "bin/MyExecutable")
#  SET(CPACK_SOURCE_STRIP_FILES "")
#ENDIF(WIN32 AND NOT UNIX)
#SET(CPACK_PACKAGE_EXECUTABLES "MyExecutable" "My Executable")

INCLUDE(CPack)

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
        #COMMAND("tar -czf doc/clucene-core-doc.tar.gz ${PROJECT_BINARY_DIR}/doc/html/")
    )
ENDIF(BUILD_DOCUMENTATION)


#use single output directory
INCLUDE_DIRECTORIES( ${clucene_SOURCE_DIR}/src )
INCLUDE_DIRECTORIES( ${clucene_BINARY_DIR}/src )

#include the projects
SUBDIRS (src/CLucene test src/demo)
