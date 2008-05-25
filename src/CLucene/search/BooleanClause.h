/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_search_BooleanClause_
#define _lucene_search_BooleanClause_

#if defined(_LUCENE_PRAGMA_ONCE)
# pragma once
#endif
#include "SearchHeader.h"

//#include "CLucene/util/StringBuffer.h"

CL_NS_DEF(search)
// A clause in a BooleanQuery. 
class BooleanClause:LUCENE_BASE {
public:
	class Compare:public CL_NS_STD(binary_function)<const BooleanClause*,const BooleanClause*,bool>
	{
	public:
		bool operator()( const BooleanClause* val1, const BooleanClause* val2 ) const {
			return val1->equals(val2);
		}
	};

	/** Specifies how clauses are to occur in matching documents. */
	//class Occur /*: Parameter */ {
	enum Occur {
		/** Use this operator for clauses that <i>must</i> appear in the matching documents. */
		MUST=1,

		/** Use this operator for clauses that <i>should</i> appear in the 
		* matching documents. For a BooleanQuery with no <code>MUST</code> 
		* clauses one or more <code>SHOULD</code> clauses must match a document 
		* for the BooleanQuery to match.
		* @see BooleanQuery#setMinimumNumberShouldMatch
		*/
		SHOULD=2,

		/** Use this operator for clauses that <i>must not</i> appear in the matching documents.
		* Note that it is not possible to search for queries that only consist
		* of a <code>MUST_NOT</code> clause. */
		MUST_NOT=4
	};
	//};

	int32_t getClauseCount();


	/** Constructs a BooleanClause with query <code>q</code>, required
	* <code>r</code> and prohibited <code>p</code>.
	* @deprecated use BooleanClause(Query, Occur) instead
	* <ul>
	*  <li>For BooleanClause(query, true, false) use BooleanClause(query, BooleanClause.Occur.MUST)
	*  <li>For BooleanClause(query, false, false) use BooleanClause(query, BooleanClause.Occur.SHOULD)
	*  <li>For BooleanClause(query, false, true) use BooleanClause(query, BooleanClause.Occur.MUST_NOT)
	* </ul>
	*/ 
	BooleanClause(Query* q, const bool DeleteQuery,const bool req, const bool p):
	query(q),
		required(req),
		prohibited(p),
		occur(SHOULD),
		deleteQuery(DeleteQuery)
	{
		if (required) {
			if (prohibited) {
				// prohibited && required doesn't make sense, but we want the old behaviour:
				occur = MUST_NOT;
			} else {
				occur = MUST;
			}
		} else {
			if (prohibited) {
				occur = MUST_NOT;
			} else {
				occur = SHOULD;
			}
		}
	}

	BooleanClause(const BooleanClause& clone):
#if defined(LUCENE_ENABLE_MEMLEAKTRACKING)
#elif defined(LUCENE_ENABLE_REFCOUNT)
#else
	LuceneVoidBase(),
#endif
		query(clone.query->clone()),
		required(clone.required),
		prohibited(clone.prohibited),
		occur(clone.occur),
		deleteQuery(true)
	{
	}

	/** Constructs a BooleanClause.
	*/ 
	BooleanClause(Query* q, const bool DeleteQuery, Occur o):
#if defined(LUCENE_ENABLE_MEMLEAKTRACKING)
#elif defined(LUCENE_ENABLE_REFCOUNT)
#else
	LuceneVoidBase(),
#endif
		query(query),
		occur(o),
		deleteQuery(DeleteQuery)
	{
		setFields(occur);
	}


	BooleanClause* clone() const {
		BooleanClause* ret = _CLNEW BooleanClause(*this);
		return ret;
	}

	~BooleanClause(){
		if ( deleteQuery )
			_CLDELETE( query );
	}

	bool deleteQuery;

	/** Returns true if <code>o</code> is equal to this. */
	bool equals(const BooleanClause* other) const {
		return this->query->equals(other->query)
			&& (this->required == other->required)
			&& (this->prohibited == other->prohibited) // TODO: Remove these
			&& (this->occur == other->getOccur() );
	}

	/** Returns a hash code value for this object.*/
	size_t hashCode() const {
		return query->hashCode() ^ ( (occur == MUST) ?1:0) ^ ( (occur == MUST_NOT)?2:0);
	}

	Occur getOccur() const { return occur; };
	void setOccur(Occur o) {
		occur = o;
		setFields(o);
	};

	Query* getQuery() const { return query; };
	void setQuery(Query* q) {
		if ( deleteQuery )
			_CLDELETE( query );
		query = q;
	}

	bool isProhibited() const { return prohibited; /* return (occur == MUST_NOT); */ }
	bool isRequired() const { return required; /* return (occur == MUST); */ }

protected:
	/** The query whose matching documents are combined by the boolean query.
	*     @deprecated use {@link #setQuery(Query)} instead */
	Query* query;

	Occur occur;

	void setFields(Occur occur) {
		if (occur == MUST) {
			required = true;
			prohibited = false;
		} else if (occur == SHOULD) {
			required = false;
			prohibited = false;
		} else if (occur == MUST_NOT) {
			required = false;
			prohibited = true;
		} else {
			_CLTHROWT (CL_ERR_UnknownOperator,  _T("Unknown operator"));
		}
	}

public: // TODO: Make private and remove for CLucene 2.3.2
	/** If true, documents documents which <i>do not</i>
	match this sub-query will <i>not</i> match the boolean query.
	@deprecated use {@link #setOccur(BooleanClause.Occur)} instead */
	bool required;

	/** If true, documents documents which <i>do</i>
	match this sub-query will <i>not</i> match the boolean query.
	@deprecated use {@link #setOccur(BooleanClause.Occur)} instead */
	bool prohibited;
};


CL_NS_END
#endif

