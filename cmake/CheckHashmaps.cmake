#check how to use hashmaps (which namespace)
#HashingValue is filled with namespace definition
#DisableHashing is set if we can't support hashing

#find hashing namespace (internal, use CHECK_HASH_MAPS) ...
MACRO(HASHMAP_TEST namespace)
	IF ( NOT ${HashingValue} )
        IF ( _CL_HAVE_EXT_HASH_MAP )
            SET(CMAKE_REQUIRED_DEFINITIONS "-D_CL_HAVE_EXT_HASH_MAP=1")
        ENDIF ( _CL_HAVE_EXT_HASH_MAP )
        IF ( _CL_HAVE_HASH_MAP )
            SET(CMAKE_REQUIRED_DEFINITIONS "-D_CL_HAVE_HASH_MAP=1")
        ENDIF ( _CL_HAVE_HASH_MAP )
        
        STRING(TOUPPER ${namespace} NAMESPACE)
        STRING(REPLACE / _ NAMESPACE ${NAMESPACE})
        	    
        CHECK_CXX_SOURCE_COMPILES("
#if defined(_CL_HAVE_HASH_MAP)
	#include <hash_map>
#elif defined(_CL_HAVE_EXT_HASH_MAP)
	#include <ext/hash_map>
#endif
int main() {  
    ${namespace}::hash_map<int,char> a; 
    return 0; 
}
        " _CL_HAVE_${NAMESPACE}_HASHMAP)
        
        IF ( _CL_HAVE_${NAMESPACE}_HASHMAP )
            SET (${HashingValue} "${namespace}::func")
        ENDIF ( _CL_HAVE_${NAMESPACE}_HASHMAP )
    
    ENDIF ( NOT ${HashingValue} )
ENDMACRO(HASHMAP_TEST)


MACRO ( CHECK_HASH_MAPS HashingValue DisableHashing)
    IF ( _CL_HAVE_EXT_HASH_MAP OR _CL_HAVE_HASH_MAP )
        HASHMAP_TEST (std)
        HASHMAP_TEST (stdext)
        HASHMAP_TEST (__gnu_cxx)
    
        IF ( NOT ${HashingValue} )
            MESSAGE ( STATUS "Namespace for hash maps/sets not determined, disabling hashing (not a big problem)")
            SET(${DisableHashing} 1)
        ENDIF ( NOT ${HashingValue} )
    
    ENDIF ( _CL_HAVE_EXT_HASH_MAP OR _CL_HAVE_HASH_MAP )
ENDMACRO ( CHECK_HASH_MAPS )
