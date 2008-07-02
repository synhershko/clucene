/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"


	void testPrefixQuery(CuTest *tc){
		WhitespaceAnalyzer analyzer;
		RAMDirectory directory;
		TCHAR* categories[] = {_T("/Computers"), _T("/Computers/Mac"), _T("/Computers/Windows")};

		IndexWriter writer( &directory, &analyzer, true);
		for (int i = 0; i < 3; i++) {
			Document *doc = _CLNEW Document();
			doc->add(*_CLNEW Field(_T("category"), categories[i], Field::STORE_YES | Field::INDEX_UNTOKENIZED));
			writer.addDocument(doc);
			_CLDELETE(doc);
		}
		writer.close();

		PrefixQuery *query = _CLNEW PrefixQuery(_CLNEW Term(_T("category"), _T("/Computers")));
		IndexSearcher searcher(&directory);
		Hits *hits = searcher.search(query);
		CLUCENE_ASSERT(3 == hits->length()); // All documents in /Computers category and below
		_CLDELETE(query);

		query = _CLNEW PrefixQuery(_CLNEW Term(_T("category"), _T("/Computers/Mac")));
		hits = searcher.search(query);
		CLUCENE_ASSERT(1 == hits->length()); // One in /Computers/Mac
		_CLDELETE(query);
		_CLDELETE(hits);
	}


CuSuite *testqueries(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene Queries Test"));

    SUITE_ADD_TEST(suite, testPrefixQuery);

    return suite; 
}
// EOF