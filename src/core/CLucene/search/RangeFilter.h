/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/

#ifndef _lucene_search_RangeFilter_
#define _lucene_search_RangeFilter_

//#include "CLucene/document/DateField.h"
CL_CLASS_DEF(index,Term)
//#include "CLucene/index/Terms.h"
//#include "CLucene/index/IndexReader.h"
//#include "CLucene/util/BitSet.h"
#include "Filter.h"

CL_NS_DEF(search)

/**
 * A Filter that restricts search results to a range of values in a given
 * field.
 *
 * <p>
 * This code borrows heavily from {@link RangeQuery}, but is implemented as a Filter
 *
 * </p>
 */
class CLUCENE_EXPORT RangeFilter: public Filter 
{
private:
	TCHAR* field;
	TCHAR* lowerValue;
	TCHAR* upperValue;
	bool   includeLower;
	bool   includeUpper;
	
protected:
	RangeFilter( const RangeFilter& copy );
	
public:
	RangeFilter( const TCHAR* fieldName, const TCHAR* lowerValue, const TCHAR* upperValue, bool includeLower, bool includeUpper );
	
	static RangeFilter* Less( TCHAR* fieldName, TCHAR* upperTerm );
	
	static RangeFilter* More( TCHAR* fieldName, TCHAR* lowerTerm );
	
	~RangeFilter();
	
	/** Returns a BitSet with true for documents which should be permitted in
	search results, and false for those that should not. */
	CL_NS(util)::BitSet* bits( CL_NS(index)::IndexReader* reader );
	
	Filter* clone() const;
	
	TCHAR* toString();
};

CL_NS_END
#endif
