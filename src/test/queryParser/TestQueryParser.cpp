/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"


/// Java QueryParser tests
/// Helper functions and classes

class QPTestFilter: public TokenFilter {
public:

	bool inPhrase;
	int32_t savedStart, savedEnd;

	/**
	* Filter which discards the token 'stop' and which expands the
	* token 'phrase' into 'phrase1 phrase2'
	*/
	QPTestFilter(TokenStream* in):
		TokenFilter(in,true),
		inPhrase(false),
		savedStart(0),
		savedEnd(0)
	{
	}

	CL_NS(analysis)::Token* next(CL_NS(analysis)::Token* token) {
		if (inPhrase) {
			inPhrase = false;
			token->set( _T("phrase2"), savedStart, savedEnd);
			return token;
		}else{
			while( input->next(token) != NULL ){
				if ( _tcscmp(token->termBuffer(), _T("phrase")) == 0 ) {
					inPhrase = true;
					savedStart = token->startOffset();
					savedEnd = token->endOffset();
					token->set( _T("phrase1"), savedStart, savedEnd);
					return token;
				}else if ( _tcscmp(token->termBuffer(), _T("stop") ) !=0 ){
					return token;
				}
			}
		}
		return NULL;
	}
};

class QPTestAnalyzer: public Analyzer {
public:
	QPTestAnalyzer() {
	}

	/** Filters LowerCaseTokenizer with StopFilter. */
	TokenStream* tokenStream(const TCHAR* /*fieldName*/, Reader* reader) {
		return _CLNEW QPTestFilter(_CLNEW LowerCaseTokenizer(reader));
	}
};

class QPTestParser : public QueryParser {
public:
	QPTestParser(TCHAR* f, Analyzer* a) : QueryParser(f, a){
	}
	virtual ~QPTestParser(){
	}

protected:
	Query* getFuzzyQuery(TCHAR* /*field*/, TCHAR* /*termStr*/, float_t /*minSimilarity*/) {
		_CLTHROWA(CL_ERR_Parse,"Fuzzy queries not allowed");
	}

	Query* getWildcardQuery(TCHAR* /*field*/, TCHAR* /*termStr*/) {
		_CLTHROWA(CL_ERR_Parse,"Wildcard queries not allowed");
	}
};

QueryParser* getParser(Analyzer* a) {
	if (a == NULL)
		return NULL;
	QueryParser* qp = _CLNEW QueryParser(_T("field"), a);
	qp->setDefaultOperator(QueryParser::OR_OPERATOR);
	return qp;
}

Query* getQuery(CuTest *tc,const TCHAR* query, Analyzer* a, int ignoreCLError=0) {
	bool del = (a==NULL);
	QueryParser* qp = NULL;
	try{
		if (a == NULL)
			a = _CLNEW SimpleAnalyzer();

		qp = getParser(a);
		Query* ret = qp->parse(query);

		_CLLDELETE(qp);
		if ( del )
			_CLLDELETE(a);
		return ret;
	}catch(CLuceneError& e){
		_CLLDELETE(qp);
		if ( del ) _CLLDELETE(a);
		if (ignoreCLError != e.number())
			CuFail(tc,e);
		else
			throw e;
		return NULL;
	}catch(...){
		_CLLDELETE(qp);
		if ( del ) _CLLDELETE(a);
		CuFail(tc,_T("/%s/ threw an error.\n"),query);
		return NULL;
	}
}

void assertQueryEquals(CuTest *tc,const TCHAR* query, Analyzer* a, const TCHAR* result)  {

	Query* q = getQuery(tc,query, a);
	if ( q == NULL ){
		CuFail(tc, _T("getQuery returned NULL unexpectedly for query /%s/\n"), query);
		return;
	}

	TCHAR* s = q->toString(_T("field"));
	int ret = _tcscmp(s,result);
	_CLDELETE(q);
	if ( ret != 0 ) {
		TCHAR str[CL_MAX_PATH];
		_tcscpy(str,s);
		_CLDELETE_CARRAY(s);
		CuFail(tc, _T("FAILED Query /%s/ yielded /%s/, expecting /%s/\n"), query, str, result);
	}
	_CLDELETE_CARRAY(s);
}

void assertWildcardQueryEquals(CuTest *tc, const TCHAR* query, bool lowercase, const TCHAR* result, bool allowLeadingWildcard=false){
	SimpleAnalyzer a;
	QueryParser* qp = getParser(&a);
	qp->setLowercaseExpandedTerms(lowercase);
	qp->setAllowLeadingWildcard(allowLeadingWildcard);
	Query* q = qp->parse(query);
	_CLLDELETE(qp);

	TCHAR* s = q->toString(_T("field"));
	_CLLDELETE(q);
	if (_tcscmp(s,result) != 0) {
		TCHAR str[CL_MAX_PATH];
		_tcscpy(str,s);
		_CLDELETE_CARRAY(s);
		CuFail(tc,_T("WildcardQuery /%s/ yielded /%s/, expecting /%s/"),query, str, result);
	}
	_CLDELETE_CARRAY(s);
}

