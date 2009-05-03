/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "IndexWriter.h"
#include "CLucene/document/Document.h"
#include "CLucene/store/Directory.h"
#include "CLucene/search/Similarity.h"
#include "CLucene/util/Misc.h"

#include "CLucene/store/_Lock.h"
#include "CLucene/store/_RAMDirectory.h"
#include "CLucene/store/_TransactionalRAMDirectory.h"
#include "CLucene/store/FSDirectory.h"
#include "CLucene/util/Array.h"
#include "CLucene/util/PriorityQueue.h"
#include "_DocumentWriter.h"
#include "_TermInfo.h"
#include "_SegmentInfos.h"
#include "_SegmentMerger.h"
#include "_SegmentHeader.h"
//#include "MergePolicy.h"


CL_NS_USE(store)
CL_NS_USE(util)
CL_NS_USE(document)
CL_NS_USE(analysis)
CL_NS_DEF(index)

  int64_t IndexWriter::WRITE_LOCK_TIMEOUT = 1000;
  const char* IndexWriter::WRITE_LOCK_NAME = "write.lock";

  const int32_t IndexWriter::MERGE_READ_BUFFER_SIZE = 4096;

  const int32_t IndexWriter::DISABLE_AUTO_FLUSH = -1;
  const int32_t IndexWriter::DEFAULT_MAX_BUFFERED_DOCS = DISABLE_AUTO_FLUSH;
  const float_t IndexWriter::DEFAULT_RAM_BUFFER_SIZE_MB = 16.0;
  const int32_t IndexWriter::DEFAULT_MAX_BUFFERED_DELETE_TERMS = DISABLE_AUTO_FLUSH;
  //const int32_t IndexWriter::DEFAULT_MAX_MERGE_DOCS = LogDocMergePolicy::DEFAULT_MAX_MERGE_DOCS;
  //const int32_t IndexWriter::DEFAULT_MERGE_FACTOR = LogMergePolicy::DEFAULT_MERGE_FACTOR;
  //int32_t IndexWriter::MAX_TERM_LENGTH = DocumentsWriter::MAX_TERM_LENGTH;

  //int32_t IndexWriter::MERGE_READ_BUFFER_SIZE = 4096;
  DEFINE_MUTEX(IndexWriter::MESSAGE_ID_LOCK);
  int32_t IndexWriter::MESSAGE_ID = 0;
  //const int32_t IndexWriter::MAX_TERM_LENGTH = DocumentsWriter::MAX_TERM_LENGTH;

/*
  IndexWriter::IndexWriter(){
    messageID = -1;
  }
*/
  IndexWriter::~IndexWriter(){
  }
CL_NS_END
