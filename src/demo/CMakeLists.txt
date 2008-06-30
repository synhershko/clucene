PROJECT(cl_demo)

INCLUDE_DIRECTORIES( ${clucene-demo_SOURCE_DIR} )


#todo: put in common config
OPTION(ENABLE_ASCII_MODE
  "enable ascii support"
  OFF)
IF(ENABLE_ASCII_MODE)
	ADD_DEFINITIONS(-D_ASCII)
ELSE(ENABLE_ASCII_MODE)
	ADD_DEFINITIONS(-D_UNICODE)
ENDIF(ENABLE_ASCII_MODE)

file(GLOB_RECURSE HEADERS ${CMAKE_SOURCE_DIR}/test/*.h)

ADD_EXECUTABLE(cl_demo 
./DeleteFiles.cpp
./IndexFiles.cpp
./Main.cpp
./SearchFiles.cpp
./Statistics.cpp
./stdafx.cpp
./dirent.cpp
${HEADERS}
)


SET_TARGET_PROPERTIES(cl_demo PROPERTIES
#    VERSION "0.9.16a"
#    SOVERSION "0.9"
DEFINE_SYMBOL CLUCENE_IMPORT_SYMBOLS
)
TARGET_LINK_LIBRARIES(cl_demo clucene-core)
