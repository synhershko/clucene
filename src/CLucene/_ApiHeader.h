/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef lucene_internal_apiheader_h
#define lucene_internal_apiheader_h

#include "CLucene/StdHeader.h"
#include "CLucene/debug/_condition.h"
#include "CLucene/LuceneThreads.h"
#include "CLucene/util/_VoidMap.h"
#include "CLucene/util/_VoidList.h"
#include "CLucene/config/repl_wchar.h"

using namespace std;


char* lucenestrdup(const char* v);
#if defined(_UCS2)
    wchar_t* lucenewcsdup(const wchar_t* v);
    #define stringDuplicate(x) lucenewcsdup(x) //don't change this... uses [] instead of malloc
#else
    #define stringDuplicate(x) lucenestrdup(x) //don't change this... uses [] instead of malloc
#endif

#define STRCPY_AtoA(target,src,len) strncpy(target,src,len)
#define STRDUP_AtoA(x) lucenestrdup(x)

#if defined(_UCS2)
	#define STRDUP_WtoW(x) lucenewcsdup(x)
    #define STRDUP_TtoT STRDUP_WtoW
    #define STRDUP_WtoT STRDUP_WtoW
    #define STRDUP_TtoW STRDUP_WtoW

    #define STRDUP_AtoW(x) CL_NS(util)::Misc::_charToWide(x)
	#define STRDUP_AtoT STRDUP_AtoW

    #define STRDUP_WtoA(x) CL_NS(util)::Misc::_wideToChar(x)
	#define STRDUP_TtoA STRDUP_WtoA
    
    #define STRCPY_WtoW(target,src,len) _tcsncpy(target,src,len)
	#define STRCPY_TtoW STRCPY_WtoW
	#define STRCPY_WtoT STRCPY_WtoW
	//#define _tcscpy STRCPY_WtoW

    #define STRCPY_AtoW(target,src,len) CL_NS(util)::Misc::_cpycharToWide(src,target,len)
    #define STRCPY_AtoT STRCPY_AtoW

    #define STRCPY_WtoA(target,src,len) CL_NS(util)::Misc::_cpywideToChar(src,target,len)
    #define STRCPY_TtoA STRCPY_WtoA
#else
    #define STRDUP_AtoT STRDUP_AtoA
    #define STRDUP_TtoA STRDUP_AtoA
    #define STRDUP_TtoT STRDUP_AtoA

    #define STRDUP_WtoT(x) xxxxxxxxxxxxxxx //not possible
    #define STRCPY_WtoT(target,src,len) xxxxxxxxxxxxxxx //not possible

    #define STRCPY_AtoT STRCPY_AtoA
    #define STRCPY_TtoA STRCPY_AtoA
    //#define _tcscpy STRCPY_AtoA
#endif

////////////////////////////////////////////////////////
// Character functions.
// Here we decide whose character functions to use
////////////////////////////////////////////////////////
#if defined(LUCENE_USE_INTERNAL_CHAR_FUNCTIONS)
    #define stringCaseFold cl_tcscasefold
	#define stringCaseFoldCmp cl_tcscasefoldcmp
    
	#undef _istspace
	#undef _istdigit
	#undef _istalnum
	#undef _istalpha
	#undef _totlower
	#undef _totupper
    #define _istalnum cl_isalnum
    #define _istalpha cl_isletter
    #define _istspace cl_isspace
    #define _istdigit cl_isdigit
    #define _totlower cl_tolower
    #define _totupper cl_toupper

    //here are some functions to help deal with utf8/ucs2 conversions
    //lets let the user decide what mb functions to use... we provide pure utf8 ones no matter what.
    /*#undef _mbtowc
    #undef _mbstowcs
    #undef _wctomb
    #undef _wcstombs
    #define _mbtowc lucene_mbstowc
    #define _mbsstowcs lucene_mbstowcs
    #define _wctomb lucene_wcto_mb
    #define _wcstombs lucene_wcstombs*/
#else 
    //we are using native functions
	//here are some functions to help deal with utf8/ucs2 conversions
	/*#define _mbtowc mbtowc
	#define _wctomb wctomb
	#define _mbstowcs mbstowcs
	#define _wcstombs wcstombs*/

    //we are using native character functions
    #if defined(_ASCII)
        #undef _istspace
        #undef _istdigit
        #undef _istalnum
        #undef _istalpha
        #undef _totlower
        #undef _totupper
        #define _istspace(x) isspace((unsigned char)x)
        #define _istdigit(x) isdigit((unsigned char)x)
        #define _istalnum(x) isalnum((unsigned char)x)
        #define _istalpha(x) isalpha((unsigned char)x)
        #define _totlower(x) tolower((unsigned char)x)
        #define _totupper(x) toupper((unsigned char)x)
    #endif
#endif

//the methods contained in gunichartables.h
typedef unsigned long  clunichar;
bool cl_isletter(clunichar c);
bool cl_isalnum(clunichar c);
bool cl_isdigit(clunichar c);
bool cl_isspace (clunichar c);
TCHAR cl_tolower (TCHAR c);
TCHAR cl_toupper (TCHAR c);

int cl_tcscasefoldcmp(const TCHAR * dst, const TCHAR * src);
TCHAR* cl_tcscasefold( TCHAR * str, int len=-1 );

//we provide utf8 conversion functions
size_t lucene_utf8towc  (wchar_t *ret, const char *s, size_t n);
size_t lucene_utf8towcs(wchar_t *,    const char *,  size_t maxslen);
size_t lucene_wctoutf8  (char * ret,   const wchar_t  str);
size_t lucene_wcstoutf8 (char *,       const wchar_t *, size_t maxslen);
size_t lucene_utf8charlen(const char *p);

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

#endif // lucene_apiheader_h
