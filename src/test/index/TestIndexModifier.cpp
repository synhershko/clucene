/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"
#include "CLucene/index/IndexModifier.h"
#include <sstream>

class bulk_modification {
public:
	void modify_index(CuTest *tc, IndexModifier& ndx);
};

class incremental_modification {
public:
	void modify_index(CuTest *tc, IndexModifier& ndx);
};

template<typename modification>
class IMinsertDelete_tester : public modification {
public:
	void invoke(Directory& storage, CuTest *tc);
};

void bulk_modification::modify_index(CuTest *tc, IndexModifier& ndx){
	std::basic_stringstream<TCHAR> field;
	for ( int i=0;i<1000;i++ ){
		field.str(_T(""));
		field << _T("fielddata") << i;

		Document doc;
		
		doc.add (
			*_CLNEW Field(
				_T("field0"),
				field.str().c_str(),
				Field::STORE_YES | Field::INDEX_UNTOKENIZED
			)
		);
		ndx.addDocument(&doc);
	}
	for ( int i=0;i<1000;i+=2 ){
		field.str(_T(""));
		field << _T("fielddata") << i;

		Term deleted(
			_T("field0"),
			field.str().c_str(),
			true
		);
		CLUCENE_ASSERT(ndx.deleteDocuments(&deleted) > 0);
	}
}

void incremental_modification::modify_index(CuTest *tc, IndexModifier& ndx){
	std::basic_stringstream<TCHAR> field;
	for ( int i=0;i<1000;i++ ){
		field.str(_T(""));
		field << _T("fielddata") << i;

		Document doc;
		
		doc.add (
			*_CLNEW Field(
				_T("field0"),
				field.str().c_str(),
				Field::STORE_YES | Field::INDEX_UNTOKENIZED
			)
		);
		ndx.addDocument(&doc);
		if ( 0 == i % 2 ) {
			Term deleted(
				_T("field0"),
				field.str().c_str(),
				true
			);
			CLUCENE_ASSERT(ndx.deleteDocuments(&deleted) > 0);
		}
	}
}

template<typename modification>
void IMinsertDelete_tester<modification>::invoke(
	Directory& storage,
	CuTest *tc
){
	SimpleAnalyzer a;

	IndexModifier ndx2(&storage,&a,true);
	ndx2.close();
	IndexModifier ndx(&storage,&a,false);

	ndx.setUseCompoundFile(false);
	ndx.setMergeFactor(2);

	this->modify_index(tc, ndx);

	ndx.optimize();
	ndx.close();

	//test the ram loading
	RAMDirectory ram2(&storage);
	IndexReader* reader2 = IndexReader::open(&ram2);
	Term* term = _CLNEW Term(_T("field0"),_T("fielddata1"));
	TermDocs* en = reader2->termDocs(term);
	CLUCENE_ASSERT(en->next());
	_CLDELETE(en);
	_CLDECDELETE(term);
	term = _CLNEW Term(_T("field0"),_T("fielddata0"));
	en = reader2->termDocs(term);
	CLUCENE_ASSERT(!en->next());
	_CLDELETE(en);
	_CLDECDELETE(term);
	_CLDELETE(reader2);
}

void testIMinsertDelete(CuTest *tc){
	char fsdir[CL_MAX_PATH];
	sprintf(fsdir,"%s/%s",cl_tempDir, "test.search");
	RAMDirectory ram;
	FSDirectory* disk = FSDirectory::getDirectory(fsdir, true);
	IMinsertDelete_tester<bulk_modification>().invoke(ram, tc);
	IMinsertDelete_tester<incremental_modification>().invoke(ram, tc);
	IMinsertDelete_tester<bulk_modification>().invoke(*disk, tc);
	IMinsertDelete_tester<incremental_modification>().invoke(*disk, tc);
	disk->close();
	_CLDECDELETE(disk);
}

CuSuite *testindexmodifier(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene IndexModifier Test"));
	SUITE_ADD_TEST(suite, testIMinsertDelete);

  return suite; 
}
// EOF
