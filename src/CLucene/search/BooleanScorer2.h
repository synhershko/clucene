/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/

#ifndef _lucene_search_BooleanScorer2_
#define _lucene_search_BooleanScorer2_

#if defined(_LUCENE_PRAGMA_ONCE)
# pragma once
#endif

#include "Scorer.h"
#include "ConjunctionScorer.h"
#include "DisjunctionSumScorer.h"

CL_NS_DEF(search)
	
	class BooleanScorer2: public Scorer {
	public:
		typedef CL_NS(util)::CLVector<Scorer*,CL_NS(util)::Deletor::Object<Scorer> > ScorersType;
	private:
		
		class Coordinator: LUCENE_BASE {
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

		class SingleMatchScorer: public Scorer {
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
		
		class NonMatchingScorer: public Scorer {
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
		
		class ReqOptSumScorer: public Scorer {
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
		
		class ReqExclScorer: public Scorer {
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
		
		class ConjunctionScorer: public CL_NS(search)::ConjunctionScorer {
		private:
			CL_NS(search)::BooleanScorer2::Coordinator* coordinator;
			int32_t lastScoredDoc;
			int32_t requiredNrMatchers;
		public:
			ConjunctionScorer( CL_NS(search)::BooleanScorer2::Coordinator* coordinator, int32_t requiredNrMatchers );
			float_t score();
		};
		
		class DisjunctionSumScorer: public CL_NS(search)::DisjunctionSumScorer {
		private:
			CL_NS(search)::BooleanScorer2::Coordinator* coordinator;
			int32_t lastScoredDoc;
		public:
			DisjunctionSumScorer( CL_NS(search)::BooleanScorer2::Coordinator* coordinator, BooleanScorer2::ScorersType* subScorers, int32_t minimumNrMatchers );
			float_t score();
		};
		
		BooleanScorer2::ScorersType requiredScorers;
		BooleanScorer2::ScorersType optionalScorers;
		BooleanScorer2::ScorersType prohibitedScorers;
		
		Coordinator *coordinator;
		Scorer *countingSumScorer;
		
		size_t minNrShouldMatch;
		bool allowDocsOutOfOrder;
		
	public:
		
		BooleanScorer2( Similarity* similarity, int32_t minNrShouldMatch, bool allowDocsOutOfOrder );
		~BooleanScorer2();
		
		void add( Scorer* scorer, bool required, bool prohibited );
		void score( HitCollector* hc );
		int32_t doc() const;
		bool next();
		float_t score();
		bool skipTo( int32_t target );
		void explain( int32_t doc, Explanation* ret );
		TCHAR* toString();
		
	protected:
		
		bool score( HitCollector* hc, const int32_t max );
		
	private:
		
		void initCountingSumScorer();
		
		Scorer* countingDisjunctionSumScorer( BooleanScorer2::ScorersType* scorers, int32_t minNrShouldMatch );
		Scorer* countingConjunctionSumScorer( BooleanScorer2::ScorersType* requiredScorers );
		Scorer* dualConjunctionSumScorer( Scorer* req1, Scorer* req2 );
		
		Scorer* makeCountingSumScorer();
		Scorer* makeCountingSumScorerNoReq();
		Scorer* makeCountingSumScorerSomeReq();
		
		Scorer* addProhibitedScorers( Scorer* requiredCountingSumScorer );
		
	};

CL_NS_END
#endif