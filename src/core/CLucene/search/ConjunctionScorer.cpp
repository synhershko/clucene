/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "_ConjunctionScorer.h"
#include "Similarity.h"
#include "CLucene/util/_Arrays.h"
#include <assert.h>
#include <algorithm>

CL_NS_USE(index)
CL_NS_USE(util)
CL_NS_DEF(search)

  Scorer* ConjunctionScorer::first()  const{ 
    if ( scorers->end() == scorers->begin() )
      return NULL;

    return *scorers->begin(); 
  } //get First

  Scorer* ConjunctionScorer::last() {
    if ( scorers->end() == scorers->begin() )
      return NULL;

    ScorersType::iterator i = scorers->end();
    --i;
    return *i; 
  } //get Last

  void ConjunctionScorer::sortScorers() {
    // move scorers to an array
    std::sort(scorers->begin(),scorers->end(), Scorer::sort);
  }

  bool ConjunctionScorer::doNext() {
    while (more && first()->doc() < last()->doc()) { // find doc w/ all clauses
      more = first()->skipTo(last()->doc());      // skip first upto last
	  Scorer* scorer = *scorers->begin();
	  scorers->delete_front();
      scorers->push_back(scorer);   // move first to last
    }
    return more;                                // found a doc with all clauses
  }

  
  void ConjunctionScorer::init()  {
    more = scorers->size() > 0;

    // compute coord factor
    coord = getSimilarity()->coord(scorers->size(), scorers->size());

    // move each scorer to its first entry
	  ScorersType::iterator i = scorers->begin();
    while (more && i!=scorers->end()) {
      more = ((Scorer*)*i)->next();
	  ++i;
    }

    if (more)
      sortScorers();                              // initial sort of list

    firstTime = false;
  }

	ConjunctionScorer::ConjunctionScorer(Similarity* similarity):
		Scorer(similarity),
		scorers(_CLNEW ScorersType(false)),
		firstTime(true),
		more(true),
		coord(0.0)
	{
    }
	ConjunctionScorer::~ConjunctionScorer(){
		scorers->setDoDelete(true);
		_CLDELETE(scorers);
	}

	TCHAR* ConjunctionScorer::toString(){
		return stringDuplicate(_T("ConjunctionScorer"));
	}
	
	void ConjunctionScorer::add(Scorer* scorer){
		scorers->push_back(scorer);
	}


  int32_t ConjunctionScorer::doc()  const{ return first()->doc(); }

  bool ConjunctionScorer::next()  {
    if (firstTime) {
      init();
    } else if (more) {
      more = last()->next();                       // trigger further scanning
    }
    return doNext();
  }
  
  bool ConjunctionScorer::skipTo(int32_t target) {
    ScorersType::iterator i = scorers->begin();
    while (more && i!=scorers->end()) {
      more = ((Scorer*)*i)->skipTo(target);
	  ++i;
    }
    if (more)
      sortScorers();                              // re-sort scorers
    return doNext();
  }

  float_t ConjunctionScorer::score(){
    float_t score = 0.0f;                           // sum scores
    ScorersType::const_iterator i = scorers->begin();
	  while (i!=scorers->end()){
      score += (*i)->score();
	    ++i;
	  }
    score *= coord;
    return score;
  }
  Explanation* ConjunctionScorer::explain(int32_t doc) {
	  _CLTHROWA(CL_ERR_UnsupportedOperation,"UnsupportedOperationException: ConjunctionScorer::explain");
  }


CL_NS_END
