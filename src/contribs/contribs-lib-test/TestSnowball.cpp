#include "test.h"

#include "CLucene/snowball/SnowballAnalyzer.h"

CL_NS_USE2(analysis, snowball);

void testSnowball(CuTest *tc) {
    SnowballAnalyzer an(_T("English"));
    CL_NS(util)::StringReader reader(_T("he abhorred accents"));

    TokenStream* ts = an.tokenStream(_T("test"), &reader);
    Token t;
    CLUCENE_ASSERT(ts->next(&t));
    CLUCENE_ASSERT(_tcscmp(t.termBuffer(), _T("he")) == 0);
    CLUCENE_ASSERT(ts->next(&t));
    CLUCENE_ASSERT(_tcscmp(t.termBuffer(), _T("abhor")) == 0);
    CLUCENE_ASSERT(ts->next(&t));
    CLUCENE_ASSERT(_tcscmp(t.termBuffer(), _T("accent")) == 0);

    CLUCENE_ASSERT(ts->next(&t) == false);
    _CLDELETE(ts);
}

CuSuite *testsnowball(void) {
    CuSuite *suite = CuSuiteNew(_T("CLucene Snowball Test"));

    SUITE_ADD_TEST(suite, testSnowball);

    return suite;
}
