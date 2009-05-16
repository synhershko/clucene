/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_debug_mem_h
#define _lucene_debug_mem_h

//todo: this is a hack...
#ifndef CND_PRECONDITION
	#define CND_PRECONDITION(x,y) assert(x)
#endif

//Macro for creating new objects
#if defined(LUCENE_ENABLE_REFCOUNT)
   #define _CLNEW new
#else
   #define _CLNEW new
   //todo: get this working again...
   //#define LUCENE_BASE_CHECK(obj) (obj)->dummy__see_mem_h_for_details
#endif
#define _CL_POINTER(x) (x==NULL?NULL:(x->__cl_addref()>=0?x:x)) //return a add-ref'd object

#if defined(_DEBUG)
  #if !defined(LUCENE_BASE_CHECK)
		#define LUCENE_BASE_CHECK(x)
	#endif
#else
	#undef LUCENE_BASE_CHECK
	#define LUCENE_BASE_CHECK(x)
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1300)
//6.0
	#define _CLDELETE_CARRAY(x) if (x!=NULL){free(x); x=NULL;}
	#define _CLDELETE_CaARRAY(x) if (x!=NULL){free(x); x=NULL;}
	#define _CLDELETE_LCARRAY(x) if (x!=NULL){free(x);}
	#define _CLDELETE_LCaARRAY(x) if (x!=NULL){free(x);}
#endif

//Macro for creating new arrays
#define _CL_NEWARRAY(type,size) (type*)calloc(size, sizeof(type))
#define _CLDELETE_ARRAY(x) if (x!=NULL){free(x); x=NULL;}
#define _CLDELETE_LARRAY(x) if (x!=NULL){free(x);}
#ifndef _CLDELETE_CARRAY
	#define _CLDELETE_CARRAY(x) if (x!=NULL){free(x); x=NULL;}
	#define _CLDELETE_LCARRAY(x) if (x!=NULL){free(x);}
#endif

//a shortcut for deleting a carray and all its contents
#define _CLDELETE_CARRAY_ALL(x) {if ( x!=NULL ){ for(int xcda=0;x[xcda]!=NULL;xcda++)_CLDELETE_CARRAY(x[xcda]);}_CLDELETE_ARRAY(x)};
#define _CLDELETE_LCARRAY_ALL(x) {if ( x!=NULL ){ for(int xcda=0;x[xcda]!=NULL;xcda++)_CLDELETE_LCARRAY(x[xcda]);}_CLDELETE_LARRAY(x)};
#define _CLDELETE_CaARRAY_ALL(x) {if ( x!=NULL ){ for(int xcda=0;x[xcda]!=NULL;xcda++)_CLDELETE_CaARRAY(x[xcda]);}_CLDELETE_ARRAY(x)};
#define _CLDELETE_ARRAY_ALL(x) {if ( x!=NULL ){ for(int xcda=0;x[xcda]!=NULL;xcda++)_CLDELETE(x[xcda]);}_CLDELETE_ARRAY(x)};
#ifndef _CLDELETE_CaARRAY
		#define _CLDELETE_CaARRAY _CLDELETE_CARRAY
		#define _CLDELETE_LCaARRAY _CLDELETE_LCARRAY
#endif

//Macro for deleting
#ifdef LUCENE_ENABLE_REFCOUNT
	#define _CLDELETE(x) if (x!=NULL){ CND_PRECONDITION((x)->__cl_refcount>=0,"__cl_refcount was < 0"); if ((x)->__cl_decref() <= 0)delete x; x=NULL; }
	#define _CLLDELETE(x) if (x!=NULL){ CND_PRECONDITION((x)->__cl_refcount>=0,"__cl_refcount was < 0"); if ((x)->__cl_decref() <= 0)delete x; }
#else
	#define _CLDELETE(x) if (x!=NULL){ LUCENE_BASE_CHECK(x); delete x; x=NULL; }
	#define _CLLDELETE(x) if (x!=NULL){ LUCENE_BASE_CHECK(x); delete x; }
#endif

//_CLDECDELETE deletes objects which are *always* refcounted
#define _CLDECDELETE(x) if (x!=NULL){ CND_PRECONDITION((x)->__cl_refcount>=0,"__cl_refcount was < 0"); if ((x)->__cl_decref() <= 0)delete x; x=NULL; }
#define _CLLDECDELETE(x) if (x!=NULL){ CND_PRECONDITION((x)->__cl_refcount>=0,"__cl_refcount was < 0"); if ((x)->__cl_decref() <= 0)delete x; }

//_VDelete should be used for deleting non-clucene objects.
//when using reference counting, _CLDELETE casts the object
//into a LuceneBase*.
#define _CLVDELETE(x) if(x!=NULL){delete x; x=NULL;}

#endif //_lucene_debug_mem_h
