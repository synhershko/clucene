/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"
#include <CLucene/search/MatchAllDocsQuery.h>

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

void testHashingBug(CuTest *tc){
  //Manuel Freiholz's indexing bug

  CL_NS(document)::Document doc;
  CL_NS(document)::Field* field;
  CL_NS(analysis::standard)::StandardAnalyzer analyzer;
  CL_NS(store)::RAMDirectory dir;
  CL_NS(index)::IndexWriter writer(&dir, &analyzer, true, true );

  field = _CLNEW CL_NS(document)::Field( _T("CNS_VERSION"), _T("1"), CL_NS(document)::Field::STORE_NO | CL_NS(document)::Field::INDEX_UNTOKENIZED );
  doc.add( (*field) );

  field = _CLNEW CL_NS(document)::Field( _T("CNS_PID"), _T("5"), CL_NS(document)::Field::STORE_YES | CL_NS(document)::Field::INDEX_UNTOKENIZED );
  doc.add( (*field) );

  field = _CLNEW CL_NS(document)::Field( _T("CNS_DATE"), _T("20090722"), CL_NS(document)::Field::STORE_YES | CL_NS(document)::Field::INDEX_UNTOKENIZED );
  doc.add( (*field) );

  field = _CLNEW CL_NS(document)::Field( _T("CNS_SEARCHDATA"), _T("all kind of data"), CL_NS(document)::Field::STORE_NO | CL_NS(document)::Field::INDEX_TOKENIZED );
  doc.add( (*field) );

  writer.addDocument( &doc ); // ADDING FIRST DOCUMENT. -> this works!

  doc.clear();

  field = _CLNEW CL_NS(document)::Field( _T("CNS_VERSION"), _T("1"), CL_NS(document)::Field::STORE_NO | CL_NS(document)::Field::INDEX_UNTOKENIZED );
  doc.add( (*field) );

  field = _CLNEW CL_NS(document)::Field( _T("CNS_PID"), _T("5"), CL_NS(document)::Field::STORE_YES | CL_NS(document)::Field::INDEX_UNTOKENIZED );
  doc.add( (*field) );

  field = _CLNEW CL_NS(document)::Field( _T("CNS_LINEID"), _T("20"), CL_NS(document)::Field::STORE_YES | CL_NS(document)::Field::INDEX_UNTOKENIZED );
  doc.add( (*field) );

  field = _CLNEW CL_NS(document)::Field( _T("CNS_VT_ORDER"), _T("456033000"), CL_NS(document)::Field::STORE_NO | CL_NS(document)::Field::INDEX_UNTOKENIZED );
  doc.add( (*field) );

  field = _CLNEW CL_NS(document)::Field( _T("CNS_VN_H"), _T("456033000"), CL_NS(document)::Field::STORE_NO | CL_NS(document)::Field::INDEX_UNTOKENIZED );
  doc.add( (*field) );

  field = _CLNEW CL_NS(document)::Field( _T("CNS_VN_HF"), _T("456033000"), CL_NS(document)::Field::STORE_NO | CL_NS(document)::Field::INDEX_UNTOKENIZED );
  doc.add( (*field) );

  field = _CLNEW CL_NS(document)::Field( _T("CNS_VN_D"), _T("456033000"), CL_NS(document)::Field::STORE_NO | CL_NS(document)::Field::INDEX_UNTOKENIZED );
  doc.add( (*field) );

  field = _CLNEW CL_NS(document)::Field( _T("CNS_VN_OD"), _T("456033000"), CL_NS(document)::Field::STORE_NO | CL_NS(document)::Field::INDEX_UNTOKENIZED );
  doc.add( (*field) );

  field = _CLNEW CL_NS(document)::Field( _T("CNS_VN_P1"), _T("456033000"), CL_NS(document)::Field::STORE_NO | CL_NS(document)::Field::INDEX_UNTOKENIZED );
  doc.add( (*field) );

  field = _CLNEW CL_NS(document)::Field( _T("CNS_VN_H1"), _T("456033000"), CL_NS(document)::Field::STORE_NO | CL_NS(document)::Field::INDEX_UNTOKENIZED );
  doc.add( (*field) ); // the problematic field!

  writer.addDocument( &doc ); // ADDING SECOND DOCUMENT - will never return from this function
  writer.optimize();          // stucks in line 222-223
  writer.close();
  _CL_DECREF(&dir);
}

