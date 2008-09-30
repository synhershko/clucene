PROJECT(clucene-contribs)

INCLUDE (DefineOptions)
DEFINE_OPTIONS(EXTRA_OPTIONS)
ADD_DEFINITIONS(${EXTRA_OPTIONS} -DMAKE_CLUCENE_CONTRIBS_LIB)

set(CMAKE_MODULE_PATH "${clucene-contribs_SOURCE_DIR}/cmake")

Include(FindIconv)
IF ( ICONV_SECOND_ARGUMENT_IS_CONST )
    SET ( ICONV_SECOND_ARGUMENT_IS_CONST 1 )
ELSE ( ICONV_SECOND_ARGUMENT_IS_CONST )
    SET ( ICONV_SECOND_ARGUMENT_IS_CONST 0 )
ENDIF ( ICONV_SECOND_ARGUMENT_IS_CONST )

CONFIGURE_FILE(${clucene-contribs_SOURCE_DIR}/CLucene/jstreams/jstreamsconfig.h.cmake ${clucene-contribs_BINARY_DIR}/CLucene/jstreams/jstreamsconfig.h)

#add the files to our groups and core
SOURCE_GROUP("analysis" ./CLucene/analysis/*)
#seems to be a bug in cmake, can't put these in analysis\\standard folder :(
SOURCE_GROUP("analysis-cjk" ./CLucene/analysis/cjk/*)
SOURCE_GROUP("highlighter" ./CLucene/highlighter/*)
SOURCE_GROUP("index" ./CLucene/index/*)
SOURCE_GROUP("jstreams" ./CLucene/jstreams/*)
SOURCE_GROUP("snowball" ./CLucene/snowball/*)
SOURCE_GROUP("snowball-impl" ./CLucene/snowball/include/*)
SOURCE_GROUP("snowball-impl" ./CLucene/snowball/libstemmer/*)
SOURCE_GROUP("snowball-impl" ./CLucene/snowball/runtime/*)
SOURCE_GROUP("snowball-impl" ./CLucene/snowball/src_c/*)

SET(clucene_contribs_Files
    ./CLucene/analysis/LanguageBasedAnalyzer.cpp
    ./CLucene/analysis/PorterStemmer.cpp
    ./CLucene/analysis/cjk/CJKAnalyzer.cpp
    
    ./CLucene/jstreams/dostime.cpp
    ./CLucene/jstreams/fileinputstream.cpp
    ./CLucene/jstreams/subinputstream.cpp
    ./CLucene/jstreams/tarinputstream.cpp
    
    ./CLucene/highlighter/Encoder.cpp
    ./CLucene/highlighter/Formatter.cpp
    ./CLucene/highlighter/Fragmenter.cpp
    ./CLucene/highlighter/Highlighter.cpp
    ./CLucene/highlighter/QueryScorer.cpp
    ./CLucene/highlighter/QueryTermExtractor.cpp
    ./CLucene/highlighter/SimpleFragmenter.cpp
    ./CLucene/highlighter/SimpleHTMLEncoder.cpp
    ./CLucene/highlighter/SimpleHTMLFormatter.cpp
    ./CLucene/highlighter/TextFragment.cpp
    ./CLucene/highlighter/TokenGroup.cpp
    ./CLucene/highlighter/TokenSources.cpp
    ./CLucene/highlighter/WeightedTerm.cpp
    
    ./CLucene/snowball/Snowball.cpp
    ./CLucene/snowball/libstemmer/libstemmer.c
    ./CLucene/snowball/runtime/api.c
    ./CLucene/snowball/runtime/utilities.c
    ./CLucene/snowball/src_c/stem_ISO_8859_1_danish.c
    ./CLucene/snowball/src_c/stem_ISO_8859_1_dutch.c
    ./CLucene/snowball/src_c/stem_ISO_8859_1_english.c
    ./CLucene/snowball/src_c/stem_ISO_8859_1_finnish.c
    ./CLucene/snowball/src_c/stem_ISO_8859_1_french.c
    ./CLucene/snowball/src_c/stem_ISO_8859_1_german.c
    ./CLucene/snowball/src_c/stem_ISO_8859_1_italian.c
    ./CLucene/snowball/src_c/stem_ISO_8859_1_norwegian.c
    ./CLucene/snowball/src_c/stem_ISO_8859_1_porter.c
    ./CLucene/snowball/src_c/stem_ISO_8859_1_portuguese.c
    ./CLucene/snowball/src_c/stem_ISO_8859_1_spanish.c
    ./CLucene/snowball/src_c/stem_ISO_8859_1_swedish.c
    ./CLucene/snowball/src_c/stem_KOI8_R_russian.c
    ./CLucene/snowball/src_c/stem_UTF_8_danish.c
    ./CLucene/snowball/src_c/stem_UTF_8_dutch.c
    ./CLucene/snowball/src_c/stem_UTF_8_english.c
    ./CLucene/snowball/src_c/stem_UTF_8_finnish.c
    ./CLucene/snowball/src_c/stem_UTF_8_french.c
    ./CLucene/snowball/src_c/stem_UTF_8_german.c
    ./CLucene/snowball/src_c/stem_UTF_8_italian.c
    ./CLucene/snowball/src_c/stem_UTF_8_norwegian.c
    ./CLucene/snowball/src_c/stem_UTF_8_porter.c
    ./CLucene/snowball/src_c/stem_UTF_8_portuguese.c
    ./CLucene/snowball/src_c/stem_UTF_8_russian.c
    ./CLucene/snowball/src_c/stem_UTF_8_spanish.c
    ./CLucene/snowball/src_c/stem_UTF_8_swedish.c
)

#add extra capabilities
find_package(ZLIB)
find_package(BZip2)
find_package(Iconv)
SET ( clucene-contrib-libs "" )
IF ( BZIP2_FOUND )
    SET ( clucene_contribs_Files ${clucene_contribs_Files}
          ./CLucene/jstreams/bz2inputstream.cpp 
        )
    INCLUDE_DIRECTORIES(${BZIP2_INCLUDE_DIR})
    SET ( clucene-contrib-libs "${clucene-contrib-libs} ${BZIP2_LIBRARIES}" )
ELSE ( BZIP2_FOUND )
    MESSAGE ( STATUS "warning: BZIP2 Library not found, BZ2InputStream will not be built" )
ENDIF ( BZIP2_FOUND )

IF ( ZLIB_FOUND )
    SET ( clucene_contribs_Files ${clucene_contribs_Files}
        ./CLucene/jstreams/gzipcompressstream.cpp
        ./CLucene/jstreams/gzipinputstream.cpp
		./CLucene/jstreams/zipinputstream.cpp
    )
    INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
    SET ( clucene-contrib-libs "${clucene-contrib-libs} ${ZLIB_LIBRARIES}" )
ELSE ( ZLIB_FOUND )
    MESSAGE ( STATUS "warning: ZLIB Library not found, GZipCompressInputStream and GZipInputStream will not be built" )
ENDIF ( ZLIB_FOUND )

IF ( ICONV_FOUND )
    SET ( clucene_contribs_Files ${clucene_contribs_Files}
        ./CLucene/jstreams/inputstreamreader.cpp
        ./CLucene/jstreams/filereader.cpp
    )
    INCLUDE_DIRECTORIES(${ICONV_INCLUDE_DIR})
    SET ( clucene-contrib-libs "${clucene-contrib-libs} ${ICONV_LIBRARIES}" )
ELSE ( ICONV_FOUND )
    MESSAGE ( STATUS "warning: ICONV Library not found, InputStreamReader and FileReader will not be built" )
ENDIF ( ICONV_FOUND )


    
IF ( USE_SHARED_OBJECT_FILES )
    GET_SHARED_FILES(clucene_shared_Files)
ENDIF ( USE_SHARED_OBJECT_FILES )

#create the libraries
INCLUDE_DIRECTORIES( ${clucene_SOURCE_DIR}/src/core )
INCLUDE_DIRECTORIES( ${clucene_SOURCE_DIR}/src/contribs-lib )
INCLUDE_DIRECTORIES( ${clucene_BINARY_DIR}/src/contribs-lib )

add_library(clucene-contribs SHARED
	${clucene_contribs_Files} ${clucene_shared_Files} ${HEADERS}
)
IF ( NOT USE_SHARED_OBJECT_FILES )
    TARGET_LINK_LIBRARIES(clucene-contribs clucene-shared)
ENDIF ( NOT USE_SHARED_OBJECT_FILES )

#set properties on the libraries
SET_TARGET_PROPERTIES(clucene-contribs PROPERTIES
    VERSION ${CLUCENE_VERSION}
    SOVERSION ${CLUCENE_SOVERSION}
    COMPILE_DEFINITIONS_DEBUG _DEBUG
)

IF ( BUILD_STATIC_LIBRARIES )
	add_library(clucene-contribs-static STATIC
		${clucene_contribs_Files} ${clucene_shared_Files} ${HEADERS}
	)
	
	SET_TARGET_PROPERTIES(clucene-contribs-static PROPERTIES
	    VERSION ${CLUCENE_VERSION}
	    SOVERSION ${CLUCENE_SOVERSION}
	    COMPILE_DEFINITIONS_DEBUG _DEBUG
	)
ENDIF ( BUILD_STATIC_LIBRARIES )
