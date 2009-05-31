prefix=${CMAKE_INSTALL_PREFIX}
exec_prefix=${CMAKE_INSTALL_PREFIX}/bin
libdir=${CMAKE_INSTALL_PREFIX}/${LIB_DESTINATION}
includedir=${CMAKE_INSTALL_PREFIX}/include

Name: libclucene
Description: CLucene - a C++ search engine, ported from the popular Apache Lucene
Version: ${CLUCENE_VERSION_MAJOR}.${CLUCENE_VERSION_MINOR}.${CLUCENE_VERSION_REVISION}.${CLUCENE_VERSION_PATCH}
Libs: -L${CMAKE_INSTALL_PREFIX}/${LIB_DESTINATION}/ -llibclucene
Cflags: -I${CMAKE_INSTALL_PREFIX}/include
~
