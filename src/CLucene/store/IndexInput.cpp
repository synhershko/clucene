 /*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/StdHeader.h"
#include "IndexInput.h"
//#include "IndexOutput.h"

CL_NS_USE(util)
CL_NS_DEF(store)

	IndexInput::IndexInput()
	{
	}
	IndexInput::IndexInput(const IndexInput& other)
	{
	}

  int32_t IndexInput::readInt() {
    int32_t b = (readByte() << 24);
    b |= (readByte() << 16);
    b |= (readByte() <<  8);
    return (b | readByte());
  }

  int32_t IndexInput::readVInt() {
    uint8_t b = readByte();
    int32_t i = b & 0x7F;
    for (int32_t shift = 7; (b & 0x80) != 0; shift += 7) {
      b = readByte();
      i |= (b & 0x7F) << shift;
    }
    return i;
  }

  int64_t IndexInput::readLong() {
    int64_t i = ((int64_t)readInt() << 32);
    return (i | ((int64_t)readInt() & 0xFFFFFFFFL));
  }

  int64_t IndexInput::readVLong() {
    uint8_t b = readByte();
    int64_t i = b & 0x7F;
    for (int32_t shift = 7; (b & 0x80) != 0; shift += 7) {
      b = readByte();
      i |= (((int64_t)b) & 0x7FL) << shift;
    }
    return i;
  }

  void IndexInput::skipChars(int32_t length)
  {
	  for (int32_t i = 0; i < length; i++) {
		  uint8_t b = readByte();
		  if ((b & 0x80) == 0){
			  //do nothing, we only need one byte
		  }
		  else if ((b & 0xE0) != 0xE0) {
			  readByte();//read an additional byte
		  } else{      
			  //read two additional bytes.
			  readByte();
			  readByte();
		  }
	  }
  }

  
/*  void IndexInput::skipChars( const int32_t count) {
	for (int32_t i = 0; i < count; i++) {
		TCHAR b = readByte();
		if ((b & 0x80) == 0) {
			// Do Nothing.
		} else if ((b & 0xE0) != 0xE0) {
			readByte();
		} else {
			readByte();
			readByte();
		}
	}
}*/

  int32_t IndexInput::readString(TCHAR* buffer, const int32_t maxLength){
    int32_t len = readVInt();
	int32_t ml=maxLength-1;
    if ( len >= ml ){
      readChars(buffer, 0, ml);
      buffer[ml] = 0;
      //we have to finish reading all the data for this string!
      if ( len-ml > 0 ){
		//seek(getFilePointer()+(len-ml)); <- that was the wrong way to "finish reading"
		skipChars(len-ml);
	  }
      return ml;
    }else{
      readChars(buffer, 0, len);
      buffer[len] = 0;
      return len;
    }
  }

   TCHAR* IndexInput::readString(const bool _unique){
    int32_t len = readVInt();
      
    if ( len == 0){
      if ( _unique ) //todo: does non unique ever occur?
         return stringDuplicate(LUCENE_BLANK_STRING);
      else
         return LUCENE_BLANK_STRING;
    }

    TCHAR* ret = _CL_NEWARRAY(TCHAR,len+1);
    readChars(ret, 0, len);
    ret[len] = 0;

    return ret;
  }

  void IndexInput::readChars( TCHAR* buffer, const int32_t start, const int32_t len) {
    const int32_t end = start + len;
    TCHAR b;
    for (int32_t i = start; i < end; ++i) {
      b = readByte();
      if ((b & 0x80) == 0) {
        b = (b & 0x7F);
      } else if ((b & 0xE0) != 0xE0) {
        b = (((b & 0x1F) << 6)
          | (readByte() & 0x3F));
      } else {
		  b = ((b & 0x0F) << 12) | ((readByte() & 0x3F) << 6);
		  b |= (readByte() & 0x3F);
      }
      buffer[i] = b;
	}
  }


