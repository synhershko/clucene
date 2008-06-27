/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef lucene_stdheader_h
#define lucene_stdheader_h

#include "CLucene/clucene-config.h"
#include "CLucene/CLConfig.h"

////////////////////////////////////////////////////////
// EXPORTS definition
#ifdef MAKE_CLUCENE_CORE_LIB
	#ifdef CLUCENE_EXPORT_SYMBOLS
		#if defined(_WIN32)
			#define CLUCENE_EXPORT __declspec(dllexport)
		#elif defined(_CL_HAVE_GCCVISIBILITYPATCH)
			#define CLUCENE_EXPORT __attribute__ ((visibility("default")))
			#define CLUCENE_LOCAL __attribute__ ((visibility("hidden"))
		#endif
    #endif
#else
	#if defined(_WIN32) && defined(CLUCENE_IMPORT_SYMBOLS)
		#define CLUCENE_EXPORT __declspec(dllimport)
	#endif
#endif
#ifndef CLUCENE_EXPORT
	#define CLUCENE_EXPORT
#endif
#ifndef CLUCENE_LOCAL
	#define CLUCENE_LOCAL
#endif
////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
//namespace helper
////////////////////////////////////////////////////////
#if defined(_LUCENE_DONTIMPLEMENT_NS_MACROS)
    //do nothing
#elif !defined(DISABLE_NAMESPACE) && defined(_CL_HAVE_NAMESPACES)
	#define CL_NS_DEF(sub) namespace lucene{ namespace sub{
	#define CL_NS_DEF2(sub,sub2) namespace lucene{ namespace sub{ namespace sub2 {

	#define CL_NS_END }}
	#define CL_NS_END2 }}}

	#define CL_NS_USE(sub) using namespace lucene::sub;
	#define CL_NS_USE2(sub,sub2) using namespace lucene::sub::sub2;

	#define CL_NS(sub) lucene::sub
	#define CL_NS2(sub,sub2) lucene::sub::sub2
	    
	#define CL_STRUCT_DEF(sub,clazz) namespace lucene { namespace sub{ struct clazz; } }
	#define CL_CLASS_DEF(sub,clazz) namespace lucene { namespace sub{ class clazz; } }
	#define CL_CLASS_DEF1(sub,clazz) namespace sub{ class clazz; }
#else
	#define CL_NS_DEF(sub)
	#define CL_NS_DEF2(sub, sub2)
	#define CL_NS_END
	#define CL_NS_END2
	#define CL_NS_USE(sub)
	#define CL_NS_USE2(sub,sub2)
	#define CL_NS(sub)
	#define CL_NS2(sub,sub2)
	#define CL_CLASS_DEF(sub,clazz) class clazz;
#endif

#if defined(LUCENE_NO_STDC_NAMESPACE)
   //todo: haven't actually tested this on a non-stdc compliant compiler
   #define CL_NS_STD(func) ::func
#else
   #define CL_NS_STD(func) std::func
#endif
//
////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
//Class interfaces
////////////////////////////////////////////////////////
#include "CLucene/debug/lucenebase.h"
////////////////////////////////////////////////////////

//Are we in unicode?
#if defined(_MBCS) || defined(_ASCII)
 #undef _ASCII
 #undef _UCS2
 #define _ASCII
#elif defined(_UNICODE)
 #define _UCS2
#elif !defined(_UCS2)
 #define _UCS2
#endif

//msvc needs unicode define so that it uses unicode library
#ifdef _UCS2
	#undef _UNICODE
	#define _UNICODE
	#undef _ASCII
#else
	#undef _UNICODE
	#undef _UCS2
#endif


#define cl_min(a,b) (a>b?b:a)
#define cl_max(a,b) (a>b?a:b)

#define LUCENE_INT32_MAX_SHOULDBE 0x7FFFFFFFL
#define __CONST_CAST(typ,var) const_cast<typ>(var)
#define __REINTERPRET_CAST(typ,var) reinterpret_cast<typ>(var)

#if _MSC_FULL_VER >= 140050320
    #define _CL_DEPRECATE_TEXT(_Text) __declspec(deprecated(_Text))
#elif _MSC_VER >= 1300
    #define _CL_DEPRECATE_TEXT(_Text) __declspec(deprecated)
#elif _MSC_VER
    #pragma warning(disable : 4786)
    #pragma warning(disable : 4150)//todo:re-enable this
    #define _CL_DEPRECATE_TEXT(_Text)
#elif (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
	#define _CL_DEPRECATE_TEXT(_Text)  __attribute__((__deprecated__))
#else
    #define _CL_DEPRECATE_TEXT(_Text)
#endif
#define _CL_DEPRECATED(_NewItem) _CL_DEPRECATE_TEXT("This function or variable has been superceded by newer library or operating system functionality. Consider using" #_NewItem "instead. See online help for details.")


//need this for wchar_t, size_t, NULL
#include <cstddef>
//need this for int32_t, etc
#ifdef _CL_HAVE_STDINT_H
    #include <stdint.h>
#endif
#include <math.h> //required for float_t
#include <string> //need to include this early...

#include "CLucene/config/repl_tchar.h"
#include "CLucene/debug/error.h"
#include "CLucene/debug/mem.h"

#define StringArrayWithDeletor       CL_NS(util)::CLVector<TCHAR*, CL_NS(util)::Deletor::tcArray >
#define StringArray                  CL_NS(util)::CLVector<TCHAR*>
#define StringArrayWithDeletor       CL_NS(util)::CLVector<TCHAR*, CL_NS(util)::Deletor::tcArray >
#define StringArrayConst             CL_NS(util)::CLVector<const TCHAR*>
#define StringArrayConstWithDeletor  CL_NS(util)::CLVector<const TCHAR*, CL_NS(util)::Deletor::tcArray >

#define AStringArray                 CL_NS(util)::CLVector<char*>
#define AStringArrayWithDeletor      CL_NS(util)::CLVector<char*, CL_NS(util)::Deletor::acArray >
#define AStringArrayConst            CL_NS(util)::CLVector<const char*>
#define AStringArrayConstWithDeletor CL_NS(util)::CLVector<const char*, CL_NS(util)::Deletor::acArray >

//some replacement functions...
int64_t lucene_filelength(int handle);

//call this at the end of running to clean up memory. 
extern CLUCENE_EXPORT void _lucene_shutdown();

#endif // lucene_apiheader_h