void assertTrue(CuTest *tc,const TCHAR* query, Analyzer* a, const char* inst, const TCHAR* msg){
	Query* q = getQuery(tc,query,a);
	bool success = q->instanceOf(inst);
	_CLDELETE(q);
	CuAssert(tc,msg,success);
}

void assertTrue(CuTest *tc,Query* q, const char* inst, bool bDeleteQuery = false){
	bool ret = q->instanceOf(inst);
	if (bDeleteQuery) _CLLDELETE(q);
	CuAssertTrue(tc,ret);
}

void assertParseException(CuTest *tc,const TCHAR* queryString) {
	try {
		getQuery(tc,queryString, NULL, CL_ERR_Parse);
	} catch (CLuceneError&){
		return;
	}
	CuFail(tc,_T("ParseException expected, not thrown"));
}

void assertEscapedQueryEquals(CuTest *tc,const TCHAR* query, Analyzer* a, const TCHAR* result){
	TCHAR* escapedQuery = QueryParser::escape(query);
	if (_tcscmp(escapedQuery, result) != 0) {
		TCHAR str[CL_MAX_PATH];
		_tcscpy(str,escapedQuery);
		_CLDELETE_LCARRAY(escapedQuery);
		CuFail(tc, _T("Query /%s/ yielded /%s/, expecting /%s/\n"), query, escapedQuery, result);
	}
	_CLDELETE_LCARRAY(escapedQuery);
}

Query* getQueryDOA(const TCHAR* query, Analyzer* a=NULL) {
	bool bOwnsAnalyzer=false;
	if (a == NULL){
		a = _CLNEW SimpleAnalyzer();
		bOwnsAnalyzer=true;
	}
	QueryParser* qp = _CLNEW QueryParser(_T("field"), a);
	qp->setDefaultOperator(QueryParser::AND_OPERATOR);
	Query* q = qp->parse(query);
	_CLLDELETE(qp);
	if (bOwnsAnalyzer) _CLLDELETE(a);
	return q;
}

void assertQueryEqualsDOA(CuTest *tc,const TCHAR* query, Analyzer* a, const TCHAR* result){
	Query* q = getQueryDOA(query, a);
	TCHAR* s = q->toString(_T("field"));
	_CLLDELETE(q);
	if (_tcscmp(s,result)!=0) {
		TCHAR str[CL_MAX_PATH];
		_tcscpy(str,s);
		_CLDELETE_LCARRAY(s);
		CuFail(tc,_T("Query /%s/ yielded /%s/, expecting /%s/"),query, str, result);
	}
	_CLDELETE_LCARRAY(s);
}

/// END Helper functions and classes

