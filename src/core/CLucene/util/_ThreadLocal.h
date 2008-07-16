/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_util_ThreadLocal_H
#define _lucene_util_ThreadLocal_H


//#include "CLucene/util/VoidMap.h"

CL_NS_DEF(util)

class _ThreadLocal{
private:
	class Internal;
    Internal* internal;
	DEFINE_MUTEX(locals_LOCK)
public:
	_ThreadLocal(CL_NS(util)::AbstractDeletor* _deletor);
    void* get();
    
	/**
	* Call this function to clear the local thread data for this
	* ThreadLocal. Calling set(NULL) does the same thing, except
	* this function is virtual and can be called without knowing
	* the template.
	*/
    void setNull();
    void set(void* t);
    virtual ~_ThreadLocal();

	/**
	* If you want to clean up thread specific memory, then you should
	* make sure this thread is called when the thread is not going to be used
	* again. This will clean up threadlocal data which can contain quite a lot
	* of data, so if you are creating lots of new threads, then it is a good idea
	* to use this function, otherwise there will be many memory leaks.
	*/
	static void UnregisterCurrentThread();

	/**
	* Call this function to shutdown CLucene
	*/
	static CLUCENE_LOCAL void _shutdown();

	/** 
	* A hook called when CLucene is starting or shutting down, 
	* this can be used for setting up and tearing down static
	* variables
	*/
	typedef void ShutdownHook(bool startup);

	/**
	* Add this function to the shutdown hook list. This function will be called
	* when CLucene is shutdown.
	*/
	static void registerShutdownHook(ShutdownHook* hook);
};

template<typename T,typename _deletor>
class ThreadLocal: public _ThreadLocal{
public:
	ThreadLocal():
		_ThreadLocal(_CLNEW _deletor)
	{
		
	}
	virtual ~ThreadLocal(){
	}
    T get(){
    	return (T)_ThreadLocal::get();
    }
    void setNull(){
    	_ThreadLocal::set((T)NULL);
    }
    void set(T t){
    	_ThreadLocal::set( (T) t);
    }
};
CL_NS_END
#endif
