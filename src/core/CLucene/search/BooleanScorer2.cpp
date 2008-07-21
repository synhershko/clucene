/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "_BooleanScorer2.h"

#include "Scorer.h"
#include "SearchHeader.h"
#include "Similarity.h"
#include "ScorerDocQueue.h"

#include "_BooleanScorer.h"
#include "_BooleanScorer.h"
#include "_ConjunctionScorer.h"
#include "_DisjunctionSumScorer.h"

CL_NS_USE(util)
CL_NS_DEF(search)


class BooleanScorer2::Internal{
public:
    typedef CL_NS(util)::CLVector<Scorer*,CL_NS(util)::Deletor::Object<Scorer> > ScorersType;
        
	ScorersType requiredScorers;
	ScorersType optionalScorers;
	ScorersType prohibitedScorers;
	
	BooleanScorer2::Coordinator *coordinator;
	Scorer* countingSumScorer;
	
	size_t minNrShouldMatch;
	bool allowDocsOutOfOrder;
	
	
	void initCountingSumScorer();
	
	Scorer* countingDisjunctionSumScorer( ScorersType* scorers, int32_t minNrShouldMatch );
	Scorer* countingConjunctionSumScorer( ScorersType* requiredScorers );
	Scorer* dualConjunctionSumScorer( Scorer* req1, Scorer* req2 );
	
	Scorer* makeCountingSumScorer();
	Scorer* makeCountingSumScorerNoReq();
	Scorer* makeCountingSumScorerSomeReq();
	
	Scorer* addProhibitedScorers( Scorer* requiredCountingSumScorer );
	
	Internal( BooleanScorer2* parent, int32_t minNrShouldMatch, bool allowDocsOutOfOrder );
	~Internal();
};



class BooleanScorer2::Coordinator: LUCENE_BASE {
public:
	int32_t maxCoord;
	int32_t nrMatchers;
	float_t* coordFactors;
	Scorer* parentScorer;

	Coordinator( Scorer* parent );
	~Coordinator();
	
	void init();
	
	void initDoc() {
	nrMatchers = 0;
	}
	
	float_t coordFactor() {
		return coordFactors[nrMatchers];
	}			
};

class BooleanScorer2::SingleMatchScorer: public Scorer {
public:
	Scorer* scorer;
	Coordinator* coordinator;
	int32_t lastScoredDoc;
	
	SingleMatchScorer( Scorer* scorer, Coordinator* coordinator );
	~SingleMatchScorer();
	
	float_t score();
	
	int32_t doc() const {
		return scorer->doc();
	}
	
	bool next() {
		return scorer->next();
	}
	
	bool skipTo( int32_t docNr ) {
		return scorer->skipTo( docNr );
	}
	
	TCHAR* toString() {
		return scorer->toString();
	}
	
	void explain(int32_t doc, Explanation* ret) {
		scorer->explain( doc, ret );	
	}
	
};

class BooleanScorer2::NonMatchingScorer: public Scorer {
public:
	
	NonMatchingScorer();
	
	int32_t doc() const {
		_CLTHROWA(CL_ERR_UnsupportedOperation, "UnsupportedOperationException: BooleanScorer2::NonMatchingScorer::doc");
		return 0;
	}
	bool next() { return false; }
	float_t score() {
		_CLTHROWA(CL_ERR_UnsupportedOperation, "UnsupportedOperationException: BooleanScorer2::NonMatchingScorer::score");
		return 0.0;
	}
	bool skipTo( int32_t target ) { return false; }
	TCHAR* toString() { return NULL; }

	void explain( int32_t doc, Explanation* ret ) {
		_CLTHROWA(CL_ERR_UnsupportedOperation,"UnsupportedOperationException: BooleanScorer2::NonMatchingScorer::explain");								
	}
	
};

class BooleanScorer2::ReqOptSumScorer: public Scorer {
private:
	Scorer* reqScorer;
	Scorer* optScorer;
	bool firstTimeOptScorer;
	
public:
	ReqOptSumScorer( Scorer* reqScorer, Scorer* optScorer );
	~ReqOptSumScorer();
	
	int32_t doc() const {
		return reqScorer->doc();
}
	
	bool next() {
		return reqScorer->next();
	}
	
