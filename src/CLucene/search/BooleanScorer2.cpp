#include "CLucene/StdHeader.h"
#include "BooleanScorer2.h"

#include "Scorer.h"
#include "BooleanScorer.h"
#include "ConjunctionScorer.h"
#include "DisjunctionSumScorer.h"
#include "Similarity.h"

CL_NS_USE(util)
CL_NS_DEF(search)

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

BooleanScorer2::SingleMatchScorer::SingleMatchScorer( Scorer* scorer, Coordinator* coordinator ) : Scorer( scorer->getSimilarity() ), scorer(scorer), coordinator(coordinator)
{
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

BooleanScorer2::ConjunctionScorer::ConjunctionScorer( CL_NS(search)::BooleanScorer2::Coordinator* coordinator, int32_t requiredNrMatchers ):
	CL_NS(search)::ConjunctionScorer( Similarity::getDefault() ),
	coordinator(coordinator),
	requiredNrMatchers(requiredNrMatchers),
	lastScoredDoc(-1)
{
}

float_t BooleanScorer2::ConjunctionScorer::score()
{
	if ( this->doc() >= lastScoredDoc ) {
		lastScoredDoc = this->doc();
		coordinator->nrMatchers += requiredNrMatchers;
	}
	return CL_NS(search)::ConjunctionScorer::score();	
}

BooleanScorer2::DisjunctionSumScorer::DisjunctionSumScorer( CL_NS(search)::BooleanScorer2::Coordinator* coordinator, CL_NS(search)::BooleanScorer2::ScorersType* subScorers, int32_t minimumNrMatchers ):
	CL_NS(search)::DisjunctionSumScorer( subScorers, minimumNrMatchers ),
	coordinator(coordinator),
	lastScoredDoc(-1)
{	
}

float_t BooleanScorer2::DisjunctionSumScorer::score() {
	if ( this->doc() >= lastScoredDoc ) {
		lastScoredDoc = this->doc();
		coordinator->nrMatchers += _nrMatchers;
	}
	return CL_NS(search)::DisjunctionSumScorer::score();
}

BooleanScorer2::BooleanScorer2( Similarity* similarity, int32_t minNrShouldMatch, bool allowDocsOutOfOrder ):
	Scorer( similarity ),
	minNrShouldMatch(minNrShouldMatch),
	allowDocsOutOfOrder(allowDocsOutOfOrder),
	requiredScorers(false),
	optionalScorers(false), 
	prohibitedScorers(false),
	countingSumScorer(NULL)
{
	if ( minNrShouldMatch < 0 ) {
		// throw some sort of exception
	}
	
	this->coordinator = _CLNEW Coordinator( this );
	
}

BooleanScorer2::~BooleanScorer2()
{
	_CLDELETE( coordinator );
	_CLDELETE( countingSumScorer );
}

void BooleanScorer2::add( Scorer* scorer, bool required, bool prohibited )
{
	if ( !prohibited ) {
		coordinator->maxCoord++;
	}
	
	if ( required ) {
		if ( prohibited ) {
			// throw some sort of exception
		}
		requiredScorers.push_back( scorer );
	} else if ( prohibited ) {
		prohibitedScorers.push_back( scorer );		
	} else {
		optionalScorers.push_back( scorer );
	}
	
}

void BooleanScorer2::score( HitCollector* hc )
{
	if ( allowDocsOutOfOrder && requiredScorers.size() == 0 && prohibitedScorers.size() < 32 ) {
		
		BooleanScorer* bs = _CLNEW BooleanScorer( getSimilarity(), minNrShouldMatch );
		ScorersType::iterator si = optionalScorers.begin();		
		while ( si != optionalScorers.end() ) {
			bs->add( (*si), false, false );
			si++;
		}
		si = prohibitedScorers.begin();
		while ( si != prohibitedScorers.begin() ) {
			bs->add( (*si), false, true );
		}
		bs->score( hc );
	} else {
		if ( countingSumScorer == NULL ) {
			initCountingSumScorer();
		}
		while ( countingSumScorer->next() ) {
			hc->collect( countingSumScorer->doc(), score() );
		}
	}
}

int32_t BooleanScorer2::doc() const
{
	return countingSumScorer->doc();
}

bool BooleanScorer2::next()
{
	if ( countingSumScorer == NULL ) {
		initCountingSumScorer();
	}
	return countingSumScorer->next();
}

float_t BooleanScorer2::score()
{
	coordinator->initDoc();
	float_t sum = countingSumScorer->score();
	return sum * coordinator->coordFactor();
}

bool BooleanScorer2::skipTo( int32_t target )
{
	if ( countingSumScorer == NULL ) {
		initCountingSumScorer();
	}
	return countingSumScorer->skipTo( target );
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
	int32_t docNr = countingSumScorer->doc();
	while ( docNr < max ) {
		hc->collect( docNr, score() );
		if ( !countingSumScorer->next() ) {
			return false;
		}
		docNr = countingSumScorer->doc();
	}
	return true;
}

void BooleanScorer2::initCountingSumScorer()
{
	coordinator->init();
	countingSumScorer = makeCountingSumScorer();
}

Scorer* BooleanScorer2::countingDisjunctionSumScorer( BooleanScorer2::ScorersType* scorers, int32_t minNrShouldMatch )
{
	return _CLNEW DisjunctionSumScorer( coordinator, scorers, minNrShouldMatch );
}

Scorer* BooleanScorer2::countingConjunctionSumScorer( BooleanScorer2::ScorersType* requiredScorers )
{
	ConjunctionScorer *cs = _CLNEW ConjunctionScorer( coordinator, requiredScorers->size() );
	ScorersType::iterator rsi = requiredScorers->begin();
	while ( rsi != requiredScorers->end() ) {
		cs->add( *rsi );
		rsi++;
	}
	return cs;
}

Scorer* BooleanScorer2::dualConjunctionSumScorer( Scorer* req1, Scorer* req2 )
{
	CL_NS(search)::ConjunctionScorer* cs = _CLNEW CL_NS(search)::ConjunctionScorer( Similarity::getDefault() );
	cs->add( req1 );
	cs->add( req2 );
	return cs;
}

Scorer* BooleanScorer2::makeCountingSumScorer()
{
	return ( requiredScorers.size() == 0 ) ? makeCountingSumScorerNoReq() : makeCountingSumScorerSomeReq();
}

Scorer* BooleanScorer2::makeCountingSumScorerNoReq()
{
	if ( optionalScorers.size() == 0 ) {
		return _CLNEW NonMatchingScorer();
	} else {
		int32_t nrOptRequired = ( minNrShouldMatch < 1 ) ? 1 : minNrShouldMatch;
		if ( optionalScorers.size() < nrOptRequired ) {
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

Scorer* BooleanScorer2::makeCountingSumScorerSomeReq()
{
	if ( optionalScorers.size() < minNrShouldMatch ) {
		return _CLNEW NonMatchingScorer();
	} else if ( optionalScorers.size() == minNrShouldMatch ) {
		ScorersType allReq( false );
		for ( ScorersType::iterator it = requiredScorers.begin(); it != requiredScorers.end(); it++ ) {
			allReq.push_back( *it );
		}
		for ( ScorersType::iterator it = optionalScorers.begin(); it != optionalScorers.end(); it++ ) {
			allReq.push_back( *it );
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

Scorer* BooleanScorer2::addProhibitedScorers( Scorer* requiredCountingSumScorer )
{
	return ( prohibitedScorers.size() == 0 )
		? requiredCountingSumScorer
		: _CLNEW ReqExclScorer( requiredCountingSumScorer,
								(( prohibitedScorers.size() == 1 )
										? (Scorer*)prohibitedScorers[0]
										: _CLNEW CL_NS(search)::DisjunctionSumScorer( &prohibitedScorers )));
}

CL_NS_END
