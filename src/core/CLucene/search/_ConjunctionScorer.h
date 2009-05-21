/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_search_ConjunctionScorer_
#define _lucene_search_ConjunctionScorer_

#include "Scorer.h"
CL_NS_DEF(search)

/** Scorer for conjunctions, sets of queries, all of which are required. */
class ConjunctionScorer: public Scorer {
private:
  typedef CL_NS(util)::CLVector<Scorer*,CL_NS(util)::Deletor::Object<Scorer> > ScorersType;
  //typedef CL_NS(util)::CLLinkedList<Scorer*,CL_NS(util)::Deletor::Object<Scorer> > ScorersType;
  ScorersType* scorers;
  bool firstTime;
  bool more;
  float_t coord;

  Scorer* first() const;
  Scorer* last();
  void sortScorers();
  bool doNext();
  void init();
public:
  ConjunctionScorer(Similarity* similarity);
  virtual ~ConjunctionScorer();
  virtual TCHAR* toString();
  void add(Scorer* scorer);
  int32_t doc() const;
  bool next();
  bool skipTo(int32_t target);
  virtual float_t score();
  virtual void explain(int32_t doc, Explanation* ret) {
    _CLTHROWA(CL_ERR_UnsupportedOperation,"UnsupportedOperationException: ConjunctionScorer::explain");
  }


};

CL_NS_END
#endif
