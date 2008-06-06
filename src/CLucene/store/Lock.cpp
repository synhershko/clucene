/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/StdHeader.h"
#include "Lock.h"
#include "CLucene/util/Misc.h"

CL_NS_USE(util)
CL_NS_DEF(store)

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


   SingleInstanceLock::SingleInstanceLock( LocksType* locks, const char* lockName )
   {
	   this->locks = locks;
	   this->lockName = lockName;
   }
   
   bool SingleInstanceLock::obtain()
   {
	   SCOPED_LOCK_MUTEX(locks->THIS_LOCK);
	   return locks->insert( lockName ).second;
   }
   
   void SingleInstanceLock::release()
   {
	   SCOPED_LOCK_MUTEX(locks->THIS_LOCK);
	   LocksType::iterator itr = locks->find( lockName );
	   if ( itr != locks->end() ) {
		   locks->remove(itr, true);
	   }
   }
   
   bool SingleInstanceLock::isLocked()
   {
	   SCOPED_LOCK_MUTEX(locks->THIS_LOCK);
	   return locks->contains( lockName );
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
   	  this->lockDir = STRDUP_AtoA(_lockDir);
   	  strcpy(lockFile,_lockDir);
   	  strcat(lockFile,PATH_DELIMITERA);
   	  strcat(lockFile,name);
   }
   
   FSLock::~FSLock()
   {
	   _CLDELETE_LCaARRAY( lockDir );
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
	       int32_t r = _open(lockFile,  O_RDWR | O_CREAT | O_RANDOM | O_EXCL, 
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
