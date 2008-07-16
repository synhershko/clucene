#check if we can do try and catch.
#bit useless, since we don't have any alternatives to try and catch currently

MACRO ( CHECK_HAVE_FUNCTION_TRY_BLOCKS result )
    #check for try/catch blocks
    CHECK_CXX_SOURCE_RUNS("
    	void foo() try{} catch( ... ){}
    	int main(){ foo(); }" ${result})
    IF ( NOT _CL_HAVE_FUNCTION_TRY_BLOCKS )
    	SET ( ${result} 1 )
    ENDIF ( NOT _CL_HAVE_FUNCTION_TRY_BLOCKS )
ENDMACRO ( CHECK_HAVE_FUNCTION_TRY_BLOCKS )