	bool skipTo( int32_t target ) {
		return reqScorer->skipTo( target );
	}
	
	TCHAR* toString() {
		return NULL;
	}
	
	void explain( int32_t doc, Explanation* ret ) {
		_CLTHROWA(CL_ERR_UnsupportedOperation,"UnsupportedOperationException: BooleanScorer2::ReqOptScorer::explain");								
	}

	float_t score();

};

class BooleanScorer2::ReqExclScorer: public Scorer {
private:
	Scorer* reqScorer;
	Scorer* exclScorer;
	bool firstTime;
	
public:
	ReqExclScorer( Scorer* reqScorer, Scorer* exclScorer );
	~ReqExclScorer();
	
	int32_t doc() const {
		return reqScorer->doc();
	}
	
	float_t score() {
		return reqScorer->score();
	}
	
	TCHAR* toString() {
		return NULL;
	}
	
	void explain( int32_t doc, Explanation* ret ) {
		_CLTHROWA(CL_ERR_UnsupportedOperation,"UnsupportedOperationException: BooleanScorer2::ReqExclScorer::explain");								
	}
	
	bool next();
	bool skipTo( int32_t target );
	
private:
	bool toNonExcluded();
	
};

class BooleanScorer2::BSConjunctionScorer: public CL_NS(search)::ConjunctionScorer {
private:
	CL_NS(search)::BooleanScorer2::Coordinator* coordinator;
	int32_t lastScoredDoc;
	int32_t requiredNrMatchers;
public:
	BSConjunctionScorer( CL_NS(search)::BooleanScorer2::Coordinator* coordinator, int32_t requiredNrMatchers );
	float_t score();
	~BSConjunctionScorer();
};

class BooleanScorer2::BSDisjunctionSumScorer: public CL_NS(search)::DisjunctionSumScorer {
private:
	CL_NS(search)::BooleanScorer2::Coordinator* coordinator;
	int32_t lastScoredDoc;
public:
	BSDisjunctionSumScorer( CL_NS(search)::BooleanScorer2::Coordinator* coordinator, BooleanScorer2::Internal::ScorersType* subScorers, int32_t minimumNrMatchers );
	float_t score();
};


	

BooleanScorer2::Coordinator::Coordinator( Scorer* parent ):
maxCoord(0),
nrMatchers(0),
coordFactors(NULL),
parentScorer(parent)
{
}

BooleanScorer2::Coordinator::~Coordinator()
{
	_CLDELETE_ARRAY(coordFactors);
}

void BooleanScorer2::Coordinator::init()
{
	coordFactors = _CL_NEWARRAY( float_t, maxCoord+1 );
	Similarity* sim = parentScorer->getSimilarity();
	for ( int32_t i = 0; i <= maxCoord; i++ ) {
		coordFactors[i] = sim->coord(i, maxCoord);
	}
}

BooleanScorer2::SingleMatchScorer::SingleMatchScorer( Scorer* scorer, Coordinator* coordinator ) : Scorer( scorer->getSimilarity() ), scorer(scorer), coordinator(coordinator), lastScoredDoc(-1)
{
}

BooleanScorer2::SingleMatchScorer::~SingleMatchScorer()
{
	_CLDELETE( scorer );
}

float_t BooleanScorer2::SingleMatchScorer::score()
{
	if ( doc() >= lastScoredDoc ) {
		lastScoredDoc = this->doc();
		coordinator->nrMatchers++;
	}
	return scorer->score();
}

BooleanScorer2::NonMatchingScorer::NonMatchingScorer() : Scorer( NULL )
{
}

BooleanScorer2::ReqOptSumScorer::ReqOptSumScorer( Scorer* reqScorer, Scorer* optScorer ) : Scorer( NULL ), reqScorer(reqScorer), optScorer(optScorer), firstTimeOptScorer(true)
{	
}

BooleanScorer2::ReqOptSumScorer::~ReqOptSumScorer()
{
	_CLDELETE( reqScorer );
	_CLDELETE( optScorer );
}

