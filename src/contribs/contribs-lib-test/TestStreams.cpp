/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"
#include "CLucene/util/gzipcompressstream.h"
#include "CLucene/util/gzipinputstream.h"
//#include "CLucene/util/_streambase.h"

CL_NS_USE(store);
CL_NS_USE(util);

void setupStreams(CuTest *tc) {
}

void cleanupStreams(CuTest *tc) {
}

void testDocument(CuTest* tc) {
    //test adding a binary field to the index
    RAMDirectory ram;
    const signed char* tmp = 0;
    const char* str2 = "we all love compressed fields";

    {
        SimpleAnalyzer an;
        IndexWriter writer(&ram, &an, true);
        Document doc2;
        AStringReader stringReader2(str2);
        GZipCompressInputStream* zipStream;
        zipStream = new GZipCompressInputStream(&stringReader2);

        doc2.add(*new Field(_T("test"), zipStream, Field::STORE_YES));
        writer.addDocument(&doc2);

        //done
        writer.close();
    }

    //now read it back...
    IndexReader* reader = IndexReader::open(&ram);
    Document doc2;
    CLUCENE_ASSERT(reader->document(0, doc2));
    InputStream* sb2 = doc2.getField(_T("test"))->streamValue();
    GZipInputStream zip2(sb2, GZipInputStream::ZLIBFORMAT);

    int rd = zip2.read(tmp, 100000, 0);
    std::string str((const char*) tmp, rd);
    CLUCENE_ASSERT(str.compare(str2) == 0);

    _CLDELETE(reader);
}

CuSuite *teststreams(void) {
    CuSuite *suite = CuSuiteNew(_T("CLucene Streams Test"));

    //SUITE_ADD_TEST(suite, setupStreams);
    SUITE_ADD_TEST(suite, testDocument);
    //SUITE_ADD_TEST(suite, cleanupStreams);
    return suite;
}
