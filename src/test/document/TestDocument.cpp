/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"

//an in memory input stream for testing binary data
class MemReader: public CL_NS(util)::Reader{
	signed char* value;
	size_t len;
	int64_t pos;
public:
	MemReader ( const char* value, const int32_t length = -1 ){
		if ( length >= 0 )
			this->len = length;
		else
			this->len = strlen(value);
		this->pos = 0;
		this->value = _CL_NEWARRAY(signed char, this->len);
		memcpy(this->value, value, this->len);
	}
	virtual ~MemReader(){
		_CLDELETE_ARRAY(this->value);
	}

    int32_t read(const signed char*& start, int32_t min, int32_t max){
		start = this->value + pos;
		int32_t r = max>min?max:min;
		if ( len-pos < r )
			r = len-pos;
		pos += r;
		return r;
	}
    int64_t position(){
		return pos;
	}
	int64_t skip(int64_t ntoskip){
		int64_t s = ntoskip;
		if ( len-pos < s )
			s = len-pos;

		this->pos += s;
		return s;
	}
	size_t size(){
		return len;
	}
};

  void TestFields(CuTest *tc){
	  Field *f = _CLNEW Field(_T("test"), _T("value"), Field::INDEX_TOKENIZED);
	  CLUCENE_ASSERT(f->isIndexed() && f->isTokenized());
	  CLUCENE_ASSERT(!f->isStored() && !f->isBinary() && !f->getOmitNorms());
	  _CLDELETE(f);

	  f = _CLNEW Field(_T("test"), _T("value"), Field::STORE_YES | Field::INDEX_NONORMS);
	  CLUCENE_ASSERT(f->isIndexed());
	  CLUCENE_ASSERT(!f->isTokenized());
	  CLUCENE_ASSERT(f->getOmitNorms());
	  CLUCENE_ASSERT(f->isStored() && !f->isBinary());
	  _CLDELETE(f);
  }

  void TestDateTools(CuTest *tc) {
	  TCHAR* t = CL_NS(document)::DateTools::timeToString( Misc::currentTimeMillis() , CL_NS(document)::DateTools::MILLISECOND_FORMAT);
	  _CLDELETE_ARRAY(t);

	  TCHAR buf[30];
	  const TCHAR* xpt = _T("19700112102054321");
	  int64_t vv = (int64_t)987654321;
	  CL_NS(document)::DateTools::timeToString( vv , CL_NS(document)::DateTools::MILLISECOND_FORMAT, buf, 30);

	  if ( _tcscmp(buf,xpt) != 0 ) {
			CuFail(tc, _T("timeToString failed\n"), buf, xpt);
	  }
	  _CLDELETE_ARRAY(t);
  }

  void TestBinaryDocument(CuTest *tc){
    char factbook[1024];
    strcpy(factbook, clucene_data_location);
	  strcat(factbook, "/reuters-21578/feldman-cia-worldfactbook-data.txt");
	  CuAssert(tc,_T("Factbook file does not exist"),Misc::dir_Exists(factbook));

    Document doc;
    Field* f;
    const signed char* _as2;
    const TCHAR *_ts, *_ts2;
	  const ValueArray<uint8_t>* strm;
    RAMDirectory ram;

    const char* areaderString = "a binary field";
    const TCHAR* treaderString = _T("a string reader field");
    size_t readerStringLen = strlen(areaderString);

	  SimpleAnalyzer an;
    IndexWriter writer(&ram,&an,true); //no analyzer needed since we are not indexing...

    ValueArray<uint8_t>* b = _CLNEW ValueArray<uint8_t>( (uint8_t*)areaderString, strlen(areaderString) );
    //use binary utf8
    doc.add( *_CLNEW Field(_T("binaryField"), b,
        Field::TERMVECTOR_NO | Field::STORE_YES | Field::INDEX_NO) );
    writer.addDocument(&doc);
    doc.clear();

    //use reader
    doc.add( *_CLNEW Field(_T("readerField"),_CLNEW StringReader (treaderString),
        Field::TERMVECTOR_NO | Field::STORE_YES | Field::INDEX_NO) );
    writer.addDocument(&doc);
    doc.clear();

    //done adding a few documents, now try and add a few more...
    writer.optimize();

    //use big file
    doc.add( *_CLNEW Field(_T("fileField"),
        _CLNEW FileReader(factbook, SimpleInputStreamReader::ASCII),
        Field::TERMVECTOR_NO | Field::STORE_YES | Field::INDEX_NO) );
    writer.addDocument(&doc);
    doc.clear();

    //another optimise...
    writer.optimize();
    writer.close();

    IndexReader* reader = IndexReader::open(&ram);

    //now check binary field
    reader->document(0, doc);
    f = doc.getField(_T("binaryField"));
    strm = f->binaryValue();

    CLUCENE_ASSERT(readerStringLen == b->length);
    for ( int i=0;i<readerStringLen;i++){
        CLUCENE_ASSERT((*strm)[i]==areaderString[i]);
    }
	  doc.clear();

    //and check reader stream
    reader->document(1, doc);
    f = doc.getField(_T("readerField"));
    _ts = f->stringValue();
    CLUCENE_ASSERT(_tcscmp(treaderString,_ts)==0);
    doc.clear();

    //now check the large field field
    reader->document(2, doc);
    f = doc.getField(_T("fileField"));
    _ts = f->stringValue();
    FileReader fbStream(factbook, FileReader::ASCII);

    int i=0;
    _ts2 = NULL;
    do{
        int32_t rd = fbStream.read(_ts2,1,1);
        if ( rd == -1 )
            break;
        CLUCENE_ASSERT(rd==1);
        CLUCENE_ASSERT(_ts[i]==*_ts2);
        i++;
    }while(true);
    CLUCENE_ASSERT(i == _tcslen(_ts));
    doc.clear();


    reader->close();
    _CLDELETE(reader);
  }

CuSuite *testdocument(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene Document Test"));

	SUITE_ADD_TEST(suite, TestFields);
	SUITE_ADD_TEST(suite, TestBinaryDocument);
	SUITE_ADD_TEST(suite, TestDateTools);
    return suite;
}
