PROJECT(cl_demo)

INCLUDE_DIRECTORIES( ${clucene-demo_SOURCE_DIR} )

ADD_EXECUTABLE(cl_demo 
./DeleteFiles.cpp
./IndexFiles.cpp
./Main.cpp
./SearchFiles.cpp
./Statistics.cpp
./stdafx.cpp
)
TARGET_LINK_LIBRARIES(cl_demo clucene-core)
