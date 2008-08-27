/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_index_FieldsReader_
#define _lucene_index_FieldsReader_


#include "CLucene/util/VoidMapSetDefinitions.h"
CL_CLASS_DEF(store,Directory)
CL_CLASS_DEF(document,Document)
CL_CLASS_DEF(document,Field)
CL_CLASS_DEF(index, FieldInfos)
CL_CLASS_DEF(store,IndexInput)
//CL_CLASS_DEF_JSTREAM(subinputstream)

CL_NS_DEF(index)

   /**
   * Class responsible for access to stored document fields.
   *
   * It uses &lt;segment&gt;.fdt and &lt;segment&gt;.fdx; files.
   */
	class FieldsReader :LUCENE_BASE{
	private:
		const FieldInfos* fieldInfos;

		// The main fieldStream, used only for cloning.
		CL_NS(store)::IndexInput* cloneableFieldsStream;

		// This is a clone of cloneableFieldsStream used for reading documents.
		// It should not be cloned outside of a synchronized context. 
		CL_NS(store)::IndexInput* fieldsStream;

		CL_NS(store)::IndexInput* indexStream;
		int32_t numTotalDocs;
		int32_t _size;
		bool closed;

		// The docID offset where our docs begin in the index
		// file.  This will be 0 if we have our own private file.
		int32_t docStoreOffset;

		//ThreadLocal fieldsStreamTL = new ThreadLocal();

		class FieldsStreamHolder;
	public:
		FieldsReader(CL_NS(store)::Directory* d, const char* segment, FieldInfos* fn,
			int32_t readBufferSize = CL_NS(store)::BufferedIndexInput::BUFFER_SIZE, int32_t docStoreOffset = -1, int32_t size = 0);
		~FieldsReader();

		/**
		* @throws an exception (CL_ERR_IllegalState) if this FieldsReader is closed
		*/
		void FieldsReader::ensureOpen();

		/**
		* Closes the underlying {@link org.apache.lucene.store.IndexInput} streams, including any ones associated with a
		* lazy implementation of a Field.  This means that the Fields values will not be accessible.
		*
		* @throws IOException
		*/
		void close();

		int32_t size() const;
		
		/** Loads the fields from n'th document into doc. returns true on success. */
		bool doc(int32_t n, CL_NS(document)::Document* doc);
	};
CL_NS_END
#endif