float_t BooleanScorer2::ReqOptSumScorer::score()
{
	int32_t curDoc = reqScorer->doc();
	float_t reqScore = reqScorer->score();

	if ( firstTimeOptScorer ) {
		firstTimeOptScorer = false;
		if ( !optScorer->skipTo( curDoc ) ) {
			optScorer = NULL;
			return reqScore;
		}
	} else if ( optScorer == NULL ) {
		return reqScore;
	} else if (( optScorer->doc() < curDoc ) && ( !optScorer->skipTo( curDoc ))) {
		optScorer = NULL;
		return reqScore;
	}

	return ( optScorer->doc() == curDoc )
		? reqScore + optScorer->score()
		: reqScore;		
}

BooleanScorer2::ReqExclScorer::ReqExclScorer( Scorer* reqScorer, Scorer* exclScorer ) : Scorer( NULL ), reqScorer(reqScorer), exclScorer(exclScorer), firstTime(true)
{	
}

BooleanScorer2::ReqExclScorer::~ReqExclScorer()
{
	_CLDELETE( reqScorer );
	_CLDELETE( exclScorer );
}

bool BooleanScorer2::ReqExclScorer::next()
{
	if ( firstTime ) {
		if ( !exclScorer->next() ) {
			_CLDELETE( exclScorer );
			exclScorer = NULL;
		}
		firstTime = false;
	}	
	if ( reqScorer == NULL ) {
		return false;
	}	
	if ( !reqScorer->next() ) {
		_CLDELETE( reqScorer );
		reqScorer = NULL;
		return false;
	}
	if ( exclScorer == NULL ) {
		return true;
	}
	return toNonExcluded();
}

bool BooleanScorer2::ReqExclScorer::skipTo( int32_t target )
{
	if ( firstTime ) {
		firstTime = false;
		if ( !exclScorer->skipTo( target )) {
			_CLDELETE( exclScorer );
			exclScorer = NULL;
		}
	}
	if ( reqScorer == NULL ) {
		return false;
	}
	if ( exclScorer == NULL ) {
		return reqScorer->skipTo( target );
	}
	if ( !reqScorer->skipTo( target )) {
		_CLDELETE( reqScorer );
		reqScorer = NULL;
		return false;
	}
	return toNonExcluded();
}

bool BooleanScorer2::ReqExclScorer::toNonExcluded()
{
	int32_t exclDoc = exclScorer->doc();

	do {
		int32_t reqDoc = reqScorer->doc();
		if ( reqDoc < exclDoc ) {
			return true;
		} else if ( reqDoc > exclDoc ) {
			_CLDELETE( exclScorer );
			exclScorer = NULL;
			return true;
		}
		exclDoc = exclScorer->doc();
		if ( exclDoc > reqDoc ) {
			return true;
		}
	} while ( reqScorer->next() );

	_CLDELETE( reqScorer );
	reqScorer = NULL;

	return false;	
}

BooleanScorer2::BSConjunctionScorer::BSConjunctionScorer( CL_NS(search)::BooleanScorer2::Coordinator* coordinator, int32_t requiredNrMatchers ):
	ConjunctionScorer( Similarity::getDefault() ),
	coordinator(coordinator),
	lastScoredDoc(-1),
	requiredNrMatchers(requiredNrMatchers)
{
}
BooleanScorer2::BSConjunctionScorer::~BSConjunctionScorer(){
}

float_t BooleanScorer2::BSConjunctionScorer::score()
{
	if ( this->doc() >= lastScoredDoc ) {
		lastScoredDoc = this->doc();
		coordinator->nrMatchers += requiredNrMatchers;
	}
	return ConjunctionScorer::score();	
}

BooleanScorer2::BSDisjunctionSumScorer::BSDisjunctionSumScorer( 
	CL_NS(search)::BooleanScorer2::Coordinator* _coordinator, 
	CL_NS(search)::BooleanScorer2::Internal::ScorersType* subScorers, 
	int32_t minimumNrMatchers ):
		DisjunctionSumScorer( subScorers, minimumNrMatchers ),
		coordinator(_coordinator),
		lastScoredDoc(-1)
{	
}

float_t BooleanScorer2::BSDisjunctionSumScorer::score() {
	if ( this->doc() >= lastScoredDoc ) {
		lastScoredDoc = this->doc();
		coordinator->nrMatchers += _nrMatchers;
	}
	return DisjunctionSumScorer::score();
}

