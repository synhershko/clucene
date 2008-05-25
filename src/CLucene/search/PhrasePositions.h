/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_search_PhrasePositions_
#define _lucene_search_PhrasePositions_

#if defined(_LUCENE_PRAGMA_ONCE)
# pragma once
#endif

#include "CLucene/index/Terms.h"

CL_NS_DEF(search)
/**
* Position of a term in a document that takes into account the term offset within the phrase. 
*/
class PhrasePositions:LUCENE_BASE {
public:
	int32_t doc;					  // current doc
	int32_t position;					  // position in doc
	int32_t count;					  // remaining pos in this doc
	int32_t offset;					  // position in phrase
	CL_NS(index)::TermPositions* tp;				  // stream of positions
	PhrasePositions* _next;				  // used to make lists
	bool repeats;       // there's other pp for same term (e.g. query="1st word 2nd word"~1) 


	//Constructor
	PhrasePositions(CL_NS(index)::TermPositions* Tp, const int32_t o);
	//Destructor
	~PhrasePositions();

	bool next();
	bool skipTo(int32_t target);

	void firstPosition();

	bool nextPosition();
};
CL_NS_END
#endif
