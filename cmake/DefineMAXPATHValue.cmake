#checks if snprintf have bugs

MACRO ( DEFINE_MAXPATH_VALUE MaxPathValue )
	#use CHOOSE_SYMBOL mechanism to determine which variable to use...
	#CHOOSE_SYMBOL (_CL_MAX_PATH "PATH_MAX;MAX_PATH;_MAX_PATH;_POSIX_PATH_MAX" DefineMaxPathValue)
	#IF ( DefineMaxPathValue )
		#now try and find its value...
	#	Include ( MacroGetVariableValue )
		
	#	SET ( _CL_MAX_PATH_VALUE )
	#	GET_VARIABLE_VALUE (${DefineMaxPathValue} d _CL_MAX_PATH_VALUE)
	#	IF ( _CL_MAX_PATH_VALUE )
	#		SET( ${MaxPathValue} "#define ${MaxPathValue} ${_CL_MAX_PATH_VALUE}" )	
	#	ENDIF( _CL_MAX_PATH_VALUE )
	#ELSE ( DefineMaxPathValue )
	#	MESSAGE ( FATAL_ERROR "_CL_MAX_PATH could not be determined")
	#ENDIF ( DefineMaxPathValue )
	
    #SET (CMAKE_REQUIRED_DEFINITIONS)
    
    #HACK!!!
    #todo: fix this
    SET( ${MaxPathValue} "#define CL_MAX_PATH 260")
ENDMACRO ( DEFINE_MAXPATH_VALUE )
