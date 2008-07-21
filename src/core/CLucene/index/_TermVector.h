/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_index_internal_termvector_h
#define _lucene_index_internal_termvector_h

#include "CLucene/util/Array.h"
#include "_FieldInfos.h"
#include "TermVector.h"
//#include "FieldInfos.h"

CL_NS_DEF(index)

/**
* Writer works by opening a document and then opening the fields within the document and then
* writing out the vectors for each field.
* 
* Rough usage:
*
<CODE>
for each document
{
writer.openDocument();
for each field on the document
{
writer.openField(field);
for all of the terms
{
writer.addTerm(...)
}
writer.closeField
}
writer.closeDocument()    
}
</CODE>
*/
class TermVectorsWriter:LUCENE_BASE {
private:
	class TVField:LUCENE_BASE {
	public:
		int32_t number;
		int64_t tvfPointer;
		int32_t length;   // number of distinct term positions
		bool storePositions;
		bool storeOffsets;
	 
		TVField(int32_t number, bool storePos, bool storeOff): 
				tvfPointer(0),length(0){
			this->number = number;
			this->storePositions = storePos;
			this->storeOffsets = storeOff;
		}
        ~TVField(){}
	};

	class TVTerm:LUCENE_BASE {
		const TCHAR* termText;
		int32_t termTextLen; //textlen cache
		
	public:
		TVTerm();
        ~TVTerm();
		
		int32_t freq;
		CL_NS(util)::Array<int32_t>* positions;
		CL_NS(util)::Array<TermVectorOffsetInfo>* offsets;

		const TCHAR* getTermText() const;
		size_t getTermTextLen();
		void setTermText(const TCHAR* val);
	};


	CL_NS(store)::IndexOutput* tvx, *tvd, *tvf;
	CL_NS(util)::CLVector<TVField*,CL_NS(util)::Deletor::Object<TVField> > fields;
	CL_NS(util)::CLVector<TVTerm*,CL_NS(util)::Deletor::Object<TVTerm> > terms;
	FieldInfos* fieldInfos;

	TVField* currentField;
	int64_t currentDocPointer;

	void addTermInternal(const TCHAR* termText, const int32_t freq, 
		CL_NS(util)::Array<int32_t>* positions, CL_NS(util)::Array<TermVectorOffsetInfo>* offsets);

	void writeField();
	void writeDoc();
  
	void openField(int32_t fieldNumber, bool storePositionWithTermVector, 
      bool storeOffsetWithTermVector);
public:
	LUCENE_STATIC_CONSTANT(int32_t, FORMAT_VERSION = 2);

	//The size in bytes that the FORMAT_VERSION will take up at the beginning of each file 
	LUCENE_STATIC_CONSTANT(int32_t, FORMAT_SIZE = 4);

	LUCENE_STATIC_CONSTANT(uint8_t, STORE_POSITIONS_WITH_TERMVECTOR = 0x1);
	LUCENE_STATIC_CONSTANT(uint8_t, STORE_OFFSET_WITH_TERMVECTOR = 0x2);
	
	static const char* LUCENE_TVX_EXTENSION;
	static const char* LUCENE_TVD_EXTENSION;
	static const char* LUCENE_TVF_EXTENSION;

	TermVectorsWriter(CL_NS(store)::Directory* directory, const char* segment,
						   FieldInfos* fieldInfos);

	~TermVectorsWriter();
	void openDocument();
	void closeDocument();

	/** Close all streams. */
	void close();
	bool isDocumentOpen() const;

	/** Start processing a field. This can be followed by a number of calls to
	*  addTerm, and a final call to closeField to indicate the end of
	*  processing of this field. If a field was previously open, it is
	*  closed automatically.
	*/
	void openField(const TCHAR* field);

	/** Finished processing current field. This should be followed by a call to
	*  openField before future calls to addTerm.
	*/
	void closeField();

	/** Return true if a field is currently open. */
	bool isFieldOpen() const;

	/**
	* Add a complete document specified by all its term vectors. If document has no
	* term vectors, add value for tvx.
	* 
	* @param vectors
	* @throws IOException
	*/
	void addAllDocVectors(CL_NS(util)::Array<TermFreqVector*>& vectors);

	/** Add term to the field's term vector. Field must already be open.
	*  Terms should be added in
	*  increasing order of terms, one call per unique termNum. ProxPointer
	*  is a pointer into the TermPosition file (prx). Freq is the number of
	*  times this term appears in this field, in this document.
	* @throws IllegalStateException if document or field is not open
	*/
	void addTerm(const TCHAR* termText, int32_t freq, 
		CL_NS(util)::Array<int32_t>* positions = NULL, CL_NS(util)::Array<TermVectorOffsetInfo>* offsets = NULL);
};

/**
 */
class SegmentTermVector: public virtual TermFreqVector {
private:
	const TCHAR* field;
	TCHAR** terms;
	int32_t termsLen; //cache
	CL_NS(util)::Array<int32_t>* termFreqs;

