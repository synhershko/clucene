/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#if defined(LUCENE_FS_MMAP)

#include "FSDirectory.h"
#include "_MMap.h"
#include "CLucene/util/Misc.h"

#ifdef _CL_HAVE_SYS_MMAN_H
	#include <sys/mman.h>
#endif
#ifdef _CL_HAVE_WINERROR_H
	#include <winerror.h>
#endif

#if defined(_CL_HAVE_FUNCTION_MAPVIEWOFFILE)
	typedef int HANDLE;
	
	#define GENERIC_READ                     (0x80000000L)
    #define FILE_SHARE_READ                 0x00000001  
    #define OPEN_EXISTING       3
    #define PAGE_READONLY          0x02     
    #define SECTION_MAP_READ    0x0004
    #define FILE_MAP_READ       SECTION_MAP_READ

	typedef struct  _SECURITY_ATTRIBUTES
    {
        _cl_dword_t nLength;
        void* lpSecurityDescriptor;
        bool bInheritHandle;
    }	SECURITY_ATTRIBUTES;
	
	extern "C" __declspec(dllimport) _cl_dword_t __stdcall GetFileSize( HANDLE hFile, _cl_dword_t* lpFileSizeHigh );
    
	extern "C" __declspec(dllimport) bool __stdcall UnmapViewOfFile( void* lpBaseAddress );

    extern "C" __declspec(dllimport) bool __stdcall CloseHandle( HANDLE hObject );
    extern "C" __declspec(dllimport) HANDLE __stdcall CreateFileA(
		const char* lpFileName,
		_cl_dword_t dwDesiredAccess,
		_cl_dword_t dwShareMode,
		SECURITY_ATTRIBUTES* lpSecurityAttributes,
		_cl_dword_t dwCreationDisposition,
		_cl_dword_t dwFlagsAndAttributes,
		HANDLE hTemplateFile
    );
    extern "C" __declspec(dllimport) HANDLE __stdcall CreateFileMappingA(
        HANDLE hFile,
        SECURITY_ATTRIBUTES* lpFileMappingAttributes,
        _cl_dword_t flProtect,
        _cl_dword_t dwMaximumSizeHigh,
        _cl_dword_t dwMaximumSizeLow,
        const char* lpName
    );
    extern "C" __declspec(dllimport) void* __stdcall MapViewOfFile(
        HANDLE hFileMappingObject,
        _cl_dword_t dwDesiredAccess,
        _cl_dword_t dwFileOffsetHigh,
        _cl_dword_t dwFileOffsetLow,
        _cl_dword_t dwNumberOfBytesToMap
    );
    extern "C" __declspec(dllimport) _cl_dword_t __stdcall GetLastError();
#endif


