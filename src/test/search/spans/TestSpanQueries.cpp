/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "TestBasics.h"
#include "TestSpans.h"


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
// Test suite for all tests of span queries
CuSuite *testSpanQueries(void)
{
	CuSuite *suite = CuSuiteNew( _T( "CLucene SpanQuery Tests" ));

	SUITE_ADD_TEST( suite, testBasics );
	SUITE_ADD_TEST( suite, testSpans );

	return suite; 
}

