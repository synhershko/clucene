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

DisjunctionSumScorer::DisjunctionSumScorer( DisjunctionSumScorer::ScorersType* _subScorers
										   , const int32_t _minimumNrMatchers ) : Scorer( NULL ), nrScorers(0),
										   minimumNrMatchers(_minimumNrMatchers),scorerDocQueue(NULL),queueSize(-1),
										   currentDoc(-1),_nrMatchers(-1),currentScore(-1.0f)
{
	if ( minimumNrMatchers <= 0 ) {
		_CLTHROWA(CL_ERR_IllegalArgument,"Minimum nr of matchers must be positive");
	}

	nrScorers = _subScorers->size();
	
	if ( nrScorers <= 1 ) {
		_CLTHROWA(CL_ERR_IllegalArgument,"There must be at least 2 subScorers");
	}
	
	for ( DisjunctionSumScorer::ScorersType::iterator itr = _subScorers->begin(); itr != _subScorers->end(); itr++ ) {
		subScorers.push_back( *itr );
	}
}

DisjunctionSumScorer::~DisjunctionSumScorer()
{
	_CLLDELETE( scorerDocQueue );
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

int32_t DisjunctionSumScorer::nrMatchers() const
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
	return stringDuplicate(_T("DisjunctionSumScorer"));
}

void DisjunctionSumScorer::explain( int32_t doc, Explanation* ret )
{
	_CLTHROWA(CL_ERR_UnsupportedOperation,"UnsupportedOperationException: DisjunctionSumScorer::explain");								
	// TODO: Change return type to Explain* before porting this
	/*
	ScorersType::iterator ssi = subScorers.begin();
	float_t sumScore = 0.0f;
	int_t nrMatches = 0;
	while (ssi != subScorers.end()) {
		Explanation es = ((Scorer*)ssi)->explain(doc, ret);
		if (es.getValue() > 0.0f) { // indicates match
			sumScore += es.getValue();
			nrMatches++;
		}
		res.addDetail(es);
		++ssi;
	}
	if (nrMatchers >= minimumNrMatchers) {
		res.setValue(sumScore);
		res.setDescription("sum over at least " + minimumNrMatchers
			+ " of " + subScorers.size() + ":");
	} else {
		res.setValue(0.0f);
		res.setDescription(nrMatches + " match(es) but at least "
			+ minimumNrMatchers + " of "
			+ subScorers.size() + " needed");
	}
	return res;
	*/
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
	do { // repeat until minimum nr of matchers
		currentDoc = scorerDocQueue->topDoc();
		currentScore = scorerDocQueue->topScore();
		
		_nrMatchers = 1;
		do { // Until all subscorers are after currentDoc 
			if ( !scorerDocQueue->topNextAndAdjustElsePop() ) {
				if ( --queueSize == 0 ) {
					break; // nothing more to advance, check for last match.
				}
			}
			if ( scorerDocQueue->topDoc() != currentDoc ) {
				break; // All remaining subscorers are after currentDoc.
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
	// No need to _CLLDELETE here since this function since this function is only called if scorerDocQueue==NULL
	scorerDocQueue = _CLNEW ScorerDocQueue( nrScorers );
	queueSize = 0;
	
	for ( ScorersType::iterator it = subScorers.begin(); it != subScorers.end(); ++it ) {
		Scorer* scorer = (Scorer*)(*it);
		if ( scorer->next() ) { // doc() method will be used in scorerDocQueue.
			if ( scorerDocQueue->insert( scorer )) {
				queueSize++;
			}
		}
	}
}

CL_NS_END