class IWlargeScaleCorrectness_tester {
public:
	void invoke(Directory& storage, CuTest *tc);
};

void IWlargeScaleCorrectness_tester::invoke(
	Directory& storage,
	CuTest *tc
){
	SimpleAnalyzer a;

	IndexWriter* ndx = _CLNEW IndexWriter(&storage,&a,true);

	ndx->setUseCompoundFile(false);

	const long documents = 200;
	const long step = 23;
	const long inverted_step = 113;
	const long repetitions = 5;

	CLUCENE_ASSERT(0 == (step * inverted_step + 1) % documents);

	long value0;
	long value1 = 0;

	long block_size = 1;
	long reopen = 1;

	for (value0 = 0; value0 < documents * repetitions; value0++) {
		if (reopen == value0) {
			ndx->optimize();
			ndx->close();
			_CLDELETE(ndx);
			ndx = _CLNEW IndexWriter(&storage,&a,false);
			ndx->setUseCompoundFile(false);
			reopen += block_size;
			block_size++;
		}

		TCHAR* value0_string = NumberTools::longToString(value0 % documents);
		TCHAR* value1_string = NumberTools::longToString(value1);

		Document doc;
		
		doc.add (
			*_CLNEW Field(
				_T("field0"),
				value0_string,
				Field::STORE_YES | Field::INDEX_UNTOKENIZED
			)
		);
		doc.add (
			*_CLNEW Field(
				_T("field1"),
				value1_string,
				Field::STORE_YES | Field::INDEX_UNTOKENIZED
			)
		);
		ndx->addDocument(&doc);

		_CLDELETE_ARRAY(value0_string);
		_CLDELETE_ARRAY(value1_string);
		value1 = (value1 + step) % documents;
	}

	ndx->optimize();
	ndx->close();

	IndexSearcher searcher(&storage);
	Query* query0 = _CLNEW MatchAllDocsQuery;
	Sort by_value1(
		_CLNEW SortField(
			_T("field1"),
			SortField::STRING,
			true
		)
	);
	Hits* hits0 = searcher.search(query0, &by_value1);
	long last = 0;
	for (long i = 0; i < hits0->length(); i++) {
		Document& retrieved = hits0->doc(i);
		TCHAR const* value = retrieved.get(_T("field0"));
		long current = NumberTools::stringToLong(value);
		long delta = (current + documents - last) % documents;
		if (0 == (i % repetitions)) {
			CLUCENE_ASSERT(inverted_step == delta);
		} else {
			CLUCENE_ASSERT(0 == delta);
		}
		last = current;
	}
	_CLDELETE(query0);
	_CLDELETE(hits0);
	_CLDELETE(ndx);
}

void testIWlargeScaleCorrectness(CuTest *tc){
	char fsdir[CL_MAX_PATH];
	sprintf(fsdir,"%s/%s",cl_tempDir, "test.search");
	RAMDirectory ram;
	FSDirectory* disk = FSDirectory::getDirectory(fsdir, true);
	IWlargeScaleCorrectness_tester().invoke(ram, tc);
	IWlargeScaleCorrectness_tester().invoke(*disk, tc);
	disk->close();
	_CLDECDELETE(disk);
}

CuSuite *testindexwriter(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene IndexWriter Test"));
	SUITE_ADD_TEST(suite, testHashingBug);
	SUITE_ADD_TEST(suite, testAddIndexes);
	SUITE_ADD_TEST(suite, testIWmergeSegments1);
  SUITE_ADD_TEST(suite, testIWmergeSegments2);
	SUITE_ADD_TEST(suite, testIWmergePhraseSegments);
	SUITE_ADD_TEST(suite, testIWlargeScaleCorrectness);

  return suite;
}
// EOF