void testSimple(CuTest *tc) {
	StandardAnalyzer a;
	KeywordAnalyzer b;
	assertQueryEquals(tc,_T("term term term"), NULL, _T("term term term"));

#ifdef _UCS2
	TCHAR tmp1[100];

	lucene_utf8towcs(tmp1,"t\xc3\xbcrm term term",100);
	assertQueryEquals(tc,tmp1, NULL, tmp1);
	assertQueryEquals(tc,tmp1, &a, tmp1);

	lucene_utf8towcs(tmp1,"\xc3\xbcmlaut",100);
	assertQueryEquals(tc,tmp1, NULL, tmp1);
	assertQueryEquals(tc,tmp1, &a, tmp1);
#endif

	// TODO: Those 2 fail, related probably to the escape function
	//assertQueryEquals(tc, _T("\"\""), &b, _T(""));
	//assertQueryEquals(tc, _T("foo:\"\""), &b, _T("foo:"));

	assertQueryEquals(tc,_T("a AND b"), NULL, _T("+a +b"));
	assertQueryEquals(tc,_T("(a AND b)"), NULL, _T("+a +b"));
	assertQueryEquals(tc,_T("c OR (a AND b)"), NULL, _T("c (+a +b)"));
	assertQueryEquals(tc,_T("a AND NOT b"), NULL, _T("+a -b"));
	assertQueryEquals(tc,_T("a AND -b"), NULL, _T("+a -b"));
	assertQueryEquals(tc,_T("a AND !b"), NULL, _T("+a -b"));
	assertQueryEquals(tc,_T("a && b"), NULL, _T("+a +b"));
	assertQueryEquals(tc,_T("a && ! b"), NULL, _T("+a -b"));

	assertQueryEquals(tc,_T("a OR b"), NULL, _T("a b"));
	assertQueryEquals(tc,_T("a || b"), NULL, _T("a b"));
	assertQueryEquals(tc,_T("a OR !b"), NULL, _T("a -b"));
	assertQueryEquals(tc,_T("a OR ! b"), NULL, _T("a -b"));
	assertQueryEquals(tc,_T("a OR -b"), NULL, _T("a -b"));

	assertQueryEquals(tc,_T("+term -term term"), NULL, _T("+term -term term"));
	assertQueryEquals(tc,_T("foo:term AND field:anotherTerm"), NULL,
					_T("+foo:term +anotherterm"));
	assertQueryEquals(tc,_T("term AND \"phrase phrase\""), NULL,
					_T("+term +\"phrase phrase\"") );
	assertQueryEquals(tc,_T("\"hello there\""), NULL, _T("\"hello there\"") );

	assertTrue(tc, _T("a AND b"), NULL,"BooleanQuery",_T("a AND b") );
	assertTrue(tc, _T("hello"), NULL,"TermQuery", _T("hello"));
	assertTrue(tc, _T("\"hello there\""), NULL,"PhraseQuery", _T("\"hello there\""));

	assertQueryEquals(tc,_T("germ term^2.0"), NULL, _T("germ term^2.0"));
    assertQueryEquals(tc,_T("(term)^2.0"), NULL, _T("term^2.0"));
	assertQueryEquals(tc,_T("(germ term)^2.0"), NULL, _T("(germ term)^2.0"));
	assertQueryEquals(tc,_T("term^2.0"), NULL, _T("term^2.0"));
	assertQueryEquals(tc,_T("term^2"), NULL, _T("term^2.0"));
	assertQueryEquals(tc,_T("term^2.3"), NULL, _T("term^2.3"));
	assertQueryEquals(tc,_T("\"germ term\"^2.0"), NULL, _T("\"germ term\"^2.0"));
	assertQueryEquals(tc,_T("\"germ term\"^2.02"), NULL, _T("\"germ term\"^2.0"));
	assertQueryEquals(tc,_T("\"term germ\"^2"), NULL, _T("\"term germ\"^2.0") );

	assertQueryEquals(tc,_T("(foo OR bar) AND (baz OR boo)"), NULL,
					_T("+(foo bar) +(baz boo)"));
	assertQueryEquals(tc,_T("((a OR b) AND NOT c) OR d"), NULL,
					_T("(+(a b) -c) d"));
	assertQueryEquals(tc,_T("+(apple \"steve jobs\") -(foo bar baz)"), NULL,
					_T("+(apple \"steve jobs\") -(foo bar baz)") );
	assertQueryEquals(tc,_T("+title:(dog OR cat) -author:\"bob dole\""), NULL,
					_T("+(title:dog title:cat) -author:\"bob dole\"") );

	QueryParser* qp = _CLNEW QueryParser(_T("field"), &a);
	// make sure OR is the default:
	CLUCENE_ASSERT(QueryParser::OR_OPERATOR == qp->getDefaultOperator());
	qp->setDefaultOperator(QueryParser::AND_OPERATOR);
	CLUCENE_ASSERT(QueryParser::AND_OPERATOR == qp->getDefaultOperator());

	// try creating a query and make sure it uses AND
	Query* bq = qp->parse(_T("term1 term2"));
	CLUCENE_ASSERT( bq != NULL );
	TCHAR* s = bq->toString(_T("field"));
	if ( _tcscmp(s,_T("+term1 +term2")) != 0 ) {
		CuFail(tc, _T("FAILED Query /term1 term2/ yielded /%s/, expecting +term1 +term2\n"), s);
	}
	_CLDELETE_CARRAY(s);
	_CLDELETE(bq);

	qp->setDefaultOperator(QueryParser::OR_OPERATOR);
	CLUCENE_ASSERT(QueryParser::OR_OPERATOR == qp->getDefaultOperator());
	_CLDELETE(qp);
}

void testPunct(CuTest *tc) {
	WhitespaceAnalyzer a;
	assertQueryEquals(tc,_T("a&b"), &a, _T("a&b"));
	assertQueryEquals(tc,_T("a&&b"), &a, _T("a&&b"));
	assertQueryEquals(tc,_T(".NET"), &a, _T(".NET"));
}

void testSlop(CuTest *tc) {
	assertQueryEquals(tc,_T("\"term germ\"~2"), NULL, _T("\"term germ\"~2") );
	assertQueryEquals(tc,_T("\"term germ\"~2 flork"), NULL, _T("\"term germ\"~2 flork") );
	assertQueryEquals(tc,_T("\"term\"~2"), NULL, _T("term"));
	assertQueryEquals(tc,_T("\" \"~2 germ"), NULL, _T("germ"));
	assertQueryEquals(tc,_T("\"term germ\"~2^2"), NULL, _T("\"term germ\"~2^2.0") );

	/*
	###  These do not work anymore with the new QP, and they do not exist in the official Java tests
	assertQueryEquals(tc,_T("term~2"), NULL, _T("term"));
	assertQueryEquals(tc,_T("term~0.5"), NULL, _T("term"));
	assertQueryEquals(tc,_T("term~0.6"), NULL, _T("term"));
	*/
}

void testNumber(CuTest *tc) {
	// The numbers go away because SimpleAnalzyer ignores them
	assertQueryEquals(tc,_T("3"), NULL, _T(""));
	assertQueryEquals(tc,_T("term 1.0 1 2"), NULL, _T("term"));
	assertQueryEquals(tc,_T("term term1 term2"), NULL, _T("term term term"));

	StandardAnalyzer a;
	assertQueryEquals(tc,_T("3"), &a, _T("3"));
	assertQueryEquals(tc,_T("term 1.0 1 2"), &a, _T("term 1.0 1 2"));
	assertQueryEquals(tc,_T("term term1 term2"), &a, _T("term term1 term2"));
}

