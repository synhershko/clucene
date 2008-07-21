/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/

#include "CLucene/_ApiHeader.h"
#include "Scorer.h"
#include "ScorerDocQueue.h"
#include "SearchHeader.h"

#include "_DisjunctionSumScorer.h"


CL_NS_DEF(search)

DisjunctionSumScorer::DisjunctionSumScorer( DisjunctionSumScorer::ScorersType* subScorers ) : Scorer(  NULL )
{
	init( subScorers, 1 );
}

DisjunctionSumScorer::DisjunctionSumScorer( DisjunctionSumScorer::ScorersType* subScorers, int32_t minimumNrMatchers ) : Scorer( NULL )
{	
	init( subScorers, minimumNrMatchers );
}

void DisjunctionSumScorer::init( DisjunctionSumScorer::ScorersType* SubScorers, int32_t MinimumNrMatchers )
{
	
	if ( MinimumNrMatchers <= 0 ) {
		// throw exception
	}
	
	if ( SubScorers->size() <= 1 ) {
		// throw exception
	}

	minimumNrMatchers = MinimumNrMatchers;
	nrScorers = SubScorers->size();
	scorerDocQueue = NULL;
	
	for ( DisjunctionSumScorer::ScorersType::iterator itr = SubScorers->begin(); itr != SubScorers->end(); itr++ ) {
		subScorers.push_back( *itr );
	}
	
}

DisjunctionSumScorer::~DisjunctionSumScorer()
{
	_CLDELETE( scorerDocQueue );
}

void DisjunctionSumScorer::score( HitCollector* hc )
{
	while( next() ) {
		hc->collect( currentDoc, currentScore );
	}
}

bool DisjunctionSumScorer::next()
{
	if ( scorerDocQueue == NULL ) {
		initScorerDocQueue();
	}
	return ( scorerDocQueue->size() >= minimumNrMatchers ) && advanceAfterCurrent();
}

float_t DisjunctionSumScorer::score()
{
	return currentScore;
}
int32_t DisjunctionSumScorer::doc() const
{
	return currentDoc;
}

int32_t DisjunctionSumScorer::nrMatchers()
{
	return _nrMatchers;
}

bool DisjunctionSumScorer::skipTo( int32_t target )
{
	if ( scorerDocQueue == NULL ) {
		initScorerDocQueue();
	}
	if ( queueSize < minimumNrMatchers ) {
		return false;
	}
	if ( target <= currentDoc ) {
		return true;
	}
	do { 		
		if ( scorerDocQueue->topDoc() >= target ) {
			return advanceAfterCurrent();
		} else if ( !scorerDocQueue->topSkipToAndAdjustElsePop( target )) {
			if ( --queueSize < minimumNrMatchers ) {
				return false;
			}
		}		
	} while ( true );
}

TCHAR* DisjunctionSumScorer::toString()
{
	return NULL;
}

void DisjunctionSumScorer::explain( int32_t doc, Explanation* ret )
{
	_CLTHROWA(CL_ERR_UnsupportedOperation,"UnsupportedOperationException: DisjunctionSumScorer::explain");								
}

bool DisjunctionSumScorer::score( HitCollector* hc, const int32_t max )
{
	while ( currentDoc < max ) {
		hc->collect( currentDoc, currentScore );
		if ( !next() ) {
			return false;
		}
	}
	return true;
}

bool DisjunctionSumScorer::advanceAfterCurrent()
{
	do {
		
		currentDoc = scorerDocQueue->topDoc();
		currentScore = scorerDocQueue->topScore();
		_nrMatchers = 1;
		
		do { 
			
			if ( !scorerDocQueue->topNextAndAdjustElsePop() ) {
				if ( --queueSize == 0 ) {
					break;
				}
			}
			if ( scorerDocQueue->topDoc() != currentDoc ) {
				break;
			}
			currentScore += scorerDocQueue->topScore();
			_nrMatchers++;
			
		} while( true );
		
		if ( _nrMatchers >= minimumNrMatchers ) {
			return true;
		} else if ( queueSize < minimumNrMatchers ) {
			return false;
		}
		
	} while( true );
}

void DisjunctionSumScorer::initScorerDocQueue()
{
	scorerDocQueue = _CLNEW ScorerDocQueue( nrScorers );
	queueSize = 0;
	
	for ( ScorersType::iterator it = subScorers.begin(); it != subScorers.end(); ++it ) {
		Scorer* scorer = (Scorer*)(*it);
		if ( scorer->next() ) {
			if ( scorerDocQueue->insert( scorer )) {
				queueSize++;
			}
		}
	}
	
}

CL_NS_END
