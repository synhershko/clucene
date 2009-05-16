/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"

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

	Token* next(Token* token) {
		if (inPhrase) {
			inPhrase = false;
			token->set( _T("phrase2"), savedStart, savedEnd);
			return token;
		}else{
			while( input->next(token) ){
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
	TokenStream* tokenStream(const TCHAR* fieldName, Reader* reader) {
		return _CLNEW QPTestFilter(_CLNEW LowerCaseTokenizer(reader));
	}
};

Query* getQuery(CuTest *tc,const TCHAR* query, Analyzer* a) {
	try{
		bool del = (a==NULL);
		if (a == NULL)
			a = _CLNEW SimpleAnalyzer();

		QueryParser qp(_T("field"), a);
		Query* ret = qp.parse(query);
		
		if ( del )
			_CLDELETE(a);
		return ret;
	}catch(CLuceneError& e){
		CuFail(tc,_T("/%s/ threw an error: %s - "),query, e.twhat());
		CuFail(tc,_T("%s\n"),e.twhat());
		return NULL;
	}catch(...){
		CuFail(tc,_T("/%s/ threw an error.\n"),query);
		return NULL;
	}
}

void assertQueryEquals(CuTest *tc,const TCHAR* query, Analyzer* a, const TCHAR* result)  {
	
	Query* q = getQuery(tc,query, a);
	if ( q == NULL )
		return;

	TCHAR* s = q->toString(_T("field"));
	int ret = _tcscmp(s,result);
	_CLDELETE_CARRAY(s);
	_CLDELETE(q);
	if ( ret != 0 ) {
		CuFail(tc, _T("FAILED Query /%s/ yielded /%s/, expecting /%s/\n"), query, s,
			result);
	}
		
}

void assertTrue(CuTest *tc,const TCHAR* query, Analyzer* a, const char* inst, const TCHAR* msg){
	Query* q = getQuery(tc,query,a);
	bool success = q->instanceOf(inst);
	_CLDELETE(q);
	CuAssert(tc,msg,success);
}

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

	//assertQueryEquals(tc, _T("\"\""), &b, _T("")); // -- new test, crashes CLucene
    //assertQueryEquals(tc, _T("foo:\"\""), &b, _T("foo:")); // -- new test, crashes CLucene

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

    //test string buffer
    StringBuffer sb;
    sb.appendFloat(0.02f,2);
    CuAssertStrEquals(tc, _T("appendFloat failed"), _T("0.02"), sb.getBuffer());

    // make sure OR is the default:
	QueryParser* qp = _CLNEW QueryParser(_T("field"), &a);
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
	assertQueryEquals(tc,_T("term~2"), NULL, _T("term"));
	assertQueryEquals(tc,_T("term~0.5"), NULL, _T("term"));
	assertQueryEquals(tc,_T("term~0.6"), NULL, _T("term"));
	assertQueryEquals(tc,_T("\" \"~2 germ"), NULL, _T("germ"));
	assertQueryEquals(tc,_T("\"term germ\"~2^2"), NULL, _T("\"term germ\"~2^2.0") );
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

void testWildcard(CuTest *tc) {

	assertQueryEquals(tc,_T("term*"), NULL, _T("term*"));
	assertQueryEquals(tc,_T("term*^2"), NULL, _T("term*^2.0"));

	assertQueryEquals(tc,_T("term~"), NULL, _T("term~0.5"));
	assertQueryEquals(tc,_T("term~0.5"), NULL, _T("term"));
	assertQueryEquals(tc,_T("term~^2"), NULL, _T("term^2.0~0.5"));
	assertQueryEquals(tc,_T("term^2~"), NULL, _T("term^2.0~0.5"));
	assertTrue(tc, _T("term~"), NULL,"FuzzyQuery", _T("term~0.5"));

	assertQueryEquals(tc,_T("term*germ"), NULL, _T("term*germ"));
	assertQueryEquals(tc,_T("term*germ^3"), NULL, _T("term*germ^3.0"));

	assertTrue(tc, _T("term*"), NULL,"PrefixQuery", _T("term*"));
	assertTrue(tc, _T("term*^2"), NULL,"PrefixQuery", _T("term*^2.0"));
	assertTrue(tc, _T("t*"), NULL,"PrefixQuery", _T("t*"));
	assertTrue(tc, _T("term*germ"), NULL,"WildcardQuery", _T("term*germ"));
}

void testQPA(CuTest *tc) {
	QPTestAnalyzer qpAnalyzer;
	assertQueryEquals(tc,_T("term term term"), &qpAnalyzer, _T("term term term") );
	assertQueryEquals(tc,_T("term +stop term"), &qpAnalyzer, _T("term term") );
	assertQueryEquals(tc,_T("term -stop term"), &qpAnalyzer, _T("term term") );
	assertQueryEquals(tc,_T("drop AND stop AND roll"), &qpAnalyzer, _T("+drop +roll") );
	assertQueryEquals(tc,_T("term phrase term"), &qpAnalyzer, 
					_T("term \"phrase1 phrase2\" term") );
	assertQueryEquals(tc,_T("term AND NOT phrase term"), &qpAnalyzer, 
					_T("+term -\"phrase1 phrase2\" term") );
	assertQueryEquals(tc,_T("stop"), &qpAnalyzer, _T("") );
	assertTrue(tc, _T("term term term"), &qpAnalyzer,"BooleanQuery", _T("term term term"));
	assertTrue(tc, _T("term +stop"), &qpAnalyzer,"TermQuery", _T("term +stop"));
}

void testRange(CuTest *tc) {
	StandardAnalyzer a;

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

	assertQueryEquals(tc,_T("[050-070]"), &a, _T("[050 TO -070]"));
}

void testEscaped(CuTest *tc) {
	WhitespaceAnalyzer a;
	assertQueryEquals(tc, _T("\\[brackets"), &a, _T("[brackets") );
	assertQueryEquals(tc, _T("\\\\\\[brackets"), &a, _T("\\[brackets") );
    assertQueryEquals(tc,_T("\\[brackets"), NULL, _T("brackets") );

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
}


CuSuite *testQueryParser(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene Query Parser Test"));

    SUITE_ADD_TEST(suite, testSimple);
    SUITE_ADD_TEST(suite, testQPA);
    SUITE_ADD_TEST(suite, testEscaped);
    SUITE_ADD_TEST(suite, testNumber);
    SUITE_ADD_TEST(suite, testPunct);

    SUITE_ADD_TEST(suite, testSlop);
    SUITE_ADD_TEST(suite, testRange);
    SUITE_ADD_TEST(suite, testWildcard);

    return suite; 
}
// EOF
