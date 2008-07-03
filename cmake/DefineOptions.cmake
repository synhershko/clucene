#define global options, this makes it easy to use ccmake, or the cmake gui
MACRO (DEFINE_OPTIONS extraOptions)
    IF(ENABLE_DEBUG)
    	SET (${extraOptions} "${${extraOptions}} -D_DEBUG")
    ENDIF(ENABLE_DEBUG)
    
    IF(ENABLE_MMAP)
    	SET (${extraOptions} "${${extraOptions}} -DLUCENE_FS_MMAP")
    ENDIF(ENABLE_MMAP)
    
    IF(ENABLE_MMAP)
    	SET (${extraOptions} "${${extraOptions}} -D_CL_DISABLE_MULTITHREADING")
    ENDIF(ENABLE_MMAP)
    
    IF(ENABLE_ASCII_MODE)
    	SET (${extraOptions} "${${extraOptions}} -D_ASCII")
    ELSE(ENABLE_ASCII_MODE)
    	SET (${extraOptions} "${${extraOptions}} -D_UCS2")
    	SET (${extraOptions} "${${extraOptions}} -D_UNICODE")
    ENDIF(ENABLE_ASCII_MODE)



ENDMACRO (DEFINE_OPTIONS)
