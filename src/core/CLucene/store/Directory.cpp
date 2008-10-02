/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#include "Directory.h"
#include "LockFactory.h"
#include "CLucene/util/Misc.h"

CL_NS_DEF(store)


Directory::Directory(){
}
Directory::~Directory(){ 
}

LuceneLock* Directory::makeLock(const char* name) {
	return lockFactory->makeLock( name );
}

void Directory::setLockFactory( LockFactory* lockFactory ) {
	this->lockFactory = lockFactory;
	
	TCHAR* lockId = getLockID();
	char* alockId = STRDUP_TtoA(lockId);
	lockFactory->setLockPrefix( alockId );
	_CLDELETE_CARRAY(lockId);
	_CLDELETE_CaARRAY(alockId);
}

LockFactory* Directory::getLockFactory() {
	return lockFactory;
}

TCHAR* Directory::getLockID() {
	return toString();
}

void Directory::clearLock(const char* name) {
	if ( lockFactory != NULL ) {
		lockFactory->clearLock( name );
	}
}

bool Directory::deleteFile(const char* name, const bool throwError){
	bool ret = doDeleteFile(name);
	if ( !ret && throwError ){
      char buffer[200];
      _snprintf(buffer,200,"couldn't delete %s",name);
      _CLTHROWA(CL_ERR_IO, buffer );
    }
    return ret;
}
IndexInput* Directory::openInput(const char* name, int32_t bufferSize){ 
	IndexInput* ret;
	CLuceneError err;
	if ( ! openInput(name, ret, err, bufferSize) )
		throw err;
	return ret;
}
char** Directory:: list() const{
	vector<string> names;
	
	list(&names);

	size_t size = names.size();
    char** ret = _CL_NEWARRAY(char*,size+1);
    for ( size_t i=0;i<size;i++ )
      ret[i] = STRDUP_AtoA(names[i].c_str());
    ret[size]=NULL;
    return ret;	
}
CL_NS_END
