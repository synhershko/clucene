/**
 * Copyright 2002-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "CLucene/_ApiHeader.h"
#include "QueryTermExtractor.h"

#include "CLucene/search/Query.h"
#include "CLucene/search/BooleanQuery.h"
#include "CLucene/search/TermQuery.h"
#include "CLucene/search/PhraseQuery.h"
#include "CLucene/index/IndexReader.h"
#include "CLucene/index/Term.h"

CL_NS_DEF2(search,highlight)
CL_NS_USE(index)

	WeightedTerm** QueryTermExtractor::getTerms(const Query *query) 
	{
		WeightedTerm** ret = getTerms(query,false);
		return ret;
	}

	WeightedTerm** QueryTermExtractor::getTerms(const Query * query, bool prohibited) 
	{
		WeightedTermList terms(false);
		getTerms(query,&terms,prohibited);

		// Return extracted terms
		WeightedTerm** ret = _CL_NEWARRAY(WeightedTerm*,terms.size()+1);
		terms.toArray(ret, true);

		return ret;
	}

	void QueryTermExtractor::getTerms(const Query * query, WeightedTermList * terms,bool prohibited) 
	{
		if (query->instanceOf( BooleanQuery::getClassName() ))
			getTermsFromBooleanQuery((BooleanQuery *) query, terms, prohibited);
		else if (query->instanceOf( PhraseQuery::getClassName() ))
			getTermsFromPhraseQuery((PhraseQuery *) query, terms);
		else if (query->instanceOf( TermQuery::getClassName() ))
			getTermsFromTermQuery((TermQuery *) query, terms);
		//else if(query->instanceOf(_T("SpanNearQuery"))
		//	getTermsFromSpanNearQuery((SpanNearQuery*) query, terms);
	}

	/**
  	* Extracts all terms texts of a given Query into an array of WeightedTerms
  	*
  	* @param query      Query to extract term texts from
  	* @param reader used to compute IDF which can be used to a) score selected fragments better
  	* b) use graded highlights eg chaning intensity of font color
  	* @param fieldName the field on which Inverse Document Frequency (IDF) calculations are based
  	* @return an array of the terms used in a query, plus their weights.
  	*/
  	WeightedTerm** QueryTermExtractor::getIdfWeightedTerms(const Query* query, IndexReader* reader, const TCHAR* fieldName)
  	{
  	    WeightedTermList terms(true);
		getTerms(query,&terms,false);

  	    int32_t totalNumDocs=reader->numDocs();
		
		WeightedTermList::iterator itr = terms.begin();
  	    while ( itr != terms.end() )
  		{
  			try
  			{
				Term* term = _CLNEW Term(fieldName,(*itr)->getTerm());
  				int32_t docFreq=reader->docFreq(term);
				_CLDECDELETE(term);

  				//IDF algorithm taken from DefaultSimilarity class
  				float_t idf=(float_t)(log(totalNumDocs/(float_t)(docFreq+1)) + 1.0);
  				(*itr)->setWeight((*itr)->getWeight() * idf);
  			}catch (CLuceneError& e){
  				if ( e.number()!=CL_ERR_IO )
					throw e;
  			}

			itr++;
  		}
  	   
		// Return extracted terms
		WeightedTerm** ret = _CL_NEWARRAY(WeightedTerm*,terms.size()+1);
		terms.toArray(ret, true);

		return ret;
  	}

	void QueryTermExtractor::getTermsFromBooleanQuery(const BooleanQuery * query, WeightedTermList * terms, bool prohibited)
	{
		uint32_t numClauses = query->getClauseCount();
		BooleanClause** queryClauses = _CL_NEWARRAY(BooleanClause*,numClauses);
		query->getClauses(queryClauses);

		for (uint32_t i = 0; i < numClauses; i++)
		{
			if (prohibited || !queryClauses[i]->prohibited){
				Query* qry = queryClauses[i]->getQuery();
				getTerms(qry, terms, prohibited);
			}
		}

		_CLDELETE_ARRAY(queryClauses);
	}

	void QueryTermExtractor::getTermsFromPhraseQuery(const PhraseQuery * query, WeightedTermList * terms)
	{
		Term** queryTerms = query->getTerms();
		int32_t i = 0;
		while ( queryTerms[i] != NULL ){
			WeightedTerm * pWT = _CLNEW WeightedTerm(query->getBoost(),queryTerms[i]->text());
			if (terms->find(pWT)==terms->end()) // possible memory leak if key already present
				terms->insert(pWT);
			else
				_CLDELETE(pWT);

			i++;
		}
		_CLDELETE_ARRAY(queryTerms);
	}

	void QueryTermExtractor::getTermsFromTermQuery(const TermQuery * query, WeightedTermList * terms)
	{
		Term * term = query->getTerm();
		WeightedTerm * pWT = _CLNEW WeightedTerm(query->getBoost(),term->text());
		_CLDECDELETE(term);
		if (terms->find(pWT)==terms->end()) // possible memory leak if key already present
			terms->insert(pWT);
		else
			_CLDELETE(pWT);
	}

	//todo: implement this when span queries are implemented
	/*void getTermsFromSpanNearQuery(SpanNearQuery* query, WeightedTermList* terms){
  	    Collection queryTerms = query.getTerms();

  	    for(Iterator iterator = queryTerms.iterator(); iterator.hasNext();){
  	        // break it out for debugging.
  	        Term term = (Term) iterator.next();
  	        const TCHAR* text = term.text();
  	        terms.add(_CLNEW WeightedTerm(query.getBoost(), text));
  	    }
  	}*/

CL_NS_END2