	int32_t binarySearch(TCHAR** a, const int32_t arraylen, const TCHAR* key) const;
public:
	//note: termFreqs must be the same length as terms
	SegmentTermVector(const TCHAR* field, TCHAR** terms, CL_NS(util)::Array<int32_t>* termFreqs);
	virtual ~SegmentTermVector();

	/**
	* 
	* @return The number of the field this vector is associated with
	*/
	const TCHAR* getField();
	TCHAR* toString() const;
	int32_t size();
	const TCHAR** getTerms();
	const CL_NS(util)::Array<int32_t>* getTermFrequencies();
	int32_t indexOf(const TCHAR* termText);
	void indexesOf(const TCHAR** termNumbers, const int32_t start, const int32_t len, CL_NS(util)::Array<int32_t>& ret);

	virtual TermPositionVector* __asTermPositionVector();
};



/**
* @version $Id:
*/
class TermVectorsReader:LUCENE_BASE {
private:
    FieldInfos* fieldInfos;
    
    CL_NS(store)::IndexInput* tvx;
    CL_NS(store)::IndexInput* tvd;
    CL_NS(store)::IndexInput* tvf;
    int64_t _size;
    
    int32_t tvdFormat;
    int32_t tvfFormat;
    
    
    int32_t checkValidFormat(CL_NS(store)::IndexInput* in);
    
	void readTermVectors(const TCHAR** fields, const int64_t* tvfPointers, const int32_t len, CL_NS(util)::Array<TermFreqVector*>& _return);

    /**
    * 
    * @param field The field to read in
    * @param tvfPointer The pointer within the tvf file where we should start reading
    * @return The TermVector located at that position
    * @throws IOException
    */
    SegmentTermVector* readTermVector(const TCHAR* field, const int64_t tvfPointer);

    int64_t size();
  
  
	DEFINE_MUTEX(THIS_LOCK)
	TermVectorsReader(const TermVectorsReader& copy);
public:
	TermVectorsReader(CL_NS(store)::Directory* d, const char* segment, FieldInfos* fieldInfos);
	~TermVectorsReader();

	void close();
	TermVectorsReader* clone() const;

	/**
	* Retrieve the term vector for the given document and field
	* @param docNum The document number to retrieve the vector for
	* @param field The field within the document to retrieve
	* @return The TermFreqVector for the document and field or null if there is no termVector for this field.
	* @throws IOException if there is an error reading the term vector files
	*/ 
	TermFreqVector* get(const int32_t docNum, const TCHAR* field);


	/**
	* Return all term vectors stored for this document or null if the could not be read in.
	* 
	* @param docNum The document number to retrieve the vector for
	* @return All term frequency vectors
	* @throws IOException if there is an error reading the term vector files 
	*/
	bool get(int32_t docNum, CL_NS(util)::Array<TermFreqVector*>& result);
};


class SegmentTermPositionVector: public SegmentTermVector, public TermPositionVector {
protected:
	CL_NS(util)::Array< CL_NS(util)::Array<int32_t> >* positions;
	CL_NS(util)::Array< CL_NS(util)::Array<TermVectorOffsetInfo> >* offsets;
	static CL_NS(util)::Array<int32_t> EMPTY_TERM_POS;
public:
	SegmentTermPositionVector(const TCHAR* field, TCHAR** terms, CL_NS(util)::Array<int32_t>* termFreqs, CL_NS(util)::Array< CL_NS(util)::Array<int32_t> >* positions, CL_NS(util)::Array< CL_NS(util)::Array<TermVectorOffsetInfo> >* offsets);
	~SegmentTermPositionVector();

	/**
	* Returns an array of TermVectorOffsetInfo in which the term is found.
	*
	* @param index The position in the array to get the offsets from
	* @return An array of TermVectorOffsetInfo objects or the empty list
	* @see org.apache.lucene.analysis.Token
	*/
	CL_NS(util)::Array<TermVectorOffsetInfo>* getOffsets(const size_t index);

	/**
	* Returns an array of positions in which the term is found.
	* Terms are identified by the index at which its number appears in the
	* term String array obtained from the <code>indexOf</code> method.
	*/
	CL_NS(util)::Array<int32_t>* getTermPositions(const size_t index);

	const TCHAR* getField(){ return SegmentTermVector::getField(); }
	TCHAR* toString() const{ return SegmentTermVector::toString(); }
	int32_t size(){ return SegmentTermVector::size(); }
	const TCHAR** getTerms(){ return SegmentTermVector::getTerms(); }
	const CL_NS(util)::Array<int32_t>* getTermFrequencies(){ return SegmentTermVector::getTermFrequencies(); }
	int32_t indexOf(const TCHAR* termText){ return SegmentTermVector::indexOf(termText); }
	void indexesOf(const TCHAR** termNumbers, const int32_t start, const int32_t len, CL_NS(util)::Array<int32_t>& ret);

	virtual TermPositionVector* __asTermPositionVector();
};

CL_NS_END
#endif
