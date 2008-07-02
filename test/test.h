/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef TEST_H
#define TEST_H
#include "CLucene/_ApiHeader.h"
#include "CLucene.h"
#include "CLucene/util/_StringBuffer.h"
#include "CLucene/store/RAMDirectory.h"
#include "CLucene/store/Lock.h"
#include "CLucene/config/repl_wchar.h"
#include "CLucene/index/TermVector.h"
#ifdef HAVE_WINDOWS_H
	#include <windows.h>
#endif


CL_NS_USE(index)
CL_NS_USE(util)
CL_NS_USE(store)
CL_NS_USE(search)
CL_NS_USE(document)
CL_NS_USE(queryParser)
CL_NS_USE(analysis)
CL_NS_USE2(analysis,standard)

#include "CuTest.h"

CuSuite *testindexwriter(void);
CuSuite *teststore(void);
CuSuite *testanalysis(void);
CuSuite *testanalyzers(void);
CuSuite *testhighfreq(void);
CuSuite *testhighlight(void);
CuSuite *testpriorityqueue(void);
CuSuite *testQueryParser(void);
CuSuite *testqueries(void);
CuSuite *testsearch(void);
CuSuite *testtermvector(void);
CuSuite *testsort(void);
CuSuite *testduplicates(void);
CuSuite *testdatefilter(void);
CuSuite *testwildcard(void);
CuSuite *testdebug(void);
CuSuite *testutf8(void);
CuSuite *testreuters(void);
CuSuite *testdocument(void);

class English{
public:
    static void IntToEnglish(int32_t i, CL_NS(util)::StringBuffer* result);
    static TCHAR* IntToEnglish(int32_t i);
    static void IntToEnglish(int32_t i, TCHAR* buf, int32_t buflen);
};

extern unittest tests[];

#define CLUCENE_DATA_LOCATION1 "../test/data/"
#define CLUCENE_DATA_LOCATION2 "./test/data/"
#define CLUCENE_DATA_LOCATION3 "../../test/data/"
#define CLUCENE_DATA_LOCATIONENV "srcdir"

extern const char* cl_tempDir;

#endif