void testWildcard(CuTest *tc)
{
	assertQueryEquals(tc,_T("term*"), NULL, _T("term*"));
	assertQueryEquals(tc,_T("term*^2"), NULL, _T("term*^2.0"));
	assertQueryEquals(tc,_T("term~"), NULL, _T("term~0.5"));
	assertQueryEquals(tc,_T("term~0.7"), NULL, _T("term~0.7"));
	assertQueryEquals(tc,_T("term~^2"), NULL, _T("term~0.5^2.0"));
	assertQueryEquals(tc,_T("term^2~"), NULL, _T("term~0.5^2.0"));
	assertQueryEquals(tc,_T("term*germ"), NULL, _T("term*germ"));
	assertQueryEquals(tc,_T("term*germ^3"), NULL, _T("term*germ^3.0"));

	assertTrue(tc, _T("term*"), NULL,"PrefixQuery", _T("term*"));
	assertTrue(tc, _T("term*^2"), NULL,"PrefixQuery", _T("term*^2.0"));
	assertTrue(tc, _T("term~"), NULL,"FuzzyQuery", _T("term~0.5"));
	assertTrue(tc, _T("term~0.7"), NULL,"FuzzyQuery", _T("term~0.7"));
	assertTrue(tc, _T("t*"), NULL,"PrefixQuery", _T("t*"));

	FuzzyQuery* fq = (FuzzyQuery*)getQuery(tc,_T("term~0.7"), NULL);
	CuAssertTrue(tc, 0.7 == fq->getMinSimilarity()/*, 0.1*/);
	CuAssertTrue(tc, FuzzyQuery::defaultPrefixLength == fq->getPrefixLength());
	_CLLDELETE(fq);
	fq = (FuzzyQuery*)getQuery(tc, _T("term~"), NULL);
	CuAssertTrue(tc, 0.5 == fq->getMinSimilarity()/*, 0.1*/);
	CuAssertTrue(tc, FuzzyQuery::defaultPrefixLength == fq->getPrefixLength());
	_CLDELETE(fq);

	assertParseException(tc,_T("term~1.1"));	// value > 1, throws exception

	assertTrue(tc, _T("term*germ"), NULL,"WildcardQuery", _T("term*germ"));

	/* Tests to see that wild card terms are (or are not) properly
	* lower-cased with propery parser configuration
	*/
	// First prefix queries:
	// by default, convert to lowercase:
	assertWildcardQueryEquals(tc,_T("Term*"), true, _T("term*"));
	// explicitly set lowercase:
	assertWildcardQueryEquals(tc,_T("term*"), true, _T("term*"));
	assertWildcardQueryEquals(tc,_T("Term*"), true, _T("term*"));
	assertWildcardQueryEquals(tc,_T("TERM*"), true, _T("term*"));
	// explicitly disable lowercase conversion:
	assertWildcardQueryEquals(tc,_T("term*"), false, _T("term*"));
	assertWildcardQueryEquals(tc,_T("Term*"), false, _T("Term*"));
	assertWildcardQueryEquals(tc,_T("TERM*"), false, _T("TERM*"));
	// Then 'full' wildcard queries:
	// by default, convert to lowercase:
	assertQueryEquals(tc,_T("Te?m"), NULL,_T("te?m"));
	// explicitly set lowercase:
	assertWildcardQueryEquals(tc,_T("te?m"), true, _T("te?m"));
	assertWildcardQueryEquals(tc,_T("Te?m"), true, _T("te?m"));
	assertWildcardQueryEquals(tc,_T("TE?M"), true, _T("te?m"));
	assertWildcardQueryEquals(tc,_T("Te?m*gerM"), true, _T("te?m*germ"));
	// explicitly disable lowercase conversion:
	assertWildcardQueryEquals(tc,_T("te?m"), false, _T("te?m"));
	assertWildcardQueryEquals(tc,_T("Te?m"), false, _T("Te?m"));
	assertWildcardQueryEquals(tc,_T("TE?M"), false, _T("TE?M"));
	assertWildcardQueryEquals(tc,_T("Te?m*gerM"), false, _T("Te?m*gerM"));
	//  Fuzzy queries:
	assertQueryEquals(tc,_T("Term~"), NULL,_T("term~0.5"));
	assertWildcardQueryEquals(tc,_T("Term~"), true, _T("term~0.5"));
	assertWildcardQueryEquals(tc,_T("Term~"), false, _T("Term~0.5"));
	//  Range queries:
	assertQueryEquals(tc,_T("[A TO C]"), NULL,_T("[a TO c]"));
	assertWildcardQueryEquals(tc,_T("[A TO C]"), true, _T("[a TO c]"));
	assertWildcardQueryEquals(tc,_T("[A TO C]"), false, _T("[A TO C]"));
	// Test suffix queries: first disallow
	assertParseException(tc,_T("*Term"));
	assertParseException(tc,_T("?Term"));

	// Test suffix queries: then allow
	assertWildcardQueryEquals(tc,_T("*Term"), true, _T("*term"), true);
	assertWildcardQueryEquals(tc,_T("?Term"), true, _T("?term"), true);

	// ### not in the Java tests
	// assertQueryEquals(tc,_T("term~0.5"), NULL, _T("term"));
}

