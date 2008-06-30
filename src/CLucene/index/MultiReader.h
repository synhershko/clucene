/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_index_MultiReader
#define _lucene_index_MultiReader


//#include "SegmentHeader.h"
#include "IndexReader.h"
CL_CLASS_DEF(document,Document)
//#include "Terms.h"
//#include "SegmentMergeQueue.h"

CL_NS_DEF(index)

class MultiReader:public IndexReader{
private:
    class Internal;
    Internal* internal;
	int32_t readerIndex(const int32_t n) const;
	bool hasNorms(const TCHAR* field);
	uint8_t* fakeNorms();
protected:
	void doSetNorm(int32_t n, const TCHAR* field, uint8_t value);
	void doUndeleteAll();
	void doCommit();
	// synchronized
	void doClose();
	
	// synchronized
	void doDelete(const int32_t n);
public:
	/** Construct reading the named set of readers. */
	MultiReader(CL_NS(store)::Directory* directory, SegmentInfos* sis, IndexReader** subReaders);

	/**
	* <p>Construct a MultiReader aggregating the named set of (sub)readers.
	* Directory locking for delete, undeleteAll, and setNorm operations is
	* left to the subreaders. </p>
	* <p>Note that all subreaders are closed if this Multireader is closed.</p>
	* @param subReaders set of (sub)readers
	* @throws IOException
	*/
	MultiReader(IndexReader** subReaders);

	~MultiReader();

	/** Return an array of term frequency vectors for the specified document.
	*  The array contains a vector for each vectorized field in the document.
	*  Each vector vector contains term numbers and frequencies for all terms
	*  in a given vectorized field.
	*  If no such fields existed, the method returns null.
	*/
	bool getTermFreqVectors(int32_t n, CL_NS(util)::Array<TermFreqVector*>& result);
	TermFreqVector* getTermFreqVector(int32_t n, const TCHAR* field);


	// synchronized
	int32_t numDocs();

	int32_t maxDoc() const;

	bool document(int32_t n, CL_NS(document)::Document* doc);

	bool isDeleted(const int32_t n);
	bool hasDeletions() const;

	// synchronized
	uint8_t* norms(const TCHAR* field);
	void norms(const TCHAR* field, uint8_t* result);

	TermEnum* terms() const;
	TermEnum* terms(const Term* term) const;
	
	//Returns the document frequency of the current term in the set
	int32_t docFreq(const Term* t=NULL) const;
	TermDocs* termDocs() const;
	TermPositions* termPositions() const;
	
		
	/**
	* @see IndexReader#getFieldNames(IndexReader.FieldOption fldOption)
	*/
	void getFieldNames(FieldOption fldOption, StringArrayWithDeletor& retarray);
};


CL_NS_END
#endif
