/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef lucene_internal_sharedheader_h
#define lucene_internal_sharedheader_h

#define LUCENE_INT32_MAX_SHOULDBE 0x7FFFFFFFL

#include "CLucene/_clucene-config.h"
#include "CLucene/SharedHeader.h"

//we always need this stuff....
#include "CLucene/debug/_condition.h"
#include "CLucene/LuceneThreads.h"
#include "CLucene/config/repl_tchar.h"
#include "CLucene/config/repl_wchar.h"


#define cl_min(a,b) (a>b?b:a)
#define cl_max(a,b) (a>b?a:b)

#ifdef _CL_HAVE_SAFE_CRT
	#define cl_sprintf sprintf_s
	#define cl_strcpy(Dst,Src,DstLen) strcpy_s(Dst,DstLen,Src)
#else
	#define cl_sprintf _snprintf
	#define cl_strcpy(Dst,Src,DstLen) strcpy(Dst,Src)
#endif


///a blank string...
extern const TCHAR* _LUCENE_BLANK_STRING;
#define LUCENE_BLANK_STRING _LUCENE_BLANK_STRING
extern const char* _LUCENE_BLANK_ASTRING;
#define LUCENE_BLANK_ASTRING _LUCENE_BLANK_ASTRING

#if defined(_WIN32) || defined(_WIN64)
    #define PATH_DELIMITERA "\\"
#else
    #define PATH_DELIMITERA "/"
#endif

#define _LUCENE_SLEEP(ms) CL_NS(util)::Misc::sleep(ms)


#include "CLucene/config/repl_tchar.h"  //replacements for functions
#include "CLucene/config/repl_wctype.h" //replacements for functions

#endif //lucene_internal_sharedheader_h