void testLeadingWildcardType(CuTest *tc) {
	SimpleAnalyzer a;
	QueryParser* qp = getParser(&a);
	qp->setAllowLeadingWildcard(true);
	assertTrue(tc, qp->parse(_T("t*erm*")), WildcardQuery::getClassName(), true);
	assertTrue(tc, qp->parse(_T("?t*erm*")), WildcardQuery::getClassName(), true); // should not throw an exception
	assertTrue(tc, qp->parse(_T("*t*erm*")), WildcardQuery::getClassName(), true);
	_CLLDELETE(qp);
}

void testQPA(CuTest *tc) {
	QPTestAnalyzer qpAnalyzer;
	assertQueryEquals(tc,_T("term term^3.0 term"), &qpAnalyzer, _T("term term^3.0 term") );
	assertQueryEquals(tc,_T("term stop^3.0 term"), &qpAnalyzer, _T("term term") );

	assertQueryEquals(tc,_T("term term term"), &qpAnalyzer, _T("term term term") );
	assertQueryEquals(tc,_T("term +stop term"), &qpAnalyzer, _T("term term") );
	assertQueryEquals(tc,_T("term -stop term"), &qpAnalyzer, _T("term term") );

	assertQueryEquals(tc,_T("drop AND (stop) AND roll"), &qpAnalyzer, _T("+drop +roll") );
	assertQueryEquals(tc,_T("term +(stop) term"), &qpAnalyzer, _T("term term") );
	assertQueryEquals(tc,_T("term -(stop) term"), &qpAnalyzer, _T("term term") );

	assertQueryEquals(tc,_T("drop AND stop AND roll"), &qpAnalyzer, _T("+drop +roll") );
	assertQueryEquals(tc,_T("term phrase term"), &qpAnalyzer,
					_T("term \"phrase1 phrase2\" term") );
	assertQueryEquals(tc,_T("term AND NOT phrase term"), &qpAnalyzer,
					_T("+term -\"phrase1 phrase2\" term") );
	assertQueryEquals(tc,_T("stop^3"), &qpAnalyzer, _T("") );
	assertQueryEquals(tc,_T("stop"), &qpAnalyzer, _T("") );
	assertQueryEquals(tc,_T("(stop)^3"), &qpAnalyzer, _T("") );
	assertQueryEquals(tc,_T("((stop))^3"), &qpAnalyzer, _T("") );
	assertQueryEquals(tc,_T("(stop^3)"), &qpAnalyzer, _T("") );
	assertQueryEquals(tc,_T("((stop)^3)"), &qpAnalyzer, _T("") );
	assertQueryEquals(tc,_T("(stop)"), &qpAnalyzer, _T("") );
	assertQueryEquals(tc,_T("((stop))"), &qpAnalyzer, _T("") );
	assertTrue(tc, _T("term term term"), &qpAnalyzer,"BooleanQuery", _T("term term term"));
	assertTrue(tc, _T("term +stop"), &qpAnalyzer,"TermQuery", _T("term +stop"));
}

void testRange(CuTest *tc) {
	StandardAnalyzer a;

    assertQueryEquals(tc, _T("[ a TO z]"), NULL, _T("[a TO z]"));
	/*
	TODO: Complete RangeQuery portion in QP and enable this
    assertTrue(getQuery("[ a TO z]", null) instanceof ConstantScoreRangeQuery);

    QueryParser qp = new QueryParser("field", new SimpleAnalyzer());
	qp.setUseOldRangeQuery(true);
    assertTrue(qp.parse("[ a TO z]") instanceof RangeQuery);
    */
	assertQueryEquals(tc, _T("[ a TO z ]"), NULL, _T("[a TO z]"));
	assertQueryEquals(tc, _T("{ a TO z}"), NULL, _T("{a TO z}"));
	assertQueryEquals(tc, _T("{ a TO z }"), NULL, _T("{a TO z}"));
	assertQueryEquals(tc, _T("{ a TO z }^2.0"), NULL, _T("{a TO z}^2.0"));
	assertQueryEquals(tc, _T("[ a TO z] OR bar"), NULL, _T("[a TO z] bar"));
	assertQueryEquals(tc, _T("[ a TO z] AND bar"), NULL, _T("+[a TO z] +bar"));
	assertQueryEquals(tc, _T("( bar blar { a TO z}) "), NULL, _T("bar blar {a TO z}"));
	assertQueryEquals(tc, _T("gack ( bar blar { a TO z}) "), NULL, _T("gack (bar blar {a TO z})"));


	// Old CLucene tests - check this is working without TO as well
	assertQueryEquals(tc,_T("[ a z]"), NULL, _T("[a TO z]"));
	assertTrue(tc, _T("[ a z]"), NULL, "RangeQuery", _T("[ a z]") );
	assertQueryEquals(tc,_T("[ a z ]"), NULL, _T("[a TO z]"));
	assertQueryEquals(tc,_T("{ a z}"), NULL, _T("{a TO z}"));
	assertQueryEquals(tc,_T("{ a z }"), NULL, _T("{a TO z}"));
	assertQueryEquals(tc,_T("{ a z }^2.0"), NULL, _T("{a TO z}^2.0"));
	assertQueryEquals(tc,_T("[ a z] OR bar"), NULL, _T("[a TO z] bar"));
	assertQueryEquals(tc,_T("[ a z] AND bar"), NULL, _T("+[a TO z] +bar"));
	assertQueryEquals(tc,_T("( bar blar { a z}) "), NULL, _T("bar blar {a TO z}"));
	assertQueryEquals(tc,_T("gack ( bar blar { a z}) "), NULL, _T("gack (bar blar {a TO z})"));

	// ### Incompatiable with new QP, and does not appear in the Java tests; use the format below instead
	// assertQueryEquals(tc,_T("[050-070]"), &a, _T("[050 TO -070]"));
	assertQueryEquals(tc,_T("[050 -070]"), &a, _T("[050 TO -070]"));
}

