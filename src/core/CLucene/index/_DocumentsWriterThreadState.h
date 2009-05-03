/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_index_DocumentsWriterThreadState_
#define _lucene_index_DocumentsWriterThreadState_

#include "CLucene/util/Equators.h"
#include "CLucene/util/Array.h"

CL_CLASS_DEF(analysis,Analyzer)
CL_CLASS_DEF(document,Document)
CL_CLASS_DEF(document,Fieldable)
CL_CLASS_DEF(store,RAMOutputStream)

CL_NS_DEF(index)

class ByteBlockPool;
class CharBlockPool;
class ReusableStringReader;
class DocumentsWriter;
class FieldInfo;
class FieldsWriter;
class FieldData;
class Posting;
class Token;
class ByteSliceReader;
class PostingVector;

CL_NS_END
#endif
