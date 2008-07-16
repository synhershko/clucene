/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "Scorer.h"
#include "SearchHeader.h"

CL_NS_DEF(search)

Scorer::Scorer(Similarity* similarity) {
 this->similarity = similarity;
}

Scorer::~Scorer(){
}

/** Returns the Similarity implementation used by this scorer. */
Similarity* Scorer::getSimilarity()  const{
 return this->similarity;
}

/** Scores all documents and passes them to a collector. */
void Scorer::score(HitCollector* hc) {
 while (next()) {
    hc->collect(doc(), score());
 }
}

bool Scorer::score( HitCollector* results, const int32_t maxDoc ) {
  while( doc() < maxDoc ) {
	  results->collect( doc(), score() );
	  if ( !next() )
		  return false;
  }
  return true;
}

CL_NS_END