void testEscaped(CuTest *tc) {
	WhitespaceAnalyzer a;
	/*assertQueryEquals(tc, _T("\\[brackets"), &a, _T("[brackets") );
	assertQueryEquals(tc, _T("\\\\\\[brackets"), &a, _T("\\[brackets") );
    assertQueryEquals(tc,_T("\\[brackets"), NULL, _T("brackets") );*/

	assertQueryEquals(tc,_T("\\a"), &a, _T("a") );

    assertQueryEquals(tc,_T("a\\-b:c"), &a, _T("a-b:c") );
    assertQueryEquals(tc,_T("a\\+b:c"), &a, _T("a+b:c") );
    assertQueryEquals(tc,_T("a\\:b:c"), &a, _T("a:b:c") );
    assertQueryEquals(tc,_T("a\\\\b:c"), &a, _T("a\\b:c") );

    assertQueryEquals(tc,_T("a:b\\-c"), &a, _T("a:b-c") );
    assertQueryEquals(tc,_T("a:b\\+c"), &a, _T("a:b+c") );
    assertQueryEquals(tc,_T("a:b\\:c"), &a, _T("a:b:c") );
    assertQueryEquals(tc,_T("a:b\\\\c"), &a, _T("a:b\\c") );

    assertQueryEquals(tc,_T("a:b\\-c*"), &a, _T("a:b-c*") );
    assertQueryEquals(tc,_T("a:b\\+c*"), &a, _T("a:b+c*") );
    assertQueryEquals(tc,_T("a:b\\:c*"), &a, _T("a:b:c*") );

    assertQueryEquals(tc,_T("a:b\\\\c*"), &a, _T("a:b\\c*") );

    assertQueryEquals(tc,_T("a:b\\-?c"), &a, _T("a:b-?c") );
    assertQueryEquals(tc,_T("a:b\\+?c"), &a, _T("a:b+?c") );
    assertQueryEquals(tc,_T("a:b\\:?c"), &a, _T("a:b:?c") );

    assertQueryEquals(tc,_T("a:b\\\\?c"), &a, _T("a:b\\?c") );

    assertQueryEquals(tc,_T("a:b\\-c~"), &a, _T("a:b-c~0.5") );
    assertQueryEquals(tc,_T("a:b\\+c~"), &a, _T("a:b+c~0.5") );
    assertQueryEquals(tc,_T("a:b\\:c~"), &a, _T("a:b:c~0.5") );
    assertQueryEquals(tc,_T("a:b\\\\c~"), &a, _T("a:b\\c~0.5") );

    assertQueryEquals(tc,_T("[ a\\- TO a\\+ ]"), &a, _T("[a- TO a+]") );
    assertQueryEquals(tc,_T("[ a\\: TO a\\~ ]"), &a, _T("[a: TO a~]") );
    assertQueryEquals(tc,_T("[ a\\\\ TO a\\* ]"), &a, _T("[a\\ TO a*]") );

    assertQueryEquals(tc, _T("[\"c\\:\\\\temp\\\\\\~foo0.txt\" TO \"c\\:\\\\temp\\\\\\~foo9.txt\"]"), &a,
                      _T("[c:\\temp\\~foo0.txt TO c:\\temp\\~foo9.txt]"));

    assertQueryEquals(tc, _T("a\\\\\\+b"), &a, _T("a\\+b"));

    assertQueryEquals(tc, _T("a \\\"b c\\\" d"), &a, _T("a \"b c\" d"));
    assertQueryEquals(tc, _T("\"a \\\"b c\\\" d\""), &a, _T("\"a \"b c\" d\""));
    assertQueryEquals(tc, _T("\"a \\+b c d\""), &a, _T("\"a +b c d\""));

    assertQueryEquals(tc, _T("c\\:\\\\temp\\\\\\~foo.txt"), &a, _T("c:\\temp\\~foo.txt"));

	assertParseException(tc, _T("XY\\")); // there must be a character after the escape char

    // test unicode escaping
    assertQueryEquals(tc,_T("a\\u0062c"), &a, _T("abc"));
    assertQueryEquals(tc,_T("XY\\u005a"), &a, _T("XYZ"));
    assertQueryEquals(tc,_T("XY\\u005A"), &a, _T("XYZ"));
    assertQueryEquals(tc,_T("\"a \\\\\\u0028\\u0062\\\" c\""), &a, _T("\"a \\(b\" c\""));

    assertParseException(tc,_T("XY\\u005G"));  // test non-hex character in escaped unicode sequence
    assertParseException(tc,_T("XY\\u005"));   // test incomplete escaped unicode sequence

    // Tests bug LUCENE-800
    assertQueryEquals(tc,_T("(item:\\\\ item:ABCD\\\\)"), &a, _T("item:\\ item:ABCD\\"));
    assertParseException(tc,_T("(item:\\\\ item:ABCD\\\\))")); // unmatched closing paranthesis
    assertQueryEquals(tc,_T("\\*"), &a, _T("*"));
    assertQueryEquals(tc,_T("\\\\"), &a, _T("\\"));  // escaped backslash

    assertParseException(tc,_T("\\")); // a backslash must always be escaped
}

