/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_SharedHeader.h"
#include "CLucene/LuceneThreads.h"
#include "_threads.h"

CL_NS_DEF(util)

#ifndef _CL_DISABLE_MULTITHREADING

#if defined(_LUCENE_DONTIMPLEMENT_THREADMUTEX)
	//do nothing
	#if defined(_LUCENE_PRAGMA_WARNINGS)
	 #pragma message ("==================Not implementing any thread mutex==================")
	#else
	 #warning "==================Not implementing any thread mutex=================="
	#endif



#elif defined(_CL_HAVE_WIN32_THREADS)
	struct mutex_thread::Internal{
	    CRITICAL_SECTION mtx;
	};

	mutex_thread::mutex_thread(const mutex_thread& clone):
		internal(new Internal)
	{
		InitializeCriticalSection(&internal->mtx); 
	}
	mutex_thread::mutex_thread():
		internal(new Internal)
	{ 
		InitializeCriticalSection(&internal->mtx); 
	}
	
	mutex_thread::~mutex_thread()
	{ 
		DeleteCriticalSection(&internal->mtx); 
		delete internal;
	}
	
	void mutex_thread::lock()
	{ 
		EnterCriticalSection(&internal->mtx); 
	}
	
	void mutex_thread::unlock()
	{ 
		LeaveCriticalSection(&internal->mtx); 
	}

    _LUCENE_THREADID_TYPE mutex_thread::_GetCurrentThreadId(){
        return GetCurrentThreadId();
    }


#elif defined(_CL_HAVE_PTHREAD)
	#ifdef _CL_HAVE_PTHREAD_MUTEX_RECURSIVE
		bool mutex_pthread_attr_initd=false;
		pthread_mutexattr_t mutex_thread_attr;
	#endif

	#ifdef _CL__CND_DEBUG
		#define _CLPTHREAD_CHECK(c,m) CND_PRECONDITION(c==0,m)
	#else
		#define _CLPTHREAD_CHECK(c,m) c;
	#endif
	
	struct mutex_thread::Internal{
    	pthread_mutex_t mtx;
    	#ifndef _CL_HAVE_PTHREAD_MUTEX_RECURSIVE
    	pthread_t lockOwner;
    	unsigned int lockCount;
    	#endif
    };

	mutex_thread::mutex_thread(const mutex_thread& clone):
		internal(new Internal)
	{
		
		#ifdef _CL_HAVE_PTHREAD_MUTEX_RECURSIVE
			_CLPTHREAD_CHECK(pthread_mutex_init(&internal->mtx, &mutex_thread_attr), "mutex_thread(clone) constructor failed")
		#else
		  	#if defined(__hpux) && defined(_DECTHREADS_)
				_CLPTHREAD_CHECK(pthread_mutex_init(&internal->mtx, pthread_mutexattr_default), "mutex_thread(clone) constructor failed")
			#else
				_CLPTHREAD_CHECK(pthread_mutex_init(&internal->mtx, 0), "mutex_thread(clone) constructor failed")
			#endif
			internal->lockCount=0;
			internal->lockOwner=0;
		#endif
	}
	mutex_thread::mutex_thread():
		internal(new Internal)
	{ 

		#ifdef _CL_HAVE_PTHREAD_MUTEX_RECURSIVE
	  	if ( mutex_pthread_attr_initd == false ){
	  		pthread_mutexattr_init(&mutex_thread_attr);
		  	pthread_mutexattr_settype(&mutex_thread_attr, PTHREAD_MUTEX_RECURSIVE);
		  	mutex_pthread_attr_initd = true;
		}
		_CLPTHREAD_CHECK(pthread_mutex_init(&internal->mtx, &mutex_thread_attr), "mutex_thread(clone) constructor failed")
		#else
	  	#if defined(__hpux) && defined(_DECTHREADS_)
			_CLPTHREAD_CHECK(pthread_mutex_init(&internal->mtx, pthread_mutexattr_default), "mutex_thread(clone) constructor failed")
		#else
			_CLPTHREAD_CHECK(pthread_mutex_init(&internal->mtx, 0), "mutex_thread(clone) constructor failed")
		#endif
		internal->lockCount=0;
		internal->lockOwner=0;
		#endif
	}
	
	mutex_thread::~mutex_thread()
	{ 
		_CLPTHREAD_CHECK(pthread_mutex_destroy(&internal->mtx), "~mutex_thread destructor failed") 
		delete internal;
	}
	
    _LUCENE_THREADID_TYPE mutex_thread::_GetCurrentThreadId(){
        return pthread_self();
    }

	void mutex_thread::lock()
	{ 
		#ifndef _CL_HAVE_PTHREAD_MUTEX_RECURSIVE
		pthread_t currentThread = pthread_self();
		if( pthread_equal( internal->lockOwner, currentThread ) ) {
			++internal->lockCount;
		} else {
			_CLPTHREAD_CHECK(pthread_mutex_lock(&internal->mtx), "mutex_thread::lock")
			internal->lockOwner = currentThread;
			internal->lockCount = 1;
		}
		#else
		_CLPTHREAD_CHECK(pthread_mutex_lock(&internal->mtx), "mutex_thread::lock")
		#endif
	}
	
	void mutex_thread::unlock()
	{ 
		#ifndef _CL_HAVE_PTHREAD_MUTEX_RECURSIVE
		--internal->lockCount;
		if( internal->lockCount == 0 )
		{
			internal->lockOwner = 0;
			_CLPTHREAD_CHECK(pthread_mutex_unlock(&internal->mtx), "mutex_thread::unlock")
		}
		#else
		_CLPTHREAD_CHECK(pthread_mutex_unlock(&internal->mtx), "mutex_thread::unlock")
		#endif
	}

#endif //thread impl choice


mutexGuard::mutexGuard(const mutexGuard& clone){
	//no autoclone
	mrMutex = NULL;
}
mutexGuard::mutexGuard( _LUCENE_THREADMUTEX& rMutex ) :
	mrMutex(&rMutex)
{
	mrMutex->lock();
}
mutexGuard::~mutexGuard()
{
	mrMutex->unlock();
}

#endif //!_CL_DISABLE_MULTITHREADING

CL_NS_END
