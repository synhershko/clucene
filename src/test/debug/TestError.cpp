/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"

	void testError(CuTest *tc){
		const char* msg = "test";
		CLuceneError err(0,msg,false);
		CLuceneError err2 = err;
		CLuceneError* err3 = &err;
		CuAssert(tc,_T("Error did not copy properly"),err.what()!=err2.what());
		CuAssert(tc,_T("Error values did not correspond"),strcmp(err.what(),err2.what())==0);
		
		IndexReader* reader = NULL;
		try{
			RAMDirectory dir;
			reader = IndexReader::open(&dir,true);
		}catch(CLuceneError&){
			_CLDELETE(reader);
		}catch(...){
			_CLDELETE(reader);
			CuAssert(tc,_T("Error did not catch properly"),false);
		}
	}
	

CuSuite *testdebug(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene Debug Test"));

    SUITE_ADD_TEST(suite, testError);

    return suite; 
}
// EOF
