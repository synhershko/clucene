/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "CLucene/util/_Misc.h"

#include "CLucene/search/Sort.h"
#include "CLucene/search/Similarity.h"
#include "CLucene/search/FieldCache.h"
#include "CLucene/search/FieldSortedHitQueue.h"
#include "CLucene/store/LockFactory.h"
#include "CLucene/util/_StringIntern.h"

#ifdef _CL_HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif

#if defined(_MSC_VER) && defined(_DEBUG)
	#define CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

CL_NS_USE(util)

const TCHAR* _LUCENE_BLANK_STRING=_T("");
const char* _LUCENE_BLANK_ASTRING="";

#if defined(_ASCII)
  #if defined(_LUCENE_PRAGMA_WARNINGS)
	 #pragma message ("==================Using ascii mode!!!==================")
	#else
	 #warning "==================Using ascii mode!!!=================="
	#endif
#endif


//clears all static memory. do not attempt to do anything else
//in clucene after calling this function
void _lucene_shutdown(){
    CL_NS(search)::FieldSortedHitQueue::shutdown();
	_CLDELETE(CL_NS(search)::Sort::RELEVANCE);
	_CLDELETE(CL_NS(search)::Sort::INDEXORDER);
	_CLDELETE(CL_NS(search)::ScoreDocComparator::INDEXORDER);
	_CLDELETE(CL_NS(search)::ScoreDocComparator::RELEVANCE);
	_CLDELETE(CL_NS(search)::SortField::FIELD_SCORE);
	_CLDELETE(CL_NS(search)::SortField::FIELD_DOC);
	_CLDELETE(CL_NS(search)::FieldCache::DEFAULT);

	_CLLDELETE(CL_NS(search)::Similarity::getDefault());

    CL_NS(util)::CLStringIntern::shutdown();
    CL_NS(store)::NoLockFactory::shutdown();
}

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


//ok, these are the exceptions, but these never
//exist on non-msvc platform, so lets put it here
#ifndef _CL_HAVE_FILELENGTH
int64_t lucene_filelength(int filehandle)
{
    struct fileStat info;
    if (fileHandleStat(filehandle, &info) == -1)
 	 _CLTHROWA( CL_ERR_IO,"fileStat error" );
    return info.st_size;
}
#endif
