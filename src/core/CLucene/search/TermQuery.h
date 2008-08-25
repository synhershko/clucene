/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_search_TermQuery_
#define _lucene_search_TermQuery_


//#include "SearchHeader.h"
//#include "Scorer.h"
CL_CLASS_DEF(index,Term)
//#include "TermScorer.h"
#include "Query.h"
//#include "CLucene/index/IndexReader.h"
CL_CLASS_DEF(util,StringBuffer)
//#include "CLucene/index/Terms.h"

CL_NS_DEF(search)


    /** A Query that matches documents containing a term.
	This may be combined with other terms with a {@link BooleanQuery}.
	*/
    class CLUCENE_EXPORT TermQuery: public Query {
    private:
		CL_NS(index)::Term* term;
    protected:
        Weight* _createWeight(Searcher* searcher);
        TermQuery(const TermQuery& clone);
	public:
		// Constructs a query for the term <code>t</code>. 
		TermQuery(CL_NS(index)::Term* t);
		~TermQuery();

		static const char* getClassName();
		const char* getQueryName() const;
	    
		//added by search highlighter
		CL_NS(index)::Term* getTerm(bool pointer=true) const;
	    
		// Prints a user-readable version of this query. 
		TCHAR* toString(const TCHAR* field) const;

		bool equals(Query* other) const;
		Query* clone() const;

		/** Returns a hash code value for this object.*/
		size_t hashCode() const;
    };
CL_NS_END
#endif

