/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "MultiReader.h"
#include "_MultiSegmentReader.h"

#include "IndexReader.h"
#include "CLucene/document/Document.h"
#include "Term.h"
#include "Terms.h"
#include "CLucene/util/PriorityQueue.h"
#include "_SegmentHeader.h"
#include "_SegmentMergeInfo.h"
#include "_SegmentMergeQueue.h"

CL_NS_USE(store)
CL_NS_USE(util)
CL_NS_DEF(index)



class MultiReader::Internal: LUCENE_BASE{
public:
	CL_NS(util)::CLHashtable<const TCHAR*,uint8_t*,
		CL_NS(util)::Compare::TChar,
			CL_NS(util)::Equals::TChar,
		CL_NS(util)::Deletor::tcArray,
		CL_NS(util)::Deletor::vArray<uint8_t> > normsCache;
	
	Internal():
  		normsCache(true, true)
	{
	}
	~Internal(){
	}
};

MultiReader::MultiReader(CL_NS(util)::ObjectArray<IndexReader>* subReaders, bool closeSubReaders)
{
	this->internal = _CLNEW Internal();
  this->init(subReaders, closeSubReaders);
}

void MultiReader::init(CL_NS(util)::ObjectArray<IndexReader>* subReaders, bool closeSubReaders){
  subReaders = subReaders;

  _maxDoc        = 0;
  _numDocs       = -1;
  ones           = NULL;

  starts = _CL_NEWARRAY(int32_t, subReaders->length + 1);    // build starts array
  decrefOnClose = _CL_NEWARRAY(bool, subReaders->length);
  for (int32_t i = 0; i < subReaders->length; i++) {
      starts[i] = _maxDoc;

      // compute maxDocs
      _maxDoc += (*subReaders)[i]->maxDoc();

      if (!closeSubReaders) {
        (*subReaders)[i]->incRef();
        decrefOnClose[i] = true;
      } else {
        decrefOnClose[i] = false;
      }

      if ((*subReaders)[i]->hasDeletions())
        _hasDeletions = true;
  }
  starts[subReaders->length] = _maxDoc;
}

MultiReader::~MultiReader() {
//Func - Destructor
//Pre  - true
//Post - The instance has been destroyed all IndexReader instances
//       this instance managed have been destroyed to

	_CLDELETE(internal);
  _CLDELETE_ARRAY(ones);
  _CLDELETE_ARRAY(starts);
  _CLDELETE_ARRAY(decrefOnClose);
  subReaders->deleteAll();
}

/*
IndexReader* MultiReader::reopen() {
  ensureOpen();

  bool reopened = false;
  IndexReader[] newSubReaders = new IndexReader[subReaders->length];
  bool[] newDecrefOnClose = new bool[subReaders->length];

  bool success = false;
  try {
    for (int32_t i = 0; i < subReaders->length; i++) {
      newSubReaders[i] = (*subReaders)[i].reopen();
      // if at least one of the subreaders was updated we remember that
      // and return a new MultiReader
      if (newSubReaders[i] != (*subReaders)[i]) {
        reopened = true;
        // this is a new subreader instance, so on close() we don't
        // decRef but close it
        newDecrefOnClose[i] = false;
//TODO: cleanup memory
      }
    }

    if (reopened) {
      for (int32_t i = 0; i < subReaders->length; i++) {
        if (newSubReaders[i] == (*subReaders)[i]) {
          newSubReaders[i].incRef();
          newDecrefOnClose[i] = true;
        }
      }

      MultiReader mr = new MultiReader(newSubReaders);
      mr.decrefOnClose = newDecrefOnClose;
      success = true;
      return mr;
    } else {
      success = true;
      return this;
    }
  } finally {
    if (!success && reopened) {
      for (int32_t i = 0; i < newSubReaders.length; i++) {
        if (newSubReaders[i] != null) {
          try {
            if (newDecrefOnClose[i]) {
              newSubReaders[i].decRef();
            } else {
              newSubReaders[i].close();
            }
          } catch (IOException ignore) {
            // keep going - we want to clean up as much as possible
          }
        }
      }
    }
  }
}
*/
ObjectArray<TermFreqVector>* MultiReader::getTermFreqVectors(int32_t n){
    ensureOpen();
	int32_t i = readerIndex(n);        // find segment num
	return (*subReaders)[i]->getTermFreqVectors(n - starts[i]); // dispatch to segment
}