BufferedIndexInput::BufferedIndexInput(int32_t _bufferSize):
		buffer(NULL),
		bufferStart(0),
		bufferLength(0),
		bufferPosition(0),
		bufferSize(_bufferSize)
  {
	      checkBufferSize(_bufferSize);
  }

  BufferedIndexInput::BufferedIndexInput(const BufferedIndexInput& other):
  	IndexInput(other),
    buffer(NULL),
	bufferSize(BUFFER_SIZE),
    bufferStart(other.getFilePointer()),
    bufferLength(0),
    bufferPosition(0)
  {
    /* DSR: Does the fact that sometime clone.buffer is not NULL even when
    ** clone.bufferLength is zero indicate memory corruption/leakage?
    **   if ( clone.buffer != NULL) { */
    if (other.bufferLength != 0 && other.buffer != NULL) {
      buffer = _CL_NEWARRAY(uint8_t,bufferSize); // was bufferLength
	  memcpy(buffer,other.buffer,bufferSize * sizeof(uint8_t));
    }
  }

	void BufferedIndexInput::readBytes(uint8_t* b, int32_t offset, int32_t len, const bool useBuffer) {
		if (len <= (bufferLength-bufferPosition)) {
			// the buffer contains enough data to satisfy this request
			if(len>0) // to allow b to be null if len is 0...
				memcpy((void*)(b + offset), (void*)(buffer + bufferPosition), len * sizeof(uint8_t));
			bufferPosition+=len;
		} else {
			// the buffer does not have enough data. First serve all we've got.
			int32_t available = bufferLength - bufferPosition;
			if(available > 0){
				memcpy((void*)(b + offset), (void*)(buffer + bufferPosition), available * sizeof(uint8_t));
				offset += available;
				len -= available;
				bufferPosition += available;
			}
			// and now, read the remaining 'len' bytes:
			if (useBuffer && len<bufferSize){
				// If the amount left to read is small enough, and
				// we are allowed to use our buffer, do it in the usual
				// buffered way: fill the buffer and copy from it:
				refill();
				if(bufferLength<len){
					// Throw an exception when refill() could not read len bytes:
					memcpy((void*)(b + offset), buffer, bufferLength * sizeof(uint8_t));
					_CLTHROWA(CL_ERR_IO, "BufferedIndexInput::readBytes read past EOF");
				} else {
					memcpy((void*)(b + offset), buffer, len * sizeof(uint8_t));
					bufferPosition=len;
				}
			} else {
				// The amount left to read is larger than the buffer
				// or we've been asked to not use our buffer -
				// there's no performance reason not to read it all
				// at once. Note that unlike the previous code of
				// this function, there is no need to do a seek
				// here, because there's no need to reread what we
				// had in the buffer.
				int64_t after = bufferStart+bufferPosition+len;
				if(after > length())
					_CLTHROWA(CL_ERR_IO, "BufferedIndexInput::readBytes read past EOF");
				readInternal(b, offset, len);
				bufferStart = after;
				bufferPosition = 0;
				bufferLength = 0;                    // trigger refill() on read
			}
		}
	}

	int64_t BufferedIndexInput::getFilePointer() const { return bufferStart + bufferPosition; }

	void BufferedIndexInput::seek(const int64_t pos) {
		if ( pos < 0 )
			_CLTHROWA(CL_ERR_IO, "IO Argument Error. Value must be a positive value.");
		if (pos >= bufferStart && pos < (bufferStart + bufferLength))
			bufferPosition = (int32_t)(pos - bufferStart);  // seek within buffer
		else {
			bufferStart = pos;
			bufferPosition = 0;
			bufferLength = 0;				  // trigger refill() on read()
			seekInternal(pos);
		}
	}

	void BufferedIndexInput::close(){
		_CLDELETE_ARRAY(buffer);
		bufferLength = 0;
		bufferPosition = 0;
		bufferStart = 0;
	}


	BufferedIndexInput::~BufferedIndexInput(){
		BufferedIndexInput::close();
	}

	void BufferedIndexInput::refill() {
		int64_t start = bufferStart + bufferPosition;
		int64_t end = start + bufferSize;
		if (end > length())				  // don't read past EOF
			end = length();
		int32_t newLength = (int32_t)(end - start);
		if (newLength <= 0)
			_CLTHROWA(CL_ERR_IO, "BufferedIndexInput::refill tried to read past EOF");

		if (buffer == NULL){
			buffer = _CL_NEWARRAY(uint8_t,bufferSize);		  // allocate buffer lazily
			seekInternal(bufferStart);
		}
		readInternal(buffer, 0, newLength);

		bufferLength = newLength;
		bufferStart = start;
		bufferPosition = 0;
	}

	void BufferedIndexInput::setBufferSize( const int32_t newSize )
	{
		CND_PRECONDITION (buffer == NULL /*|| bufferSize == buffer.length*/, "Buffer was not initialized properly");
		if (newSize != bufferSize) {
			checkBufferSize(newSize);
			bufferSize = newSize;
			if (buffer != NULL) {
				// Resize the existing buffer and carefully save as
				// many bytes as possible starting from the current
				// bufferPosition
				uint8_t* newBuffer = _CL_NEWARRAY(uint8_t, newSize);
				const int32_t leftInBuffer = bufferLength-bufferPosition;
				int32_t numToCopy;
				if (leftInBuffer > newSize)
					numToCopy = newSize;
				else
					numToCopy = leftInBuffer;

				memcpy((void*)newBuffer, (void*)(buffer + bufferPosition), numToCopy *  sizeof(uint8_t));

				bufferStart += bufferPosition;
				bufferPosition = 0;
				bufferLength = numToCopy;

				_CLDELETE_LARRAY(buffer);
				buffer = newBuffer;
			}
		}
	}

IndexInputStream::IndexInputStream(IndexInput* input){
	this->input = input;
	this->size = input->length();
    this->position = input->getFilePointer();
}
IndexInputStream::~IndexInputStream(){
}
int32_t IndexInputStream::fillBuffer(char* start, int32_t space){
    int64_t avail = input->length()-input->getFilePointer();
    if ( avail == 0 )
		return -1;
	else if ( avail<space )
        space = (int32_t)avail;
	
	input->readBytes((uint8_t*)start,0,space); // TODO: make sure offset should be 0 by default
    return space;
}

CL_NS_END

