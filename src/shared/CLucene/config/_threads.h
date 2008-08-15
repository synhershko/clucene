/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _config_threads_h
#define _config_threads_h

#ifndef _CL_DISABLE_MULTITHREADING
	#if defined(_LUCENE_DONTIMPLEMENT_THREADMUTEX)
		//do nothing
	#elif defined(_CL_HAVE_WIN32_THREADS)
      //we have not explicity included windows.h and windows.h has
      //not been included (check _WINBASE_), then we must define
      //our own definitions to the thread locking functions:
      #ifndef _WINBASE_
      extern "C"{
          struct CRITICAL_SECTION
          {
             struct critical_section_debug * DebugInfo;
             long LockCount;
             long RecursionCount;
             void * OwningThread;
             void * LockSemaphore;
             _cl_dword_t SpinCount;
          };
          __declspec(dllimport) void __stdcall InitializeCriticalSection(CRITICAL_SECTION *);
          __declspec(dllimport) void __stdcall EnterCriticalSection(CRITICAL_SECTION *);
          __declspec(dllimport) void __stdcall LeaveCriticalSection(CRITICAL_SECTION *);
          __declspec(dllimport) void __stdcall DeleteCriticalSection(CRITICAL_SECTION *);
    	  __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId();
      }
      #endif //_WINBASE_
	#elif defined(_CL_HAVE_PTHREAD)
	    #include <pthread.h>
	#endif
#endif

CL_NS_DEF(util)

#ifndef _CL_DISABLE_MULTITHREADING

#if defined(_LUCENE_DONTIMPLEMENT_THREADMUTEX)

#elif defined(_CL_HAVE_WIN32_THREADS)
	class CLuceneThreadIdCompare
	{
	public:
			
		enum
		{	// parameters for hash table
			bucket_size = 4,	// 0 < bucket_size
			min_buckets = 8
		};	// min_buckets = 2 ^^ N, 0 < N

		bool operator()( uint64_t t1, uint64_t t2 ) const{
			return t1 < t2;
		}
	};
	

#elif defined(_CL_HAVE_PTHREAD)

    class CLuceneThreadIdCompare
    {
    public:
    	enum
    	{	// parameters for hash table
    		bucket_size = 4,	// 0 < bucket_size
    		min_buckets = 8
    	};	// min_buckets = 2 ^^ N, 0 < N
    
    	bool operator()( pthread_t t1, pthread_t t2 ) const{
    		return t1 < t2;
    	}
    };
	
#endif //thread impl choice


#else //!_CL_DISABLE_MULTITHREADING
	class CLuceneThreadIdCompare
	{
	public:
		enum
		{	// parameters for hash table
			bucket_size = 4,	// 0 < bucket_size
			min_buckets = 8
		};	// min_buckets = 2 ^^ N, 0 < N

		bool operator()( char t1, char t2 ) const{
			return t1 < t2;
		}
	};
#endif //!_CL_DISABLE_MULTITHREADING

CL_NS_END


#endif //_config_threads_h
