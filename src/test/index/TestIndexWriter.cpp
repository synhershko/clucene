/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"
#include <iostream>

//checks if a merged index finds phrases correctly
void testIWmergePhraseSegments(CuTest *tc){
	char fsdir[CL_MAX_PATH];
	sprintf(fsdir,"%s/%s",cl_tempDir, "test.indexwriter");
	SimpleAnalyzer a;
  Directory* dir = FSDirectory::getDirectory(fsdir, true);

	IndexWriter ndx2(dir,&a,true);
	ndx2.setUseCompoundFile(false);
	Document doc0;
	doc0.add(
		*_CLNEW Field(
			_T("field0"),
			_T("value0 value1"),
			Field::STORE_YES | Field::INDEX_TOKENIZED
		)
	);
	ndx2.addDocument(&doc0);
	ndx2.optimize();
	ndx2.close();

	IndexWriter ndx(fsdir,&a,false);
	ndx.setUseCompoundFile(false);
	Document doc1;
	doc1.add(
		*_CLNEW Field(
			_T("field0"),
			_T("value1 value0"),
			Field::STORE_YES | Field::INDEX_TOKENIZED
		)
	);
	ndx.addDocument(&doc1);
	ndx.optimize();
	ndx.close();

	//test the index querying
	IndexSearcher searcher(fsdir);
	Query* query0 = QueryParser::parse(
		_T("\"value0 value1\""),
		_T("field0"),
		&a
	);
	Hits* hits0 = searcher.search(query0);
	CLUCENE_ASSERT(hits0->length() > 0);
	Query* query1 = QueryParser::parse(
		_T("\"value1 value0\""),
		_T("field0"),
		&a
	);
	Hits* hits1 = searcher.search(query1);
	CLUCENE_ASSERT(hits1->length() > 0);
	_CLDELETE(query0);
	_CLDELETE(query1);
	_CLDELETE(hits0);
	_CLDELETE(hits1);
	_CLDECDELETE(dir);
}

//checks that adding more than the min_merge value goes ok...
//checks for a mem leak that used to occur
void testIWmergeSegments1(CuTest *tc){
	RAMDirectory ram;
	SimpleAnalyzer a;

  IndexWriter ndx2(&ram,&a,true);
	ndx2.close(); //test immediate closing bug reported

	IndexWriter ndx(&ram,&a,true); //set create to false

	ndx.setUseCompoundFile(false);
	ndx.setMergeFactor(2);
	TCHAR fld[1000];
	for ( int i=0;i<1000;i++ ){
    English::IntToEnglish(i,fld,1000);

		Document doc;

		doc.add ( *_CLNEW Field(_T("field0"),fld,Field::STORE_YES | Field::INDEX_TOKENIZED) );
		doc.add ( *_CLNEW Field(_T("field1"),fld,Field::STORE_YES | Field::INDEX_TOKENIZED) );
		doc.add ( *_CLNEW Field(_T("field2"),fld,Field::STORE_YES | Field::INDEX_TOKENIZED) );
		doc.add ( *_CLNEW Field(_T("field3"),fld,Field::STORE_YES | Field::INDEX_TOKENIZED) );
		ndx.addDocument(&doc);
	}
	//ndx.optimize(); //optimize so we can read terminfosreader with segmentreader
	ndx.close();

	//test the ram loading
	RAMDirectory ram2(&ram);
	IndexReader* reader2 = IndexReader::open(&ram2);
	Term* term = _CLNEW Term(_T("field0"),fld);
	TermEnum* en = reader2->terms(term);
	CLUCENE_ASSERT(en->next());
	_CLDELETE(en);
	_CLDECDELETE(term);
	_CLDELETE(reader2);
}

//checks if appending to an index works correctly
void testIWmergeSegments2(CuTest *tc){
	char fsdir[CL_MAX_PATH];
	sprintf(fsdir,"%s/%s",cl_tempDir, "test.indexwriter");
	SimpleAnalyzer a;
  Directory* dir = FSDirectory::getDirectory(fsdir, true);

	IndexWriter ndx2(dir,&a,true);
	ndx2.setUseCompoundFile(false);
	Document doc0;
	doc0.add(
		*_CLNEW Field(
			_T("field0"),
			_T("value0"),
			Field::STORE_YES | Field::INDEX_TOKENIZED
		)
	);
	ndx2.addDocument(&doc0);
	ndx2.optimize();
	ndx2.close();

	IndexWriter ndx(fsdir,&a,false);
	ndx.setUseCompoundFile(false);
	Document doc1;
	doc1.add(
		*_CLNEW Field(
			_T("field0"),
			_T("value1"),
			Field::STORE_YES | Field::INDEX_TOKENIZED
		)
	);
	ndx.addDocument(&doc1);
	ndx.optimize();
	ndx.close();

	//test the ram querying
	IndexSearcher searcher(fsdir);
	Term* term0 = _CLNEW Term(_T("field0"),_T("value1"));
	Query* query0 = QueryParser::parse(_T("value0"),_T("field0"),&a);
	Hits* hits0 = searcher.search(query0);
	CLUCENE_ASSERT(hits0->length() > 0);
	Term* term1 = _CLNEW Term(_T("field0"),_T("value0"));
	Query* query1 = QueryParser::parse(_T("value1"),_T("field0"),&a);
	Hits* hits1 = searcher.search(query1);
	CLUCENE_ASSERT(hits1->length() > 0);
	_CLDELETE(query0);
	_CLDELETE(query1);
	_CLDELETE(hits0);
  _CLDELETE(hits1);
	_CLDECDELETE(term0);
	_CLDECDELETE(term1);
	_CLDECDELETE(dir);
}

void testAddIndexes(CuTest *tc){
	char reuters_origdirectory[1024];
  strcpy(reuters_origdirectory, clucene_data_location);
  strcat(reuters_origdirectory, "/reuters-21578-index");

  {
    RAMDirectory dir;
    WhitespaceAnalyzer a;
    IndexWriter w(&dir, &a, true);
    ValueArray<Directory*> dirs(2);
    dirs[0] = FSDirectory::getDirectory(reuters_origdirectory);
    dirs[1] = FSDirectory::getDirectory(reuters_origdirectory);
    w.addIndexesNoOptimize(dirs);
    w.flush();
    CLUCENE_ASSERT(w.docCount()==62); //31 docs in reuters...
  }
  {
    RAMDirectory dir;
    WhitespaceAnalyzer a;
    IndexWriter w(&dir, &a, true);
    ValueArray<Directory*> dirs(2);
    dirs[0] = FSDirectory::getDirectory(reuters_origdirectory);
    dirs[1] = FSDirectory::getDirectory(reuters_origdirectory);
    w.addIndexes(dirs);
    w.flush();
    CLUCENE_ASSERT(w.docCount()==62); //31 docs in reuters...
  }
}

CuSuite *testindexwriter(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene IndexWriter Test"));
	SUITE_ADD_TEST(suite, testAddIndexes);
	SUITE_ADD_TEST(suite, testIWmergeSegments1);
  SUITE_ADD_TEST(suite, testIWmergeSegments2);
	SUITE_ADD_TEST(suite, testIWmergePhraseSegments);

  return suite;
}
// EOF
