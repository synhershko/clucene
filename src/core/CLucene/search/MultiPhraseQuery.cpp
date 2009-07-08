/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "MultiPhraseQuery.h"
#include "SearchHeader.h"

#include "BooleanClause.h"
#include "BooleanQuery.h"
#include "TermQuery.h"
#include "Explanation.h"
#include "Similarity.h"

#include "CLucene/index/_Term.h"
#include "CLucene/index/Term.h"
#include "CLucene/index/Terms.h"
#include "CLucene/index/IndexReader.h"
#include "CLucene/index/MultipleTermPositions.h"

#include "CLucene/util/StringBuffer.h"
#include "CLucene/util/VoidList.h"
#include "CLucene/util/_Arrays.h"

#include "_ExactPhraseScorer.h"
#include "_SloppyPhraseScorer.h"

CL_NS_USE(index)
CL_NS_USE(util)

CL_NS_DEF(search)

class MultiPhraseWeight : public Weight {
private:
	Similarity* similarity;
    float_t value;
    float_t idf;
    float_t queryNorm;
    float_t queryWeight;

	MultiPhraseQuery* parentQuery;

public:
	MultiPhraseWeight(Searcher* searcher, MultiPhraseQuery* _parentQuery) : similarity(_parentQuery->getSimilarity(searcher)),
		value(0), idf(0), queryNorm(0), queryWeight(0), parentQuery(_parentQuery) {

		// compute idf
		for (size_t i = 0; i < parentQuery->termArrays->size(); i++){
			Term** terms = parentQuery->termArrays->at(i);
			size_t j = 0;
			while ( terms[j] != NULL ) {
				idf += parentQuery->getSimilarity(searcher)->idf(terms[j], searcher);
				++j;
			}
		}
	}
	virtual ~MultiPhraseWeight(){};

    Query* getQuery() { return parentQuery; }
    float_t getValue() { return value; }

    float_t sumOfSquaredWeights() {
      queryWeight = idf * parentQuery->getBoost();             // compute query weight
      return queryWeight * queryWeight;           // square it
    }

    void normalize(float_t _queryNorm) {
      this->queryNorm = _queryNorm;
      queryWeight *= _queryNorm;                   // normalize query weight
      value = queryWeight * idf;                  // idf for document
    }

	Scorer* scorer(IndexReader* reader) {
		const size_t termArraysSize = parentQuery->termArrays->size();
		if (termArraysSize == 0)                  // optimize zero-term case
			return NULL;

		TermPositions** tps = _CL_NEWARRAY(TermPositions*,termArraysSize+1);
		for (size_t i=0; i<termArraysSize; i++) {
			Term** terms = parentQuery->termArrays->at(i);

			TermPositions* p;
			if (terms[1] != NULL) // terms.length > 1
				p = _CLNEW MultipleTermPositions(reader, terms);
			else
				p = reader->termPositions(terms[0]);

			if (p == NULL)
				return NULL;

			tps[i] = p;
		}
		tps[termArraysSize] = NULL;

		Scorer* ret = NULL;

		ValueArray<int32_t> positions;
		parentQuery->getPositions(positions);
		const int32_t slop = parentQuery->getSlop();
		if (slop == 0)
			ret = _CLNEW ExactPhraseScorer(this, tps, positions.values, similarity,
																reader->norms(parentQuery->field));
		else
			ret = _CLNEW SloppyPhraseScorer(this, tps, positions.values, similarity,
															slop, reader->norms(parentQuery->field));

		positions.deleteArray();

		//tps can be deleted safely. SloppyPhraseScorer or ExactPhraseScorer will take care
		//of its values
		_CLDELETE_LARRAY(tps);

		return ret;
	}

