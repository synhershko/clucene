/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"

/// TestBooleanQuery.java, ported 5/9/2009
void testEquality(CuTest *tc) {
    BooleanQuery* bq1 = _CLNEW BooleanQuery();
    Term* t = _CLNEW Term(_T("field"), _T("value1"));
    bq1->add(_CLNEW TermQuery(t), true, BooleanClause::SHOULD);
    _CLDECDELETE(t);
    t = _CLNEW Term(_T("field"), _T("value2"));
    bq1->add(_CLNEW TermQuery(t), true, BooleanClause::SHOULD);
    _CLDECDELETE(t);
    BooleanQuery* nested1 = _CLNEW BooleanQuery();
    t = _CLNEW Term(_T("field"), _T("nestedvalue1"));
    nested1->add(_CLNEW TermQuery(t), true, BooleanClause::SHOULD);
    _CLDECDELETE(t);
    t = _CLNEW Term(_T("field"), _T("nestedvalue2"));
    nested1->add(_CLNEW TermQuery(t), true, BooleanClause::SHOULD);
    _CLDECDELETE(t);
    bq1->add(nested1, true, BooleanClause::SHOULD);

    BooleanQuery* bq2 = _CLNEW BooleanQuery();
    t = _CLNEW Term(_T("field"), _T("value1"));
    bq2->add(_CLNEW TermQuery(t), true, BooleanClause::SHOULD);
    _CLDECDELETE(t);
    t = _CLNEW Term(_T("field"), _T("value2"));
    bq2->add(_CLNEW TermQuery(t), true, BooleanClause::SHOULD);
    _CLDECDELETE(t);
    BooleanQuery* nested2 = _CLNEW BooleanQuery();
    t = _CLNEW Term(_T("field"), _T("nestedvalue1"));
    nested2->add(_CLNEW TermQuery(t), true, BooleanClause::SHOULD);
    _CLDECDELETE(t);
    t = _CLNEW Term(_T("field"), _T("nestedvalue2"));
    nested2->add(_CLNEW TermQuery(t), true, BooleanClause::SHOULD);
    _CLDECDELETE(t);
    bq2->add(nested2, true, BooleanClause::SHOULD);

    CLUCENE_ASSERT(bq1->equals(bq2));

    _CLLDELETE(bq1);
    _CLLDELETE(bq2);
}
void testException(CuTest *tc) {
    try {
        BooleanQuery::setMaxClauseCount(0);
        CuFail(tc, _T("setMaxClauseCount(0) did not throw an exception"));
    } catch (CLuceneError&) {
        // okay
    }
}

/// TestBooleanScorer.java, ported 5/9/2009
void testBooleanScorer(CuTest *tc) {
    const TCHAR* FIELD = _T("category");
    RAMDirectory directory;

    TCHAR* values[] = { _T("1"), _T("2"), _T("3"), _T("4"), NULL};

    try {
        WhitespaceAnalyzer a;
        IndexWriter* writer = _CLNEW IndexWriter(&directory, &a, true);
        for (size_t i = 0; values[i]!=NULL; i++) {
            Document* doc = _CLNEW Document();
            doc->add(*_CLNEW Field(FIELD, values[i], Field::STORE_YES | Field::INDEX_TOKENIZED));
            writer->addDocument(doc);
            _CLLDELETE(doc);
        }
        writer->close();
        _CLLDELETE(writer);

        BooleanQuery* booleanQuery1 = _CLNEW BooleanQuery();
        Term *t = _CLNEW Term(FIELD, _T("1"));
        booleanQuery1->add(_CLNEW TermQuery(t), true, BooleanClause::SHOULD);
        _CLDECDELETE(t);
        t = _CLNEW Term(FIELD, _T("2"));
        booleanQuery1->add(_CLNEW TermQuery(t), true, BooleanClause::SHOULD);
        _CLDECDELETE(t);

        BooleanQuery* query = _CLNEW BooleanQuery();
        query->add(booleanQuery1, true, BooleanClause::MUST);
        t = _CLNEW Term(FIELD, _T("9"));
        query->add(_CLNEW TermQuery(t), true, BooleanClause::MUST_NOT);
        _CLDECDELETE(t);

        IndexSearcher *indexSearcher = _CLNEW IndexSearcher(&directory);
        Hits *hits = indexSearcher->search(query);
        CLUCENE_ASSERT(2 == hits->length()); // Number of matched documents
        _CLLDELETE(hits);
        _CLLDELETE(indexSearcher);

        _CLLDELETE(query);
    }
    catch (CLuceneError& e) {
        CuFail(tc, e.twhat());
    }
}

CuSuite *testBoolean(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene Boolean Tests"));

	SUITE_ADD_TEST(suite, testEquality);
    SUITE_ADD_TEST(suite, testException);

    SUITE_ADD_TEST(suite, testBooleanScorer);

    //_CrtSetBreakAlloc(1266);

	return suite; 
}
// EOF