TermFreqVector* MultiReader::getTermFreqVector(int32_t n, const TCHAR* field){
    ensureOpen();
	int32_t i = readerIndex(n);        // find segment num
	return (*subReaders)[i]->getTermFreqVector(n - starts[i], field);
}

void MultiReader::getTermFreqVector(int32_t docNumber, const TCHAR* field, TermVectorMapper* mapper) {
  ensureOpen();
  int32_t i = readerIndex(docNumber);        // find segment num
  (*subReaders)[i].getTermFreqVector(docNumber - starts[i], field, mapper);
}

void MultiReader::getTermFreqVector(int32_t docNumber, TermVectorMapper* mapper) {
  ensureOpen();
  int32_t i = readerIndex(docNumber);        // find segment num
  (*subReaders)[i].getTermFreqVector(docNumber - starts[i], mapper);
}

bool MultiReader::isOptimized() {
  return false;
}


int32_t MultiReader::numDocs() {
	SCOPED_LOCK_MUTEX(THIS_LOCK)
    // Don't call ensureOpen() here (it could affect performance)
	if (_numDocs == -1) {			  // check cache
	  int32_t n = 0;				  // cache miss--recompute
	  for (int32_t i = 0; i < subReaders->length; i++)
	    n += (*subReaders)[i]->numDocs();		  // sum from readers
	  _numDocs = n;
	}
	return _numDocs;
}

int32_t MultiReader::maxDoc() const {
    // Don't call ensureOpen() here (it could affect performance)
	return _maxDoc;
}

bool MultiReader::document(int32_t n, CL_NS(document)::Document& doc, FieldSelector* fieldSelector){
	ensureOpen();
    int32_t i = readerIndex(n);			  // find segment num
	return (*subReaders)[i]->document(n - starts[i],doc, fieldSelector);	  // dispatch to segment reader
}

bool MultiReader::isDeleted(const int32_t n) {
    // Don't call ensureOpen() here (it could affect performance)
	int32_t i = readerIndex(n);			  // find segment num
	return (*subReaders)[i]->isDeleted(n - starts[i]);	  // dispatch to segment reader
}

bool MultiReader::hasDeletions() const{
    // Don't call ensureOpen() here (it could affect performance)
    return _hasDeletions;
}

uint8_t* MultiReader::norms(const TCHAR* field){
	SCOPED_LOCK_MUTEX(THIS_LOCK)
    ensureOpen();
	uint8_t* bytes;
	bytes = internal->normsCache.get(field);
	if (bytes != NULL){
	  return bytes;				  // cache hit
	}
	
	if ( !hasNorms(field) )
		return fakeNorms();
	
	bytes = _CL_NEWARRAY(uint8_t,maxDoc());
	for (int32_t i = 0; i < subReaders->length; i++)
	  (*subReaders)[i]->norms(field, bytes + starts[i]);
	
	//Unfortunately the data in the normCache can get corrupted, since it's being loaded with string
	//keys that may be deleted while still in use by the map. To prevent this field is duplicated
	//and then stored in the normCache
	TCHAR* key = STRDUP_TtoT(field);
	//update cache
	internal->normsCache.put(key, bytes);
	
	return bytes;
}

