/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_search_MultiPhraseQuery_
#define _lucene_search_MultiPhraseQuery_

#include "Query.h"
#include "CLucene/util/Array.h"
#include "CLucene/util/VoidList.h"

CL_CLASS_DEF(index,Term)

CL_NS_DEF(search)

class MultiPhraseWeight;

/**
* MultiPhraseQuery is a generalized version of PhraseQuery, with an added
* method {@link #add(Term[])}.
* To use this class, to search for the phrase "Microsoft app*" first use
* add(Term) on the term "Microsoft", then find all terms that have "app" as
* prefix using IndexReader.terms(Term), and use MultiPhraseQuery.add(Term[]
* terms) to add them to the query.
*
* @author Anders Nielsen
* @version 1.0
*/
class CLUCENE_EXPORT MultiPhraseQuery : public Query {
private:
	TCHAR* field;
  CL_NS(util)::CLArrayList<CL_NS(util)::ArrayBase<CL_NS(index)::Term*>*>* termArrays;
	CL_NS(util)::CLVector<int32_t,CL_NS(util)::Deletor::DummyInt32>* positions;

	int32_t slop;

public:
	MultiPhraseQuery();
	virtual ~MultiPhraseQuery();
	friend class MultiPhraseWeight;

	/** Sets the phrase slop for this query.
	* @see PhraseQuery#setSlop(int)
	*/
	void setSlop(const int32_t s);

	/** Sets the phrase slop for this query.
	* @see PhraseQuery#getSlop()
	*/
	int32_t getSlop() const;

	/** Add a single term at the next position in the phrase.
	* @see PhraseQuery#add(Term)
  * @memory A pointer is taken to term
	*/
	void add(CL_NS(index)::Term* term);

	/** Add multiple terms at the next position in the phrase.  Any of the terms
	* may match.
	* @memory A pointer is taken of each term, the array memory must be cleaned up by calle
	* @see PhraseQuery#add(Term)
	*/
	void add(const CL_NS(util)::ArrayBase<CL_NS(index)::Term*>* terms);

	/**
	* Allows to specify the relative position of terms within the phrase.
	* 
	* @see PhraseQuery#add(Term, int)
	* @param terms
	* @param position
  * @memory A pointer is taken of each term, the array memory must be cleaned up by calle
	*/
  void add(const CL_NS(util)::ArrayBase<CL_NS(index)::Term*>* terms, const int32_t position);

	/**
	* Returns a List<Term[]> of the terms in the multiphrase.
	* Do not modify the List or its contents.
	*/
	/*
	TODO:
	// the problem here is the need to _CLDECDELETE the terms within the arrays when done - or just assume
	// the query will not terminate before the user is done with the returned array. Probably the best
	// path here is to create a dedicated data container type to hold the term arrays, and run the 
	// process of deleting termArrays from ~MultiPhraseQuery in it's destructor.
	List getTermArrays(CL_NS(util)::ValueArray<Term**>& result) {
		return Collections.unmodifiableList(termArrays);
	}
	*/

	/**
	* Returns the relative positions of terms in this phrase.
	*/
	void getPositions(CL_NS(util)::ValueArray<int32_t>& result) const;

	/*
	TODO:
	void extractTerms(Set terms) {
		for (Iterator iter = termArrays.iterator(); iter.hasNext();) {
			Term[] arr = (Term[])iter.next();
			for (int i=0; i<arr.length; i++) {
				terms.add(arr[i]);
			}
		}
	}
	*/

	Query* rewrite(CL_NS(index)::IndexReader* reader);

protected:
	Weight* _createWeight(Searcher* searcher);

public:
	/** Prints a user-readable version of this query. */
	TCHAR* toString(const TCHAR* f) const;

	/** Returns true if <code>o</code> is equal to this. */
	bool equals(Query* o) const;

	/** Returns a hash code value for this object.*/
	size_t hashCode() const;

	// TODO:
	Query* clone() const {return NULL;}

	const char* getObjectName() const { return getClassName(); }
	static const char* getClassName(){ return "MultiPhraseQuery"; }
};
CL_NS_END
#endif
