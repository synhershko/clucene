/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_search_DisjunctionSumScorer_
#define _lucene_search_DisjunctionSumScorer_

#if defined(_LUCENE_PRAGMA_ONCE)
# pragma once
#endif

#include "Scorer.h"
#include "CLucene/util/ScorerDocQueue.h"

CL_NS_USE(util)
CL_NS_DEF(search)

class DisjunctionSumScorer : public Scorer {
public:
	typedef CL_NS(util)::CLVector<Scorer*,CL_NS(util)::Deletor::Object<Scorer> > ScorersType;
private:
	int32_t nrScorers;
	int32_t minimumNrMatchers;
	
	DisjunctionSumScorer::ScorersType subScorers;
	ScorerDocQueue* scorerDocQueue;
	
	int32_t queueSize;
	int32_t currentDoc;
	float_t currentScore;
	
protected:
	int32_t _nrMatchers;
	
public:
	
	DisjunctionSumScorer( CL_NS(util)::CLVector<Scorer*,CL_NS(util)::Deletor::Object<Scorer> >* subScorers );
	DisjunctionSumScorer( CL_NS(util)::CLVector<Scorer*,CL_NS(util)::Deletor::Object<Scorer> >* subScorers, int32_t minimumNrMatchers );
	~DisjunctionSumScorer();
	
	void score( HitCollector* hc );
	bool next();
	float_t score();
	int32_t doc() const;
	int32_t nrMatchers();
	bool skipTo( int32_t target );
	TCHAR* toString();
	void explain( int32_t doc, Explanation* ret );
	
protected:
	
	bool score( HitCollector* hc, const int32_t max );
	bool advanceAfterCurrent();
	
private:
	
	void init( CL_NS(util)::CLVector<Scorer*,CL_NS(util)::Deletor::Object<Scorer> >* subScorers, int32_t minimumNrMatchers );
	void initScorerDocQueue();
	
};

CL_NS_END
#endif
