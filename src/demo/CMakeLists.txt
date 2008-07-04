PROJECT(cl_demo)

INCLUDE (DefineOptions)
DEFINE_OPTIONS(EXTRA_OPTIONS)
ADD_DEFINITIONS(${EXTRA_OPTIONS})

INCLUDE_DIRECTORIES( ${clucene-demo_SOURCE_DIR} )

CHECK_FUNCTION_EXISTS (gettimeofday _CL_HAVE_GETTIMEOFDAY)
CONFIGURE_FILE(${cl_demo_SOURCE_DIR}/config.h.cmake ${cl_demo_BINARY_DIR}/config.h)

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
