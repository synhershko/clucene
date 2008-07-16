/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"
#include "CLucene/store/Directory.h"
#include "CLucene/store/IndexInput.h"

#ifdef _CL_HAVE_WINDOWS_H
 #include <windows.h>
#endif

//todo: this is testing internal stuff
void hashTest(CuTest *tc){
	CLHashMap<const char*,int,Compare::Char,Equals::Char,Deletor::acArray,Deletor::ConstNullVal<int> > map(true,true);
	map.put(STRDUP_AtoA("a1"),1);
	map.put(STRDUP_AtoA("a1"),2);
	map.put(STRDUP_AtoA("a2"),3);
	map.put(STRDUP_AtoA("a3"),4);

	
	CuAssertIntEquals(tc, _T("map size!=3"), 3, map.size());
	
	map.remove("a1");
	CuAssertIntEquals(tc, _T("map size!=2"), 2, map.size());

	map.put(STRDUP_AtoA("a1"),5);
	CuAssertIntEquals(tc, _T("mapsize!=3"),3, map.size());
}

void StoreTest(CuTest *tc,int32_t count, bool ram){
	srand(1251971);
	int32_t i;
	
	uint64_t veryStart = Misc::currentTimeMillis();
	uint64_t start = Misc::currentTimeMillis();

	char fsdir[CL_MAX_PATH];
	sprintf(fsdir,"%s/%s",cl_tempDir, "test.store");
	Directory* store = (ram?(Directory*)_CLNEW RAMDirectory():(Directory*)FSDirectory::getDirectory(fsdir, true) );
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
	CuMessageA(tc, "%d total milliseconds to create\n", Misc::currentTimeMillis() - start);

	if (!ram){
		store->close();
		_CLDECDELETE(store);
		store = (Directory*)FSDirectory::getDirectory(fsdir, false);
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

	CuMessageA(tc,"%d total milliseconds to read\n", Misc::currentTimeMillis() - start);

	srand(1251971);
	start = Misc::currentTimeMillis();

	for (i = 0; i < count; i++) {
		_snprintf(name,260,"%d.dat",i);
		store->deleteFile(name);
	}

	CuMessageA(tc, "%d total milliseconds to delete\n",Misc::currentTimeMillis() - start);
	CuMessageA(tc, "%d total milliseconds \n", Misc::currentTimeMillis() - veryStart);

	//test makeLock::toString
	CL_NS(store)::LuceneLock* lf = store->makeLock("testlock");
	TCHAR* ts = lf->toString();
	CLUCENE_ASSERT(_tcscmp(ts,_T("fail")));
    _CLDELETE_CARRAY(ts);
    _CLDELETE(lf);
	
	store->close();
	_CLDECDELETE(store);
}

void ramtest(CuTest *tc){
	StoreTest(tc,10000,true);
}
void fstest(CuTest *tc){
	StoreTest(tc,1000,false);
}

CuSuite *teststore(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene Store Test"));

    SUITE_ADD_TEST(suite, hashTest);
    SUITE_ADD_TEST(suite, ramtest);
    SUITE_ADD_TEST(suite, fstest);

    return suite; 
}
// EOF
