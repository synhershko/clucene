/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_Directory
#define _lucene_store_Directory

#if defined(_LUCENE_PRAGMA_ONCE)
# pragma once
#endif

#include "CLucene/store/Lock.h"
#include "CLucene/util/VoidList.h"
#include "CLucene/util/Misc.h"

#include "IndexInput.h"
#include "IndexOutput.h"

CL_NS_DEF(store)

   /** A Directory is a flat list of files.  Files may be written once, when they
   * are created.  Once a file is created it may only be opened for read, or
   * deleted.  Random access is permitted both when reading and writing.
   *
   * <p> Direct i/o is not used directly, but rather all i/o is
   * through this API.  This permits things such as: <ul>
   * <li> implementation of RAM-based indices;
   * <li> implementation indices stored in a database, via a database;
   * <li> implementation of an index as a single file;
   * </ul>
   *
   */
	class Directory: LUCENE_REFBASE {
	protected:
		Directory(){
		}
		// Removes an existing file in the directory. 
		virtual bool doDeleteFile(const char* name) = 0;
	public:
		DEFINE_MUTEX(THIS_LOCK)
	   
		virtual ~Directory(){ };

		// Returns an null terminated array of strings, one for each file in the directory. 
		char** list() const{
    		vector<string> names;
    		
    		list(&names);
    
			size_t size = names.size();
		    char** ret = _CL_NEWARRAY(char*,size+1);
		    for ( size_t i=0;i<size;i++ )
		      ret[i] = STRDUP_AtoA(names[i].c_str());
		    ret[size]=NULL;
		    return ret;	
		}
		virtual void list(vector<string>* names) const = 0;
		       
		// Returns true iff a file with the given name exists. 
		virtual bool fileExists(const char* name) const = 0;

		// Returns the time the named file was last modified. 
		virtual int64_t fileModified(const char* name) const = 0;

		// Returns the length of a file in the directory. 
		virtual int64_t fileLength(const char* name) const = 0;

		// Returns a stream reading an existing file. 
		virtual IndexInput* openInput(const char* name) = 0;
		virtual IndexInput* openInput(const char* name, int32_t bufferSize){ 
			return openInput(name); //implementation didnt overload the bufferSize
		}

		/// Set the modified time of an existing file to now. */
		virtual void touchFile(const char* name) = 0;

		// Removes an existing file in the directory. 
		virtual bool deleteFile(const char* name, const bool throwError=true){
			bool ret = doDeleteFile(name);
			if ( !ret && throwError ){
		      char buffer[200];
		      _snprintf(buffer,200,"couldn't delete %s",name);
		      _CLTHROWA(CL_ERR_IO, buffer );
		    }
		    return ret;
		}

		// Renames an existing file in the directory.
		//	If a file already exists with the new name, then it is replaced.
		//	This replacement should be atomic. 
		virtual void renameFile(const char* from, const char* to) = 0;

		// Creates a new, empty file in the directory with the given name.
		//	Returns a stream writing this file. 
		virtual IndexOutput* createOutput(const char* name) = 0;

		// Construct a {@link Lock}.
		// @param name the name of the lock file
		virtual LuceneLock* makeLock(const char* name) = 0;

		// Closes the store. 
		virtual void close() = 0;
		
		virtual TCHAR* toString() const = 0;

		virtual const char* getDirectoryType() const = 0;

		/**
		* Copy contents of a directory src to a directory dest.
		* If a file in src already exists in dest then the
		* one in dest will be blindly overwritten.
		*
		* @param src source directory
		* @param dest destination directory
		* @param closeDirSrc if <code>true</code>, call {@link #close()} method on source directory
		* @throws IOException
		*/
		static void copy(Directory* src, Directory* dest, const bool closeDirSrc) {
			vector<string> files;

			if (src)
				src->list(&files);

			if ((!src) || files.size() <= 0)
				_CLTHROWA(CL_ERR_IO, "cannot read source directory: list() returned null");

			uint8_t buf[CL_NS(store)::BufferedIndexOutput::BUFFER_SIZE];
			for (size_t i = 0; i < files.size(); i++) {
				IndexOutput* os = NULL;
				IndexInput* is = NULL;
				try {
					// TODO: Use code below?
					//if ( !CL_NS(index)::IndexReader::isLuceneFile(files[i].c_str()))
					//	continue;

					// create file in dest directory
					os = dest->createOutput(files[i].c_str());
					// read current file
					is = src->openInput(files[i].c_str());
					// and copy to dest directory
					//todo: this could be a problem when copying from big indexes... 
					int64_t len = is->length();
					int64_t readCount = 0;
					while (readCount < len) {
						int32_t toRead = readCount + CL_NS(store)::BufferedIndexOutput::BUFFER_SIZE > len ? (int32_t)(len - readCount) : CL_NS(store)::BufferedIndexOutput::BUFFER_SIZE;
						is->readBytes(buf, 0, toRead);
						os->writeBytes(buf, toRead);
						readCount += toRead;
					}
				} _CLFINALLY(
					// graceful cleanup
					try {
						if (os) {
							os->close();
							_CLDELETE(os);
						}
				} _CLFINALLY(
					if (is) {
						is->close();
						_CLDELETE(is);
					}
					);
					);
			}
			if(closeDirSrc)
				src->close();
		}
	};
CL_NS_END
#endif


