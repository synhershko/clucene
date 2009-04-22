/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "MultiReader.h"
#include "_MultiReader.h"

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
    bool _hasDeletions;
	uint8_t* ones;
	MultiReader* _this;

	CL_NS(util)::CLHashtable<const TCHAR*,uint8_t*,
		CL_NS(util)::Compare::TChar,
			CL_NS(util)::Equals::TChar,
		CL_NS(util)::Deletor::tcArray,
		CL_NS(util)::Deletor::vArray<uint8_t> > normsCache;
	int32_t _maxDoc;
	int32_t _numDocs;
	
	Internal(CL_NS(util)::ObjectArray<IndexReader>* subReaders, MultiReader* _this, bool closeSubReaders):
  		normsCache(true, true)
	{
		this->_this = _this;
		_this->subReaders = subReaders;

	  _maxDoc        = 0;
	  _numDocs       = -1;
	  ones           = NULL;
	
	  _this->starts = _CL_NEWARRAY(int32_t, _this->subReaders->length + 1);    // build starts array
	  decrefOnClose = new boolean[subReaders.length];
    for (int32_t i = 0; i < _this->subReaders->length; i++) {
	     _this->starts[i] = _maxDoc;
	
	     // compute maxDocs
	     _maxDoc += _this->subReaders[i]->maxDoc();

        if (!closeSubReaders) {
          subReaders[i].incRef();
          decrefOnClose[i] = true;
        } else {
          decrefOnClose[i] = false;
        }

	     if (_this->subReaders[i]->hasDeletions())
	        _hasDeletions = true;
	  }
	  _this->starts[_this->subReaders->length] = _maxDoc;
	}
	~Internal(){
		_CLDELETE_ARRAY(ones);
		_CLDELETE_ARRAY(_this->starts);
		
		//Iterate through the subReaders and destroy each reader
		if (_this->subReaders && _this->subReaders->length > 0) {
			for (int32_t i = 0; i < _this->subReaders->length; i++) {
				_CLDELETE(_this->subReaders[i]);
			}
		}
		//Destroy the subReaders array
		_CLDELETE_ARRAY(_this->subReaders);
	}
};

MultiReader::MultiReader(CL_NS(util)::ObjectArray<IndexReader>* subReaders, bool closeSubReaders):
  IndexReader(subReaders == NULL || subReaders[0] == NULL ? NULL : subReaders[0]->getDirectory())
{
	this->internal = _CLNEW Internal(subReaders, this, closeSubReaders);
}

MultiReader::MultiReader(Directory* directory, SegmentInfos* sis, IndexReader** subReaders):
	IndexReader(directory, sis, false)
{
	this->internal = _CLNEW Internal(subReaders, this);
}


MultiReader::~MultiReader() {
//Func - Destructor
//Pre  - true
//Post - The instance has been destroyed all IndexReader instances
//       this instance managed have been destroyed to

	_CLDELETE(internal);
}

ObjectArray<TermFreqVector>* MultiReader::getTermFreqVectors(int32_t n){
    ensureOpen();
	int32_t i = readerIndex(n);        // find segment num
	return subReaders[i]->getTermFreqVectors(n - starts[i]); // dispatch to segment
}

TermFreqVector* MultiReader::getTermFreqVector(int32_t n, const TCHAR* field){
    ensureOpen();
	int32_t i = readerIndex(n);        // find segment num
	return subReaders[i]->getTermFreqVector(n - starts[i], field);
}


int32_t MultiReader::numDocs() {
	SCOPED_LOCK_MUTEX(THIS_LOCK)
    // Don't call ensureOpen() here (it could affect performance)
	if (internal->_numDocs == -1) {			  // check cache
	  int32_t n = 0;				  // cache miss--recompute
	  for (int32_t i = 0; i < subReaders->length; i++)
	    n += subReaders[i]->numDocs();		  // sum from readers
	  internal->_numDocs = n;
	}
	return internal->_numDocs;
}

int32_t MultiReader::maxDoc() const {
    // Don't call ensureOpen() here (it could affect performance)
	return internal->_maxDoc;
}

bool MultiReader::document(int32_t n, CL_NS(document)::Document& doc, FieldSelector* fieldSelector){
	ensureOpen();
    int32_t i = readerIndex(n);			  // find segment num
	return subReaders[i]->document(n - starts[i],doc, fieldSelector);	  // dispatch to segment reader
}

bool MultiReader::isDeleted(const int32_t n) {
    // Don't call ensureOpen() here (it could affect performance)
	int32_t i = readerIndex(n);			  // find segment num
	return subReaders[i]->isDeleted(n - starts[i]);	  // dispatch to segment reader
}

bool MultiReader::hasDeletions() const{
    // Don't call ensureOpen() here (it could affect performance)
    return internal->_hasDeletions; 
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
	  subReaders[i]->norms(field, bytes + starts[i]);
	
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
	  subReaders[i]->norms(field, result + starts[i]);
}


void MultiReader::doSetNorm(int32_t n, const TCHAR* field, uint8_t value){
	internal->normsCache.remove(field);                         // clear cache
	int32_t i = readerIndex(n);                           // find segment num
	subReaders[i]->setNorm(n-starts[i], field, value); // dispatch
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
	  total += subReaders[i]->docFreq(t);
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
	internal->_numDocs = -1;				  // invalidate cache
	int32_t i = readerIndex(n);			  // find segment num
	subReaders[i]->deleteDocument(n - starts[i]);		  // dispatch to segment reader
	internal->_hasDeletions = true;
}

int32_t MultiReader::readerIndex(const int32_t n) const {	  // find reader for doc n:
    return MultiSegmentReader.readerIndex(n, this.starts, this.subReaders.length);
}

bool MultiReader::hasNorms(const TCHAR* field) {
    ensureOpen();
	for (int i = 0; i < subReaders->length; i++) {
		if (subReaders[i]->hasNorms(field)) 
			return true;
	}
	return false;
}
uint8_t* MultiReader::fakeNorms() {
	if (internal->ones==NULL) 
		internal->ones=SegmentReader::createFakeNorms(maxDoc());
	return internal->ones;
}

void MultiReader::doUndeleteAll(){
	for (int32_t i = 0; i < subReaders->length; i++)
		subReaders[i]->undeleteAll();
	internal->_hasDeletions = false;
	internal->_numDocs = -1;
}
void MultiReader::doCommit() {
	for (int32_t i = 0; i < subReaders->length; i++)
	  subReaders[i]->commit();
}

void MultiReader::doClose() {
	SCOPED_LOCK_MUTEX(THIS_LOCK)
	for (int32_t i = 0; i < subReaders->length; i++){
    if (decrefOnClose[i]) {
        subReaders[i].decRef();
      } else {
        subReaders[i]->close();
      }
    }
	}
}


void MultiReader::getFieldNames(FieldOption fldOption, StringArrayWithDeletor& retarray){
    ensureOpen();
    return MultiSegmentReader.getFieldNames(fieldNames, this.subReaders);
}

CL_NS_END
