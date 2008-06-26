/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/

#include "CLucene/_ApiHeader.h"
#include "LockFactory.h"
#include "_Lock.h"
#include "CLucene/util/Misc.h"

#ifdef _CL_HAVE_WINDOWS_H
	#include <windows.h>
#endif
#ifdef _CL_HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif

CL_NS_USE(util)
CL_NS_DEF(store)

class LocksType: public CL_NS(util)::CLHashSet<const char*, CL_NS(util)::Compare::Char, CL_NS(util)::Deletor::acArray>
{
public:
	LocksType(bool del)
	{
		setDoDelete(del);
	}
	virtual ~LocksType(){
	}
};

LockFactory::LockFactory()
{
	lockPrefix = NULL;
}

LockFactory::~LockFactory()
{
	_CLDELETE_CARRAY( lockPrefix );
}

void LockFactory::setLockPrefix( TCHAR* lockPrefix )
{
        _CLDELETE_CARRAY( lockPrefix );
	this->lockPrefix = lockPrefix;
}

TCHAR* LockFactory::getLockPrefix()
{
	return lockPrefix;	
}

SingleInstanceLockFactory::SingleInstanceLockFactory()
{
	locks = _CLNEW LocksType(false);
}

SingleInstanceLockFactory::~SingleInstanceLockFactory()
{
	_CLDELETE( locks );
}

LuceneLock* SingleInstanceLockFactory::makeLock( const char* lockName )
{
	return _CLNEW SingleInstanceLock( locks, lockName );
}

void SingleInstanceLockFactory::clearLock( const char* lockName )
{
	SCOPED_LOCK_MUTEX(locks->THIS_LOCK);
	LocksType::iterator itr = locks->find( lockName );
	if ( itr != locks->end() ) {
		locks->remove( itr );
	}
}


NoLockFactory* NoLockFactory::singleton = NULL;
NoLock* NoLockFactory::singletonLock = NULL;

void NoLockFactory::shutdown(){
	_CLDELETE(NoLockFactory::singleton);
	_CLDELETE(NoLockFactory::singletonLock);
}

NoLockFactory* NoLockFactory::getNoLockFactory()
{
	if ( singleton == NULL ) {
		singleton = _CLNEW NoLockFactory();
	}
	return singleton;
}

LuceneLock* NoLockFactory::makeLock( const char* lockName )
{
	if ( singletonLock == NULL ) {
		singletonLock = _CLNEW NoLock();
	}
	return singletonLock;
}

void NoLockFactory::clearLock( const char* lockName )
{	
}


FSLockFactory::FSLockFactory( const char* lockDir )
{
	setLockDir( lockDir );
}

FSLockFactory::~FSLockFactory()
{
}

void FSLockFactory::setLockDir( const char* lockDir )
{
	this->lockDir = lockDir;
}

LuceneLock* FSLockFactory::makeLock( const char* lockName )
{
	char name[CL_MAX_DIR];
	
	if ( lockPrefix != NULL ) {
		STRCPY_TtoA(name,lockPrefix,_tcslen(lockPrefix)+1);
		strcat(name,"-");
		strcat(name,lockName);
	} else {
		strcpy(name,lockName);
	}
	
	return _CLNEW FSLock( lockDir, name );
}

void FSLockFactory::clearLock( const char* lockName )
{
	if ( Misc::dir_Exists( lockDir )) {
		char name[CL_MAX_DIR];
		char path[CL_MAX_DIR];
		struct fileStat buf;
		
		if ( lockPrefix != NULL ) {
			STRCPY_TtoA(name,lockPrefix,_tcslen(lockPrefix)+1);
			strcat(name,"-");
			strcat(name,lockName);
		} else {
			strcpy(name,lockName);
		}
		
		_snprintf(path,CL_MAX_DIR,"%s/%s",lockDir,name);
		
		int32_t ret = fileStat(path,&buf);
		if ( ret==0 && !(buf.st_mode & S_IFDIR) && _unlink( path ) == -1 ) {
			_CLTHROWA(CL_ERR_IO, "Couldn't delete file" ); // TODO: make richer error
		}
	}			
}


CL_NS_END