	Explanation* explain(IndexReader* reader, int32_t doc){
		ComplexExplanation* result = _CLNEW ComplexExplanation();

		StringBuffer buf(100);
		buf.append(_T("weight("));
		TCHAR* queryString = getQuery()->toString();
		buf.append(queryString);
		buf.append(_T(" in "));
		buf.appendInt(doc);
		buf.append(_T("), product of:"));
		result->setDescription(buf.getBuffer());
		buf.clear();

		buf.append(_T("idf("));
		buf.append(queryString);
		buf.appendChar(_T(')'));
		Explanation* idfExpl = _CLNEW Explanation(idf, buf.getBuffer());
		buf.clear();

		// explain query weight
		Explanation* queryExpl = _CLNEW Explanation();
		buf.append(_T("queryWeight("));
		buf.append(queryString);
		buf.append(_T("), product of:"));
		queryExpl->setDescription(buf.getBuffer());
		buf.clear();

		Explanation* boostExpl = _CLNEW Explanation(parentQuery->getBoost(), _T("boost"));
		if (parentQuery->getBoost() != 1.0f)
			queryExpl->addDetail(boostExpl);

		queryExpl->addDetail(idfExpl);

		Explanation* queryNormExpl = _CLNEW Explanation(queryNorm,_T("queryNorm"));
		queryExpl->addDetail(queryNormExpl);

		queryExpl->setValue(boostExpl->getValue() *
			idfExpl->getValue() *
			queryNormExpl->getValue());

		result->addDetail(queryExpl);

		// explain field weight
		ComplexExplanation* fieldExpl = _CLNEW ComplexExplanation();
		buf.append(_T("fieldWeight("));
		buf.append(queryString);
		buf.append(_T(" in "));
		buf.appendInt(doc);
		buf.append(_T("), product of:"));
		fieldExpl->setDescription(buf.getBuffer());
		buf.clear();
		_CLDELETE_LCARRAY(queryString);

		Explanation* tfExpl = scorer(reader)->explain(doc);
		fieldExpl->addDetail(tfExpl);
		fieldExpl->addDetail(idfExpl);

		Explanation* fieldNormExpl = _CLNEW Explanation();
		uint8_t* fieldNorms = reader->norms(parentQuery->field);
		float_t fieldNorm =
			fieldNorms!=NULL ? Similarity::decodeNorm(fieldNorms[doc]) : 0.0f;
		fieldNormExpl->setValue(fieldNorm);

		buf.append(_T("fieldNorm(field="));
		buf.append(parentQuery->field);
		buf.append(_T(", doc="));
		buf.appendInt(doc);
		buf.appendChar(_T(')'));
		fieldNormExpl->setDescription(buf.getBuffer());
		buf.clear();

		fieldExpl->addDetail(fieldNormExpl);

		fieldExpl->setMatch(tfExpl->isMatch());
		fieldExpl->setValue(tfExpl->getValue() *
			idfExpl->getValue() *
			fieldNormExpl->getValue());

		if (queryExpl->getValue() == 1.0f){
			_CLLDELETE(result);
			return fieldExpl;
		}

		result->addDetail(fieldExpl);
		result->setMatch(fieldExpl->getMatch());

		// combine them
		result->setValue(queryExpl->getValue() * fieldExpl->getValue());

		return result;
	}
  };

  Query* MultiPhraseQuery::rewrite(IndexReader* reader) {
	  if (termArrays->size() == 1) {                 // optimize one-term case
		  Term** terms = termArrays->at(0);
		  BooleanQuery* boq = _CLNEW BooleanQuery(true);
		  size_t i = 0;
		  while (terms[i] != NULL) {
			  // TODO: _CL_POINTER(terms[i]) ??
			  boq->add(_CLNEW TermQuery(terms[i]), BooleanClause::SHOULD);
			  ++i;
		  }
		  boq->setBoost(getBoost());
		  return boq;
	  } else {
		  return this;
	  }
  }

  MultiPhraseQuery::MultiPhraseQuery():field(NULL),termArrays(NULL),positions(NULL),slop(0){
  }

MultiPhraseQuery::~MultiPhraseQuery(){
	for (size_t i = 0; i < termArrays->size(); i++){
		size_t j = 0;
		while ( termArrays[i][j] != NULL ) {
			_CLLDECDELETE(termArrays->at(i)[j]);
			++j;
		}
		_CLDELETE_LARRAY(termArrays->at(i));
	}
	_CLLDELETE(termArrays);
	_CLLDELETE(positions);
}

void MultiPhraseQuery::setSlop(const int32_t s) { slop = s; }

int32_t MultiPhraseQuery::getSlop() const { return slop; }

void MultiPhraseQuery::add(CL_NS(index)::Term* term) {
	CL_NS(index)::Term** _terms = _CL_NEWARRAY(CL_NS(index)::Term*, 2);
	_terms[0] = term; // TODO: should we ref-count this here, or request the caller to do so (since the other overrides do not do this)?
	_terms[1] = NULL; // indicate array's end
	add(_terms);
}

void MultiPhraseQuery::add(CL_NS(index)::Term** terms) {
	int32_t position = 0;
	if (positions->size() > 0)
		position = ((int32_t)(*positions->end())) + 1;

	add(terms, position);
}

