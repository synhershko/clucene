#include "test.h"
#include "CLucene/util/dirent.h"
#include "CLucene/util/CLStreams.h"
#include "CLucene/LuceneThreads.h"

#ifdef _CL_HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif

CL_NS_USE(util)

	//an extremelly simple analyser. this eliminates differences
	//caused by differences in character classifications functions
	class ReutersTokenizer:public CharTokenizer {
	public:
		// Construct a new LetterTokenizer. 
		ReutersTokenizer(CL_NS(util)::Reader* in):
		  CharTokenizer(in) {}
	
	    ~ReutersTokenizer(){}
	protected:
		bool isTokenChar(const TCHAR c) const{
			if ( c == ' ' || c == '\t' ||
    		 c == '-' || c == '.' ||
    		 c == '\n' || c == '\r' ||
    		 c == ',' || c == '<' ||
    		 c == '>' || c<=9){
    			return false;		 	
			}else
    			return true;
		}
		TCHAR normalize(const TCHAR c) const{
			return c;
		}
	};

	class ReutersAnalyzer: public Analyzer {
     public:
		 TokenStream* tokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader){
			return _CLNEW ReutersTokenizer(reader);
		 }
	  ~ReutersAnalyzer(){}
  };

	
	char reuters_fsdirectory[CL_MAX_PATH];
	bool reuters_ready = false;

	char reuters_srcdirectory[1024];
	char reuters_origdirectory[1024];

	//indexes the reuters-21578 data.
	void testReuters(CuTest *tc) {
		strcpy(reuters_srcdirectory, clucene_data_location);
		strcat(reuters_srcdirectory, "/reuters-21578");
		CuAssert(tc,_T("Data does not exist"),Misc::dir_Exists(reuters_srcdirectory));

		strcpy(reuters_origdirectory, clucene_data_location);
		strcat(reuters_origdirectory, "/reuters-21578-index");
		CuAssert(tc,_T("Index does not exist"),Misc::dir_Exists(reuters_origdirectory));

		FSDirectory* fsdir = FSDirectory::getDirectory(reuters_fsdirectory,true);
		ReutersAnalyzer a;

		IndexWriter writer(fsdir,&a,true);
		writer.setUseCompoundFile(false);
		writer.setMaxFieldLength(10000);
		DIR* srcdir = opendir(reuters_srcdirectory);
		struct dirent* fl = readdir(srcdir);
		struct cl_stat_t buf;
		char tmppath[CL_MAX_DIR];
		strncpy(tmppath,reuters_srcdirectory,CL_MAX_DIR);
		strcat(tmppath,"/");
		char* tmppathP = tmppath + strlen(tmppath);
		TCHAR tpath[CL_MAX_PATH];

		while ( fl != NULL ){
			strcpy(tmppathP,fl->d_name);
			STRCPY_AtoT(tpath,fl->d_name,CL_MAX_PATH);
			fileStat(tmppath,&buf);
			if ( buf.st_mode & S_IFREG){
				Document* doc = _CLNEW Document;
				doc->add(*_CLNEW Field(_T("path"),tpath,Field::INDEX_TOKENIZED));
				doc->add(*_CLNEW Field(_T("contents"), _CLNEW FileReader(tmppath, "ASCII"),Field::INDEX_TOKENIZED));

				writer.addDocument( doc );
				_CLDELETE(doc);
			}
			fl = readdir(srcdir);
		}
		closedir(srcdir);

		writer.close();
		fsdir->close();
		_CLDECDELETE(fsdir);

		//note: for those comparing 0.9.16 to later, the optimize() has been removed so
		//we can do certain tests with the multi-* classes (reader,etc)
		//performance will naturally be worse

		reuters_ready = true;
	}

	void testByteForByte(CuTest* tc){
		CLUCENE_ASSERT(reuters_ready);

		char tmppath[CL_MAX_DIR];

		strcpy(tmppath,reuters_origdirectory);
		strcat(tmppath,"/_z.cfs");
		FILE* f1 = fopen(tmppath,"rb");
		CLUCENE_ASSERT(f1!=NULL);
		
		strcpy(tmppath,reuters_fsdirectory);
		strcat(tmppath,"/_z.cfs");
		FILE* f2 = fopen(tmppath,"rb");
		CLUCENE_ASSERT(f2!=NULL);

		uint8_t buf1[1024];
		uint8_t buf2[1024];

		int s1,s2;
		
		while ( true){
			s1 = fread(buf1,sizeof(uint8_t),1024,f1);
			s2 = fread(buf2,sizeof(uint8_t),1024,f2);
			CuAssert(tc,_T("comparison yielded different lengths"),s1==s2);
			if ( s1 == 0 )
				break;

			for ( int i=0;i<1024;i++ )
				CuAssert(tc,_T("comparison with original failed"),buf1[i]==buf2[i]);
		}

		fclose(f1);
		fclose(f2);
	}

	void testBySection(CuTest* tc){
		IndexReader* reader1 = IndexReader::open(reuters_origdirectory);
		IndexReader* reader2 = IndexReader::open(reuters_fsdirectory);

		//misc
		CuAssertIntEquals(tc,_T("reader maxDoc not equal"), reader1->maxDoc(), reader2->maxDoc());
		CuAssertIntEquals(tc,_T("reader numDocs not equal"), reader1->numDocs(), reader2->numDocs());


		//test field names
        StringArrayWithDeletor fn1;
        StringArrayWithDeletor fn2;
        reader1->getFieldNames(IndexReader::ALL, fn1);
		reader2->getFieldNames(IndexReader::ALL, fn2);

        //make sure field length is the same
        int fn1count = fn1.size();
        int fn2count = fn2.size();
        CuAssertIntEquals(tc, _T("reader fieldnames count not equal"), fn1count, fn2count );

		
		for (int n=0;n<fn1count;n++ ){
            //field names aren't always in the same order, so find it.
            int fn2n = 0;
            bool foundField = false;
            while ( fn2[fn2n] != NULL ){
                if ( _tcscmp(fn1[n],fn2[fn2n])==0 ){
                    foundField = true;
                    break;
                }
                fn2n++;
            }
			CLUCENE_ASSERT( foundField==true );

			//test field norms
			uint8_t* norms1 = reader1->norms(fn1[n]);
			uint8_t* norms2 = reader2->norms(fn1[n]);
			if ( norms1 != NULL ){
				CLUCENE_ASSERT(norms2 != NULL);
				for ( int i=0;i<reader1->maxDoc();i++ ){
					int diff = norms1[i]-norms2[i];
					if ( diff < 0 )
					    diff *= -1;
					if ( diff > 16 ){
                        TCHAR tmp[1024];
                        _sntprintf(tmp,1024,_T("Norms are off by more than the threshold! %d, should be %d"), (int32_t)norms2[i], (int32_t)norms1[i]);
						CuAssert(tc,tmp,false);
					}
				}
			}else
				CLUCENE_ASSERT(norms2 == NULL);
			////////////////////
		}
		fn1.clear(); //save memory
		fn2.clear(); //save memory


		//test Terms
		TermEnum* te1 = reader1->terms();
		TermEnum* te2 = reader2->terms();
		while ( te1->next() ){
			CLUCENE_ASSERT(te2->next());

			CuAssertStrEquals(tc,_T("term enum text not equal"), te1->term(false)->text(), te2->term(false)->text() );
			CuAssertIntEquals(tc,_T("term enum docFreq"), te1->docFreq(), te2->docFreq() );
            
		    //test term docs
			//todo: this isn't useful until we search the td2 for the doc of td1
			/*
		    TermDocs* td1 = reader1->termDocs(te1->term(false));
		    TermDocs* td2 = reader2->termDocs(te1->term(false));
		    while (td1->next()){
                CLUCENE_ASSERT(td2->next());

			    //CLUCENE_ASSERT(td1->doc()==td2->doc()); todo: doc's aren't always the same, but should check that
														//doc is the same content
			    CLUCENE_ASSERT(td1->freq()==td2->freq());

                //todo: add some data into the index for the termfreqvector.
			    //test term freq vector
			    TermFreqVector** tfv1 = reader1->getTermFreqVectors(td1->doc());
			    TermFreqVector** tfv2 = reader1->getTermFreqVectors(td1->doc());
			    if ( tfv1 != NULL ){
				    int t=0;
				    while ( tfv1[t] != NULL ){
					    CLUCENE_ASSERT(tfv2[t] != NULL);

					    TermFreqVector* v1 = tfv1[t];
					    TermFreqVector* v2 = tfv2[t];

					    CLUCENE_ASSERT(_tcscmp(v1->getField(),v2->getField())==0);
					    CLUCENE_ASSERT(v1->size()==v2->size());
    					
					    //todo: should check a few more things here...

					    t++;
				    }
				    CLUCENE_ASSERT(tfv2[t] == NULL);
			    }else
				    CLUCENE_ASSERT(tfv2==NULL);

		    }
		    _CLDELETE(td1);
		    _CLDELETE(td2);
			*/

		    //test term positions
			//todo: this isn't useful until we search the td2 for the doc of td1
		    /*TermPositions* tp1 = reader1->termPositions(te1->term(false));
		    TermPositions* tp2 = reader2->termPositions(te1->term(false));
		    while ( tp1->next() ){
			    CLUCENE_ASSERT(tp2->next());

			    CLUCENE_ASSERT(tp1->doc()==tp2->doc());todo: doc's aren't always the same, but should check that
														//doc is the same content
			    CLUCENE_ASSERT(tp1->freq()==tp2->freq());
			    CLUCENE_ASSERT(tp1->nextPosition()==tp2->nextPosition());
		    }
		    _CLDELETE(tp1);
		    _CLDELETE(tp2);*/
            
		}
		te1->close();
		te2->close();
		_CLDELETE(te1);
		_CLDELETE(te2);

		reader1->close();
		reader2->close();
		_CLDELETE(reader1);
		_CLDELETE(reader2);
	}
	
	#define threadsCount 10
	
	StandardAnalyzer threadAnalyzer;
	void threadSearch(IndexSearcher* searcher, const TCHAR* qry){
	    Query* q = NULL;
		Hits* h = NULL;
		try{
			q = QueryParser::parse(qry , _T("contents"), &threadAnalyzer);
			if ( q != NULL ){
			    h = searcher->search( q );
			    
			    if ( h->length() > 0 ){
			        //check for explanation memory leaks...
					Explanation expl1;
					searcher->explain(q, h->id(0), &expl1);
					TCHAR* tmp = expl1.toString();
					_CLDELETE_CARRAY(tmp);
					if ( h->length() > 1 ){ //do a second one just in case
						Explanation expl2;
						searcher->explain(q, h->id(1), &expl2);
						tmp = expl2.toString();
						_CLDELETE_CARRAY(tmp);
					}
				}
			}
		}_CLFINALLY(
    		_CLDELETE(h);
    		_CLDELETE(q);
		);
	}
	_LUCENE_THREAD_FUNC(threadedSearcherTest, arg){
	    IndexSearcher* searcher = (IndexSearcher*)arg;
	    
	    for ( int i=0;i<100;i++ ){
    	    threadSearch(searcher, _T("test") );
    	    threadSearch(searcher, _T("reuters") );
    	    threadSearch(searcher, _T("data") );
    	}
		_LUCENE_THREAD_FUNC_RETURN(0);
	}
	
	void testThreaded(CuTest* tc){
		CLUCENE_ASSERT(reuters_ready);
		IndexSearcher searcher(reuters_origdirectory);
        
        //read using multiple threads...
        _LUCENE_THREADID_TYPE threads[threadsCount];
        
        int i;
        for ( i=0;i<threadsCount;i++ )
             threads[i] = _LUCENE_THREAD_CREATE(&threadedSearcherTest, &searcher);
        
        CL_NS(util)::Misc::sleep(3000);

        for ( i=0;i<threadsCount;i++ )
            _LUCENE_THREAD_JOIN(threads[i]);
        
        searcher.close();
	}

CuSuite *testreuters(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene Reuters Test"));

	//setup some variables
	strcpy(reuters_fsdirectory,cl_tempDir);
	strcat(reuters_fsdirectory,"/reuters-index");

    SUITE_ADD_TEST(suite, testReuters);
    //SUITE_ADD_TEST(suite, testByteForByte); this test rarely works currently, use more robust by section test...
    SUITE_ADD_TEST(suite, testBySection);
    
    //we still do this, but it'll be slow because the 'threads' will be run serially.
    
    SUITE_ADD_TEST(suite, testThreaded);
    return suite; 
}
// EOF