BooleanScorer2::Internal::Internal( BooleanScorer2* parent, int32_t minNrShouldMatch, bool allowDocsOutOfOrder ):
	requiredScorers(false),
	optionalScorers(false),
	prohibitedScorers(false),
    countingSumScorer(NULL),
	minNrShouldMatch(minNrShouldMatch),
	allowDocsOutOfOrder(allowDocsOutOfOrder)
{
	if ( minNrShouldMatch < 0 ) {
		// todO: throw some sort of exception
	}

	this->coordinator = _CLNEW Coordinator( parent );
	
}
BooleanScorer2::Internal::~Internal(){
	_CLDELETE( coordinator );
	_CLDELETE( countingSumScorer );
}

BooleanScorer2::BooleanScorer2( Similarity* similarity, int32_t minNrShouldMatch, bool allowDocsOutOfOrder ):
	Scorer( similarity )
{
	internal = new Internal(this, minNrShouldMatch,allowDocsOutOfOrder);
}

BooleanScorer2::~BooleanScorer2()
{
	delete internal;
}

void BooleanScorer2::add( Scorer* scorer, bool required, bool prohibited )
{
	if ( !prohibited ) {
		internal->coordinator->maxCoord++;
	}

	if ( required ) {
		if ( prohibited ) {
			// throw some sort of exception
		}
		internal->requiredScorers.push_back( scorer );
	} else if ( prohibited ) {
		internal->prohibitedScorers.push_back( scorer );		
	} else {
		internal->optionalScorers.push_back( scorer );
	}

}

void BooleanScorer2::score( HitCollector* hc )
{
	if ( internal->allowDocsOutOfOrder && internal->requiredScorers.size() == 0 && internal->prohibitedScorers.size() < 32 ) {

		BooleanScorer* bs = _CLNEW BooleanScorer( getSimilarity(), internal->minNrShouldMatch );
		Internal::ScorersType::iterator si = internal->optionalScorers.begin();		
		while ( si != internal->optionalScorers.end() ) {
			bs->add( (*si), false, false );
			si++;
		}
		si = internal->prohibitedScorers.begin();
		while ( si != internal->prohibitedScorers.begin() ) {
			bs->add( (*si), false, true );
		}
		bs->score( hc );
	} else {
		if ( internal->countingSumScorer == NULL ) {
			internal->initCountingSumScorer();
		}
		while ( internal->countingSumScorer->next() ) {
			hc->collect( internal->countingSumScorer->doc(), score() );
		}
	}
}

int32_t BooleanScorer2::doc() const
{
	return internal->countingSumScorer->doc();
}

bool BooleanScorer2::next()
{
	if ( internal->countingSumScorer == NULL ) {
		internal->initCountingSumScorer();
	}
	return internal->countingSumScorer->next();
}

float_t BooleanScorer2::score()
{
	internal->coordinator->initDoc();
	float_t sum = internal->countingSumScorer->score();
	return sum * internal->coordinator->coordFactor();
}

bool BooleanScorer2::skipTo( int32_t target )
{
	if ( internal->countingSumScorer == NULL ) {
		internal->initCountingSumScorer();
	}
	return internal->countingSumScorer->skipTo( target );
}

TCHAR* BooleanScorer2::toString()
{
	return NULL;
}

void BooleanScorer2::explain( int32_t doc, Explanation* ret )
{
	_CLTHROWA(CL_ERR_UnsupportedOperation,"UnsupportedOperationException: BooleanScorer2::explain");	
}

bool BooleanScorer2::score( HitCollector* hc, int32_t max )
{
	int32_t docNr = internal->countingSumScorer->doc();
	while ( docNr < max ) {
		hc->collect( docNr, score() );
		if ( !internal->countingSumScorer->next() ) {
			return false;
		}
		docNr = internal->countingSumScorer->doc();
	}
	return true;
}

void BooleanScorer2::Internal::initCountingSumScorer()
{
	Internal::coordinator->init();
	countingSumScorer = Internal::makeCountingSumScorer();
}

Scorer* BooleanScorer2::Internal::countingDisjunctionSumScorer( BooleanScorer2::Internal::ScorersType* scorers, int32_t minNrShouldMatch )
{
	return _CLNEW BSDisjunctionSumScorer( coordinator, scorers, minNrShouldMatch );
}

