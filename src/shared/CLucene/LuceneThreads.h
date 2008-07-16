/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _LuceneThreads_h
#define  _LuceneThreads_h


CL_NS_DEF(util)
class CLuceneThreadIdCompare;

#if defined(_CL_DISABLE_MULTITHREADING)
	#define SCOPED_LOCK_MUTEX(theMutex)
	#define DEFINE_MUTEX(x)
	#define DEFINE_MUTABLE_MUTEX(x)
	#define STATIC_DEFINE_MUTEX(x)
	#define _LUCENE_CURRTHREADID 1
	#define _LUCENE_THREADID_TYPE char

#else
	#if defined(_LUCENE_DONTIMPLEMENT_THREADMUTEX)
		//do nothing
    #else
    	#if defined(_CL_HAVE_PTHREAD)
            #define _LUCENE_THREADID_TYPE pthread_t
    	    class CLUCENE_EXPORT mutex_thread
            {
            private:
                struct Internal;
                Internal* internal;
            public:
            	mutex_thread(const mutex_thread& clone);
            	mutex_thread();
            	~mutex_thread();
            	void lock();
            	void unlock();
            	static _LUCENE_THREADID_TYPE _GetCurrentThreadId();
            };
    
    	#elif defined(_CL_HAVE_WIN32_THREADS)
        	#define _LUCENE_THREADID_TYPE uint64_t
    	    class CLUCENE_EXPORT mutex_thread
        	{
        	private:
        		struct Internal;
        		Internal* internal;
        	public:
        		mutex_thread(const mutex_thread& clone);
        		mutex_thread();
        		~mutex_thread();
        		void lock();
        		void unlock();
        		static _LUCENE_THREADID_TYPE _GetCurrentThreadId();
        	};
        	
        	
    	#else
    		#error A valid thread library was not found
    	#endif //mutex types
    	
    	
        #define _LUCENE_CURRTHREADID mutex_thread::_GetCurrentThreadId()
        #define _LUCENE_THREADMUTEX CL_NS(util)::mutex_thread
    #endif //don't implement
	
	/** @internal */
	class CLUCENE_EXPORT mutexGuard
	{
	private:
		_LUCENE_THREADMUTEX* mrMutex;
		mutexGuard(const mutexGuard& clone);
	public:
		mutexGuard( _LUCENE_THREADMUTEX& rMutex );
		~mutexGuard();
	};
	
	#define SCOPED_LOCK_MUTEX(theMutex) 	CL_NS(util)::mutexGuard theMutexGuard(theMutex);
	#define DEFINE_MUTEX(theMutex) 			_LUCENE_THREADMUTEX theMutex;
	#define DEFINE_MUTABLE_MUTEX(theMutex)  mutable _LUCENE_THREADMUTEX theMutex;
	#define STATIC_DEFINE_MUTEX(theMutex) 	static _LUCENE_THREADMUTEX theMutex;

#endif //_CL_DISABLE_MULTITHREADING
CL_NS_END

#endif