// TODO: testLegacyDateRange, testDateRange

void testQueryStringEscaping(CuTest *tc) {
	WhitespaceAnalyzer a;

	assertEscapedQueryEquals(tc, _T("a-b:c"), &a, _T("a\\-b\\:c"));
	assertEscapedQueryEquals(tc,_T("a+b:c"), &a, _T("a\\+b\\:c"));
	assertEscapedQueryEquals(tc, _T("a:b:c"), &a, _T("a\\:b\\:c"));
	assertEscapedQueryEquals(tc, _T("a\\b:c"), &a, _T("a\\\\b\\:c"));

	assertEscapedQueryEquals(tc,_T("a:b-c"), &a, _T("a\\:b\\-c"));
	assertEscapedQueryEquals(tc,_T("a:b+c"), &a, _T("a\\:b\\+c"));
	assertEscapedQueryEquals(tc,_T("a:b:c"), &a, _T("a\\:b\\:c"));
	assertEscapedQueryEquals(tc,_T("a:b\\c"), &a, _T("a\\:b\\\\c"));

	assertEscapedQueryEquals(tc,_T("a:b-c*"), &a, _T("a\\:b\\-c\\*"));
	assertEscapedQueryEquals(tc,_T("a:b+c*"), &a, _T("a\\:b\\+c\\*"));
	assertEscapedQueryEquals(tc,_T("a:b:c*"), &a, _T("a\\:b\\:c\\*"));

	assertEscapedQueryEquals(tc,_T("a:b\\\\c*"), &a, _T("a\\:b\\\\\\\\c\\*"));

	assertEscapedQueryEquals(tc,_T("a:b-?c"), &a, _T("a\\:b\\-\\?c"));
	assertEscapedQueryEquals(tc,_T("a:b+?c"), &a, _T("a\\:b\\+\\?c"));
	assertEscapedQueryEquals(tc,_T("a:b:?c"), &a, _T("a\\:b\\:\\?c"));

	assertEscapedQueryEquals(tc,_T("a:b?c"), &a, _T("a\\:b\\?c"));

	assertEscapedQueryEquals(tc,_T("a:b-c~"), &a, _T("a\\:b\\-c\\~"));
	assertEscapedQueryEquals(tc,_T("a:b+c~"), &a, _T("a\\:b\\+c\\~"));
	assertEscapedQueryEquals(tc,_T("a:b:c~"), &a, _T("a\\:b\\:c\\~"));
	assertEscapedQueryEquals(tc,_T("a:b\\c~"), &a, _T("a\\:b\\\\c\\~"));

	assertEscapedQueryEquals(tc,_T("[ a - TO a+ ]"), NULL, _T("\\[ a \\- TO a\\+ \\]"));
	assertEscapedQueryEquals(tc,_T("[ a : TO a~ ]"), NULL, _T("\\[ a \\: TO a\\~ \\]"));
	assertEscapedQueryEquals(tc,_T("[ a\\ TO a* ]"), NULL, _T("\\[ a\\\\ TO a\\* \\]"));

	// LUCENE-881
	assertEscapedQueryEquals(tc,_T("|| abc ||"), &a, _T("\\|\\| abc \\|\\|"));
	assertEscapedQueryEquals(tc,_T("&& abc &&"), &a, _T("\\&\\& abc \\&\\&"));
}

void testTabNewlineCarriageReturn(CuTest *tc){
	assertQueryEqualsDOA(tc,_T("+weltbank +worlbank"), NULL,
		_T("+weltbank +worlbank"));

	assertQueryEqualsDOA(tc,_T("+weltbank\n+worlbank"), NULL,
		_T("+weltbank +worlbank"));
	assertQueryEqualsDOA(tc,_T("weltbank \n+worlbank"), NULL,
		_T("+weltbank +worlbank"));
	assertQueryEqualsDOA(tc,_T("weltbank \n +worlbank"), NULL,
		_T("+weltbank +worlbank"));

	assertQueryEqualsDOA(tc,_T("+weltbank\r+worlbank"), NULL,
		_T("+weltbank +worlbank"));
	assertQueryEqualsDOA(tc,_T("weltbank \r+worlbank"), NULL,
		_T("+weltbank +worlbank"));
	assertQueryEqualsDOA(tc,_T("weltbank \r +worlbank"), NULL,
		_T("+weltbank +worlbank"));

	assertQueryEqualsDOA(tc,_T("+weltbank\r\n+worlbank"), NULL,
		_T("+weltbank +worlbank"));
	assertQueryEqualsDOA(tc,_T("weltbank \r\n+worlbank"), NULL,
		_T("+weltbank +worlbank"));
	assertQueryEqualsDOA(tc,_T("weltbank \r\n +worlbank"), NULL,
		_T("+weltbank +worlbank"));
	assertQueryEqualsDOA(tc,_T("weltbank \r \n +worlbank"), NULL,
		_T("+weltbank +worlbank"));

	assertQueryEqualsDOA(tc,_T("+weltbank\t+worlbank"), NULL,
		_T("+weltbank +worlbank"));
	assertQueryEqualsDOA(tc,_T("weltbank \t+worlbank"), NULL,
		_T("+weltbank +worlbank"));
	assertQueryEqualsDOA(tc,_T("weltbank \t +worlbank"), NULL,
		_T("+weltbank +worlbank"));
}

