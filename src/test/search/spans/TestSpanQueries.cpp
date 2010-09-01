/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "TestBasics.h"
#include "TestSpans.h"
#include "TestNearSpansOrdered.h"
#include "TestSpansAdvanced2.h"

/////////////////////////////////////////////////////////////////////////////
void testBasics( CuTest * tc )
{
    TestBasics basicsTest( tc );
    basicsTest.setUp();
    basicsTest.testTerm();
    basicsTest.testTerm2();
    basicsTest.testPhrase();
    basicsTest.testPhrase2();
    basicsTest.testBoolean();
    basicsTest.testBoolean2();
    basicsTest.testSpanNearExact();
    basicsTest.testSpanNearUnordered();
    basicsTest.testSpanNearOrdered();
    basicsTest.testSpanNot();
    basicsTest.testSpanWithMultipleNotSingle();
    basicsTest.testSpanWithMultipleNotMany();
    basicsTest.testNpeInSpanNearWithSpanNot();
    basicsTest.testNpeInSpanNearInSpanFirstInSpanNot();
    basicsTest.testSpanFirst();
    basicsTest.testSpanOr();
    basicsTest.testSpanExactNested(); 
    basicsTest.testSpanNearOr();
    basicsTest.testSpanComplex1();
}

/////////////////////////////////////////////////////////////////////////////
void testSpans( CuTest * tc )
{
    TestSpans spansTest( tc );
    spansTest.setUp();
    spansTest.testSpanNearOrdered();
    spansTest.testSpanNearOrderedEqual();
    spansTest.testSpanNearOrderedEqual1();
    spansTest.testSpanNearOrderedOverlap();
    
    // CLucene specific: SpanOr query with no clauses are not allowed
    // spansTest.testSpanOrEmpty();     
    
    spansTest.testSpanOrSingle();
    spansTest.testSpanOrDouble();
    spansTest.testSpanOrDoubleSkip();
    spansTest.testSpanOrUnused();
    spansTest.testSpanOrTripleSameDoc();
}

/////////////////////////////////////////////////////////////////////////////
void testNearSpansOrdered( CuTest * tc )
{
    TestNearSpansOrdered test( tc );
    test.setUp();
    test.testSpanNearQuery();
    test.testNearSpansNext();
    test.testNearSpansSkipToLikeNext();
    test.testNearSpansNextThenSkipTo();
    test.testNearSpansNextThenSkipPast();
    test.testNearSpansSkipPast();
    test.testNearSpansSkipTo0();
    test.testNearSpansSkipTo1();
    test.testSpanNearScorerSkipTo1();
    test.testSpanNearScorerExplain();
}

/////////////////////////////////////////////////////////////////////////////
void testSpansAdvanced( CuTest * tc )
{
    TestSpansAdvanced test( tc );
    test.setUp();
    test.testBooleanQueryWithSpanQueries();
}

/////////////////////////////////////////////////////////////////////////////
void testSpansAdvanced2( CuTest * tc )
{
    TestSpansAdvanced2 test( tc );
    test.setUp();
    test.testVerifyIndex();
    test.testSingleSpanQuery();
    test.testMultipleDifferentSpanQueries();
    test.testBooleanQueryWithSpanQueries();
}

/////////////////////////////////////////////////////////////////////////////
// Test suite for all tests of span queries
CuSuite *testSpanQueries(void)
{
	CuSuite *suite = CuSuiteNew( _T( "CLucene SpanQuery Tests" ));

    SUITE_ADD_TEST( suite, testBasics );
    SUITE_ADD_TEST( suite, testSpans );
    SUITE_ADD_TEST( suite, testNearSpansOrdered );
    SUITE_ADD_TEST( suite, testSpansAdvanced );
    SUITE_ADD_TEST( suite, testSpansAdvanced2 );

	return suite; 
}

