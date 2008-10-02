/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "CLStreams.h"

#include <fcntl.h>
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
#include <errno.h>

#include "_bufferedstream.h"

CL_NS_DEF(util)

StringReader::StringReader ( TCHAR* value, const int32_t length, bool copyData )
{
	this->m_size = length;
	this->pos = 0;
	if ( copyData ){
		this->value = _CL_NEWARRAY(TCHAR, this->m_size);
		_tcsncpy(this->value, value, this->m_size);
	}else{
		this->value = value;
	}
	this->ownValue = copyData;
}

StringReader::StringReader ( const TCHAR* value, const int32_t length ){
	if ( length >= 0 )
		this->m_size = length;
	else
		this->m_size = _tcslen(value);
	this->pos = 0;
	this->value = _CL_NEWARRAY(TCHAR, this->m_size);
	_tcsncpy(this->value, value, this->m_size);
	this->ownValue = true;
}
StringReader::~StringReader(){
	if ( ownValue )
		_CLDELETE_ARRAY(this->value);
}

size_t StringReader::size(){
	return m_size;
}
int32_t StringReader::read(const TCHAR*& start, int32_t min, int32_t max){
	if ( m_size == pos )
		return -1;
	start = this->value + pos;
	int32_t r = (int32_t)cl_min(cl_max(min,max),m_size-pos);
	pos += r;
	return r;
}
int64_t StringReader::position(){
	return pos;
}
void StringReader::setMinBufSize(int32_t s){
}
int64_t StringReader::reset(int64_t pos){
	if ( pos >= 0 && pos < this->m_size )
		this->pos = pos;
	return this->pos;
}
int64_t StringReader::skip(int64_t ntoskip){
	int64_t s = cl_min(ntoskip, m_size-pos);
	this->pos += s;
	return s;
}

class FileInputStream::Internal{
public:
	class JStreamsBuffer: public jstreams::BufferedInputStream{
		int32_t fhandle;
	protected:
		int32_t fillBuffer(signed char* start, int32_t space){
			if (fhandle == 0) return -1;
	    // read into the buffer
	    int32_t nwritten = _read(fhandle, start, space);

	    // check the file stream status
	    if (nwritten == -1 ) {
	        m_error = "Could not read from file";
			m_status = jstreams::Error;
			if ( fhandle > 0 ){
				::_close(fhandle);
				fhandle = 0;
			}
	        return -1;
	    }else if ( nwritten == 0 ) {
	        ::_close(fhandle);
	        fhandle = 0;
	    }
	    return nwritten;
		}
	public:
		int encoding;

		JStreamsBuffer(int32_t fhandle, int32_t buffersize){
			this->fhandle = fhandle;
			
			m_size = fileSize(fhandle); // no need to know the file length...
			
			// allocate memory in the buffer
			int32_t bufsize = (int32_t)((m_size <= buffersize) ?m_size+1 :buffersize);
			setMinBufSize(bufsize);
		}
		void _setMinBufSize(int32_t bufsize){
			this->setMinBufSize(bufsize);
		}

		~JStreamsBuffer(){
			if ( fhandle > 0 ){
				if ( ::_close(fhandle) != 0 )
					_CLTHROWA(CL_ERR_IO, "File IO Close error");
			}
		}
	};

	JStreamsBuffer* jsbuffer;

	Internal(const char* path, int32_t buffersize){
		int32_t fhandle = _cl_open(path, _O_BINARY | O_RDONLY | _O_RANDOM, _S_IREAD );
		
		//Check if a valid handle was retrieved
	   if (fhandle < 0){
			int err = errno;
			if ( err == ENOENT )
		    		_CLTHROWA(CL_ERR_IO, "File does not exist");
			else if ( err == EACCES )
				_CLTHROWA(CL_ERR_IO, "File Access denied");
			else if ( err == EMFILE )
				_CLTHROWA(CL_ERR_IO, "Too many open files");
			else
	    			_CLTHROWA(CL_ERR_IO, "Could not open file");
	   }
		jsbuffer = new JStreamsBuffer(fhandle, buffersize);
		
	}
	~Internal(){
		delete jsbuffer;
	}
};


FileInputStream::FileInputStream ( const char* path, int32_t buflen  )
{
	if ( buflen == -1 )
		buflen = DEFAULT_BUFFER_SIZE;
	internal = new Internal(path, buflen);
}

size_t FileInputStream::size(){
	return internal->jsbuffer->size();
}

FileInputStream::~FileInputStream ()
{
	delete internal;
}

int32_t FileInputStream::read(const signed char*& start, int32_t min, int32_t max){
	return internal->jsbuffer->read(start,min,max);
}
int64_t FileInputStream::position(){
	return internal->jsbuffer->position();
}
int64_t FileInputStream::reset(int64_t to){
	return internal->jsbuffer->reset(to);
}
int64_t FileInputStream::skip(int64_t ntoskip){
	return internal->jsbuffer->skip(ntoskip);
}
void FileInputStream::setMinBufSize(int32_t minbufsize){
	internal->jsbuffer->_setMinBufSize(minbufsize);
}


