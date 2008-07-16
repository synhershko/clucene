/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_MMap_
#define _lucene_store_MMap_


#if defined(LUCENE_FS_MMAP)
		class MMapIndexInput: public IndexInput{
			uint8_t* data;
			int64_t pos;
#ifdef _CL_HAVE_MAPVIEWOFFILE
			HANDLE mmaphandle;
			HANDLE fhandle;
#else
			int fhandle;
#endif
    		bool isClone;
			int64_t _length;

			MMapIndexInput(const MMapIndexInput& clone);
		public:
    		MMapIndexInput(const char* path);
			~MMapIndexInput();
			IndexInput* clone() const;

			inline uint8_t readByte();
			int32_t readVInt();
			void readBytes(uint8_t* b, const int32_t len);
			void close();
			int64_t getFilePointer() const;
			void seek(const int64_t pos);
			int64_t length(){ return _length; }
			
			static const char* DirectoryType(){ return "MMAP"; }
			const char* getDirectoryType() const{ return DirectoryType(); }
			
			const char* getObjectName(){ return MMapIndexInput::getClassName(); }
			static const char* getClassName(){ return "MMapIndexInput"; }			
		};
		friend class FSDirectory::MMapIndexInput;
#endif
