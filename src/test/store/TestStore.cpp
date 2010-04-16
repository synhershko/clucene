/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"
#include "CLucene/store/Directory.h"
#include "CLucene/store/IndexInput.h"
#include <stdlib.h>


void StoreTest(CuTest *tc,int32_t count, bool ram){
	srand(1251971);
	int32_t i;

	uint64_t veryStart = Misc::currentTimeMillis();
	uint64_t start = Misc::currentTimeMillis();

	char fsdir[CL_MAX_PATH];
	_snprintf(fsdir, CL_MAX_PATH, "%s/%s",cl_tempDir, "test.store");
	Directory* store = (ram?(Directory*)_CLNEW RAMDirectory():(Directory*)FSDirectory::getDirectory(fsdir) );
	int32_t LENGTH_MASK = 0xFFF;
	char name[260];

	for (i = 0; i < count; i++) {
		_snprintf(name,260,"%d.dat",i);

		int32_t length = rand() & LENGTH_MASK;
		uint8_t b = (uint8_t)(rand() & 0x7F);

		IndexOutput* file = store->createOutput(name);

		for (int32_t j = 0; j < length; j++)
			file->writeByte(b);

		file->close();
		_CLDELETE(file);
	}
	CuMessageA(tc, "%d total milliseconds to create\n", (int32_t)(Misc::currentTimeMillis() - start));

	if (!ram){
		store->close();
		_CLDECDELETE(store);
		store = (Directory*)FSDirectory::getDirectory(fsdir);
  }else{
    CuMessageA(tc, "Memory used at end: %l", ((RAMDirectory*)store)->sizeInBytes);
  }

	srand(1251971);
	start = Misc::currentTimeMillis();

	for (i = 0; i < count; i++) {
		_snprintf(name,260,"%d.dat",i);
		size_t length = rand() & LENGTH_MASK;
		uint8_t b = (uint8_t)(rand() & 0x7F);
		IndexInput* file = store->openInput(name);

		if (file->length() != length)
			_CLTHROWA(CL_ERR_Runtime, "length incorrect" );

		for (size_t j = 0; j < length; j++){
			if (file->readByte() != b){
				TCHAR buf[100];
				_tprintf(buf,_T("contents incorrect in file %d.dat"),i);
				_CLTHROWT(CL_ERR_Runtime, buf);
			}
		}

		file->close();
		_CLDELETE(file);
	}

	CuMessageA(tc,"%d total milliseconds to read\n", (int32_t)(Misc::currentTimeMillis() - start));

	srand(1251971);
	start = Misc::currentTimeMillis();

	for (i = 0; i < count; i++) {
		_snprintf(name,260,"%d.dat",i);
		store->deleteFile(name);
	}

	CuMessageA(tc, "%d total milliseconds to delete\n",(int32_t)(Misc::currentTimeMillis() - start));
	CuMessageA(tc, "%d total milliseconds \n", (int32_t)(Misc::currentTimeMillis() - veryStart));

	//test makeLock::toString
	CL_NS(store)::LuceneLock* lf = store->makeLock("testlock");
	std::string ts = lf->toString();
	CLUCENE_ASSERT( ts.compare("fail") != 0 );
  _CLDELETE(lf);

	store->close();
	_CLDECDELETE(store);
}

void ramtest(CuTest *tc){
	StoreTest(tc,1000,true);
}
void fstest(CuTest *tc){
	StoreTest(tc,100,false);
}

CuSuite *teststore(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene Store Test"));

    SUITE_ADD_TEST(suite, ramtest);
    SUITE_ADD_TEST(suite, fstest);

    return suite;
}
// EOF
