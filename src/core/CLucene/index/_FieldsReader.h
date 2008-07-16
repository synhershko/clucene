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
CL_CLASS_DEF_JSTREAM(subinputstream)

CL_NS_DEF(index)

   /**
   * Class responsible for access to stored document fields.
   *
   * It uses &lt;segment&gt;.fdt and &lt;segment&gt;.fdx; files.
   */
	class FieldsReader :LUCENE_BASE{
	private:
		const FieldInfos* fieldInfos;
		CL_NS(store)::IndexInput* fieldsStream;
		CL_NS(store)::IndexInput* indexStream;
		int32_t _size;
		class FieldsStreamHolder;
	public:
		FieldsReader(CL_NS(store)::Directory* d, const char* segment, FieldInfos* fn);
		~FieldsReader();
		void close();
		int32_t size() const;
		///loads the fields from n'th document into doc. returns true on success.
		bool doc(int32_t n, CL_NS(document)::Document* doc);
	};
CL_NS_END
#endif
