/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "Lock.h"
#include "_Lock.h"
#include "CLucene/util/Misc.h"

#ifdef _CL_HAVE_IO_H
	#include <io.h>
#endif
#ifdef _CL_HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif
#ifdef _CL_HAVE_UNISTD_H
	#include <unistd.h>
#endif
#ifdef _CL_HAVE_DIRECT_H
	#include <direct.h>
#endif
#include <fcntl.h>

  
CL_NS_USE(util)
CL_NS_DEF(store)

  LuceneLock::~LuceneLock()
  {
  }
   bool LuceneLock::obtain(int64_t lockWaitTimeout) {
      bool locked = obtain();
      if ( lockWaitTimeout < 0 && lockWaitTimeout != LOCK_OBTAIN_WAIT_FOREVER ) {
    	  _CLTHROWA(CL_ERR_IllegalArgument,"lockWaitTimeout should be LOCK_OBTAIN_WAIT_FOREVER or a non-negative number");
      }
      
      int64_t maxSleepCount = lockWaitTimeout / LOCK_POLL_INTERVAL;
      int64_t sleepCount = 0;
      
      while (!locked) {
         if ( lockWaitTimeout != LOCK_OBTAIN_WAIT_FOREVER && sleepCount++ == maxSleepCount ) {
            _CLTHROWA(CL_ERR_IO,"Lock obtain timed out");
         }
         _LUCENE_SLEEP(LOCK_POLL_INTERVAL);
         locked = obtain();
      }
      
      return locked;
   }
   TCHAR* NoLock::toString()
   {
        return STRDUP_TtoT(_T("NoLock"));
   }
   bool NoLock::obtain() { return true; }
   void NoLock::release() {}
   bool NoLock::isLocked() { return false; }
   
  const char* NoLock::getClassName(){
    return "NoLock";
  }
  const char* NoLock::getObjectName() const{
    return getClassName();
  }
   


   SingleInstanceLock::SingleInstanceLock( LocksType* locks, _LUCENE_THREADMUTEX* locks_LOCK, const char* lockName )
   {
	   this->locks = locks;
	   this->locks_LOCK = locks_LOCK;
	   this->lockName = lockName;
   }
  const char* SingleInstanceLock::getClassName(){
    return "SingleInstanceLock";
  }
  const char* SingleInstanceLock::getObjectName() const{
    return getClassName();
  }
   
   bool SingleInstanceLock::obtain()
   {
	   SCOPED_LOCK_MUTEX(*locks_LOCK);
	   return locks->insert( lockName ).second;
   }
   
   void SingleInstanceLock::release()
   {
	   SCOPED_LOCK_MUTEX(*locks_LOCK);
	   LocksType::iterator itr = locks->find( lockName );
	   if ( itr != locks->end() ) {
		   locks->remove(itr, true);
	   }
   }
   
   bool SingleInstanceLock::isLocked()
   {
	   SCOPED_LOCK_MUTEX(*locks_LOCK);
	   return locks->find( lockName ) == locks->end();
   }
   
   TCHAR* SingleInstanceLock::toString()
   {
	   TCHAR* ret = _CL_NEWARRAY(TCHAR,strlen(lockName)+20); // 20 = strlen("SingleInstanceLock:");
	   _tcscpy(ret,_T("SingleInstanceLock:"));
	   STRCPY_AtoT(ret+19,lockName,strlen(lockName)+1);
	   
	   return ret;
   }
        
   
   FSLock::FSLock( const char* _lockDir, const char* name )
   {
	  this->lockFile = _CL_NEWARRAY(char,CL_MAX_PATH);
   	  this->lockDir = STRDUP_AtoA(_lockDir);
   	  strcpy(lockFile,_lockDir);
   	  strcat(lockFile,PATH_DELIMITERA);
   	  strcat(lockFile,name);
   }
   
   FSLock::~FSLock()
   {
	   _CLDELETE_ARRAY( lockFile );
	   _CLDELETE_LCaARRAY( lockDir );
   }
   
  const char* FSLock::getClassName(){
    return "FSLock";
  }
  const char* FSLock::getObjectName() const{
    return getClassName();
  }

   bool FSLock::obtain()
   {
	   	if ( !Misc::dir_Exists(lockDir) ){
	         if ( _mkdir(lockDir) == -1 ){
	   		  char* err = _CL_NEWARRAY(char,34+strlen(lockDir)+1); //34: len of "Couldn't create lock directory: "
	   		  strcpy(err,"Couldn't create lock directory: ");
	   		  strcat(err,lockDir);
	   		  _CLTHROWA_DEL(CL_ERR_IO, err );
	         }
	       }
	       int32_t r = _cl_open(lockFile,  O_RDWR | O_CREAT | _O_RANDOM | O_EXCL, 
	       	_S_IREAD | _S_IWRITE); //must do this or file will be created Read only
	   	if ( r < 0 ) {
	   	  return false;
	   	} else {
	   	  _close(r);
	   	  return true;
	   	}
   }
   
   void FSLock::release()
   {
	   _unlink( lockFile );
   }
   
   bool FSLock::isLocked()
   {
	   return Misc::dir_Exists(lockFile);	   
   }
   
   TCHAR* FSLock::toString()
   {
	   TCHAR* ret = _CL_NEWARRAY(TCHAR,strlen(lockFile)+14); // 14 = strlen("SimpleFSLock@")
	   _tcscpy(ret,_T("SimpleFSLock@"));
	   STRCPY_AtoT(ret+13,lockFile,strlen(lockFile)+1);
	   
	   return ret;
   }
   
CL_NS_END
