/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "SearchHeader.h"
#include "CLucene/document/Document.h"
#include "Similarity.h"
#include "BooleanQuery.h"
#include "Searchable.h"
#include "Hits.h"
#include "_FieldDocSortedHitQueue.h"

CL_NS_USE(index)
CL_NS_DEF(search)

CL_NS(document)::Document* Searchable::doc(const int32_t i){
    CL_NS(document)::Document* ret = _CLNEW CL_NS(document)::Document;
    if (!doc(i,ret) )
        _CLDELETE(ret);
    return ret;
}

//static
Query* Query::mergeBooleanQueries(Query** queries) {
    CL_NS(util)::CLVector<BooleanClause*> allClauses;
    int32_t i = 0;
    int32_t queriesLength = 0;
    
    while ( queries[i] != NULL ){
		BooleanQuery* bq = (BooleanQuery*)queries[i];
		
		int32_t size = bq->getClauseCount();
		BooleanClause** clauses = _CL_NEWARRAY(BooleanClause*, size);
		bq->getClauses(clauses);
		
		for (int32_t j = 0;j<size;++j ){
			allClauses.push_back(clauses[j]);
			j++;
		}
		_CLDELETE_ARRAY(clauses);
		i++;
		queriesLength++;
    }

    bool coordDisabled = ( queriesLength == 0 ) ? false : ((BooleanQuery*)queries[0])->isCoordDisabled();
    BooleanQuery* result = _CLNEW BooleanQuery(coordDisabled);
    
    CL_NS(util)::CLVector<BooleanClause*>::iterator itr = allClauses.begin();
    while (itr != allClauses.end() ) {
		result->add(*itr);
    }
    return result;
}

Query::Query(const Query& clone):boost(clone.boost){
		//constructor
}
Weight* Query::_createWeight(Searcher* searcher){
	_CLTHROWA(CL_ERR_UnsupportedOperation,"UnsupportedOperationException: Query::_createWeight");
}

Query::Query():
   boost(1.0f)
{
	//constructor
}
Query::~Query(){
}

/** Expert: called to re-write queries into primitive queries. */
Query* Query::rewrite(CL_NS(index)::IndexReader* reader){
   return this;
}

Query* Query::combine(Query** queries){
   _CLTHROWA(CL_ERR_UnsupportedOperation,"UnsupportedOperationException: Query::combine");
}
Similarity* Query::getSimilarity(Searcher* searcher) {
   return searcher->getSimilarity();
}
bool Query::instanceOf(const char* other) const{
   const char* t = getQueryName();
	if ( t==other || strcmp( t, other )==0 )
		return true;
	else
		return false;
}
TCHAR* Query::toString() const{
   return toString(LUCENE_BLANK_STRING);
}

void Query::setBoost(float_t b) { boost = b; }

float_t Query::getBoost() const { return boost; }

Weight* Query::weight(Searcher* searcher){
    Query* query = searcher->rewrite(this);
    Weight* weight = query->_createWeight(searcher);
    float_t sum = weight->sumOfSquaredWeights();
    float_t norm = getSimilarity(searcher)->queryNorm(sum);
    weight->normalize(norm);
    return weight;
}

TopFieldDocs::TopFieldDocs (int32_t totalHits, FieldDoc** fieldDocs, int32_t scoreDocsLen, SortField** fields):
 TopDocs (totalHits, NULL, scoreDocsLen)
{
	this->fields = fields;
	this->fieldDocs = fieldDocs;
	this->scoreDocs = _CL_NEWARRAY(ScoreDoc,scoreDocsLen);
	for (int32_t i=0;i<scoreDocsLen;i++ )
		this->scoreDocs[i] = this->fieldDocs[i]->scoreDoc;
}
TopFieldDocs::~TopFieldDocs(){
	if ( fieldDocs ){
		for (int32_t i=0;i<scoreDocsLength;i++)
			_CLDELETE(fieldDocs[i]);
		_CLDELETE_ARRAY(fieldDocs);
	}
	if ( fields != NULL ){
       for ( int i=0;fields[i]!=NULL;i++ )
           _CLDELETE(fields[i]);
       _CLDELETE_ARRAY(fields);
    }
}

TopDocs::TopDocs(const int32_t th, ScoreDoc*sds, int32_t scoreDocsLen):
    totalHits(th),
	scoreDocs(sds),
	scoreDocsLength(scoreDocsLen)
{
//Func - Constructor
//Pre  - sds may or may not be NULL
//       sdLength >= 0
//Post - The instance has been created

}

TopDocs::~TopDocs(){
//Func - Destructor
//Pre  - true
//Post - The instance has been destroyed

	_CLDELETE_ARRAY(scoreDocs);
}



Searcher::Searcher(){
	similarity = Similarity::getDefault();
}
Searcher::~Searcher(){

}

Hits* Searcher::search(Query* query) {
	return search(query, (Filter*)NULL );
}

Hits* Searcher::search(Query* query, Filter* filter) {
	return _CLNEW Hits(this, query, filter);
}

Hits* Searcher::search(Query* query, const Sort* sort){
	return _CLNEW Hits(this, query, NULL, sort);
}

Hits* Searcher::search(Query* query, Filter* filter, const Sort* sort){
	return _CLNEW Hits(this, query, filter, sort);
}

void Searcher::_search(Query* query, HitCollector* results) {
	_search(query, NULL, results);
}

void Searcher::setSimilarity(Similarity* similarity) {
	this->similarity = similarity;
}

Similarity* Searcher::getSimilarity(){
	return this->similarity;
}

const char* Searcher::getClassName(){
	return "Searcher";
}

const char* Searcher::getObjectName(){
	return Searcher::getClassName();
}


Weight::~Weight(){
}

TCHAR* Weight::toString(){
     return STRDUP_TtoT(_T("Weight"));
}


Searchable::~Searchable(){
}


CL_NS_END
