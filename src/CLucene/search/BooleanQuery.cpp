/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "BooleanQuery.h"

#include "BooleanClause.h"
#include "CLucene/index/IndexReader.h"
#include "CLucene/util/_StringBuffer.h"
#include "CLucene/util/_Arrays.h"
#include "SearchHeader.h"
#include "_BooleanScorer.h"
#include "_ConjunctionScorer.h"
#include "Similarity.h"
#include "Explanation.h"
#include "_BooleanScorer2.h"
#include "Scorer.h"

CL_NS_USE(index)
CL_NS_USE(util)
CL_NS_DEF(search)

	class BooleanClause_Compare:public CL_NS_STD(binary_function)<const BooleanClause*,const BooleanClause*,bool>
	{
	public:
		bool operator()( const BooleanClause* val1, const BooleanClause* val2 ) const {
			return val1->equals(val2);
		}
	};


	class BooleanWeight: public Weight {
	protected:
		Searcher* searcher;
		Similarity* similarity;
		CL_NS(util)::CLVector<Weight*,CL_NS(util)::Deletor::Object<Weight> > weights;
		BooleanQuery::ClausesType* clauses;
		BooleanQuery* parentQuery;
	public:
		BooleanWeight(Searcher* searcher,
			CL_NS(util)::CLVector<BooleanClause*,CL_NS(util)::Deletor::Object<BooleanClause> >* clauses, 
			BooleanQuery* parentQuery);
		virtual ~BooleanWeight();
		Query* getQuery();
		float_t getValue();
		float_t sumOfSquaredWeights();
		void normalize(float_t norm);
		Scorer* scorer(CL_NS(index)::IndexReader* reader);
		void explain(CL_NS(index)::IndexReader* reader, int32_t doc, Explanation* ret);
	};// BooleanWeight

	class BooleanWeight2: public BooleanWeight {
	public:
		BooleanWeight2( Searcher* searcher, CL_NS(util)::CLVector<BooleanClause*,CL_NS(util)::Deletor::Object<BooleanClause> >* clauses, BooleanQuery* parentQuery );
		Scorer* scorer( CL_NS(index)::IndexReader* reader );
		virtual ~BooleanWeight2();
	};// BooleanWeight2


		BooleanQuery::BooleanQuery( bool disableCoord ):
			clauses(_CLNEW BooleanQuery::ClausesType(true))
		{
			this->minNrShouldMatch = 0;
			this->disableCoord = disableCoord;
		}

    Weight* BooleanQuery::_createWeight(Searcher* searcher) {
			//return _CLNEW BooleanWeight(searcher, clauses,this);
			if ( 0 < minNrShouldMatch ) {
				return _CLNEW BooleanWeight2( searcher, clauses, this );
			}

			return getUseScorer14() ? (Weight*) _CLNEW BooleanWeight( searcher, clauses, this )
				: (Weight*) _CLNEW BooleanWeight2( searcher, clauses, this );
		}
		
	BooleanQuery::BooleanQuery(const BooleanQuery& clone):
		Query(clone),
		disableCoord(clone.disableCoord),
		clauses(_CLNEW ClausesType(true))
	{
		minNrShouldMatch = clone.minNrShouldMatch;
		for ( uint32_t i=0;i<clone.clauses->size();i++ ){
			BooleanClause* clause = (*clone.clauses)[i]->clone();
			clause->deleteQuery=true;
			add(clause);
		}
	}

    BooleanQuery::~BooleanQuery(){
		clauses->clear();
		_CLDELETE(clauses);
    }

	size_t BooleanQuery::hashCode() const {
		//todo: do cachedHashCode, and invalidate on add/remove clause
		size_t ret = 0;
		for (uint32_t i = 0 ; i < clauses->size(); i++) {
			BooleanClause* c = (*clauses)[i];
			ret = 31 * ret + c->hashCode();
		}
		ret = ret ^ Similarity::floatToByte(getBoost());
		return ret;
	}

    const TCHAR* BooleanQuery::getQueryName() const{
      return getClassName();
    }
	const TCHAR* BooleanQuery::getClassName(){
      return _T("BooleanQuery");
    }

   /**
   * Default value is 1024.  Use <code>org.apache.lucene.maxClauseCount</code>
   * system property to override.
   */
   size_t BooleanQuery::maxClauseCount = LUCENE_BOOLEANQUERY_MAXCLAUSECOUNT;
   size_t BooleanQuery::getMaxClauseCount(){
      return maxClauseCount;
   }

   void BooleanQuery::setMaxClauseCount(size_t maxClauseCount){
	   BooleanQuery::maxClauseCount = maxClauseCount;
   }

   Similarity* BooleanQuery::getSimilarity( Searcher* searcher ) {

	   Similarity* result = Query::getSimilarity( searcher );
	   return result;

   }

  void BooleanQuery::add(Query* query, const bool deleteQuery, const bool required, const bool prohibited) {
		BooleanClause* bc = _CLNEW BooleanClause(query,deleteQuery,required, prohibited);
		try{
			add(bc);
		}catch(...){
			_CLDELETE(bc);
			throw;
		}
  }

  void BooleanQuery::add(Query* query, const bool deleteQuery, BooleanClause::Occur occur) {
		BooleanClause* bc = _CLNEW BooleanClause(query,deleteQuery,occur);
		try{
			add(bc);
		}catch(...){
			_CLDELETE(bc);
			throw;
		}
  }

  void BooleanQuery::add(BooleanClause* clause) {
    if (clauses->size() >= getMaxClauseCount())
      _CLTHROWA(CL_ERR_TooManyClauses,"Too Many Clauses");

    clauses->push_back(clause);
  }

  int32_t BooleanQuery::getMinNrShouldMatch(){
  	return minNrShouldMatch;
  }
  bool BooleanQuery::useScorer14 = false;

  bool BooleanQuery::getUseScorer14() {
	  return useScorer14;
  }

  void BooleanQuery::setUseScorer14( bool use14 ) {
	  useScorer14 = use14;
  }

  size_t BooleanQuery::getClauseCount() const {
    return (int32_t) clauses->size();
  }

  TCHAR* BooleanQuery::toString(const TCHAR* field) const{
    StringBuffer buffer;
    if (getBoost() != 1.0) {
      buffer.append(_T("("));
    }

    for (uint32_t i = 0 ; i < clauses->size(); i++) {
      BooleanClause* c = (*clauses)[i];
      if (c->prohibited)
        buffer.append(_T("-"));
      else if (c->required)
        buffer.append(_T("+"));

      if ( c->getQuery()->instanceOf(BooleanQuery::getClassName()) ) {	  // wrap sub-bools in parens
        buffer.append(_T("("));

        TCHAR* buf = c->getQuery()->toString(field);
        buffer.append(buf);
        _CLDELETE_CARRAY( buf );

        buffer.append(_T(")"));
      } else {
        TCHAR* buf = c->getQuery()->toString(field);
        buffer.append(buf);
        _CLDELETE_CARRAY( buf );
      }
      if (i != clauses->size()-1)
        buffer.append(_T(" "));

      if (getBoost() != 1.0) {
         buffer.append(_T(")^"));
         buffer.appendFloat(getBoost(),1);
      }
    }
    return buffer.toString();
  }

		bool BooleanQuery::isCoordDisabled() { return disableCoord; }
		void BooleanQuery::setCoordDisabled( bool disableCoord ) { this->disableCoord = disableCoord; }

    BooleanClause** BooleanQuery::getClauses() const
	{
		CND_MESSAGE(false, "Warning: BooleanQuery::getClauses() is deprecated")
		BooleanClause** ret = _CL_NEWARRAY(BooleanClause*, clauses->size()+1);
		getClauses(ret);
		return ret;
	}
	
	void BooleanQuery::getClauses(BooleanClause** ret) const
	{
		size_t size=clauses->size();
		for ( uint32_t i=0;i<size;i++ )
			ret[i] = (*clauses)[i];
	}
	  Query* BooleanQuery::rewrite(IndexReader* reader) {
         if (clauses->size() == 1) {                    // optimize 1-clause queries
            BooleanClause* c = (*clauses)[0];
            if (!c->prohibited) {			  // just return clause
				Query* query = c->getQuery()->rewrite(reader);    // rewrite first

				//if the query doesn't actually get re-written,
				//then return a clone (because the BooleanQuery
				//will register different to the returned query.
				if ( query == c->getQuery() )
					query = query->clone();

				if (getBoost() != 1.0f) {                 // incorporate boost
					query->setBoost(getBoost() * query->getBoost());
				}

				return query;
            }
         }

         BooleanQuery* clone = NULL;                    // recursively rewrite
		 for (uint32_t i = 0 ; i < clauses->size(); i++) {
            BooleanClause* c = (*clauses)[i];
            Query* query = c->getQuery()->rewrite(reader);
            if (query != c->getQuery()) {                     // clause rewrote: must clone
               if (clone == NULL)
                  clone = (BooleanQuery*)this->clone();
			   //todo: check if delete query should be on...
			   //in fact we should try and get rid of these
			   //for compatibility sake
               clone->clauses->set (i, _CLNEW BooleanClause(query, true, c->required, c->prohibited));
            }
         }
         if (clone != NULL) {
			 return clone;                               // some clauses rewrote
         } else
            return this;                                // no clauses rewrote
      }


      Query* BooleanQuery::clone()  const{
		 BooleanQuery* clone = _CLNEW BooleanQuery(*this);
         return clone;
      }

      /** Returns true iff <code>o</code> is equal to this. */
      bool BooleanQuery::equals(Query* o)const {
         if (!(o->instanceOf(BooleanQuery::getClassName())))
            return false;
         const BooleanQuery* other = (BooleanQuery*)o;

		 bool ret = (this->getBoost() == other->getBoost());
		 if ( ret ){
			 CLListEquals<BooleanClause,BooleanClause_Compare, const BooleanQuery::ClausesType, const BooleanQuery::ClausesType> comp;
			 ret = comp.equals(this->clauses,other->clauses);
		 }
		return ret;
      }


	float_t BooleanWeight::getValue() { return parentQuery->getBoost(); }
	Query* BooleanWeight::getQuery() { return (Query*)parentQuery; }

	BooleanWeight::BooleanWeight(Searcher* searcher, 
		CLVector<BooleanClause*,Deletor::Object<BooleanClause> >* clauses, BooleanQuery* parentQuery) 
	{
		this->searcher = searcher;
		this->similarity = parentQuery->getSimilarity( searcher );
		this->parentQuery = parentQuery;
		this->clauses = clauses;
		for (uint32_t i = 0 ; i < clauses->size(); i++) {
			weights.push_back((*clauses)[i]->getQuery()->_createWeight(searcher));
		}
	}
	BooleanWeight::~BooleanWeight(){
		this->weights.clear();
	}

    float_t BooleanWeight::sumOfSquaredWeights() {
      float_t sum = 0.0f;
      for (uint32_t i = 0 ; i < weights.size(); i++) {
        BooleanClause* c = (*clauses)[i];
        Weight* w = weights[i];
        if (!c->prohibited)
          sum += w->sumOfSquaredWeights();         // sum sub weights
      }
      sum *= parentQuery->getBoost() * parentQuery->getBoost();             // boost each sub-weight
      return sum ;
    }

    void BooleanWeight::normalize(float_t norm) {
      norm *= parentQuery->getBoost();                         // incorporate boost
      for (uint32_t i = 0 ; i < weights.size(); i++) {
        BooleanClause* c = (*clauses)[i];
        Weight* w = weights[i];
        if (!c->prohibited)
          w->normalize(norm);
      }
    }

    Scorer* BooleanWeight::scorer(IndexReader* reader){
      // First see if the (faster) ConjunctionScorer will work.  This can be
      // used when all clauses are required.  Also, at this point a
      // BooleanScorer cannot be embedded in a ConjunctionScorer, as the hits
      // from a BooleanScorer are not always sorted by document number (sigh)
      // and hence BooleanScorer cannot implement skipTo() correctly, which is
      // required by ConjunctionScorer.
      bool allRequired = true;
      bool noneBoolean = true;
	  { //msvc6 scope fix
		  for (uint32_t i = 0 ; i < weights.size(); i++) {
			BooleanClause* c = (*clauses)[i];
			if (!c->required)
			  allRequired = false;
			if (c->getQuery()->instanceOf(BooleanQuery::getClassName()))
			  noneBoolean = false;
		  }
	  }

      if (allRequired && noneBoolean) {           // ConjunctionScorer is okay
        ConjunctionScorer* result =
          _CLNEW ConjunctionScorer(parentQuery->getSimilarity(searcher));
        for (uint32_t i = 0 ; i < weights.size(); i++) {
          Weight* w = weights[i];
          Scorer* subScorer = w->scorer(reader);
          if (subScorer == NULL)
            return NULL;
          result->add(subScorer);
        }
        return result;
      }

      // Use good-old BooleanScorer instead.
      BooleanScorer* result = _CLNEW BooleanScorer(parentQuery->getSimilarity(searcher));

	  { //msvc6 scope fix
		  for (uint32_t i = 0 ; i < weights.size(); i++) {
			BooleanClause* c = (*clauses)[i];
			Weight* w = weights[i];
			Scorer* subScorer = w->scorer(reader);
			if (subScorer != NULL)
			  result->add(subScorer, c->required, c->prohibited);
			else if (c->required)
			  return NULL;
		  }
	  }

      return result;
    }

	void BooleanWeight::explain(IndexReader* reader, int32_t doc, Explanation* result){
      int32_t coord = 0;
      int32_t maxCoord = 0;
      float_t sum = 0.0f;
      Explanation* sumExpl = _CLNEW Explanation;
      for (uint32_t i = 0 ; i < weights.size(); i++) {
        BooleanClause* c = (*clauses)[i];
        Weight* w = weights[i];
        Explanation* e = _CLNEW Explanation;
        w->explain(reader, doc, e);
        if (!c->prohibited) 
           maxCoord++;
        if (e->getValue() > 0) {
          if (!c->prohibited) {
            sumExpl->addDetail(e);
            sum += e->getValue();
            coord++;
            e = NULL; //prevent e from being deleted
          } else {
            //we want to return something else...
            _CLDELETE(sumExpl);
            result->setValue(0.0f);
            result->setDescription(_T("match prohibited"));
            return;
          }
        } else if (c->required) {
            _CLDELETE(sumExpl);
            result->setValue(0.0f);
            result->setDescription(_T("match prohibited"));
            return;
        }
        
        _CLDELETE(e);
      }
      sumExpl->setValue(sum);

      if (coord == 1){                               // only one clause matched
		  Explanation* tmp = sumExpl;
		  sumExpl = sumExpl->getDetail(0)->clone();          // eliminate wrapper
		  _CLDELETE(tmp);
      }

      sumExpl->setDescription(_T("sum of:"));
	  float_t coordFactor = parentQuery->getSimilarity(searcher)->coord(coord, maxCoord);
	  if (coordFactor == 1.0f){                      // coord is no-op
        result->set(*sumExpl);                       // eliminate wrapper
		_CLDELETE(sumExpl);
	  } else {
        result->setDescription( _T("product of:"));
        result->addDetail(sumExpl);

        StringBuffer explbuf;
        explbuf.append(_T("coord("));
        explbuf.appendInt(coord);
        explbuf.append(_T("/"));
        explbuf.appendInt(maxCoord);
        explbuf.append(_T(")"));
        result->addDetail(_CLNEW Explanation(coordFactor, explbuf.getBuffer()));
        result->setValue(sum*coordFactor);
      }
    }

	BooleanWeight2::BooleanWeight2( 
		Searcher* searcher,
		CL_NS(util)::CLVector<BooleanClause*,CL_NS(util)::Deletor::Object<BooleanClause> >* clauses,
		BooleanQuery* parentQuery 
		): BooleanWeight( searcher, clauses, parentQuery )
	{		
	}
	BooleanWeight2::~BooleanWeight2(){
	}

	Scorer* BooleanWeight2::scorer( CL_NS(index)::IndexReader* reader )
	{
		BooleanScorer2* result = _CLNEW BooleanScorer2( similarity, parentQuery->getMinNrShouldMatch(), false );

		for ( size_t i = 0; i < weights.size(); i++ ) {
			BooleanClause* c = (*clauses)[i];
			Weight* w = (Weight*)weights[i];
			Scorer* subScorer = w->scorer( reader );
			if ( subScorer != NULL ) {
				result->add( subScorer, c->required, c->prohibited );
			} else if ( c->required ) {
				return NULL;
			}
		}

		return result;

	}
	
	
	
	
	BooleanClause::BooleanClause(Query* q, const bool DeleteQuery,const bool req, const bool p):
	query(q),
		required(req),
		prohibited(p),
		occur(SHOULD),
		deleteQuery(DeleteQuery)
	{
		if (required) {
			if (prohibited) {
				// prohibited && required doesn't make sense, but we want the old behaviour:
				occur = MUST_NOT;
			} else {
				occur = MUST;
			}
		} else {
			if (prohibited) {
				occur = MUST_NOT;
			} else {
				occur = SHOULD;
			}
		}
	}

	BooleanClause::BooleanClause(const BooleanClause& clone):
    	LuceneVoidBase(),
		query(clone.query->clone()),
		required(clone.required),
		prohibited(clone.prohibited),
		occur(clone.occur),
		deleteQuery(true)
	{
	}

	BooleanClause::BooleanClause(Query* q, const bool DeleteQuery, Occur o):
    	LuceneVoidBase(),
		query(query),
		occur(o),
		deleteQuery(DeleteQuery)
	{
		setFields(occur);
	}


	BooleanClause* BooleanClause::clone() const {
		BooleanClause* ret = _CLNEW BooleanClause(*this);
		return ret;
	}

	BooleanClause::~BooleanClause(){
		if ( deleteQuery )
			_CLDELETE( query );
	}


	/** Returns true if <code>o</code> is equal to this. */
	bool BooleanClause::equals(const BooleanClause* other) const {
		return this->query->equals(other->query)
			&& (this->required == other->required)
			&& (this->prohibited == other->prohibited) // TODO: Remove these
			&& (this->occur == other->getOccur() );
	}

	/** Returns a hash code value for this object.*/
	size_t BooleanClause::hashCode() const {
		return query->hashCode() ^ ( (occur == MUST) ?1:0) ^ ( (occur == MUST_NOT)?2:0);
	}

	BooleanClause::Occur BooleanClause::getOccur() const { return occur; };
	void BooleanClause::setOccur(Occur o) {
		occur = o;
		setFields(o);
	};

	Query* BooleanClause::getQuery() const { return query; };
	void BooleanClause::setQuery(Query* q) {
		if ( deleteQuery )
			_CLDELETE( query );
		query = q;
	}

	bool BooleanClause::isProhibited() const { return prohibited; /* TODO: return (occur == MUST_NOT); */ }
	bool BooleanClause::isRequired() const { return required; /* TODO: return (occur == MUST); */ }

	TCHAR* BooleanClause::toString() const {
		CL_NS(util)::StringBuffer buffer;
		if (occur == MUST)
			buffer.append(_T("+"));
		else if (occur == MUST_NOT)
			buffer.append(_T("-"));
		buffer.append( query->toString() );
		return buffer.toString();
	}
	
	void BooleanClause::setFields(Occur occur) {
		if (occur == MUST) {
			required = true;
			prohibited = false;
		} else if (occur == SHOULD) {
			required = false;
			prohibited = false;
		} else if (occur == MUST_NOT) {
			required = false;
			prohibited = true;
		} else {
			_CLTHROWT (CL_ERR_UnknownOperator,  _T("Unknown operator"));
		}
	}

CL_NS_END
