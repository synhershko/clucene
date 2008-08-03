/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "CLucene/LuceneThreads.h"
#include "_ThreadLocal.h"
#include "CLucene/config/_threads.h"

CL_NS_DEF(util)

//todo: using http://en.wikipedia.org/wiki/Thread-local_storage#Pthreads_implementation
//would work better... but lots of testing would be needed first...
typedef CL_NS(util)::CLSetList<_ThreadLocal::ShutdownHook*, 
		CL_NS(util)::Compare::Void<_ThreadLocal::ShutdownHook>, 
		CL_NS(util)::Deletor::ConstNullVal<_ThreadLocal::ShutdownHook*> > ShutdownHooksType;

typedef CL_NS(util)::CLMultiMap<_LUCENE_THREADID_TYPE, _ThreadLocal*, 
		CL_NS(util)::CLuceneThreadIdCompare, 
		CL_NS(util)::Deletor::ConstNullVal<_LUCENE_THREADID_TYPE>, 
		CL_NS(util)::Deletor::ConstNullVal<_ThreadLocal*> > ThreadLocalsType;
		
#ifdef _LUCENE_THREADMUTEX
    //the lock for locking ThreadLocalBase_threadLocals
    //we don't use STATIC_DEFINE_MUTEX, because then the initialisation order will be undefined.
    static _LUCENE_THREADMUTEX *ThreadLocalBase_LOCK = NULL; 
#endif

static ThreadLocalsType*  ThreadLocalBase_threadLocals; //list of thread locals
//todo: make shutdown hooks generic
static ShutdownHooksType* ThreadLocalBase_shutdownHooks; //list of shutdown hooks.


class _ThreadLocal::Internal{
public:
	typedef CL_NS(util)::CLSet<_LUCENE_THREADID_TYPE, void*, 
			CL_NS(util)::CLuceneThreadIdCompare, 
			CL_NS(util)::Deletor::ConstNullVal<_LUCENE_THREADID_TYPE>, 
			CL_NS(util)::Deletor::ConstNullVal<void*> > LocalsType;
	LocalsType locals;
	DEFINE_MUTEX(locals_LOCK)
	AbstractDeletor* _deletor;
	
	Internal(AbstractDeletor* _deletor):
		locals(false,false){
			this->_deletor = _deletor;
	}
	~Internal(){
		//remove all the thread local data for this object
		LocalsType::iterator itr = locals.begin();
		while ( itr != locals.end() ){
			void* val = itr->second;
			locals.removeitr(itr);
			_deletor->Delete(val);
			itr = locals.begin();
		}
		
		delete _deletor;
	}
};

_ThreadLocal::_ThreadLocal(CL_NS(util)::AbstractDeletor* _deletor):
	internal(_CLNEW Internal(_deletor))
{
	
	//add this object to the base's list of ThreadLocalBase_threadLocals to be
	//notified in case of UnregisterThread()
	_LUCENE_THREADID_TYPE id = _LUCENE_CURRTHREADID;
	
	#ifdef _LUCENE_THREADMUTEX
	    //slightly un-usual way of initialising mutex, because otherwise our initialisation order would be undefined
    	if ( ThreadLocalBase_LOCK == NULL ){
    		ThreadLocalBase_LOCK = _CLNEW _LUCENE_THREADMUTEX;
    	}
	#endif
	
	if ( ThreadLocalBase_threadLocals == NULL ){
		ThreadLocalBase_threadLocals = _CLNEW ThreadLocalsType(false,false);
	}

	SCOPED_LOCK_MUTEX(*ThreadLocalBase_LOCK)
	ThreadLocalBase_threadLocals->put( id, this );
}

_ThreadLocal::~_ThreadLocal(){
	//remove this object from the ThreadLocalBase threadLocal list
	_LUCENE_THREADID_TYPE id = _LUCENE_CURRTHREADID;
	SCOPED_LOCK_MUTEX(*ThreadLocalBase_LOCK)

	ThreadLocalsType::iterator itr = ThreadLocalBase_threadLocals->lower_bound(id);
	ThreadLocalsType::iterator end = ThreadLocalBase_threadLocals->upper_bound(id);
	while ( itr != end ){
		if ( itr->second == this){
			ThreadLocalBase_threadLocals->erase(itr);
			break;
		}
		++itr;
	}
	delete internal;	
}


void* _ThreadLocal::get(){
	return internal->locals.get(_LUCENE_CURRTHREADID);
}

void _ThreadLocal::setNull(){
	set(NULL);
}

void _ThreadLocal::set(void* t){
	_LUCENE_THREADID_TYPE id = _LUCENE_CURRTHREADID;
	Internal::LocalsType::iterator itr = internal->locals.find(id);
	if ( itr != internal->locals.end() ){
		void* val = itr->second;
		internal->locals.removeitr(itr);
		internal->_deletor->Delete(val);
	}
	
	if ( t != NULL )
		internal->locals.put( id, t );
}

void _ThreadLocal::UnregisterCurrentThread(){
	_LUCENE_THREADID_TYPE id = _LUCENE_CURRTHREADID;
	SCOPED_LOCK_MUTEX(*ThreadLocalBase_LOCK)
	
	ThreadLocalsType::iterator itr = ThreadLocalBase_threadLocals->lower_bound(id);
	ThreadLocalsType::iterator end = ThreadLocalBase_threadLocals->upper_bound(id);
	while ( itr != end ){
		itr->second->setNull();
		++itr;
	}
}
void _ThreadLocal::_shutdown(){
	SCOPED_LOCK_MUTEX(*ThreadLocalBase_LOCK)
	
	ThreadLocalsType::iterator itr = ThreadLocalBase_threadLocals->begin();
	while ( itr != ThreadLocalBase_threadLocals->end() ){
		itr->second->setNull();
		++itr;
	}

	if ( ThreadLocalBase_shutdownHooks != NULL ){
		ShutdownHooksType::iterator itr2 = ThreadLocalBase_shutdownHooks->begin();
		while ( itr2 != ThreadLocalBase_shutdownHooks->end() ){
			ShutdownHook* hook = *itr2;
			hook(false);
		}
	}
}
void _ThreadLocal::registerShutdownHook(ShutdownHook* hook){
	SCOPED_LOCK_MUTEX(*ThreadLocalBase_LOCK)
	if ( ThreadLocalBase_shutdownHooks == NULL )
		ThreadLocalBase_shutdownHooks = _CLNEW ShutdownHooksType(false);
	ThreadLocalBase_shutdownHooks->insert(hook);
}


CL_NS_END
