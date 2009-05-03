/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#include "CLucene/store/Directory.h"
#include "CLucene/store/IndexOutput.h"
#include "CLucene/store/_RAMDirectory.h"
#include "CLucene/util/Array.h"
#include "CLucene/util/Misc.h"
#include "CLucene/util/CLStreams.h"
#include "CLucene/document/Field.h"
#include "CLucene/search/Similarity.h"
#include "CLucene/document/Document.h"
#include "_FieldInfos.h"
#include "_CompoundFile.h"
#include "IndexWriter.h"
#include "_IndexFileNames.h"
#include "_FieldsWriter.h"
#include "Term.h"
#include "_Term.h"
#include "_TermInfo.h"
#include "_TermVector.h"
#include "_TermInfosWriter.h"
#include "CLucene/analysis/AnalysisHeader.h"
#include "CLucene/search/Similarity.h"
#include "_TermInfosWriter.h"
#include "_FieldsWriter.h"
#include "_DocumentsWriter.h"
#include <assert.h>

CL_NS_USE(util)
CL_NS_USE(store)
CL_NS_USE(analysis)
CL_NS_USE(document)
CL_NS_USE(search)
CL_NS_DEF(index)


  const int32_t DocumentsWriter::MAX_THREAD_STATE = 5;
  const uint8_t DocumentsWriter::defaultNorm = Similarity::encodeNorm(1.0f);
  const int32_t DocumentsWriter::nextLevelArray[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 9};
  const int32_t DocumentsWriter::levelSizeArray[10] = {5, 14, 20, 30, 40, 40, 80, 80, 120, 200};
  const int32_t DocumentsWriter::POSTING_NUM_BYTE = OBJECT_HEADER_BYTES + 9*INT_NUM_BYTE + 5*POINTER_NUM_BYTE;
  
  const int32_t DocumentsWriter::BYTE_BLOCK_SHIFT = 15;
  const int32_t DocumentsWriter::BYTE_BLOCK_SIZE = (int32_t)pow(2.0, BYTE_BLOCK_SHIFT);
  const int32_t DocumentsWriter::BYTE_BLOCK_MASK = BYTE_BLOCK_SIZE - 1;
  const int32_t DocumentsWriter::BYTE_BLOCK_NOT_MASK = ~BYTE_BLOCK_MASK;

  const int32_t DocumentsWriter::CHAR_BLOCK_SHIFT = 14;
  const int32_t DocumentsWriter::CHAR_BLOCK_SIZE = (int32_t)pow(2.0, CHAR_BLOCK_SHIFT);
  const int32_t DocumentsWriter::CHAR_BLOCK_MASK = CHAR_BLOCK_SIZE - 1;
  	
  int32_t DocumentsWriter::OBJECT_HEADER_BYTES = 8;
  int32_t DocumentsWriter::OBJECT_POINTER_BYTES = 4;    // TODO: should be 8 on 64-bit platform
  int32_t DocumentsWriter::BYTES_PER_CHAR = 2;
  int32_t DocumentsWriter::BYTES_PER_INT = 4;

  const int32_t DocumentsWriter::POINTER_NUM_BYTE = 4;
  const int32_t DocumentsWriter::INT_NUM_BYTE = 4;
  const int32_t DocumentsWriter::CHAR_NUM_BYTE = 2;

  const int32_t DocumentsWriter::MAX_TERM_LENGTH = DocumentsWriter::CHAR_BLOCK_SIZE-1;

  DocumentsWriter::DocumentsWriter(CL_NS(store)::Directory* directory, IndexWriter* writer):
  	waitingThreadStates( CL_NS(util)::ObjectArray<ThreadState>(MAX_THREAD_STATE) ),
  	termInfo(_CLNEW TermInfo()),
	postingsFreeList(CL_NS(util)::ValueArray<Posting>(0))
  {
    this->directory = directory;
    this->writer = writer;
    fieldInfos = _CLNEW FieldInfos();
    
  	maxBufferedDeleteTerms = IndexWriter::DEFAULT_MAX_BUFFERED_DELETE_TERMS;
		ramBufferSize = (int64_t) (IndexWriter::DEFAULT_RAM_BUFFER_SIZE_MB*1024*1024);
		maxBufferedDocs = IndexWriter::DEFAULT_MAX_BUFFERED_DOCS;

	numBufferedDeleteTerms = 0;
    copyByteBuffer = _CL_NEWARRAY(uint8_t, 4096);
    _files = NULL;
    _abortedFiles = NULL;
    skipListWriter = NULL;
	infoStream = NULL;
  }

  /** If non-NULL, various details of indexing are printed
   *  here. */
  void DocumentsWriter::setInfoStream(std::ostream* infoStream) {
    this->infoStream = infoStream;
  }

  /** Set how much RAM we can use before flushing. */
  void DocumentsWriter::setRAMBufferSizeMB(double mb) {
	if (mb == IndexWriter::DISABLE_AUTO_FLUSH) {
	  ramBufferSize = IndexWriter::DISABLE_AUTO_FLUSH;
    } else {
      ramBufferSize = (int64_t) (mb*1024*1024);
    }
  }

  double DocumentsWriter::getRAMBufferSizeMB() {
    if (ramBufferSize == IndexWriter::DISABLE_AUTO_FLUSH) {
      return ramBufferSize;
    } else {
      return ramBufferSize/1024.0/1024.0;
    }
  }

  /** Set max buffered docs, which means we will flush by
   *  doc count instead of by RAM usage. */
  void DocumentsWriter::setMaxBufferedDocs(int32_t count) {
    maxBufferedDocs = count;
  }

  int32_t DocumentsWriter::getMaxBufferedDocs() {
    return maxBufferedDocs;
  }

  /** Get current segment name we are writing. */
  std::string DocumentsWriter::getSegment() {
    return segment;
  }

  /** Returns how many docs are currently buffered in RAM. */
  int32_t DocumentsWriter::getNumDocsInRAM() {
    return numDocsInRAM;
  }

  /** Returns the current doc store segment we are writing
   *  to.  This will be the same as segment when autoCommit
   *  * is true. */
  const std::string& DocumentsWriter::getDocStoreSegment() {
    return docStoreSegment;
  }

  /** Returns the doc offset into the shared doc store for
   *  the current buffered docs. */
  int32_t DocumentsWriter::getDocStoreOffset() {
    return docStoreOffset;
  }

  /** Closes the current open doc stores an returns the doc
   *  store segment name.  This returns NULL if there are *
   *  no buffered documents. */
  std::string DocumentsWriter::closeDocStore() {

    assert (allThreadsIdle());

	const std::vector<string>& flushedFiles = files();

    if (infoStream != NULL)
      (*infoStream) << "\ncloseDocStore: " << flushedFiles.size() << " files to flush to segment " << docStoreSegment << " numDocs=" << numDocsInStore << "\n";

    if (flushedFiles.size() > 0) {
      _CLDELETE(_files);

      if (tvx != NULL) {
        // At least one doc in this run had term vectors enabled
        assert ( !docStoreSegment.empty());
        tvx->close();
        _CLDELETE(tvx);
        tvf->close();
        _CLDELETE(tvf);
        tvd->close();
        _CLDELETE(tvd);

		string vectorFN = docStoreSegment + "." + IndexFileNames::VECTORS_INDEX_EXTENSION;
        assert ( 4+numDocsInStore*8 == directory->fileLength(vectorFN.c_str()) ); // "after flush: tvx size mismatch: " + numDocsInStore + " docs vs " + directory->fileLength(docStoreSegment + "." + IndexFileNames::VECTORS_INDEX_EXTENSION) + " length in bytes of " + docStoreSegment + "." + IndexFileNames::VECTORS_INDEX_EXTENSION;
      }

      if (fieldsWriter != NULL) {
        assert (!docStoreSegment.empty());
        fieldsWriter->close();
        _CLDELETE(fieldsWriter);

		string fieldFN = docStoreSegment + "." + IndexFileNames::FIELDS_INDEX_EXTENSION;
        assert(numDocsInStore*8 == directory->fileLength( fieldFN.c_str() ) );// "after flush: fdx size mismatch: " + numDocsInStore + " docs vs " + directory->fileLength(docStoreSegment + "." + IndexFileNames::FIELDS_INDEX_EXTENSION) + " length in bytes of " + docStoreSegment + "." + IndexFileNames::FIELDS_INDEX_EXTENSION;
      }

      std::string s = docStoreSegment;
      docStoreSegment.clear();
      docStoreOffset = 0;
      numDocsInStore = 0;
      return s;
    } else {
      return NULL;
    }
  }

  const std::vector<string>* DocumentsWriter::abortedFiles() {
    return _abortedFiles;
  }

  /* Returns list of files in use by this instance,
   * including any flushed segments. */
  const std::vector<std::string>& DocumentsWriter::files() {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)

    if (_files != NULL)
      return *_files;

	_files = _CLNEW std::vector<string>;

    // Stored fields:
    if (fieldsWriter != NULL) {
	  assert ( !docStoreSegment.empty());
	  _files->push_back(docStoreSegment + "." + IndexFileNames::FIELDS_EXTENSION);
	  _files->push_back(docStoreSegment + "." + IndexFileNames::FIELDS_INDEX_EXTENSION);
    }

    // Vectors:
    if (tvx != NULL) {
      assert ( !docStoreSegment.empty());
      _files->push_back(docStoreSegment + "." + IndexFileNames::VECTORS_INDEX_EXTENSION);
      _files->push_back(docStoreSegment + "." + IndexFileNames::VECTORS_FIELDS_EXTENSION);
      _files->push_back(docStoreSegment + "." + IndexFileNames::VECTORS_DOCUMENTS_EXTENSION);
    }

    return *_files;
  }

  void DocumentsWriter::setAborting() {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
  	
    abortCount++;
  }

  /** Called if we hit an exception when adding docs,
   *  flushing, etc.  This resets our state, discarding any
   *  docs added since last flush.  If ae is non-NULL, it
   *  contains the root cause exception (which we re-throw
   *  after we are done aborting). */
  void DocumentsWriter::abort(AbortException* ae) {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)

    // Anywhere that throws an AbortException must first
    // mark aborting to make sure while the exception is
    // unwinding the un-synchronized stack, no thread grabs
    // the corrupt ThreadState that hit the aborting
    // exception:
    assert (ae == NULL || abortCount>0);

    try {

      if (infoStream != NULL)
        (*infoStream) << "docWriter: now abort\n";

      // Forcefully remove waiting ThreadStates from line
      for(int32_t i=0;i<numWaiting;i++)
        waitingThreadStates[i]->isIdle = true;
      numWaiting = 0;

      // Wait for all other threads to finish with DocumentsWriter:
      pauseAllThreads();

      assert (0 == numWaiting);

      try {
        bufferedDeleteTerms->clear();
        bufferedDeleteDocIDs.clear();
        numBufferedDeleteTerms = 0;

        try {
			const std::vector<string>& __abortedFiles = files();
			_abortedFiles = _CLNEW std::vector<string>;
			for ( std::vector<string>::const_iterator itr = __abortedFiles.begin();
				itr != __abortedFiles.end(); itr ++ ){
				_abortedFiles->push_back(*itr);
			}
        } catch (...) {
			_CLDELETE(_abortedFiles);
        }

		docStoreSegment.clear();
        numDocsInStore = 0;
        docStoreOffset = 0;
        _CLDELETE(_files);

        // Clear vectors & fields from ThreadStates
        for(int32_t i=0;i<threadStates.length;i++) {
          ThreadState* state = threadStates[i];
          state->tvfLocal->reset();
          state->fdtLocal->reset();
          if (state->localFieldsWriter != NULL) {
            try {
              state->localFieldsWriter->close();
            } catch (...) {
            }
            _CLDELETE(state->localFieldsWriter);
          }
        }

        // Reset vectors writer
        if (tvx != NULL) {
          try {
            tvx->close();
          } catch (...) {
          }
          _CLDELETE(tvx);
        }
        if (tvd != NULL) {
          try {
            tvd->close();
          } catch (...) {
          }
          _CLDELETE(tvd);
        }
        if (tvf != NULL) {
          try {
            tvf->close();
          } catch (...) {
          }
          _CLDELETE(tvf);
        }

        // Reset fields writer
        if (fieldsWriter != NULL) {
          try {
            fieldsWriter->close();
          } catch (...) {
          }
          _CLDELETE(fieldsWriter);
        }

        // Discard pending norms:
        const int32_t numField = fieldInfos->size();
        for (int32_t i=0;i<numField;i++) {
          FieldInfo* fi = fieldInfos->fieldInfo(i);
          if (fi->isIndexed && !fi->omitNorms) {
            BufferedNorms* n = norms[i];
            if (n != NULL)
              try {
                n->reset();
              } catch (...) {
              }
          }
        }

        // Reset all postings data
        resetPostingsData();

      } _CLFINALLY (
        resumeAllThreads();
	  )

      // If we have a root cause exception, re-throw it now:
      if (ae != NULL) {
        CLuceneError& t = ae->getCause();
		if (t.number() == CL_ERR_IO)
          throw t;
		else if (t.number() == CL_ERR_Runtime)
          throw t;
        else if (t.number() == CL_ERR_Error)
          throw t;
        else
          // Should not get here
          assert (false);// "unknown exception: " + t;
      }
    } _CLFINALLY (
      if (ae != NULL)
        abortCount--;
      CONDITION_NOTIFYALL(THIS_WAIT_CONDITION)
 	)
  }

  /** Reset after a flush */
  void DocumentsWriter::resetPostingsData() {
    // All ThreadStates should be idle when we are called
    assert ( allThreadsIdle() );
    threadBindings.clear();
    segment.erase();
    numDocsInRAM = 0;
    nextDocID = 0;
    nextWriteDocID = 0;
    _CLDELETE(_files);
    balanceRAM();
    bufferIsFull = false;
    flushPending = false;
    for(int32_t i=0;i<threadStates.length;i++) {
      threadStates[i]->numThreads = 0;
      threadStates[i]->resetPostings();
    }
    numBytesUsed = 0;
  }

  // Returns true if an abort is in progress
  bool DocumentsWriter::pauseAllThreads() {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
  	
    pauseThreads++;
    while(!allThreadsIdle()) {
      try {
        CONDITION_WAIT(THIS_LOCK, THIS_WAIT_CONDITION)
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
      }
    }
    return abortCount > 0;
  }

  void DocumentsWriter::resumeAllThreads() {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
  	
    pauseThreads--;
    assert ( pauseThreads >= 0 );
	if (0 == pauseThreads){
      CONDITION_NOTIFYALL(THIS_WAIT_CONDITION)
	}
  }

  bool DocumentsWriter::allThreadsIdle() {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)

    for(int32_t i=0;i<threadStates.length;i++)
      if (!threadStates[i]->isIdle)
        return false;
    return true;
  }

  /** Flush all pending docs to a new segment */
  int32_t DocumentsWriter::flush(bool _closeDocStore) {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)

    assert ( allThreadsIdle() );

	if (segment.empty()){
      // In case we are asked to flush an empty segment
      segment = writer->newSegmentName();
	}

	newFiles.clear();

    docStoreOffset = numDocsInStore;

    int32_t docCount;

    assert ( numDocsInRAM > 0 );

    if (infoStream != NULL)
      (*infoStream) << "\nflush postings as segment " << segment << " numDocs=" << numDocsInRAM << "\n";

    bool success = false;

    try {

      if (_closeDocStore) {
        assert ( !docStoreSegment.empty());
		assert ( docStoreSegment.compare(segment) == 0 );
		const std::vector<string>& tmp = files();
		for (std::vector<string>::const_iterator itr = tmp.begin();
			itr != tmp.end(); itr++ )
			newFiles.push_back(*itr);
        closeDocStore();
      }

      fieldInfos->write(directory, (segment + ".fnm").c_str() );

      docCount = numDocsInRAM;

	  std::vector<string> flushedFiles;
	  writeSegment(flushedFiles);
	  for (std::vector<string>::const_iterator itr = flushedFiles.begin();
			itr != flushedFiles.end(); itr ++ )
			newFiles.push_back(*itr);

      success = true;

    } _CLFINALLY(
      if (!success)
        abort(NULL);
	)

    return docCount;
  }

  /** Build compound file for the segment we just flushed */
  void DocumentsWriter::createCompoundFile(const std::string& segment)
  {
    CompoundFileWriter* cfsWriter = _CLNEW CompoundFileWriter(directory, (segment + "." + IndexFileNames::COMPOUND_FILE_EXTENSION).c_str());
    for (std::vector<string>::const_iterator itr = newFiles.begin();
			itr != newFiles.end(); itr ++ )
      cfsWriter->addFile( (*itr).c_str() );

    // Perform the merge
    cfsWriter->close();
	_CLDELETE(cfsWriter);
  }

  /** Set flushPending if it is not already set and returns
   *  whether it was set. This is used by IndexWriter to *
   *  trigger a single flush even when multiple threads are
   *  * trying to do so. */
  bool DocumentsWriter::setFlushPending() {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
  	
    if (flushPending)
      return false;
    else {
      flushPending = true;
      return true;
    }
  }

  void DocumentsWriter::clearFlushPending() {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
  	
    flushPending = false;
  }

  /** Write norms in the "true" segment format.  This is
  *  called only during commit, to create the .nrm file. */
  void DocumentsWriter::writeNorms(const std::string& segmentName, int32_t totalNumDoc) {
	  IndexOutput* normsOut = directory->createOutput(segmentName + "." + IndexFileNames::NORMS_EXTENSION);

    try {
		normsOut->writeBytes(SegmentMerger::NORMS_HEADER, SegmentMerger::NORMS_HEADER.length);

      const int32_t numField = fieldInfos->size();

      for (int32_t fieldIdx=0;fieldIdx<numField;fieldIdx++) {
        FieldInfo* fi = fieldInfos->fieldInfo(fieldIdx);
        if (fi->isIndexed && !fi->omitNorms) {
          BufferedNorms* n = norms[fieldIdx];
          int64_t v;
          if (n == NULL)
            v = 0;
          else {
            v = n->out->getFilePointer();
            n->out->writeTo(normsOut);
            n->reset();
          }
          if (v < totalNumDoc)
            fillBytes(normsOut, defaultNorm, (int32_t) (totalNumDoc-v));
        }
      }
    } _CLFINALLY (
      normsOut->close();
	  _CLDELETE(normsOut);
	  )
  }

  /** Creates a segment from all Postings in the Postings
   *  hashes across all ThreadStates & FieldDatas. */
  void DocumentsWriter::writeSegment(std::vector<std::string>& flushedFiles) {

    assert ( allThreadsIdle() );

    assert ( nextDocID == numDocsInRAM );

    const std::string segmentName = segment;

	TermInfosWriter* termsOut = _CLNEW TermInfosWriter(directory, segmentName.c_str(), fieldInfos,
                                                   writer->getTermIndexInterval());

    IndexOutput* freqOut = directory->createOutput( (segmentName + ".frq").c_str() );
    IndexOutput* proxOut = directory->createOutput( (segmentName + ".prx").c_str() );

    // Gather all FieldData's that have postings, across all
    // ThreadStates
	std::vector<ThreadState::FieldData*> allFields;
    assert ( allThreadsIdle() );
    for(int32_t i=0;i<threadStates.length;i++) {
      ThreadState* state = threadStates[i];
      state->trimFields();
      const int32_t numFields = state->numAllFieldData;
      for(int32_t j=0;j<numFields;j++) {
        ThreadState::FieldData* fp = state->allFieldDataArray[j];
        if (fp->numPostings > 0)
			allFields.push_back(fp);
      }
    }

    // Sort by field name
    Collections.sort(allFields);
    const int32_t numAllFields = allFields.size();

    skipListWriter = _CLNEW DefaultSkipListWriter(termsOut->skipInterval,
                                               termsOut->maxSkipLevels,
                                               numDocsInRAM, freqOut, proxOut);

    int32_t start = 0;
    while(start < numAllFields) {

      const TCHAR* fieldName = allFields[start]->fieldInfo->name;

      int32_t end = start+1;
      while(end < numAllFields && _tcscmp(allFields[end]->fieldInfo->name, fieldName)==0 )
        end++;

      ObjectArray<ThreadState::FieldData*> fields(end-start);
      for(int32_t i=start;i<end;i++)
        fields[i-start]->values = allFields[i];

      // If this field has postings then add them to the
      // segment
      appendPostings(fields, termsOut, freqOut, proxOut);

      for(int32_t i=0;i<fields.length;i++)
        fields[i]->resetPostingArrays();

      start = end;
    }

    freqOut->close();
    proxOut->close();
    termsOut->close();

    // Record all files we have flushed
    flushedFiles.push_back(segmentFileName(IndexFileNames::FIELD_INFOS_EXTENSION));
    flushedFiles.push_back(segmentFileName(IndexFileNames::FREQ_EXTENSION));
    flushedFiles.push_back(segmentFileName(IndexFileNames::PROX_EXTENSION));
    flushedFiles.push_back(segmentFileName(IndexFileNames::TERMS_EXTENSION));
    flushedFiles.push_back(segmentFileName(IndexFileNames::TERMS_INDEX_EXTENSION));

    if (hasNorms) {
      writeNorms(segmentName, numDocsInRAM);
      flushedFiles.push_back(segmentFileName(IndexFileNames::NORMS_EXTENSION));
    }

    if (infoStream != NULL) {
      const int64_t newSegmentSize = segmentSize(segmentName);

	  char bufDocs[40];
	  sprintf(buf,"%0.2f", numDocsInRAM/(newSegmentSize/1024.0/1024.0));
	  char bufNew[40];
	  sprintf(buf,"%0.2f", 100.0*newSegmentSize/numBytesUsed);
      (*infoStream) << "  oldRAMSize=" << numBytesUsed << 
					" newFlushedSize=" << newSegmentSize << 
					" docs/MB=" << bufDocs << 
					" new/old=" << bufNew << "%\n";
    }

    resetPostingsData();

    nextDocID = 0;
    nextWriteDocID = 0;
    numDocsInRAM = 0;
    _CLDELETE(_files);

    // Maybe downsize postingsFreeList array
    if (postingsFreeList.length > 1.5*postingsFreeCount) {
      int32_t newSize = postingsFreeList.length;
      while(newSize > 1.25*postingsFreeCount) {
        newSize = (int32_t) (newSize*0.8);
      }
      Posting* newArray = _CL_NEWARRAY(Posting,newSize);
      System.arraycopy(postingsFreeList, 0, newArray, 0, postingsFreeCount);
      _CLDELETE_ARRAY(postingsFreeList.values);
	  postingsFreeList.values = newArray;
	  postingsFreeList.length = newSize;
    }

  }

  std::string DocumentsWriter::segmentFileName(const char* extension) {
    return segmentFileName( string(extension) );
  }
  std::string DocumentsWriter::segmentFileName(const std::string& extension) {
    return segment + "." + extension;
  }

