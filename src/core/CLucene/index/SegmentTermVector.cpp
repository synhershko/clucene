/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "_FieldInfos.h"
#include "_TermVector.h"
#include "CLucene/util/StringBuffer.h"
#include "CLucene/util/Array.h"

CL_NS_USE(util)
CL_NS_DEF(index)

ValueArray<int32_t> SegmentTermPositionVector::EMPTY_TERM_POS;

SegmentTermVector::SegmentTermVector(const TCHAR* _field, TCHAR** _terms, ValueArray<int32_t>* _termFreqs) {
	this->field = STRDUP_TtoT(_field); // TODO: Try and avoid this dup (using intern'ing perhaps?)
	this->terms = _terms;
	this->termsLen = -1; //lazily get the size of the terms array
	this->termFreqs = _termFreqs;
}

SegmentTermVector::~SegmentTermVector(){
  _CLDELETE_LCARRAY(field);
  _CLDELETE_LCARRAY_ALL(terms);
  _CLDELETE(termFreqs);
}
TermPositionVector* SegmentTermVector::__asTermPositionVector(){
	return NULL;
}

const TCHAR* SegmentTermVector::getField() {
	return field;
}

TCHAR* SegmentTermVector::toString() const{
	StringBuffer sb;
	sb.appendChar('{');
	sb.append(field);
	sb.append(_T(": "));

	int32_t i=0;
	while ( terms && terms[i] != NULL ){
		if (i>0) 
			sb.append(_T(", "));
		sb.append(terms[i]);
		sb.appendChar('/');

		sb.appendInt((*termFreqs)[i]);
	}
	sb.appendChar('}');
	return sb.toString();
}

int32_t SegmentTermVector::size() {
	if ( terms == NULL )
		return 0;

	if ( termsLen == -1 ){
		termsLen=0;
		while ( terms[termsLen] != 0 )
			termsLen++;
	}
	return termsLen;
}

const TCHAR** SegmentTermVector::getTerms() {
	return (const TCHAR**)terms;
}

const ValueArray<int32_t>* SegmentTermVector::getTermFrequencies() {
	return termFreqs;
}

int32_t SegmentTermVector::binarySearch(TCHAR** a, const int32_t arraylen, const TCHAR* key) const
{
	int32_t low = 0;
	int32_t hi = arraylen - 1;
	int32_t mid = 0;
	while (low <= hi)
	{
		mid = (low + hi) >> 1;

		int32_t c = _tcscmp(a[mid],key);
		if (c==0)
			return mid;
		else if (c > 0)
			hi = mid - 1;
		else // This gets the insertion point right on the last loop.
			low = ++mid;
	}
	return -mid - 1;
}

int32_t SegmentTermVector::indexOf(const TCHAR* termText) {
	if(terms == NULL)
		return -1;
	int32_t res = binarySearch(terms, size(), termText);
	return res >= 0 ? res : -1;
}

ValueArray<int32_t>* SegmentTermVector::indexesOf(const TCHAR** termNumbers, const int32_t start, const int32_t len) {
	// TODO: there must be a more efficient way of doing this.
	//       At least, we could advance the lower bound of the terms array
	//       as we find valid indexes. Also, it might be possible to leverage
	//       this even more by starting in the middle of the termNumbers array
	//       and thus dividing the terms array maybe in half with each found index.
	ValueArray<int32_t>* ret = _CLNEW ValueArray<int32_t>(len);
	for (int32_t i=0; i<len; ++i) {
	  ret->values[i] = indexOf(termNumbers[start+ i]);
	}
	return ret;
}
void SegmentTermVector::indexesOf(const TCHAR** terms, const int32_t start, const int32_t len, ValueArray<int32_t>& ret){
	ret = *indexesOf(terms,start,len);
}


    
SegmentTermPositionVector::SegmentTermPositionVector(const TCHAR* field, TCHAR** terms, ValueArray<int32_t>* termFreqs, 
  ArrayBase< ValueArray<int32_t>* >* _positions, 
  ArrayBase< ArrayBase<TermVectorOffsetInfo*>* >* _offsets)
	: SegmentTermVector(field,terms,termFreqs),
  offsets(_offsets),
  positions(_positions)
{
}
SegmentTermPositionVector::~SegmentTermPositionVector(){
	_CLLDELETE(offsets);
	_CLLDELETE(positions);
}

ValueArray<int32_t>* SegmentTermPositionVector::indexesOf(const TCHAR** termNumbers, const int32_t start, const int32_t len)
	{ return SegmentTermVector::indexesOf(termNumbers, start, len); }

TermPositionVector* SegmentTermPositionVector::__asTermPositionVector(){
	return this;
}

ArrayBase<TermVectorOffsetInfo*>* SegmentTermPositionVector::getOffsets(const size_t index) {
	if(offsets == NULL)
		return NULL;
	if (index >=0 && index < offsets->length)
		return offsets->values[index];
	else
		return TermVectorOffsetInfo_EMPTY_OFFSET_INFO;
}

ValueArray<int32_t>* SegmentTermPositionVector::getTermPositions(const size_t index) {
	if(positions == NULL)
		return NULL;

	if (index >=0 && index < positions->length)
		return positions->values[index];
	else
		return &EMPTY_TERM_POS;
}

void SegmentTermPositionVector::indexesOf(const TCHAR** termNumbers, const int32_t start, const int32_t len, CL_NS(util)::ValueArray<int32_t>& ret)
{
	ret = *indexesOf(termNumbers,start,len);
}

CL_NS_END

