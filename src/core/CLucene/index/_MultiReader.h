/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_index_internal_MultiReader_
#define _lucene_index_internal_MultiReader_

#include "Terms.h"
#include "CLucene/util/PriorityQueue.h"
#include "_SegmentMergeInfo.h"
#include "_SegmentMergeQueue.h"

CL_NS_DEF(index)

/** An IndexReader which reads multiple indexes, appending their content.
*/
class MultiTermDocs:public virtual TermDocs {
protected:
	TermDocs** readerTermDocs;

	IndexReader** subReaders;
  int32_t subReadersLength;
	const int32_t* starts;
	Term* term;

	int32_t base;
	int32_t pointer;

	TermDocs* current;              // == segTermDocs[pointer]
	TermDocs* termDocs(const int32_t i) const; //< internal use only
	virtual TermDocs* termDocs(const IndexReader* reader) const;
public:
	MultiTermDocs();
	MultiTermDocs(IndexReader** subReaders, const int32_t* s);
	virtual ~MultiTermDocs();

	int32_t doc() const;
	int32_t freq() const;

    void seek(TermEnum* termEnum);
	void seek(Term* tterm);
	bool next();

	/** Optimized implementation. */
	int32_t read(int32_t* docs, int32_t* freqs, int32_t length);

	/** As yet unoptimized implementation. */
	bool skipTo(const int32_t target);

	void close();

	virtual TermPositions* __asTermPositions();
};


//MultiTermEnum represents the enumeration of all terms of all readers
class MultiTermEnum:public TermEnum {
private:
	SegmentMergeQueue* queue;

	Term* _term;
	int32_t _docFreq;
public:
	//Constructor
	//Opens all enumerations of all readers
	MultiTermEnum(IndexReader** subReaders, const int32_t* starts, const Term* t);

	//Destructor
	~MultiTermEnum();

	//Move the current term to the next in the set of enumerations
	bool next();

	//Returns a pointer to the current term of the set of enumerations
	Term* term();
	Term* term(bool pointer);

	//Returns the document frequency of the current term in the set
	int32_t docFreq() const;

	//Closes the set of enumerations in the queue
	void close();

		
	const char* getObjectName(){ return MultiTermEnum::getClassName(); }
	static const char* getClassName(){ return "MultiTermEnum"; }
};


#if _MSC_VER
    #pragma warning(disable : 4250)
#endif
class MultiTermPositions:public MultiTermDocs,public TermPositions {
protected:
	TermDocs* termDocs(const IndexReader* reader) const;
public:
	MultiTermPositions(IndexReader** subReaders, const int32_t* s);
	~MultiTermPositions() {};
	int32_t nextPosition();

	/**
	* Not implemented.
	* @throws UnsupportedOperationException
	*/
	int32_t getPayloadLength() const {
		_CLTHROWA(CL_ERR_UnsupportedOperation,"UnsupportedOperationException: MultiTermPositions::getPayloadLength");
	}

	/**
	* Not implemented.
	* @throws UnsupportedOperationException
	*/
	uint8_t* getPayload(uint8_t* data, const int32_t offset) {
		_CLTHROWA(CL_ERR_UnsupportedOperation,"UnsupportedOperationException: MultiTermPositions::getPayload");
	}

	/**
	*
	* @return false
	*/
	// TODO: Remove warning after API has been finalized
	bool isPayloadAvailable() const {
		return false;
	}

	virtual TermDocs* __asTermDocs();
	virtual TermPositions* __asTermPositions();
};
CL_NS_END

#endif
