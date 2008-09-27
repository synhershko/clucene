/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_Internal_IndexInput_
#define _lucene_store_Internal_IndexInput_

#include "CLucene/LuceneThreads.h"
#include "CLucene/util/_bufferedstream.h"

CL_NS_DEF(store)

	/**
	* JStream InputStream which reads from an IndexInput. This class is 
	* used by the FieldReader to create binary fields. You can then use 
	* a GZipInputStream to read compressed data or any of the other 
	* JStream stream types.
	* todo: should we really use Buffered? isn't the underlying stream buffered enough?
	*/
	class IndexInputStream: public jstreams::BufferedInputStream{
		IndexInput* input;
	public:
		IndexInputStream(IndexInput* input);
		~IndexInputStream();
	  int32_t fillBuffer(_stg_byte_t* start, int32_t space);
	};

CL_NS_END
#endif
