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

    class RAMDirectory;

	class RAMFile:LUCENE_BASE {
	private:
		CL_NS(util)::CLVector<uint8_t*,CL_NS(util)::Deletor::Array<uint8_t> > buffers;
		int64_t length;
		uint64_t lastModified;
		RAMDirectory* directory;
		int64_t sizeInBytes;

	public:
        DEFINE_MUTEX(THIS_LOCK)
				
        #ifdef _DEBUG
		const char* filename;
        #endif

		RAMFile();
		RAMFile( RAMDirectory* directory );
		~RAMFile();
		
		int64_t getLength();
		void setLength( int64_t length );
		
		uint64_t getLastModified();
		void setLastModified( uint64_t lastModified );
		
		uint8_t* addBuffer( int32_t size );
		uint8_t* getBuffer( int32_t index );
		int32_t numBuffers();
		uint8_t* newBuffer( int32_t size );
		
		int64_t getSizeInBytes();
	};

	class RAMIndexOutput: public IndexOutput {		
	protected:
		RAMFile* file;
		bool deleteFile;
		
		uint8_t* currentBuffer;
		int32_t currentBufferIndex;
		
		int32_t bufferPosition;
		int64_t bufferStart;
		int32_t bufferLength;
		
		void switchCurrentBuffer();
		void setFileLength();
				
	public:
		LUCENE_STATIC_CONSTANT(int32_t,BUFFER_SIZE=1024);
		
		RAMIndexOutput(RAMFile* f);
		RAMIndexOutput();
  	    /** Construct an empty output buffer. */
		virtual ~RAMIndexOutput();

		virtual void close();

		int64_t length();
        /** Resets this to an empty buffer. */
        void reset();
  	    /** Copy the current contents of this buffer to the named output. */
        void writeTo(IndexOutput* output);
        
    	void writeByte(const uint8_t b);
    	void writeBytes(const uint8_t* b, const int32_t len);

    	void seek(const int64_t pos);
    	
    	void flush();
    	
    	int64_t getFilePointer() const;
    	
		const char* getObjectName(){ return RAMIndexOutput::getClassName(); }
		static const char* getClassName(){ return "RAMIndexOutput"; }    	
    	
	};

	class RAMIndexInput:public IndexInput {				
	private:
		RAMFile* file;
		int64_t _length;
		
		uint8_t* currentBuffer;
		int32_t currentBufferIndex;
		
		int32_t bufferPosition;
		int64_t bufferStart;
		int32_t bufferLength;
		
		void switchCurrentBuffer();
		
	protected:
		/** IndexInput methods */
		RAMIndexInput(const RAMIndexInput& clone);
		
	public:
		LUCENE_STATIC_CONSTANT(int32_t,BUFFER_SIZE=RAMIndexOutput::BUFFER_SIZE);

		RAMIndexInput(RAMFile* f);
		~RAMIndexInput();
		IndexInput* clone() const;

		void close();
		int64_t length();
		const char* getDirectoryType() const;
		
		uint8_t readByte();
		void readBytes( uint8_t* dest, const int32_t len );
		
		int64_t getFilePointer() const;
		
		void seek(const int64_t pos);
		
		const char* getObjectName(){ return RAMIndexInput::getClassName(); }
		static const char* getClassName(){ return "RAMIndexInput"; }
		
	};


   /**
   * A memory-resident {@link Directory} implementation.
   */
	class RAMDirectory:public Directory{

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
		int64_t sizeInBytes;
		
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
