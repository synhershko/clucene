/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "_StringIntern.h"
CL_NS_DEF(util)

typedef CL_NS(util)::CLHashMap<const TCHAR*,int,CL_NS(util)::Compare::TChar,CL_NS(util)::Equals::TChar,CL_NS(util)::Deletor::tcArray, CL_NS(util)::Deletor::DummyInt32 > __wcsintrntype;
typedef CL_NS(util)::CLHashMap<const char*,int,CL_NS(util)::Compare::Char,CL_NS(util)::Equals::Char,CL_NS(util)::Deletor::acArray, CL_NS(util)::Deletor::DummyInt32 > __strintrntype;
__wcsintrntype StringIntern_stringPool(true);
__strintrntype StringIntern_stringaPool(true);

bool StringIntern_blanksinitd=false;
__wcsintrntype::iterator StringIntern_wblank;

//STATIC_DEFINE_MUTEX(StringIntern_THIS_LOCK);
DEFINE_MUTEX(StringIntern_THIS_LOCK)
	  

    void CLStringIntern::shutdown(){
    #ifdef _DEBUG
		SCOPED_LOCK_MUTEX(StringIntern_THIS_LOCK)
        if ( StringIntern_stringaPool.size() > 0 ){
            printf("ERROR: stringaPool still contains intern'd strings (refcounts):\n");
            __strintrntype::iterator itr = StringIntern_stringaPool.begin();
            while ( itr != StringIntern_stringaPool.end() ){
                printf(" %s (%d)\n",(itr->first), (itr->second));
                ++itr;
            }
        }
        
        if ( StringIntern_stringPool.size() > 0 ){
            printf("ERROR: stringPool still contains intern'd strings (refcounts):\n");
            __wcsintrntype::iterator itr = StringIntern_stringPool.begin();
            while ( itr != StringIntern_stringPool.end() ){
                _tprintf(_T(" %s (%d)\n"),(itr->first), (itr->second));
                ++itr;
            }
        }
    #endif
    }

	const TCHAR* CLStringIntern::intern(const TCHAR* str){
		if ( str == NULL )
			return NULL;
		if ( str[0] == 0 )
			return LUCENE_BLANK_STRING;

		SCOPED_LOCK_MUTEX(StringIntern_THIS_LOCK)

		__wcsintrntype::iterator itr = StringIntern_stringPool.find(str);
		if ( itr==StringIntern_stringPool.end() ){
#ifdef _UCS2
			TCHAR* ret = lucenewcsdup(str);
#else
			TCHAR* ret = lucenestrdup(str);
#endif
			StringIntern_stringPool[ret]= 1;
			return ret;
		}else{
			(itr->second)++;
			return itr->first;
		}
	}

	bool CLStringIntern::unintern(const TCHAR* str){
		if ( str == NULL )
			return false;
		if ( str[0] == 0 )
			return false;

		SCOPED_LOCK_MUTEX(StringIntern_THIS_LOCK)

		__wcsintrntype::iterator itr = StringIntern_stringPool.find(str);
		if ( itr != StringIntern_stringPool.end() ){
			if ( (itr->second) == 1 ){
				StringIntern_stringPool.removeitr(itr);
				return true;
			}else
				(itr->second)--;
		}
		return false;
	}
	
	const char* CLStringIntern::internA(const char* str){
		if ( str == NULL )
			return NULL;
		if ( str[0] == 0 )
			return _LUCENE_BLANK_ASTRING;

		SCOPED_LOCK_MUTEX(StringIntern_THIS_LOCK)

		__strintrntype::iterator itr = StringIntern_stringaPool.find(str);
		if ( itr==StringIntern_stringaPool.end() ){
			char* ret = lucenestrdup(str);
			StringIntern_stringaPool[ret] = 1;
			return ret;
		}else{
			(itr->second)++;
			return itr->first;
		}
	}
	
	bool CLStringIntern::uninternA(const char* str){
		if ( str == NULL )
			return false;
		if ( str[0] == 0 )
			return false;

		SCOPED_LOCK_MUTEX(StringIntern_THIS_LOCK)

		__strintrntype::iterator itr = StringIntern_stringaPool.find(str);
		if ( itr!=StringIntern_stringaPool.end() ){
			if ( (itr->second) == 1 ){
				StringIntern_stringaPool.removeitr(itr);
				return true;
			}else
				(itr->second)--;
		}
		return false;
	}

	/* removed because of multi-threading problems...
	__wcsintrntype::iterator CLStringIntern::internitr(const TCHAR* str){
		if ( str[0] == 0 ){
			if ( !StringIntern_blanksinitd ){
				StringIntern_stringPool.put(LUCENE_BLANK_STRING,1);
				StringIntern_wblank=stringPool.find(str); 
				StringIntern_blanksinitd=true;
			}
			return StringIntern_wblank;
		}
		__wcsintrntype::iterator itr = StringIntern_stringPool.find(str);
		if (itr==StringIntern_stringPool.end()){
#ifdef _UCS2
			TCHAR* ret = lucenewcsdup(str);
#else
			TCHAR* ret = lucenestrdup(str);
#endif
			StringIntern_stringPool.put(ret,1);
			return StringIntern_stringPool.find(str);
		}else{
			(itr->second)++;
			return itr;
		}
	}
	bool CLStringIntern::uninternitr(__wcsintrntype::iterator itr){
		if ( itr!=StringIntern_stringPool.end() ){
			if ( itr==StringIntern_wblank )
				return false;	
			if ( (itr->second) == 1 ){
				StringIntern_stringPool.removeitr(itr);
				return true;
			}else
				(itr->second)--;
		}
		return false;
	}
*/

CL_NS_END
