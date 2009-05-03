/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "RAMDirectory.h"
#include "_RAMDirectory.h"
#include "Lock.h"
#include "LockFactory.h"
#include "Directory.h"
#include "FSDirectory.h"
#include "CLucene/index/IndexReader.h"
//#include "CLucene/util/VoidMap.h"
#include "CLucene/util/Misc.h"

CL_NS_USE(util)
CL_NS_DEF(store)


	// *****
	// Lock acquisition sequence:  RAMDirectory, then RAMFile
	// *****

	class RAMLock: public LuceneLock{
	private:
		RAMDirectory* directory;
		char* fname;
	public:
		RAMLock(const char* name, RAMDirectory* dir);
		virtual ~RAMLock();
		bool obtain();
		void release();
		bool isLocked();
		virtual TCHAR* toString();
	};



  RAMFile::RAMFile( RAMDirectory* _directory )
  {
     length = 0;  
	 lastModified = Misc::currentTimeMillis();
	 this->directory = _directory;
	 sizeInBytes = 0;
  }
  
  RAMFile::~RAMFile(){
  }

  int64_t RAMFile::getLength()
  {
	  SCOPED_LOCK_MUTEX(THIS_LOCK);
	  return length;
  }
  
  void RAMFile::setLength( int64_t length )
  {
	  SCOPED_LOCK_MUTEX(THIS_LOCK);
	  this->length = length;
  }
  
  uint64_t RAMFile::getLastModified()
  {
	  SCOPED_LOCK_MUTEX(THIS_LOCK);
	  return lastModified;
  }
  
  void RAMFile::setLastModified( const uint64_t lastModified )
  {
	  SCOPED_LOCK_MUTEX(THIS_LOCK);
	  this->lastModified = lastModified;
  }
  
  uint8_t* RAMFile::addBuffer( const int32_t size )
  {
	  SCOPED_LOCK_MUTEX(THIS_LOCK);
	  uint8_t* buffer = newBuffer(size);
	  RAMFileBuffer* rfb = _CLNEW RAMFileBuffer(buffer, size);
	  if ( directory != NULL ) {
		  SCOPED_LOCK_MUTEX(directory->THIS_LOCK);
		  buffers.push_back( rfb );
		  directory->sizeInBytes += size;
	  } else {
		buffers.push_back(rfb);
	  }
	  return buffer;
  }
  
  uint8_t* RAMFile::getBuffer( const int32_t index )
  {
	  SCOPED_LOCK_MUTEX(THIS_LOCK);
	  return buffers[index]->_buffer;
  }
  
  int32_t RAMFile::numBuffers() const
  {
	  return buffers.size();
  }
  
  uint8_t* RAMFile::newBuffer( const int32_t size )
  {
	  return _CL_NEWARRAY( uint8_t, size );
  }
  
  int64_t RAMFile::getSizeInBytes() const
  {
	  if ( directory != NULL ) {
		  SCOPED_LOCK_MUTEX(directory->THIS_LOCK);
		  return sizeInBytes;
	  }
	  return 0;
  }
  

  RAMIndexOutput::~RAMIndexOutput(){
	if ( deleteFile ){
        _CLDELETE(file);
	}else
     	file = NULL;
  }
  RAMIndexOutput::RAMIndexOutput(RAMFile* f):
	file(f),
	deleteFile(false),
	currentBuffer(NULL),
	currentBufferIndex(-1),
	bufferPosition(0),
	bufferStart(0),
	bufferLength(0)
  {
  }
  
  RAMIndexOutput::RAMIndexOutput():
    file(_CLNEW RAMFile),
    deleteFile(true),
    currentBuffer(NULL),
    currentBufferIndex(-1),
    bufferPosition(0),
    bufferStart(0),
    bufferLength(0)
  {
  }

  void RAMIndexOutput::writeTo(IndexOutput* out){
    flush();
    const int64_t end = file->getLength();
    int64_t pos = 0;
    int32_t p = 0;
    while (pos < end) {
      int32_t length = BUFFER_SIZE;
      int64_t nextPos = pos + length;
      if (nextPos > end) {                        // at the last buffer
        length = (int32_t)(end - pos);
      }
      out->writeBytes(file->getBuffer(p++), length);
      pos = nextPos;
    }
  }

  void RAMIndexOutput::reset(){
	seek((int64_t)0);
    file->setLength((int64_t)0);
  }

  void RAMIndexOutput::close() {
    flush();
  }

  /** Random-at methods */
  void RAMIndexOutput::seek( const int64_t pos ) {
          // set the file length in case we seek back
          // and flush() has not been called yet
	  setFileLength();
	  if ( pos < bufferStart || pos >= bufferStart + bufferLength ) {
		  currentBufferIndex = (int32_t)(pos / BUFFER_SIZE);
		  switchCurrentBuffer();
	  }
	  
	  bufferPosition = (int32_t)( pos % BUFFER_SIZE );
  }
  
  int64_t RAMIndexOutput::length() const {
    return file->getLength();
  }

  void RAMIndexOutput::writeByte( const uint8_t b ) {  
	  if ( bufferPosition == bufferLength ) {
		  currentBufferIndex++;
		  switchCurrentBuffer();
	  }
	  currentBuffer[bufferPosition++] = b;
  }

  void RAMIndexOutput::writeBytes( const uint8_t* b, const int32_t len ) {
	  int32_t srcOffset = 0;
	  
	  while ( srcOffset != len ) {
		  if ( bufferPosition == bufferLength ) {
			  currentBufferIndex++;
			  switchCurrentBuffer();
		  }
		  
		  int32_t remainInSrcBuffer = len - srcOffset;
		  int32_t bytesInBuffer = bufferLength - bufferPosition;
		  int32_t bytesToCopy = bytesInBuffer >= remainInSrcBuffer ? remainInSrcBuffer : bytesInBuffer;
		  
		  memcpy( currentBuffer+bufferPosition, b+srcOffset, bytesToCopy * sizeof(uint8_t) );
		  
		  srcOffset += bytesToCopy;
		  bufferPosition += bytesToCopy;
	  }	  	  
  }
  
  void RAMIndexOutput::switchCurrentBuffer() {
	  
	  if ( currentBufferIndex == file->numBuffers() ) {
		  currentBuffer = file->addBuffer( BUFFER_SIZE );
		  bufferLength = BUFFER_SIZE;
	  } else {
		  currentBuffer = file->getBuffer( currentBufferIndex );
		  bufferLength = file->getBufferLen(currentBufferIndex);
	  }
	  
	  bufferPosition = 0;
	  bufferStart = (int64_t)BUFFER_SIZE * (int64_t)currentBufferIndex;
  }


  
  void RAMIndexOutput::setFileLength() {
	  int64_t pointer = bufferStart + bufferPosition;
	  if ( pointer > file->getLength() ) {
		  file->setLength( pointer );
	  }
  }
  
  void RAMIndexOutput::flush() {
	  file->setLastModified( Misc::currentTimeMillis() );
	  setFileLength();
  }
  
  int64_t RAMIndexOutput::getFilePointer() const {
	  return currentBufferIndex < 0 ? 0 : bufferStart + bufferPosition;
  }
  
  
  RAMIndexInput::RAMIndexInput(RAMFile* f):
  	file(f), 
  	currentBuffer(NULL), 
  	currentBufferIndex(-1), 
  	bufferPosition(0), 
  	bufferStart(0), 
  	bufferLength(0) 
  {
    _length = f->getLength();
    
    if ( _length/BUFFER_SIZE >= 0x7FFFFFFFL ) {
    	// TODO: throw exception
    }    
  }
  
  RAMIndexInput::RAMIndexInput(const RAMIndexInput& other):
    IndexInput(other)
  {
  	file = other.file;
    _length = other._length;
    currentBufferIndex = other.currentBufferIndex;
    currentBuffer = other.currentBuffer;
    bufferPosition = other.bufferPosition;
    bufferStart = other.bufferStart;
    bufferLength = other.bufferLength;
  }
  
  RAMIndexInput::~RAMIndexInput(){
      RAMIndexInput::close();
  }
  
  IndexInput* RAMIndexInput::clone() const
  {
    RAMIndexInput* ret = _CLNEW RAMIndexInput(*this);
    return ret;
  }
  
  int64_t RAMIndexInput::length() const {
    return _length;
  }
  
  const char* RAMIndexInput::getDirectoryType() const{ 
    return RAMDirectory::getClassName(); 
  }
	const char* RAMIndexInput::getObjectName() const{ return getClassName(); }
	const char* RAMIndexInput::getClassName(){ return "RAMIndexInput"; }
  
  uint8_t RAMIndexInput::readByte()
  {
	  if ( bufferPosition >= bufferLength ) {
		  currentBufferIndex++;
		  switchCurrentBuffer();
	  }
	  return currentBuffer[bufferPosition++];
  }
  
  void RAMIndexInput::readBytes( uint8_t* dest, const int32_t len ) {
	  
	  int32_t destOffset = 0;
	  int32_t remainder = len;
	  
	  while ( remainder > 0 ) {
		  if ( bufferPosition >= bufferLength ) {
			  currentBufferIndex++;
			  switchCurrentBuffer();
		  }
		  
		  int32_t remainInBuffer = bufferLength - bufferPosition;
		  int32_t bytesToCopy = len < remainInBuffer ? len : remainInBuffer;
		  
		  memcpy( dest+destOffset, currentBuffer+bufferPosition, bytesToCopy * sizeof(uint8_t) );
		  
		  destOffset += bytesToCopy;
		  remainder -= bytesToCopy;
		  bufferPosition += bytesToCopy;
	  }
	  
  }
  
  int64_t RAMIndexInput::getFilePointer() const {
	  return currentBufferIndex < 0 ? 0 : bufferStart + bufferPosition;
  }
  
  void RAMIndexInput::seek( const int64_t pos ) {
	  if ( currentBuffer == NULL || pos < bufferStart || pos >= bufferStart + BUFFER_SIZE ) {
		  currentBufferIndex = (int32_t)( pos / BUFFER_SIZE );
		  switchCurrentBuffer();
	  }
	  bufferPosition = (int32_t)(pos % BUFFER_SIZE);
  }
  
  void RAMIndexInput::close() {
  }
  
  void RAMIndexInput::switchCurrentBuffer() {
	  if ( currentBufferIndex >= file->numBuffers() ) {
		  // end of file reached, no more buffers left
		  _CLTHROWA(CL_ERR_IO, "Read past EOF");
	  } else {
		  currentBuffer = file->getBuffer( currentBufferIndex );
		  bufferPosition = 0;
		  bufferStart = (int64_t)BUFFER_SIZE * (int64_t)currentBufferIndex;
		  int64_t bufLen = _length - bufferStart;
		  bufferLength = bufLen > BUFFER_SIZE ? BUFFER_SIZE : static_cast<int32_t>(bufLen);
	  }	  
  }
  



  void RAMDirectory::list(vector<string>* names) const{
    SCOPED_LOCK_MUTEX(files_mutex);

	FileMap::const_iterator itr = files->begin();
    while (itr != files->end()){
        names->push_back(itr->first);
        ++itr;
    }
  }

  RAMDirectory::RAMDirectory():
   Directory(),files(_CLNEW FileMap(true,true))
  {
	  setLockFactory( _CLNEW SingleInstanceLockFactory() );
  }
    
  RAMDirectory::~RAMDirectory(){
   //todo: should call close directory?
	  _CLDELETE( lockFactory );
	  _CLDELETE( files );
  }

  void RAMDirectory::_copyFromDir(Directory* dir, bool closeDir)
  {
  	vector<string> names;
    dir->list(&names);
    uint8_t buf[CL_NS(store)::BufferedIndexOutput::BUFFER_SIZE];

    for (size_t i=0;i<names.size();++i ){
		if ( !CL_NS(index)::IndexReader::isLuceneFile(names[i].c_str()))
            continue;
            
        // make place on ram disk
        IndexOutput* os = createOutput(names[i].c_str());
        // read current file
        IndexInput* is = dir->openInput(names[i].c_str());
        // and copy to ram disk
        //todo: this could be a problem when copying from big indexes... 
        int64_t len = is->length();
        int64_t readCount = 0;
        while (readCount < len) {
            int32_t toRead = (int32_t)(readCount + CL_NS(store)::BufferedIndexOutput::BUFFER_SIZE > len ? len - readCount : CL_NS(store)::BufferedIndexOutput::BUFFER_SIZE);
            is->readBytes(buf, toRead);
            os->writeBytes(buf, toRead);
            readCount += toRead;
        }
        
        // graceful cleanup
        is->close();
        _CLDELETE(is);
        os->close();
        _CLDELETE(os);
    }
    if (closeDir)
       dir->close();
  }
  RAMDirectory::RAMDirectory(Directory* dir):
   Directory(),files( _CLNEW FileMap(true,true) )
  {
	setLockFactory( _CLNEW SingleInstanceLockFactory() );
    _copyFromDir(dir,false);    
  }
  
   RAMDirectory::RAMDirectory(const char* dir):
      Directory(),files( _CLNEW FileMap(true,true) )
   {
      Directory* fsdir = FSDirectory::getDirectory(dir,false);
      try{
         _copyFromDir(fsdir,false);
      }_CLFINALLY(fsdir->close();_CLDECDELETE(fsdir););

   }

  bool RAMDirectory::fileExists(const char* name) const {
    SCOPED_LOCK_MUTEX(files_mutex);
    return files->exists(name);
  }

  int64_t RAMDirectory::fileModified(const char* name) const {
	  SCOPED_LOCK_MUTEX(files_mutex);
	  RAMFile* f = files->get(name);
	  return f->getLastModified();
  }

  int64_t RAMDirectory::fileLength(const char* name) const {
	  SCOPED_LOCK_MUTEX(files_mutex);
	  RAMFile* f = files->get(name);
          return f->getLength();
  }


  bool RAMDirectory::openInput(const char* name, IndexInput*& ret, CLuceneError& error, int32_t bufferSize) {
    SCOPED_LOCK_MUTEX(files_mutex);
    RAMFile* file = files->get(name);
    if (file == NULL) { /* DSR:PROPOSED: Better error checking. */
		error.set(CL_ERR_IO, "[RAMDirectory::open] The requested file does not exist.");
		return false;
    }
    ret = _CLNEW RAMIndexInput( file );
	return true;
  }

  void RAMDirectory::close(){
      SCOPED_LOCK_MUTEX(files_mutex);
      files->clear();
      _CLDELETE(files);
  }

  bool RAMDirectory::doDeleteFile(const char* name) {
    SCOPED_LOCK_MUTEX(files_mutex);
    files->remove(name);
    return true;
  }

  void RAMDirectory::renameFile(const char* from, const char* to) {
	SCOPED_LOCK_MUTEX(files_mutex);
	FileMap::iterator itr = files->find(from);

    /* DSR:CL_BUG_LEAK:
    ** If a file named $to already existed, its old value was leaked.
    ** My inclination would be to prevent this implicit deletion with an
    ** exception, but it happens routinely in CLucene's internals (e.g., during
    ** IndexWriter.addIndexes with the file named 'segments'). */
    if (files->exists(to)) {
      files->remove(to);
    }
	if ( itr == files->end() ){
		char tmp[1024];
		_snprintf(tmp,1024,"cannot rename %s, file does not exist",from);
		_CLTHROWT(CL_ERR_IO,tmp);
	}
	CND_PRECONDITION(itr != files->end(), "itr==files->end()")
	RAMFile* file = itr->second;
    files->removeitr(itr,false,true);
    files->put(STRDUP_AtoA(to), file);
  }

  
  void RAMDirectory::touchFile(const char* name) {
    RAMFile* file = NULL;
    {
      SCOPED_LOCK_MUTEX(files_mutex);
      file = files->get(name);
	}
    const uint64_t ts1 = file->getLastModified();
    uint64_t ts2 = Misc::currentTimeMillis();

	//make sure that the time has actually changed
    while ( ts1==ts2 ) {
        _LUCENE_SLEEP(1);
        ts2 = Misc::currentTimeMillis();
    };

    file->setLastModified(ts2);
  }

  IndexOutput* RAMDirectory::createOutput(const char* name) {
    /* Check the $files VoidMap to see if there was a previous file named
    ** $name.  If so, delete the old RAMFile object, but reuse the existing
    ** char buffer ($n) that holds the filename.  If not, duplicate the
    ** supplied filename buffer ($name) and pass ownership of that memory ($n)
    ** to $files. */

    SCOPED_LOCK_MUTEX(files_mutex);

    const char* n = files->getKey(name);
    if (n != NULL) {
	   RAMFile* rf = files->get(name);
      _CLDELETE(rf);
    } else {
      n = STRDUP_AtoA(name);
    }

    RAMFile* file = _CLNEW RAMFile();
    #ifdef _DEBUG
      file->filename = n;
    #endif
    (*files)[n] = file;

    return _CLNEW RAMIndexOutput(file);
  }

  std::string RAMDirectory::toString() const{
	  return "RAMDirectory";
  }

  const char* RAMDirectory::getClassName(){
	  return "RAMDirectory";
  }
  const char* RAMDirectory::getObjectName() const{
	  return getClassName();
  }

CL_NS_END
