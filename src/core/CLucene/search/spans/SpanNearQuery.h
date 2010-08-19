/*------------------------------------------------------------------------------
 * Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
 * 
 * Distributable under the terms of either the Apache License (Version 2.0) or 
 * the GNU Lesser General Public License, as specified in the COPYING file.
 ------------------------------------------------------------------------------*/
#ifndef _lucene_search_spans_SpanNearQuery_
#define _lucene_search_spans_SpanNearQuery_

CL_CLASS_DEF(index, IndexReader);
#include "SpanQuery.h"

CL_NS_DEF2( search, spans )

/** Matches spans which are near one another.  One can specify <i>slop</i>, the
 * maximum number of intervening unmatched positions, as well as whether
 * matches are required to be in-order. */
class CLUCENE_EXPORT SpanNearQuery : public SpanQuery 
{
private:
    SpanQuery **    clauses;
    size_t          clausesCount;
    bool            bDeleteClauses;

    int32_t         slop;
    bool            inOrder;

    TCHAR *         field;

protected:
    SpanNearQuery( const SpanNearQuery& clone );

public:
    /** Construct a SpanNearQuery.  Matches spans matching a span from each
     * clause, with up to <code>slop</code> total unmatched positions between
     * them.  * When <code>inOrder</code> is true, the spans from each clause
     * must be * ordered as in <code>clauses</code>. */
    SpanNearQuery( CL_NS(util)::ArrayBase<SpanQuery *> * clauses, int32_t slop, bool inOrder, bool bDeleteClauses );
    virtual ~SpanNearQuery();

    CL_NS(search)::Query * clone() const;

    static const char * getClassName();
	const char * getObjectName() const;

    /** Return the clauses whose spans are matched. 
     * CLucene: pointer to the internal array 
     */
    SpanQuery ** getClauses() const;
    size_t getClausesCount() const;

    /** Return the maximum number of intervening unmatched positions permitted.*/
    int32_t getSlop() const;

    /** Return true if matches are required to be in-order.*/
    bool isInOrder() const;

    const TCHAR * getField() const;
  
    /** Returns a collection of all terms matched by this query.
     * @deprecated use extractTerms instead
     * @see #extractTerms(Set)
     */
//    public Collection getTerms() 
  
    void extractTerms( CL_NS(search)::TermSet * terms );
  
    CL_NS(search)::Query * rewrite( CL_NS(index)::IndexReader * reader );

    using Query::toString;
    TCHAR* toString( const TCHAR* field ) const;
    bool equals( Query* other ) const;
    size_t hashCode() const;

    Spans * getSpans( CL_NS(index)::IndexReader * reader );
};

CL_NS_END2
#endif // _lucene_search_spans_SpanNearQuery_