CL_NS_DEF(store)
CL_NS_USE(util)

    class MMapIndexInput::Internal: LUCENE_BASE{
	public:
		uint8_t* data;
		int64_t pos;
#if defined(_CL_HAVE_FUNCTION_MAPVIEWOFFILE)
		HANDLE mmaphandle;
		HANDLE fhandle;
#elif defined(_CL_HAVE_FUNCTION_MMAP)
		int fhandle;
#else
        #error no mmap implementation set
#endif
		bool isClone;
		int64_t _length;
		
		Internal():
    		data(NULL),
    		pos(0),
    		isClone(false),
    		_length(0)
    	{
    	}
        ~Internal(){
        }
    };

	MMapIndexInput::MMapIndexInput(const char* path):
	    internal(_CLNEW Internal)
	{
	//Func - Constructor.
	//       Opens the file named path
	//Pre  - path != NULL
	//Post - if the file could not be opened  an exception is thrown.

	  CND_PRECONDITION(path != NULL, "path is NULL");

#if defined(_CL_HAVE_FUNCTION_MAPVIEWOFFILE)
	  internal->mmaphandle = NULL;
	  internal->fhandle = CreateFileA(path,GENERIC_READ,FILE_SHARE_READ, 0,OPEN_EXISTING,0,0);
	  
	  //Check if a valid fhandle was retrieved
	  if (internal->fhandle < 0){
		_cl_dword_t err = GetLastError();
        if ( err == ERROR_FILE_NOT_FOUND )
		    _CLTHROWA(CL_ERR_IO, "File does not exist");
        else if ( err == EACCES )
            _CLTHROWA(ERROR_ACCESS_DENIED, "File Access denied");
        else if ( err == ERROR_TOO_MANY_OPEN_FILES )
            _CLTHROWA(CL_ERR_IO, "Too many open files");
		else
			_CLTHROWA(CL_ERR_IO, "File IO Error");
	  }

	  _cl_dword_t dummy=0;
	  internal->_length = GetFileSize(internal->fhandle, &dummy);

	  if ( internal->_length > 0 ){
		internal->mmaphandle = CreateFileMappingA(internal->fhandle,NULL,PAGE_READONLY,0,0,NULL);
		if ( internal->mmaphandle != NULL ){
			void* address = MapViewOfFile(internal->mmaphandle,FILE_MAP_READ,0,0,0);
			if ( address != NULL ){
				internal->data = (uint8_t*)address;
				return; //SUCCESS!
			}
		}
		CloseHandle(internal->mmaphandle);

		char* lpMsgBuf=0;
		_cl_dword_t dw = GetLastError(); 

		/*FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			lpMsgBuf,
			0, NULL );

		char* errstr = _CL_NEWARRAY(char, strlen(lpMsgBuf)+40); 
		sprintf(errstr, "MMapIndexInput::MMapIndexInput failed with error %d: %s", dw, lpMsgBuf); 
		LocalFree(lpMsgBuf);
	    
		_CLTHROWA_DEL(CL_ERR_IO,errstr);*/
	  }

#else //_CL_HAVE_FUNCTION_MAPVIEWOFFILE
	 internal->fhandle = ::open (path, O_RDONLY);
  	 if (internal->fhandle < 0){
		_CLTHROWA(CL_ERR_IO,strerror(errno));	
  	 }else{
		// stat it
		struct stat sb;
		if (::fstat (internal->fhandle, &sb)){
			_CLTHROWA(CL_ERR_IO,strerror(errno));
		}else{
			// get length from stat
			internal->_length = sb.st_size;
			
			// mmap the file
			void* address = ::mmap(0, internal->_length, PROT_READ, MAP_SHARED, internal->fhandle, 0);
			if (address == MAP_FAILED){
				_CLTHROWA(CL_ERR_IO,strerror(errno));
			}else{
				internal->data = (uint8_t*)address;
			}
		}
  	 }
#endif
  }

  MMapIndexInput::MMapIndexInput(const MMapIndexInput& clone): IndexInput(clone){
  //Func - Constructor
  //       Uses clone for its initialization
  //Pre  - clone is a valide instance of FSIndexInput
  //Post - The instance has been created and initialized by clone
        internal = _CLNEW Internal;
        
#if defined(_CL_HAVE_FUNCTION_MAPVIEWOFFILE)
	  internal->mmaphandle = NULL;
	  internal->fhandle = NULL;
#endif

	  internal->data = clone.internal->data;
	  internal->pos = clone.internal->pos;

	  //clone the file length
	  internal->_length  = clone.internal->_length;
	  //Keep in mind that this instance is a clone
	  internal->isClone = true;
  }

  uint8_t MMapIndexInput::readByte(){
	  return *(internal->data+(internal->pos++));
  }

  void MMapIndexInput::readBytes(uint8_t* b, const int32_t len){
	memcpy(b, internal->data+internal->pos, len);
	internal->pos+=len;
  }
  int32_t MMapIndexInput::readVInt(){
	  uint8_t b = *(internal->data+(internal->pos++));
	  int32_t i = b & 0x7F;
	  for (int shift = 7; (b & 0x80) != 0; shift += 7) {
	    b = *(internal->data+(internal->pos++));
	    i |= (b & 0x7F) << shift;
	  }
	  return i;
  }
  int64_t MMapIndexInput::getFilePointer() const{
	return internal->pos;
  }
  void MMapIndexInput::seek(const int64_t pos){
	  this->internal->pos=pos;
  }
  int64_t MMapIndexInput::length() const{ return internal->_length; }

  MMapIndexInput::~MMapIndexInput(){
  //Func - Destructor
  //Pre  - True
  //Post - The file for which this instance is responsible has been closed.
  //       The instance has been destroyed

	  close();
  }

  IndexInput* MMapIndexInput::clone() const
  {
    return _CLNEW MMapIndexInput(*this);
  }
  void MMapIndexInput::close()  {
	if ( !internal->isClone ){
#if defined(_CL_HAVE_FUNCTION_MAPVIEWOFFILE)
		if ( internal->data != NULL ){
			if ( ! UnmapViewOfFile(internal->data) ){
				CND_PRECONDITION( false, "UnmapViewOfFile(data) failed"); //todo: change to rich error
			}
		}

		if ( internal->mmaphandle != NULL ){
			if ( ! CloseHandle(internal->mmaphandle) ){
				CND_PRECONDITION( false, "CloseHandle(mmaphandle) failed");
			}
		}
		if ( internal->fhandle != NULL ){
			if ( !CloseHandle(internal->fhandle) ){
				CND_PRECONDITION( false, "CloseHandle(fhandle) failed");
			}
		}
		internal->mmaphandle = NULL;
		internal->fhandle = NULL;
#else
		if ( internal->data != NULL )
	  		::munmap(internal->data, internal->_length);
	  	if ( internal->fhandle > 0 )
	  		::close(internal->fhandle);
	  	internal->fhandle = 0;
#endif
	}
	internal->data = NULL;
	internal->pos = 0;
  }


CL_NS_END
#endif