void testSimpleDAO(CuTest *tc){
	assertQueryEqualsDOA(tc,_T("term term term"), NULL, _T("+term +term +term"));
	assertQueryEqualsDOA(tc,_T("term +term term"), NULL, _T("+term +term +term"));
	assertQueryEqualsDOA(tc,_T("term term +term"), NULL, _T("+term +term +term"));
	assertQueryEqualsDOA(tc,_T("term +term +term"), NULL, _T("+term +term +term"));
	assertQueryEqualsDOA(tc,_T("-term term term"), NULL, _T("-term +term +term"));
}

void testBoost(CuTest *tc){
	const TCHAR* stopWords[] = {_T("on"), NULL};
	StandardAnalyzer* oneStopAnalyzer = _CLNEW StandardAnalyzer(reinterpret_cast<const TCHAR**>(&stopWords));
	QueryParser* qp = _CLNEW QueryParser(_T("field"), oneStopAnalyzer);
	Query* q = qp->parse(_T("on^1.0"));
	CLUCENE_ASSERT(q != NULL);
	_CLLDELETE(q);
	q = qp->parse(_T("\"hello\"^2.0"));
	CLUCENE_ASSERT(q != NULL);
	CLUCENE_ASSERT(q->getBoost() == 2.0f);
	_CLLDELETE(q);
	q = qp->parse(_T("hello^2.0"));
	CLUCENE_ASSERT(q != NULL);
	CLUCENE_ASSERT(q->getBoost() == 2.0f);
	_CLLDELETE(q);
	q = qp->parse(_T("\"on\"^1.0"));
	CLUCENE_ASSERT(q != NULL);
	_CLLDELETE(q);
	_CLLDELETE(qp);
	_CLLDELETE(oneStopAnalyzer);

	StandardAnalyzer a;
	QueryParser* qp2 = _CLNEW QueryParser(_T("field"), &a);
	q = qp2->parse(_T("the^3"));
	// "the" is a stop word so the result is an empty query:
	CLUCENE_ASSERT(q != NULL);
	TCHAR* tmp = q->toString();
	CLUCENE_ASSERT( _tcscmp(tmp, _T("")) == 0 );
	_CLDELETE_LCARRAY(tmp);
	CLUCENE_ASSERT(1.0f == q->getBoost());
	_CLLDELETE(q);
	_CLLDELETE(qp2);
}

void testMatchAllDocs(CuTest *tc) {
	WhitespaceAnalyzer a;
	QueryParser* qp = _CLNEW QueryParser(_T("field"), &a);
	assertTrue(tc,qp->parse(_T("*:*")),"MatchAllDocsQuery",true);
	assertTrue(tc,qp->parse(_T("(*:*)")),"MatchAllDocsQuery",true);

	BooleanQuery* bq = (BooleanQuery*)qp->parse(_T("+*:* -*:*"));
	BooleanClause** clauses = _CL_NEWARRAY(BooleanClause*, bq->getClauseCount() + 1);
	bq->getClauses(clauses);
	assertTrue(tc, clauses[0]->getQuery(), "MatchAllDocsQuery");
	assertTrue(tc, clauses[1]->getQuery(), "MatchAllDocsQuery");
	_CLDELETE_LARRAY(clauses);
	_CLLDELETE(bq);
	_CLLDELETE(qp);
}


CuSuite *testQueryParser(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene Query Parser Test"));

	SUITE_ADD_TEST(suite, testSimple);
	SUITE_ADD_TEST(suite, testPunct);
	SUITE_ADD_TEST(suite, testSlop);
	SUITE_ADD_TEST(suite, testNumber);
	SUITE_ADD_TEST(suite, testWildcard);
	SUITE_ADD_TEST(suite, testLeadingWildcardType);
	SUITE_ADD_TEST(suite, testQPA);
	SUITE_ADD_TEST(suite, testRange);
	SUITE_ADD_TEST(suite, testEscaped);
	SUITE_ADD_TEST(suite, testQueryStringEscaping);
	SUITE_ADD_TEST(suite, testTabNewlineCarriageReturn);
	SUITE_ADD_TEST(suite, testSimpleDAO);
	SUITE_ADD_TEST(suite, testBoost);

	SUITE_ADD_TEST(suite, testMatchAllDocs);

	return suite;
}
// EOF
