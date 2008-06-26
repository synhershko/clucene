/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_repl_wchar_h
#define _lucene_repl_wchar_h

#include <cstdarg>

//string function replacements
#if defined(LUCENE_USE_INTERNAL_CHAR_FUNCTIONS) || (defined(_UCS2) && !defined(_CL_HAVE_WCSCASECMP)) || (defined(_ASCII) && !defined(_CL_HAVE_STRCASECMP))
    int lucene_tcscasecmp(const TCHAR *, const TCHAR *);
    #undef _tcsicmp
    #define _tcsicmp lucene_tcscasecmp
#endif
#if defined(LUCENE_USE_INTERNAL_CHAR_FUNCTIONS) || (defined(_UCS2) && !defined(_CL_HAVE_WCSLWR)) || (defined(_ASCII) && !defined(_CL_HAVE_STRLWR))
    TCHAR* lucene_tcslwr( TCHAR* str );
    #undef _tcslwr
    #define _tcslwr lucene_tcslwr
#endif

//conversion functions
#if (defined(_ASCII) && !defined(_CL_HAVE_LLTOA)) || (defined(_UCS2) && !defined(_CL_HAVE_LLTOW))
    TCHAR* lucene_i64tot( int64_t value, TCHAR* str, int radix);
    #undef _i64tot
    #define _i64tot lucene_i64tot
#endif
#if (defined(_UCS2) && !defined(_CL_HAVE_WCSTOLL)) || (defined(_ASCII) && !defined(_CL_HAVE_STRTOLL))
	int64_t lucene_tcstoi64(const TCHAR* str, TCHAR**end, int radix);
    #undef _tcstoi64
    #define _tcstoi64 lucene_tcstoi64
#endif
#if defined(_UCS2) && !defined(_CL_HAVE_WCSTOD)
    double lucene_tcstod(const TCHAR *value, TCHAR **end);
    #undef _tcstod
    #define _tcstod lucene_tcstod
#endif

//printf functions
#if defined(_UCS2) && (!defined(_snwprintf) || defined(_CL_HAVE_SNWPRINTF_BUG) )
    #undef _sntprintf
    #define _sntprintf lucene_snwprintf
    int lucene_snwprintf(wchar_t* strbuf, size_t count, const wchar_t * format, ...);
#endif
#if defined(_UCS2) && !defined(_CL_HAVE_WPRINTF)
    #undef _tprintf
    #define _tprintf lucene_wprintf
    void lucene_wprintf(const wchar_t * format, ...);
#endif
#if defined(_UCS2) && (!defined(_CL_HAVE_VSNWPRINTF) || defined(_CL_HAVE_SNWPRINTF_BUG) )
    #undef _vsntprintf
    #define _vsntprintf lucene_vsnwprintf
    int lucene_vsnwprintf(wchar_t * strbuf, size_t count, const wchar_t * format, va_list& ap);
#endif



//todo: if _CL_HAVE_SNPRINTF_BUG fails(snprintf overflow),we should use our own
//function. but we don't have it currently, and our functions are dubious anyway...

#endif //end of _lucene_repl_wchar_h
