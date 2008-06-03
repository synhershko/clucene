/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_RAMDirectory_
#define _lucene_store_RAMDirectory_

#if defined(_LUCENE_PRAGMA_ONCE)
# pragma once
#endif

#include "Lock.h"
#include "Directory.h"
#include "CLucene/util/VoidMap.h"
#include "CLucene/util/Arrays.h"

CL_NS_DEF(store)

	// forward declaration
	class RAMDirectory;

	class RAMFile:LUCENE_BASE {
	public: // TODO: privatize all below
		LUCENE_STATIC_CONSTANT(int64_t, serialVersionUID=11);

		struct RAMFileBuffer:LUCENE_BASE {
			uint8_t* _buffer; size_t _len;
			RAMFileBuffer(uint8_t* buf = NULL, size_t len=0) : _buffer(buf), _len(len) {};
			~RAMFileBuffer() { _CLDELETE_LARRAY(_buffer); };
		};
	
		// TODO: Should move to ArrayList ?
		//CL_NS(util)::CLVector<uint8_t*,CL_NS(util)::Deletor::Array<uint8_t> > buffers;
		CL_NS(util)::CLVector<RAMFileBuffer*,CL_NS(util)::Deletor::Object<RAMFileBuffer> > buffers;
		int64_t length;
		RAMDirectory* directory;
		uint64_t sizeInBytes;                  // Only maintained if in a directory; updates synchronized on directory

		// This is publicly modifiable via Directory::touchFile(), so direct access not supported
		uint64_t lastModified;

        #ifdef _DEBUG
		const char* filename;
        #endif
	
	public:
		// File used as buffer, in no RAMDirectory
		RAMFile(RAMDirectory* _directory=NULL);
		~RAMFile();

		// For non-stream access from thread that might be concurrent with writing
		int64_t getLength() const { return length; }
		void setLength(const int64_t _length) { this->length = _length; }

		// For non-stream access from thread that might be concurrent with writing
		uint64_t getLastModified() const { return lastModified;}
		void setLastModified(const uint64_t lastModified) { this->lastModified = lastModified; }

		uint8_t* addBuffer(const int32_t size) {
			uint8_t* buffer = newBuffer(size);
			if (directory!=NULL) {
				// TODO: implement below
				/*
				synchronized (directory) {             // Ensure addition of buffer and adjustment to directory size are atomic wrt directory
					buffers.add(buffer);
					directory.sizeInBytes += size;
					*/
			}
			else {
				RAMFileBuffer* rfb = _CLNEW RAMFileBuffer(buffer, size);
				buffers.push_back(rfb);
			}
			return buffer;
		}

		uint8_t* getBuffer(const int32_t index) const { return buffers[index]->_buffer; }
		size_t getBufferLen(const int32_t index) const { return buffers[index]->_len; }

		size_t numBuffers() const { return buffers.size(); }

		/**
		* Expert: allocate a new buffer. 
		* Subclasses can allocate differently. 
		* @param size size of allocated buffer.
		* @return allocated buffer.
		*/
		uint8_t* newBuffer(const int32_t size) {
			return _CL_NEWARRAY(uint8_t, size);
		}

		// Only valid if in a directory
		uint64_t getSizeInBytes() {
			/*
			synchronized (directory) {
				return sizeInBytes;
			}*/
			return sizeInBytes;
		}

	};

	/**
	* A memory-resident {@link IndexOutput} implementation.
	*
	*/
	class RAMIndexOutput: public /*Buffered*/IndexOutput {
	public:
		LUCENE_STATIC_CONSTANT(int32_t, BUFFER_SIZE=LUCENE_STREAM_BUFFER_SIZE);
	private:
		RAMFile* file;

		uint8_t* currentBuffer;
		int32_t currentBufferIndex;

		int32_t bufferPosition;
		int64_t bufferStart;
		int32_t bufferLength;

		bool deleteFile;
		
		// output methods: 
		//void flushBuffer(const uint8_t* src, const int32_t len);
	public:
		RAMIndexOutput(RAMFile* f);
		RAMIndexOutput();
  	    /** Construct an empty output buffer. */
		virtual ~RAMIndexOutput();

		virtual void close();

		// Random-at methods
		virtual void seek(const int64_t pos);
		int64_t length();
        /** Resets this to an empty buffer. */
        void reset();
  	    /** Copy the current contents of this buffer to the named output. */
        void writeTo(IndexOutput* output);

		inline void writeByte(const uint8_t b)
		{
			if (bufferPosition == bufferLength) {
				currentBufferIndex++;
				switchCurrentBuffer();
			}
			currentBuffer[bufferPosition++] = b;
		}

		void writeBytes(const uint8_t* b, int32_t offset, int32_t len)
		{
			while (len > 0) {
				if (bufferPosition == bufferLength) {
					currentBufferIndex++;
					switchCurrentBuffer();
				}

				const int32_t remainInBuffer = bufferLength - bufferPosition;
				const int32_t bytesToCopy = len < remainInBuffer ? len : remainInBuffer;
				memcpy((void*)(currentBuffer + bufferPosition), (void*)(b + offset), bytesToCopy * sizeof(uint8_t)); // sizeof wasn't here
				offset += bytesToCopy;
				len -= bytesToCopy;
				bufferPosition += bytesToCopy;
			}
		}

	private:
		void switchCurrentBuffer()
		{
			if (currentBufferIndex == file->numBuffers()) {
				currentBuffer = file->addBuffer(BUFFER_SIZE);
				bufferLength = BUFFER_SIZE;
			} else {
				currentBuffer = file->getBuffer(currentBufferIndex);
				bufferLength = file->getBufferLen(currentBufferIndex);
			}
			bufferPosition = 0;
			bufferStart = (int64_t) BUFFER_SIZE * (int64_t) currentBufferIndex;
		}

		inline void setFileLength() {
			const int64_t pointer = bufferStart + bufferPosition;
			if (pointer > file->length) {
				file->setLength(pointer);
			}
		}

	public:
		void flush()
		{
			file->setLastModified(CL_NS(util)::Misc::currentTimeMillis());
			setFileLength();
		}

		inline int64_t getFilePointer() const {
			return (currentBufferIndex < 0) ? 0 : bufferStart + bufferPosition;
		}

		virtual const char* getObjectName() { return "RAMIndexOutput"; };
	};

	/**
	* A memory-resident {@link IndexInput} implementation.
	*
	*/
	class RAMIndexInput:public IndexInput/*BufferedIndexInput*/ {
		LUCENE_STATIC_CONSTANT(int32_t, BUFFER_SIZE=RAMIndexOutput::BUFFER_SIZE);
	private:
		RAMFile* file;
		int64_t _length;

		uint8_t* currentBuffer;
		uint32_t currentBufferIndex;
  
		int32_t bufferPosition;
		int64_t bufferStart;
		int32_t bufferLength;

	protected:
		/** IndexInput methods */
		RAMIndexInput(const RAMIndexInput& clone);
		//void readInternal(uint8_t *dest, const int32_t offset, const int32_t len);
		
        /** Random-at methods */
		//void seekInternal(const int64_t pos);
	public:
		RAMIndexInput(RAMFile* f);
		~RAMIndexInput();
		IndexInput* clone() const;

		inline uint8_t readByte();

		void readBytes(uint8_t* b, int32_t offset, int32_t len);

		void close();
		int64_t length() const;
		const char* getDirectoryType() const;

		inline int64_t getFilePointer() const {
			return (currentBufferIndex < 0) ? 0 : (bufferStart + bufferPosition);
		}

		void seek(const int64_t pos);

		virtual const char* getObjectName() { return "RAMIndexInput"; };
	private:
		void switchCurrentBuffer();
	};


	/**
	* A memory-resident {@link Directory} implementation.  Locking
	* implementation is by default the {@link SingleInstanceLockFactory}
	* but can be changed with {@link #setLockFactory}.
	*
	*/
	class RAMDirectory:public Directory{
		//private static final long serialVersionUID = 1l;

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

		typedef CL_NS(util)::CLHashMap<const char*,RAMFile*, 
				CL_NS(util)::Compare::Char, CL_NS(util)::Equals::Char,
				CL_NS(util)::Deletor::acArray , CL_NS(util)::Deletor::Object<RAMFile> > FileMap;
	protected:
		/// Removes an existing file in the directory. 
		virtual bool doDeleteFile(const char* name);
		
		/**
		* Creates a new <code>RAMDirectory</code> instance from a different
		* <code>Directory</code> implementation.  This can be used to load
		* a disk-based index into memory.
		* <P>
		* This should be used only with indices that can fit into memory.
		*
		* @param dir a <code>Directory</code> value
		* @exception IOException if an error occurs
		*/
		void _copyFromDir(Directory* dir, bool closeDir);
		FileMap files; // unlike the java Hashtable, FileMap is not synchronized, and all access must be protected by a lock
	public:
#ifndef _CL_DISABLE_MULTITHREADING //do this so that the mutable keyword still works without mt enabled
	    mutable DEFINE_MUTEX(files_mutex) // mutable: const methods must also be able to synchronize properly
#endif

		/// Returns a null terminated array of strings, one for each file in the directory. 
		void list(vector<string>* names) const;

      /** Constructs an empty {@link Directory}. */
		RAMDirectory();
		
	  ///Destructor - only call this if you are sure the directory
	  ///is not being used anymore. Otherwise use the ref-counting
	  ///facilities of dir->close
		virtual ~RAMDirectory();
		RAMDirectory(Directory* dir);
		
	  /**
	   * Creates a new <code>RAMDirectory</code> instance from the {@link FSDirectory}.
	   *
	   * @param dir a <code>String</code> specifying the full index directory path
	   */
		RAMDirectory(const char* dir);
		       
		/// Returns true iff the named file exists in this directory. 
		bool fileExists(const char* name) const;

		/// Returns the time the named file was last modified. 
		int64_t fileModified(const char* name) const;

		/// Returns the length in bytes of a file in the directory. 
		int64_t fileLength(const char* name) const;

		/// Removes an existing file in the directory. 
		virtual void renameFile(const char* from, const char* to);

		/** Set the modified time of an existing file to now. */
		void touchFile(const char* name);

		/// Creates a new, empty file in the directory with the given name.
		///	Returns a stream writing this file. 
		virtual IndexOutput* createOutput(const char* name);

		/// Construct a {@link Lock}.
		/// @param name the name of the lock file
		LuceneLock* makeLock(const char* name);

		/// Returns a stream reading an existing file. 
		IndexInput* openInput(const char* name);

		virtual void close();
		
		TCHAR* toString() const;

		static const char* DirectoryType() { return "RAM"; }
	    const char* getDirectoryType() const{ return DirectoryType(); }
	};
CL_NS_END
#endif
