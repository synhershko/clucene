#ifndef _lucene_util_ScorerDocQueue_
#define _lucene_util_ScorerDocQueue_

#if defined(_LUCENE_PRAGMA_ONCE)
# pragma once
#endif

#include "CLucene/search/Scorer.h"

CL_NS_USE(search)
CL_NS_DEF(util)

class ScorerDocQueue:LUCENE_BASE {
private:
	class HeapedScorerDoc:LUCENE_BASE {
	public:
		Scorer* _scorer;
		int32_t _doc;
		
		HeapedScorerDoc( Scorer* s );
		HeapedScorerDoc( Scorer* s, int32_t doc );
		~HeapedScorerDoc();
		
		void adjust();
	};
	
	HeapedScorerDoc** heap;
	HeapedScorerDoc* topHsd;
	
	int32_t maxSize;
	int32_t _size;
	
public:
	
	ScorerDocQueue( int32_t maxSize );
	~ScorerDocQueue();
	
	void put( Scorer* scorer );
	bool insert( Scorer* scorer );
	Scorer* pop();
	void adjustTop();
	int32_t size();
	void clear();
	
	Scorer* top();
	int32_t topDoc();
	float_t topScore();
	bool topNextAndAdjustElsePop();
	bool topSkipToAndAdjustElsePop( int32_t target );
	
private:
	
	bool checkAdjustElsePop( bool cond );
	void popNoResult();
	void upHeap();
	void downHeap();
	
};

CL_NS_END
#endif