void MultiPhraseQuery::add(CL_NS(index)::Term** terms, const int32_t position) {
	if (termArrays->size() == 0)
		field = const_cast<TCHAR*>(terms[0]->field());

	size_t i = 0;
	while ( terms[i] != NULL ) {
		if (terms[i]->field() != field) {	//can use != because fields are interned
			TCHAR buf[250];
			_sntprintf(buf,250,_T("All phrase terms must be in the same field (%s): %s"),field, terms[i]);
			_CLTHROWT(CL_ERR_IllegalArgument,buf);
		}
		++i;
	}
	termArrays->push_back(terms);
	positions->push_back(position);
}

void MultiPhraseQuery::getPositions(ValueArray<int32_t>& result) const {
	result.length = positions->size();
	result.values = _CL_NEWARRAY(int32_t,result.length);
	for (size_t i = 0; i < result.length; i++)
		result.values[i] = (*positions)[i];
}

Weight* MultiPhraseQuery::_createWeight(Searcher* searcher) {
	return _CLNEW MultiPhraseWeight(searcher, this);
}

TCHAR* MultiPhraseQuery::toString(const TCHAR* f) const {
	StringBuffer buffer(100);
	if (_tcscmp(f, field)!=0) {
		buffer.append(field);
		buffer.appendChar(_T(':'));
	}

	buffer.appendChar(_T('"'));
	/*
	TODO:
	Iterator i = termArrays.iterator();
	while (i.hasNext()) {
		Term[] terms = (Term[])i.next();
		if (terms.length > 1) {
			buffer.appendChar(_T('('));
			for (int j = 0; j < terms.length; j++) {
				buffer.append(terms[j].text());
				if (j < terms.length-1)
					buffer.appendChar(_T(' '));
			}
			buffer.appendChar(_T(')'));
		} else {
			buffer.append(terms[0].text());
		}
		if (i.hasNext())
			buffer.appendChar(_T(' '));
	}
	*/
	buffer.appendChar(_T('"'));

	if (slop != 0) {
		buffer.appendChar(_T('~'));
		buffer.appendInt(slop);
	}

	buffer.appendBoost(getBoost());

	return buffer.getBuffer();
}

class TermArray_Equals:public CL_NS_STD(binary_function)<const Term**,const Term**,bool>
{
public:
	bool operator()( Term** val1, Term** val2 ) const{
		size_t i = 0;
		while ( val1[i] != NULL ){
			if (val2[i] == NULL) return false;
			if (!val1[i]->equals(val2[i])) return false;
			++i;
		}
		return true;
	}
};

bool MultiPhraseQuery::equals(Query* o) const {
	if (!(o->instanceOf(MultiPhraseQuery::getObjectName()))) return false;
	MultiPhraseQuery* other = static_cast<MultiPhraseQuery*>(o);
	bool ret = (this->getBoost() == other->getBoost()) && (this->slop == other->slop);

	if (ret){
		CLListEquals<int32_t,Equals::Int32,
			const CL_NS(util)::CLVector<int32_t,CL_NS(util)::Deletor::DummyInt32>,
			const CL_NS(util)::CLVector<int32_t,CL_NS(util)::Deletor::DummyInt32> > comp;
		ret = comp.equals(this->positions,other->positions);
	}

	if (ret){
		if (this->termArrays->size() != other->termArrays->size())
			return false;

		for (size_t i=0; i<this->termArrays->size();i++){
			CLListEquals<Term*,TermArray_Equals,
				const CL_NS(util)::CLVector<Term**>,
				const CL_NS(util)::CLVector<Term**> > comp;
			ret = comp.equals(this->termArrays,other->termArrays);
		}
	}
	return ret;
}

// TODO: Test hashed value if conforms with JL
size_t MultiPhraseQuery::hashCode() const {
	size_t ret = Similarity::floatToByte(getBoost()) ^ slop;

	{ //msvc6 scope fix
		for ( size_t i=0;termArrays->size();i++ ) {
			size_t j = 0;
			while ( termArrays->at(j) != NULL ) {
				ret = 31 * ret + termArrays->at(j)[i]->hashCode();
				++j;
			}
		}
	}
	{ //msvc6 scope fix
		for ( size_t i=0;positions->size();i++ )
			ret = 31 * ret + (*positions)[i];
	}
	ret ^= 0x4AC65113;

	return ret;
}

CL_NS_END
