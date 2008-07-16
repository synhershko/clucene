/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/

#include "CLucene/_SharedHeader.h"


#ifdef _CL_HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif


const TCHAR* _LUCENE_BLANK_STRING=_T("");
const char* _LUCENE_BLANK_ASTRING="";


#if defined(_ASCII)
  #if defined(_LUCENE_PRAGMA_WARNINGS)
	 #pragma message ("==================Using ascii mode!!!==================")
	#else
	 #warning "==================Using ascii mode!!!=================="
	#endif
#endif


//these are functions that lucene uses which
//are not replacement functions
char* lucenestrdup(const char* v){
    size_t len = strlen(v);
    char* ret = new char[len+1];
    strncpy(ret,v,len+1);
    return ret;
}

#ifdef _UCS2
wchar_t* lucenewcsdup(const wchar_t* v){
    size_t len = _tcslen(v);
    wchar_t* ret = new wchar_t[len+1];
    _tcsncpy(ret,v,len+1);
    return ret;
}
#endif //ucs2