//TODO: can't we just use _tcscmp???
  int32_t DocumentsWriter::compareText(const TCHAR* text1, const TCHAR* text2) {
	int32_t pos1=0;
	int32_t pos2=0;
    while(true) {
      const TCHAR c1 = text1[pos1++];
      const TCHAR c2 = text2[pos2++];
      if (c1 < c2)
        if (0xffff == c2)
          return 1;
        else
          return -1;
      else if (c2 < c1)
        if (0xffff == c1)
          return -1;
        else
          return 1;
      else if (0xffff == c1)
        return 0;
    }
  }

  /* Walk through all unique text tokens (Posting
   * instances) found in this field and serialize them
   * into a single RAM segment. */
  void DocumentsWriter::appendPostings(ObjectArray<ThreadState::FieldData>* fields,
                      TermInfosWriter* termsOut,
                      IndexOutput* freqOut,
                      IndexOutput* proxOut) {

    const int32_t fieldNumber = fields[0]->fieldInfo->number;
    int32_t numFields = fields->length;

    ObjectArray<FieldMergeState*> mergeStates(numFields);

    for(int32_t i=0;i<numFields;i++) {
      FieldMergeState* fms = mergeStates[i] = _CLNEW FieldMergeState();
      fms->field = fields[i];
      fms->postings = fms->field->sortPostings();

      assert ( fms->field->fieldInfo == fields[0]->fieldInfo );

      // Should always be true
      bool result = fms->nextTerm();
      assert (result);
    }

    const int32_t skipInterval = termsOut->skipInterval;
    currentFieldStorePayloads = fields[0].fieldInfo->storePayloads;

    FieldMergeState[] termStates = _CLNEW FieldMergeState[numFields];

    while(numFields > 0) {

      // Get the next term to merge
      termStates[0] = mergeStates[0];
      int32_t numToMerge = 1;

      for(int32_t i=1;i<numFields;i++) {
        const char[] text = mergeStates[i]->text;
        const int32_t textOffset = mergeStates[i]->textOffset;
        const int32_t cmp = compareText(text, textOffset, termStates[0]->text, termStates[0]->textOffset);

        if (cmp < 0) {
          termStates[0] = mergeStates[i];
          numToMerge = 1;
        } else if (cmp == 0)
          termStates[numToMerge++] = mergeStates[i];
      }

      int32_t df = 0;
      int32_t lastPayloadLength = -1;

      int32_t lastDoc = 0;

      const char[] text = termStates[0]->text;
      const int32_t start = termStates[0]->textOffset;
      int32_t pos = start;
      while(text[pos] != 0xffff)
        pos++;

      int64_t freqPointer = freqOut->getFilePointer();
      int64_t proxPointer = proxOut->getFilePointer();

      skipListWriter->resetSkip();

      // Now termStates has numToMerge FieldMergeStates
      // which all share the same term.  Now we must
      // interleave the docID streams.
      while(numToMerge > 0) {

        if ((++df % skipInterval) == 0) {
          skipListWriter->setSkipData(lastDoc, currentFieldStorePayloads, lastPayloadLength);
          skipListWriter->bufferSkip(df);
        }

        FieldMergeState minState = termStates[0];
        for(int32_t i=1;i<numToMerge;i++)
          if (termStates[i]->docID < minState->docID)
            minState = termStates[i];

        const int32_t doc = minState->docID;
        const int32_t termDocFreq = minState->termFreq;

        assert (doc < numDocsInRAM);
        assert ( doc > lastDoc || df == 1 );

        const int32_t newDocCode = (doc-lastDoc)<<1;
        lastDoc = doc;

        const ByteSliceReader prox = minState->prox;

        // Carefully copy over the prox + payload info,
        // changing the format to match Lucene's segment
        // format.
        for(int32_t j=0;j<termDocFreq;j++) {
          const int32_t code = prox->readVInt();
          if (currentFieldStorePayloads) {
            const int32_t payloadLength;
            if ((code & 1) != 0) {
              // This position has a payload
              payloadLength = prox->readVInt();
            } else
              payloadLength = 0;
            if (payloadLength != lastPayloadLength) {
              proxOut->writeVInt(code|1);
              proxOut->writeVInt(payloadLength);
              lastPayloadLength = payloadLength;
            } else
              proxOut->writeVInt(code & (~1));
            if (payloadLength > 0)
              copyBytes(prox, proxOut, payloadLength);
          } else {
            assert ( 0 == (code & 1) );
            proxOut->writeVInt(code>>1);
          }
        }

        if (1 == termDocFreq) {
          freqOut->writeVInt(newDocCode|1);
        } else {
          freqOut->writeVInt(newDocCode);
          freqOut->writeVInt(termDocFreq);
        }

        if (!minState->nextDoc()) {

          // Remove from termStates
          int32_t upto = 0;
          for(int32_t i=0;i<numToMerge;i++)
            if (termStates[i] != minState)
              termStates[upto++] = termStates[i];
          numToMerge--;
          assert (upto == numToMerge);

          // Advance this state to the next term

          if (!minState->nextTerm()) {
            // OK, no more terms, so remove from mergeStates
            // as well
            upto = 0;
            for(int32_t i=0;i<numFields;i++)
              if (mergeStates[i] != minState)
                mergeStates[upto++] = mergeStates[i];
            numFields--;
            assert (upto == numFields);
          }
        }
      }

      assert (df > 0);

      // Done merging this term

      int64_t skipPointer = skipListWriter->writeSkip(freqOut);

      // Write term
      termInfo->set(df, freqPointer, proxPointer, (int32_t) (skipPointer - freqPointer));
      termsOut->add(fieldNumber, text, start, pos-start, termInfo);
    }
  }

  void DocumentsWriter::close() {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
  	
    closed = true;
    CONDITION_NOTIFYALL(THIS_WAIT_CONDITION)
  }

  /** Returns a free (idle) ThreadState that may be used for
   * indexing this one document.  This call also pauses if a
   * flush is pending.  If delTerm is non-NULL then we
   * buffer this deleted term after the thread state has
   * been acquired. */
  DocumentsWriter::ThreadState* DocumentsWriter::getThreadState(Document* doc, Term* delTerm) {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)

    // First, find a thread state.  If this thread already
    // has affinity to a specific ThreadState, use that one
    // again.
    ThreadState* state = threadBindings[Thread.currentThread()];
    if (state == NULL) {
      // First time this thread has called us since last flush
      ThreadState* minThreadState = NULL;
      for(int32_t i=0;i<threadStates.length;i++) {
        ThreadState* ts = threadStates[i];
        if (minThreadState == NULL || ts->numThreads < minThreadState->numThreads)
          minThreadState = ts;
      }
      if (minThreadState != NULL && (minThreadState->numThreads == 0 || threadStates.length == MAX_THREAD_STATE)) {
        state = minThreadState;
        state->numThreads++;
      } else {
        // Just create a new "private" thread state
        ThreadState** newArray = _CL_NEWARRAY(ThreadState*,1+threadStates.length);
        if (threadStates.length > 0)
          System.arraycopy(threadStates, 0, newArray, 0, threadStates.length);
        state = newArray[threadStates.length] = _CLNEW ThreadState();

		_CLDELETE_ARRAY(threadStates.values);
        threadStates.values = newArray;
		threadStates.length+=1;
      }
      threadBindings.put(Thread.currentThread(), state);
    }

    // Next, wait until my thread state is idle (in case
    // it's shared with other threads) and for threads to
    // not be paused nor a flush pending:
    while(!closed && (!state->isIdle || pauseThreads != 0 || flushPending || abortCount > 0))
      try {
        CONDITION_WAIT(THIS_LOCK, THIS_WAIT_CONDITION)
      } catch (InterruptedException e) {
        Thread.currentThread()->interrupt();
      }

    if (closed)
      throw new AlreadyClosedException("this IndexWriter is closed");

	if (segment.empty())
      segment = writer->newSegmentName();

    state->isIdle = false;

    try {
      bool success = false;
      try {
        state->init(doc, nextDocID);
        if (delTerm != NULL) {
          addDeleteTerm(delTerm, state->docID);
          state->doFlushAfter = timeToFlushDeletes();
        }
        // Only increment nextDocID & numDocsInRAM on successful init
        nextDocID++;
        numDocsInRAM++;

        // We must at this point commit to flushing to ensure we
        // always get N docs when we flush by doc count, even if
        // > 1 thread is adding documents:
        if (!flushPending && maxBufferedDocs != IndexWriter::DISABLE_AUTO_FLUSH
            && numDocsInRAM >= maxBufferedDocs) {
          flushPending = true;
          state->doFlushAfter = true;
        }

        success = true;
      } _CLFINALLY (
        if (!success) {
          // Forcefully idle this ThreadState:
          state->isIdle = true;
          CONDITION_NOTIFYALL(THIS_WAIT_CONDITION)
          if (state->doFlushAfter) {
            state->doFlushAfter = false;
            flushPending = false;
          }
        }
	  )
    } catch (AbortException& ae) {
      abort(&ae);
    }

    return state;
  }

  /** Returns true if the caller (IndexWriter) should now
   * flush. */
  bool DocumentsWriter::addDocument(Document* doc, Analyzer* analyzer){
    return updateDocument(doc, analyzer, NULL);
  }

  bool DocumentsWriter::updateDocument(Term* t, Document* doc, Analyzer* analyzer){
    return updateDocument(doc, analyzer, t);
  }

  bool DocumentsWriter::updateDocument(Document* doc, Analyzer* analyzer, Term* delTerm) {

    // This call is synchronized but fast
    ThreadState* state = getThreadState(doc, delTerm);
    try {
      bool success = false;
      try {
        try {
          // This call is not synchronized and does all the work
          state->processDocument(analyzer);
        } _CLFINALLY (
          // This call is synchronized but fast
          finishDocument(state);
		)
        success = true;
      } _CLFINALLY (
        if (!success) {
  				SCOPED_LOCK_MUTEX(THIS_LOCK)

          // If this thread state had decided to flush, we
          // must clear it so another thread can flush
          if (state->doFlushAfter) {
            state->doFlushAfter = false;
            flushPending = false;
            CONDITION_NOTIFYALL(THIS_WAIT_CONDITION)
          }

          // Immediately mark this document as deleted
          // since likely it was partially added.  This
          // keeps indexing as "all or none" (atomic) when
          // adding a document:
          addDeleteDocID(state->docID);
		}
	  )
    } catch (AbortException& ae) {
      abort(&ae);
    }

    return state->doFlushAfter || timeToFlushDeletes();
  }

  int32_t DocumentsWriter::getNumBufferedDeleteTerms() {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
    return numBufferedDeleteTerms;
  }

  HashMap DocumentsWriter::getBufferedDeleteTerms() {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
    return bufferedDeleteTerms;
  }

  const CL_NS(util)::CLArrayList<int32_t>* DocumentsWriter::getBufferedDeleteDocIDs() {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
    return &bufferedDeleteDocIDs;
  }

  // Reset buffered deletes.
  void DocumentsWriter::clearBufferedDeletes() {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
    bufferedDeleteTerms->clear();
	bufferedDeleteDocIDs.clear();
    numBufferedDeleteTerms = 0;
    if (numBytesUsed > 0)
      resetPostingsData();
  }

  bool DocumentsWriter::bufferDeleteTerms(const ObjectArray<Term*>* terms) {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
    while(pauseThreads != 0 || flushPending)
      try {
        CONDITION_WAIT(THIS_LOCK, THIS_WAIT_CONDITION)
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
      }
      for (int32_t i = 0; i < terms->length; i++)
        addDeleteTerm(terms[i], numDocsInRAM);
    return timeToFlushDeletes();
  }

  bool DocumentsWriter::bufferDeleteTerm(Term* term) {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
    while(pauseThreads != 0 || flushPending)
      try {
        CONDITION_WAIT(THIS_LOCK, THIS_WAIT_CONDITION)
      } catch (InterruptedException e) {
        Thread.currentThread()->interrupt();
      }
    addDeleteTerm(term, numDocsInRAM);
    return timeToFlushDeletes();
  }

  bool DocumentsWriter::timeToFlushDeletes() {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
    return (bufferIsFull
            || (maxBufferedDeleteTerms != IndexWriter::DISABLE_AUTO_FLUSH
                && numBufferedDeleteTerms >= maxBufferedDeleteTerms))
           && setFlushPending();
  }

  void DocumentsWriter::setMaxBufferedDeleteTerms(int32_t maxBufferedDeleteTerms) {
    this->maxBufferedDeleteTerms = maxBufferedDeleteTerms;
  }

  int32_t DocumentsWriter::getMaxBufferedDeleteTerms() {
    return maxBufferedDeleteTerms;
  }

  bool DocumentsWriter::hasDeletes() {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
    return bufferedDeleteTerms->size() > 0 || bufferedDeleteDocIDs.size() > 0;
  }

  // Buffer a term in bufferedDeleteTerms, which records the
  // current number of documents buffered in ram so that the
  // delete term will be applied to those documents as well
  // as the disk segments.
  void DocumentsWriter::addDeleteTerm(Term* term, int32_t docCount) {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
    Num num = (Num) bufferedDeleteTerms->get(term);
    if (num == NULL) {
      bufferedDeleteTerms->put(term, new Num(docCount));
      // This is coarse approximation of actual bytes used:
      numBytesUsed += (term->field().length() + term.text().length()) * BYTES_PER_CHAR
          + 4 + 5 * OBJECT_HEADER_BYTES + 5 * OBJECT_POINTER_BYTES;
      if (ramBufferSize != IndexWriter::DISABLE_AUTO_FLUSH
          && numBytesUsed > ramBufferSize) {
        bufferIsFull = true;
      }
    } else {
      num->setNum(docCount);
    }
    numBufferedDeleteTerms++;
  }

  // Buffer a specific docID for deletion.  Currently only
  // used when we hit a exception when adding a document
  void DocumentsWriter::addDeleteDocID(int32_t docId) {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
    bufferedDeleteDocIDs.add(docId);
    numBytesUsed += OBJECT_HEADER_BYTES + BYTES_PER_INT + OBJECT_POINTER_BYTES;
  }

  /** Does the synchronized work to finish/flush the
   * inverted document. */
  void DocumentsWriter::finishDocument(ThreadState* state) {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
    if (abortCount > 0) {
      // Forcefully idle this threadstate -- its state will
      // be reset by abort()
      state->isIdle = true;
      CONDITION_NOTIFYALL(THIS_WAIT_CONDITION)
      return;
    }

    // Now write the indexed document to the real files.
    if (nextWriteDocID == state->docID) {
      // It's my turn, so write everything now:
      nextWriteDocID++;
      state->writeDocument();
      state->isIdle = true;
      CONDITION_NOTIFYALL(THIS_WAIT_CONDITION)

      // If any states were waiting on me, sweep through and
      // flush those that are enabled by my write.
      if (numWaiting > 0) {
        bool any = true;
        while(any) {
          any = false;
          for(int32_t i=0;i<numWaiting;) {
            const ThreadState s = waitingThreadStates[i];
            if (s->docID == nextWriteDocID) {
              s->writeDocument();
              s->isIdle = true;
              nextWriteDocID++;
              any = true;
              if (numWaiting > i+1)
                // Swap in the last waiting state to fill in
                // the hole we just created.  It's important
                // to do this as-we-go and not at the end of
                // the loop, because if we hit an aborting
                // exception in one of the s.writeDocument
                // calls (above), it leaves this array in an
                // inconsistent state:
                waitingThreadStates[i] = waitingThreadStates[numWaiting-1];
              numWaiting--;
            } else {
              assert (!s->isIdle);
              i++;
            }
          }
        }
      }
    } else {
      // Another thread got a docID before me, but, it
      // hasn't finished its processing.  So add myself to
      // the line but don't hold up this thread.
      waitingThreadStates[numWaiting++] = state;
    }
  }

  int64_t getRAMUsed() {
    return numBytesUsed;
  }

  int64_t numBytesAlloc;
  int64_t numBytesUsed;

  /* Used only when writing norms to fill in default norm
   * value into the holes in docID stream for those docs
   * that didn't have this field. */
  void DocumentsWriter::fillBytes(IndexOutput out, uint8_t b, int32_t numBytes) {
    for(int32_t i=0;i<numBytes;i++)
      out->writeByte(b);
  }

  /** Copy numBytes from srcIn to destIn */
  void DocumentsWriter::copyBytes(IndexInput srcIn, IndexOutput destIn, int64_t numBytes) {
    // TODO: we could do this more efficiently (save a copy)
    // because it's always from a ByteSliceReader ->
    // IndexOutput
    while(numBytes > 0) {
      const int32_t chunk;
      if (numBytes > 4096)
        chunk = 4096;
      else
        chunk = (int32_t) numBytes;
      srcIn->readBytes(copyByteBuffer, 0, chunk);
      destIn->writeBytes(copyByteBuffer, 0, chunk);
      numBytes -= chunk;
    }
  }


  // Used only when infoStream != NULL
  int64_t DocumentsWriter::segmentSize(std::string segmentName) {
    assert (infoStream != NULL);

    int64_t size = directory->fileLength(segmentName + ".tii") +
      directory->fileLength(segmentName + ".tis") +
      directory->fileLength(segmentName + ".frq") +
      directory->fileLength(segmentName + ".prx");

    const std::string normFileName = segmentName + ".nrm";
    if (directory->fileExists(normFileName))
      size += directory->fileLength(normFileName);

    return size;
  }

  /* Allocate more Postings from shared pool */
  void DocumentsWriter::getPostings(Posting[] postings) {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
    numBytesUsed += postings->length * POSTING_NUM_BYTE;
    const int32_t numToCopy;
    if (postingsFreeCount < postings->length)
      numToCopy = postingsFreeCount;
    else
      numToCopy = postings->length;
    const int32_t start = postingsFreeCount-numToCopy;
    System.arraycopy(postingsFreeList, start,
                     postings, 0, numToCopy);
    postingsFreeCount -= numToCopy;

    // Directly allocate the remainder if any
    if (numToCopy < postings->length) {
      const int32_t extra = postings->length - numToCopy;
      const int32_t newPostingsAllocCount = postingsAllocCount + extra;
      if (newPostingsAllocCount > postingsFreeList.length)
        postingsFreeList = _CLNEW Posting[(int32_t) (1.25 * newPostingsAllocCount)];

      balanceRAM();
      for(int32_t i=numToCopy;i<postings.length;i++) {
        postings[i] = _CLNEW Posting();
        numBytesAlloc += POSTING_NUM_BYTE;
        postingsAllocCount++;
      }
    }
  }

  void recyclePostings(Posting[] postings, int32_t numPostings) {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
    // Move all Postings from this ThreadState back to our
    // free list.  We pre-allocated this array while we were
    // creating Postings to make sure it's large enough
    assert (postingsFreeCount + numPostings <= postingsFreeList.length);
    System.arraycopy(postings, 0, postingsFreeList, postingsFreeCount, numPostings);
    postingsFreeCount += numPostings;
  }

  /* Allocate another uint8_t[] from the shared pool */
  uint8_t[] DocumentsWriter::getByteBlock(bool trackAllocations) {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
    const int32_t size = freeByteBlocks->size();
    const uint8_t[] b;
    if (0 == size) {
      numBytesAlloc += BYTE_BLOCK_SIZE;
      balanceRAM();
      b = _CLNEW uint8_t[BYTE_BLOCK_SIZE];
    } else
      b = (uint8_t[]) freeByteBlocks->remove(size-1);
    if (trackAllocations)
      numBytesUsed += BYTE_BLOCK_SIZE;
    return b;
  }

  /* Return a uint8_t[] to the pool */
  void recycleByteBlocks(uint8_t[][] blocks, int32_t start, int32_t end) {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
    for(int32_t i=start;i<end;i++)
      freeByteBlocks->add(blocks[i]);
  }

  /* Allocate another char[] from the shared pool */
  char[] getCharBlock() {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)

    const int32_t size = freeCharBlocks->size();
    const char[] c;
    if (0 == size) {
      numBytesAlloc += CHAR_BLOCK_SIZE * CHAR_NUM_BYTE;
      balanceRAM();
      c = _CLNEW char[CHAR_BLOCK_SIZE];
    } else
      c = (char[]) freeCharBlocks->remove(size-1);
    numBytesUsed += CHAR_BLOCK_SIZE * CHAR_NUM_BYTE;
    return c;
  }

  /* Return a char[] to the pool */
  void DocumentsWriter::recycleCharBlocks(TCHAR** blocks, int32_t numBlocks) {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)
  	
    for(int32_t i=0;i<numBlocks;i++)
      freeCharBlocks->add(blocks[i]);
  }

  std::string toMB(int64_t v) {
	  char buf[40];
	  sprintf(buf,"%0.2f", v/1024.0/1024.0);
	  return string(buf);
  }

  /* We have three pools of RAM: Postings, uint8_t blocks
   * (holds freq/prox posting data) and char blocks (holds
   * characters in the term).  Different docs require
   * varying amount of storage from these three classes.
   * For example, docs with many unique single-occurrence
   * short terms will use up the Postings RAM and hardly any
   * of the other two.  Whereas docs with very large terms
   * will use alot of char blocks RAM and relatively less of
   * the other two.  This method just frees allocations from
   * the pools once we are over-budget, which balances the
   * pools to match the current docs. */
  void DocumentsWriter::balanceRAM() {
  	SCOPED_LOCK_MUTEX(THIS_LOCK)

    if (ramBufferSize == IndexWriter::DISABLE_AUTO_FLUSH || bufferIsFull)
      return;

    // We free our allocations if we've allocated 5% over
    // our allowed RAM buffer
    const int64_t freeTrigger = (int64_t) (1.05 * ramBufferSize);
    const int64_t freeLevel = (int64_t) (0.95 * ramBufferSize);

    // We flush when we've used our target usage
    const int64_t flushTrigger = (int64_t) ramBufferSize;

    if (numBytesAlloc > freeTrigger) {
      if (infoStream != NULL)
        (*infoStream) << "  RAM: now balance allocations: usedMB=" << toMB(numBytesUsed) +
                           " vs trigger=" << toMB(flushTrigger) <<
                           " allocMB=" << toMB(numBytesAlloc) <<
                           " vs trigger=" << toMB(freeTrigger) <<
                           " postingsFree=" << toMB(postingsFreeCount*POSTING_NUM_BYTE) <<
                           " byteBlockFree=" << toMB(freeByteBlocks.size()*BYTE_BLOCK_SIZE) <<
                           " charBlockFree=" << toMB(freeCharBlocks.size()*CHAR_BLOCK_SIZE*CHAR_NUM_BYTE) << "\n";

      // When we've crossed 100% of our target Postings
      // RAM usage, try to free up until we're back down
      // to 95%
      const int64_t startBytesAlloc = numBytesAlloc;

      const int32_t postingsFreeChunk = (int32_t) (BYTE_BLOCK_SIZE / POSTING_NUM_BYTE);

      int32_t iter = 0;

      // We free equally from each pool in 64 KB
      // chunks until we are below our threshold
      // (freeLevel)

      while(numBytesAlloc > freeLevel) {
        if (0 == freeByteBlocks->size() && 0 == freeCharBlocks->size() && 0 == postingsFreeCount) {
          // Nothing else to free -- must flush now.
          bufferIsFull = true;
          if (infoStream != NULL)
            (*infoStream) <<"    nothing to free; now set bufferIsFull\n";
          break;
        }

        if ((0 == iter % 3) && freeByteBlocks->size() > 0) {
          freeByteBlocks->remove(freeByteBlocks->size()-1);
          numBytesAlloc -= BYTE_BLOCK_SIZE;
        }

        if ((1 == iter % 3) && freeCharBlocks->size() > 0) {
          freeCharBlocks->remove(freeCharBlocks->size()-1);
          numBytesAlloc -= CHAR_BLOCK_SIZE * CHAR_NUM_BYTE;
        }

        if ((2 == iter % 3) && postingsFreeCount > 0) {
          const int32_t numToFree;
          if (postingsFreeCount >= postingsFreeChunk)
            numToFree = postingsFreeChunk;
          else
            numToFree = postingsFreeCount;
          Arrays.fill(postingsFreeList, postingsFreeCount-numToFree, postingsFreeCount, NULL);
          postingsFreeCount -= numToFree;
          postingsAllocCount -= numToFree;
          numBytesAlloc -= numToFree * POSTING_NUM_BYTE;
        }

        iter++;
      }

	  if (infoStream != NULL){
		char bufFree[40];
	    sprintf(bufFree,"%0.2f", (startBytesAlloc-numBytesAlloc)/1024.0/1024.0 );
		char bufUsed[40];
	    sprintf(bufUsed,"%0.2f", numBytesUsed/1024.0/1024.0 );
		char bufAlloc[40];
	    sprintf(bufUsed,"%0.2f", numBytesAlloc/1024.0/1024.0 );

        (*infoStream) << "    after free: freedMB=" + bufFree + " usedMB=" + bufUsed + " allocMB=" + bufAlloc << "\n";
	  }

    } else {
      // If we have not crossed the 100% mark, but have
      // crossed the 95% mark of RAM we are actually
      // using, go ahead and flush.  This prevents
      // over-allocating and then freeing, with every
      // flush.
      if (numBytesUsed > flushTrigger) {
		  if (infoStream != NULL){
			char bufUsed[40];
			sprintf(bufUsed,"%0.2f", numBytesUsed/1024.0/1024.0 );
			char bufAlloc[40];
			sprintf(bufAlloc,"%0.2f", numBytesAlloc/1024.0/1024.0 );
			char bufTrig[40];
			sprintf(bufFree,"%0.2f", lushTrigger/1024.0/1024.0 );

			  (*infoStream) << "  RAM: now flush @ usedMB=" << bufUsed <<
								 " allocMB=" << bufAlloc <<
								 " triggerMB=" << bufTrig << "\n";
		  }

        bufferIsFull = true;
      }
    }
  }

}
CL_NS_END
