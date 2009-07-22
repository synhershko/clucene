/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"
#include "CLucene/index/IndexModifier.h"
#include <iostream>
#include <sstream>

void testIMinsertDelete(CuTest *tc){
	RAMDirectory ram;
	SimpleAnalyzer a;

	IndexModifier ndx2(&ram,&a,true);
	ndx2.close();
	IndexModifier ndx(&ram,&a,false);

	ndx.setUseCompoundFile(false);
	ndx.setMergeFactor(2);
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
	ndx.optimize();
	ndx.close();

	//test the ram loading
	RAMDirectory ram2(&ram);
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
CuSuite *testindexmodifier(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene IndexModifier Test"));
	SUITE_ADD_TEST(suite, testIMinsertDelete);

  return suite; 
}
// EOF