void MultiReader::norms(const TCHAR* field, uint8_t* result) {
	SCOPED_LOCK_MUTEX(THIS_LOCK)
    ensureOpen();
	uint8_t* bytes = internal->normsCache.get(field);
	if (bytes==NULL && !hasNorms(field)) 
		bytes=fakeNorms();
    
	if (bytes != NULL){                            // cache hit
	   int32_t len = maxDoc();
	   memcpy(result,bytes,len * sizeof(int32_t));
	}
	
	for (int32_t i = 0; i < subReaders->length; i++)      // read from segments
	  (*subReaders)[i]->norms(field, result + starts[i]);
}


void MultiReader::doSetNorm(int32_t n, const TCHAR* field, uint8_t value){
	internal->normsCache.remove(field);                         // clear cache
	int32_t i = readerIndex(n);                           // find segment num
	(*subReaders)[i]->setNorm(n-starts[i], field, value); // dispatch
}

TermEnum* MultiReader::terms() const {
    ensureOpen();
	return _CLNEW MultiTermEnum(subReaders, starts, NULL);
}

TermEnum* MultiReader::terms(const Term* term) const {
    ensureOpen();
	return _CLNEW MultiTermEnum(subReaders, starts, term);
}

int32_t MultiReader::docFreq(const Term* t) const {
    ensureOpen();
	int32_t total = 0;				  // sum freqs in Multi
	for (int32_t i = 0; i < subReaders->length; i++)
	  total += (*subReaders)[i]->docFreq(t);
	return total;
}

TermDocs* MultiReader::termDocs() const {
    ensureOpen();
	TermDocs* ret =  _CLNEW MultiTermDocs(subReaders, starts);
	return ret;
}

TermPositions* MultiReader::termPositions() const {
    ensureOpen();
	TermPositions* ret = (TermPositions*)_CLNEW MultiTermPositions(subReaders, starts);
	return ret;
}

void MultiReader::doDelete(const int32_t n) {
	_numDocs = -1;				  // invalidate cache
	int32_t i = readerIndex(n);			  // find segment num
	(*subReaders)[i]->deleteDocument(n - starts[i]);		  // dispatch to segment reader
	internal->_hasDeletions = true;
}

int32_t MultiReader::readerIndex(const int32_t n) const {	  // find reader for doc n:
    return MultiSegmentReader.readerIndex(n, this.starts, this.subReaders->length);
}

bool MultiReader::hasNorms(const TCHAR* field) {
    ensureOpen();
	for (int32_t i = 0; i < subReaders->length; i++) {
		if ((*subReaders)[i]->hasNorms(field))
			return true;
	}
	return false;
}
uint8_t* MultiReader::fakeNorms() {
	if (ones==NULL)
		internal->ones=SegmentReader::createFakeNorms(maxDoc());
	return internal->ones;
}

void MultiReader::doUndeleteAll(){
	for (int32_t i = 0; i < subReaders->length; i++)
		(*subReaders)[i]->undeleteAll();
	_hasDeletions = false;
	_numDocs = -1;
}
void MultiReader::doCommit() {
	for (int32_t i = 0; i < subReaders->length; i++)
	  (*subReaders)[i]->commit();
}

void MultiReader::doClose() {
	SCOPED_LOCK_MUTEX(THIS_LOCK)
	for (int32_t i = 0; i < subReaders->length; i++){
    if (decrefOnClose[i]) {
        (*subReaders)[i].decRef();
      } else {
        (*subReaders)[i]->close();
      }
    }
	}
}


void MultiReader::getFieldNames(FieldOption fldOption, StringArrayWithDeletor& retarray){
    ensureOpen();
    return MultiSegmentReader.getFieldNames(fieldNames, this.subReaders);
}

public bool MultiReader::isCurrent() throws CorruptIndexException, IOException {
    for (int32_t i = 0; i < subReaders->length; i++) {
      if (!(*subReaders)[i].isCurrent()) {
        return false;
      }
    }

    // all subreaders are up to date
    return true;
  }

  /** Not implemented.
   * @throws UnsupportedOperationException
   */
  public long MultiReader::getVersion() {
    throw new UnsupportedOperationException("MultiReader does not support this method.");
  }


CL_NS_END