Scorer* BooleanScorer2::Internal::countingConjunctionSumScorer( BooleanScorer2::Internal::ScorersType* requiredScorers )
{
	BSConjunctionScorer *cs = _CLNEW BSConjunctionScorer( coordinator, requiredScorers->size() );
	for ( Internal::ScorersType::iterator rsi = requiredScorers->begin(); rsi != requiredScorers->end(); rsi++ ) {
		cs->add( *rsi );
	}
	return cs;
}

Scorer* BooleanScorer2::Internal::dualConjunctionSumScorer( Scorer* req1, Scorer* req2 )
{
	CL_NS(search)::ConjunctionScorer* cs = _CLNEW CL_NS(search)::ConjunctionScorer( Similarity::getDefault() );
	cs->add( req1 );
	cs->add( req2 );
	return cs;
}

Scorer* BooleanScorer2::Internal::makeCountingSumScorer()
{
	return ( requiredScorers.size() == 0 ) ? makeCountingSumScorerNoReq() : makeCountingSumScorerSomeReq();
}

Scorer* BooleanScorer2::Internal::makeCountingSumScorerNoReq()
{
	if ( optionalScorers.size() == 0 ) {
		optionalScorers.setDoDelete(true);
		return _CLNEW NonMatchingScorer();
	} else {
		size_t nrOptRequired = ( minNrShouldMatch < 1 ) ? 1 : minNrShouldMatch;
		if ( optionalScorers.size() < nrOptRequired ) {
			optionalScorers.setDoDelete(true);
			return _CLNEW NonMatchingScorer();
		} else {
			Scorer* requiredCountingSumScorer = 
				( optionalScorers.size() > nrOptRequired )
				? countingDisjunctionSumScorer( &optionalScorers, nrOptRequired )
				:
				( optionalScorers.size() == 1 )
				? _CLNEW SingleMatchScorer((Scorer*) optionalScorers[0], coordinator)
				: countingConjunctionSumScorer( &optionalScorers );
			return addProhibitedScorers( requiredCountingSumScorer );
		}
	}
}

Scorer* BooleanScorer2::Internal::makeCountingSumScorerSomeReq()
{
	if ( optionalScorers.size() < minNrShouldMatch ) {
		requiredScorers.setDoDelete(true);
		optionalScorers.setDoDelete(true);
		return _CLNEW NonMatchingScorer();
	} else if ( optionalScorers.size() == minNrShouldMatch ) {
		Internal::ScorersType allReq( false );
		for ( Internal::ScorersType::iterator it = requiredScorers.begin(); it != requiredScorers.end(); it++ ) {
			allReq.push_back( *it );
		}
		for ( Internal::ScorersType::iterator it2 = optionalScorers.begin(); it2 != optionalScorers.end(); it2++ ) {
			allReq.push_back( *it2 );
		}		
		return addProhibitedScorers( countingConjunctionSumScorer( &allReq ));
	} else {
		Scorer* requiredCountingSumScorer =
			( requiredScorers.size() == 1 )
			? _CLNEW SingleMatchScorer( (Scorer*)requiredScorers[0], coordinator )
			: countingConjunctionSumScorer( &requiredScorers );
		if ( minNrShouldMatch > 0 ) {
			return addProhibitedScorers(
				dualConjunctionSumScorer(
						requiredCountingSumScorer,
						countingDisjunctionSumScorer(
							&optionalScorers,
							minNrShouldMatch )));
		} else {
			return _CLNEW ReqOptSumScorer(
					addProhibitedScorers( requiredCountingSumScorer ),
					(( optionalScorers.size() == 1 )
							? _CLNEW SingleMatchScorer( (Scorer*)optionalScorers[0], coordinator )
							: countingDisjunctionSumScorer( &optionalScorers, 1 )));
		}
	}
}

Scorer* BooleanScorer2::Internal::addProhibitedScorers( Scorer* requiredCountingSumScorer )
{
	return ( prohibitedScorers.size() == 0 )
		? requiredCountingSumScorer
		: _CLNEW ReqExclScorer( requiredCountingSumScorer,
								(( prohibitedScorers.size() == 1 )
										? (Scorer*)prohibitedScorers[0]
										: _CLNEW CL_NS(search)::DisjunctionSumScorer( &prohibitedScorers )));
}

CL_NS_END