FileReader::FileReader(const char *path, const char *enc, int32_t buflen)
{
	int encoding;
	if ( strcmp(enc,"ASCII")==0 )
		encoding = ASCII;
#ifdef _UCS2
	else if ( strcmp(enc,"UTF-8")==0 )
		encoding = UTF8;
	else if ( strcmp(enc,"UCS-2LE")==0 )
		encoding = UCS2_LE;
#endif
	else
		_CLTHROWA(CL_ERR_IllegalArgument,"Unsupported encoding, use jstreams iconv based instead");
	init( _CLNEW FileInputStream(path, buflen), encoding);
}
FileReader::FileReader(const char *path, int encoding, int32_t buflen)
{
	init(_CLNEW FileInputStream(path, buflen), encoding);
}
FileReader::~FileReader(){
}

class SimpleInputStreamReader::Internal{
public:
	
	class JStreamsBuffer: public jstreams::BufferedReader{
		InputStream* input;
		char utf8buf[6]; //< buffer used for converting utf8 characters
	protected:
		int readChar(){
			const signed char* buf;
			if ( encoding == ASCII ){
				int32_t ret = this->input->read(buf, 1, 1) ;
				if ( ret == 1 ){
					return buf[0];
				}else
					return -1;

			}else if ( encoding == UCS2_LE ){
				int32_t ret = this->input->read(buf, 2, 2);
				if ( ret < 0 )
					return -1;
				else if ( ret == 1 ){
					return buf[0];
				}else{
					uint8_t c1 = *buf;
					uint8_t c2 = *(buf+1);
					return c1 | (c2<<8);
				}
			}else if ( encoding == UTF8 ){
				int32_t ret = this->input->read(buf, 1, 1);

				if ( ret == 1 ){
					size_t len = lucene_utf8charlen(buf[0]);
					if ( len > 1 ){
						*utf8buf = buf[0];
						ret = this->input->read(buf, len-1, len-1);
					}else
						return buf[0];

					if ( ret >= 0 ){
						if ( ret == len-1 ){
							memcpy(utf8buf+1,buf,ret);
							wchar_t wcbuf=0;
							lucene_utf8towc(wcbuf, utf8buf);
							return wcbuf;
						}
					}
				}else if ( ret == -1 )
					return -1;
				this->m_error = "Invalid multibyte sequence.";
				this->m_status = jstreams::Error;
			}else{
				this->m_error = "Unexpected encoding";
				this->m_status = jstreams::Error;
			}
			return -1;
		}
		int32_t fillBuffer(TCHAR* start, int32_t space){
			if ( input == NULL ) return -1;

			int c;
			int32_t i;
			for(i=0;i<space;i++){
				c = readChar();
				if ( c == -1 ){
					if ( this->m_status == jstreams::Ok ){
						if ( i == 0 )
							return -1;
						break;
					}
					return -1;
				}
				start[i] = c;
			}
			return i;
		}
	public:
		int encoding;

		JStreamsBuffer(InputStream* input, int encoding){
			this->input = input;
			this->encoding = encoding;
		}
		~JStreamsBuffer(){
			_CLDELETE(input);
		}
		void _setMinBufSize(int32_t min){
			this->setMinBufSize(min);
		}
	};

	JStreamsBuffer* jsbuffer;

	Internal(InputStream* input, int encoding){
		jsbuffer = new JStreamsBuffer(input, encoding);
	}
	~Internal(){
		delete jsbuffer;
	}
};

SimpleInputStreamReader::SimpleInputStreamReader(){
	internal = NULL;
}
void SimpleInputStreamReader::init(InputStream *i, int encoding){
	internal = new Internal(i, encoding);
}
SimpleInputStreamReader::~SimpleInputStreamReader(){
	delete internal;
}

int32_t SimpleInputStreamReader::read(const TCHAR*& start, int32_t min, int32_t max){
	return internal->jsbuffer->read(start, min, max);
}
int64_t SimpleInputStreamReader::position(){
	return internal->jsbuffer->position();
}
int64_t SimpleInputStreamReader::reset(int64_t to){
	return internal->jsbuffer->reset(to);
}
int64_t SimpleInputStreamReader::skip(int64_t ntoskip){
	return internal->jsbuffer->skip(ntoskip);
}
size_t SimpleInputStreamReader::size(){
	return internal->jsbuffer->size();
}
void SimpleInputStreamReader::setMinBufSize(int32_t minbufsize){
	internal->jsbuffer->_setMinBufSize(minbufsize);
}

CL_NS_END
