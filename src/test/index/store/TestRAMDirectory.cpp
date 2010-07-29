/*------------------------------------------------------------------------------
* Copyright (C) 2010 Borivoj Kostka and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"
#include <string>
#include <iostream>

#include "CLucene/analysis/Analyzers.h"
#include "CLucene/document/Document.h"
#include "CLucene/document/Field.h"
#include "CLucene/index/IndexReader.h"
#include "CLucene/index/IndexWriter.h"
#include "CLucene/search/IndexSearcher.h"
#include "CLucene/store/Directory.h"
#include "CLucene/store/FSDirectory.h"
#include "CLucene/store/RAMDirectory.h"

// BK> No idea what it is, doesn't exist in JLucene 2_3_2
//import org.apache.lucene.store.MockRAMDirectory; 

/**
 * JUnit testcase to test RAMDirectory. RAMDirectory itself is used in many testcases,
 * but not one of them uses an different constructor other than the default constructor.
 * 
 * @author Bernhard Messer
 * 
 * @version $Id: RAMDirectory.java 150537 2004-09-28 22:45:26 +0200 (Di, 28 Sep 2004) cutting $
 */

static int docsToAdd = 500;
static char indexDir[CL_MAX_PATH] = "";
  
// setup the index
void testRAMDirectorySetUp (CuTest *tc) {

    if (strlen(cl_tempDir) + 13 > CL_MAX_PATH)
        CuFail(tc, _T("Not enough space in indexDir buffer"));

    sprintf(indexDir, "%s/RAMDirIndex", cl_tempDir);
    
    IndexWriter * writer  = new IndexWriter(indexDir, new WhitespaceAnalyzer(), true);

    // add some documents
    Document doc;
    for (int i = 0; i < docsToAdd; i++) {
      doc.add(* new Field(_T("content"), English::IntToEnglish(i), Field::STORE_YES | Field::INDEX_UNTOKENIZED));
      writer->addDocument(&doc);
      doc.clear();
    }

    CuAssertEquals(tc, docsToAdd, writer->docCount());
    writer->close();
}

// BK> all test functions are the same except RAMDirectory constructor, so shared code moved here
void checkDir(CuTest *tc, RAMDirectory * ramDir) {

    // Check size
    // BK> getRecomputedSizeInBytes() not implemented
    //CuAssertEquals(tc, ramDir->sizeInBytes, ramDir->getRecomputedSizeInBytes());

    // open reader to test document count
    IndexReader * reader = IndexReader::open(ramDir);
    CuAssertEquals(tc, docsToAdd, reader->numDocs());

    // open search to check if all doc's are there
    IndexSearcher * searcher = _CLNEW IndexSearcher(reader);

    // search for all documents
    Document doc;
    for (int i = 0; i < docsToAdd; i++) {
        searcher->doc(i, doc);
        CuAssertTrue(tc, doc.getField(_T("content")) != NULL);
    }

    // cleanup
    reader->close();
    searcher->close();
    _CLLDELETE(reader);
    _CLLDELETE(searcher);
}

void testRAMDirectory (CuTest *tc) {

    Directory * dir = FSDirectory::getDirectory(indexDir);
    // BK> No idea what it is, doesn't exist in JLucene 2_3_2
    //MockRAMDirectory ramDir = new MockRAMDirectory(dir);
    RAMDirectory * ramDir = _CLNEW RAMDirectory(dir);

    // close the underlaying directory
    dir->close();

    checkDir(tc, ramDir);

    ramDir->close();
    _CLLDELETE(ramDir);
}

void testRAMDirectoryString (CuTest *tc) {

    RAMDirectory * ramDir = _CLNEW RAMDirectory(indexDir);

    checkDir(tc, ramDir);

    ramDir->close();
    _CLLDELETE(ramDir);
}

static int numThreads = 50;
static int docsPerThread = 40;

_LUCENE_THREAD_FUNC(indexDocs, _writer){

    IndexWriter * writer = (IndexWriter *)_writer;
    int num = 1;
    Document doc;
    for (int j=1; j<docsPerThread; j++) {
        doc.add(*new Field(_T("sizeContent"), English::IntToEnglish(num*docsPerThread+j), Field::STORE_YES | Field::INDEX_UNTOKENIZED));
        writer->addDocument(&doc);
        doc.clear();
        //synchronized (ramDir) {
        //    assertEquals(ramDir.sizeInBytes(), ramDir.getRecomputedSizeInBytes());
        //}
    }
}
  
void testRAMDirectorySize(CuTest * tc)  {
      
    RAMDirectory * ramDir = _CLNEW RAMDirectory(indexDir);
    IndexWriter * writer;
    
    writer  = _CLNEW IndexWriter(ramDir, new WhitespaceAnalyzer(), false);
    writer->optimize();
    
    //assertEquals(ramDir.sizeInBytes(), ramDir.getRecomputedSizeInBytes());

    _LUCENE_THREADID_TYPE* threads = _CL_NEWARRAY(_LUCENE_THREADID_TYPE, numThreads);
    
    for (int i=0; i<numThreads; i++) {
      threads[i] = _LUCENE_THREAD_CREATE(&indexDocs, writer);
    }

    for (int i=0; i<numThreads; i++){
        _LUCENE_THREAD_JOIN(threads[i]);
    }

    _CLDELETE_ARRAY(threads);

    writer->optimize();
    //assertEquals(ramDir.sizeInBytes(), ramDir.getRecomputedSizeInBytes());
    
    CuAssertEquals(tc, docsToAdd + (numThreads * (docsPerThread-1)), writer->docCount());

    writer->close();
    _CLLDELETE(writer);

    ramDir->close();
    _CLLDELETE(ramDir);
}

#if 0
  public void testSerializable() throws IOException {
    Directory dir = new RAMDirectory();
    ByteArrayOutputStream bos = new ByteArrayOutputStream(1024);
    assertEquals("initially empty", 0, bos.size());
    ObjectOutput out = new ObjectOutputStream(bos);
    int headerSize = bos.size();
    out.writeObject(dir);
    out.close();
    assertTrue("contains more then just header", headerSize < bos.size());
  } 
#endif

void testRAMDirectoryTearDown(CuTest * tc) {
    // cleanup 
    if (*indexDir) {
        // delete index dir + all files in it (portable way!)
    }

}
  

CuSuite *testRAMDirectory(void)
{
    CuSuite *suite = CuSuiteNew(_T("CLucene RAMDirectory Test"));

    SUITE_ADD_TEST(suite, testRAMDirectorySetUp);

    SUITE_ADD_TEST(suite, testRAMDirectory);
    SUITE_ADD_TEST(suite, testRAMDirectoryString);
    SUITE_ADD_TEST(suite, testRAMDirectorySize);

    SUITE_ADD_TEST(suite, testRAMDirectoryTearDown);

    return suite;
}

