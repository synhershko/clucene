/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"

#include "CLucene/search/RangeFilter.h"
#include "CLucene/search/ConstantScoreQuery.h"

#include <limits>

static const size_t getMaxIntLength()
{
    TCHAR buf[40];
    _itot(std::numeric_limits<int>::max(), buf, 10);
    return _tcslen(buf);
}

class BaseTestRangeFilter 
{
public:
    static const bool F = false;
    static const bool T = true;
    
    RAMDirectory* index;
    
    int maxR;
    int minR;

    int minId;
    int maxId;

    const size_t intLength;

    CuTest* tc;

    BaseTestRangeFilter(CuTest* _tc)
        : index(new RAMDirectory())
        , maxR(std::numeric_limits<int>::min()), minR(std::numeric_limits<int>::max())
        , minId(0), maxId(10000), intLength(getMaxIntLength())
        , tc(_tc)
    {
        srand ( 101 ); // use a set seed to test is deterministic
        build();
    }
    virtual ~BaseTestRangeFilter()
    {
        index->close();
        _CLLDECDELETE(index);
    }
    
    /**
     * a simple padding function that should work with any int
     */
    std::tstring pad(int n) {
        std::tstring b;
        if (n < 0) {
            b = _T("-");
            n = std::numeric_limits<int>::max() + n + 1;
        }
        else
            b = _T("0");

        TCHAR buf[40];
        _itot(n, buf, 10);
        for (size_t i = _tcslen(buf); i <= intLength; i++) {
            b += _T("0");
        }
        b += buf;
        
        return b;
    }
    
private:
    void build() {
        try
        {    
            /* build an index */
            SimpleAnalyzer a;
            IndexWriter* writer = _CLNEW IndexWriter(index,
                                                 &a, T);

            for (int d = minId; d <= maxId; d++) {
                Document doc;
                std::tstring paddedD = pad(d);
                doc.add(* _CLNEW Field(_T("id"),paddedD.c_str(), Field::STORE_YES | Field::INDEX_UNTOKENIZED));
                int r= rand();
                if (maxR < r) {
                    maxR = r;
                }
                if (r < minR) {
                    minR = r;
                }
                std::tstring paddedR = pad(r);
                doc.add( * _CLNEW Field(_T("rand"),paddedR.c_str(), Field::STORE_YES | Field::INDEX_UNTOKENIZED));
                doc.add( * _CLNEW Field(_T("body"),_T("body"), Field::STORE_YES | Field::INDEX_UNTOKENIZED));
                writer->addDocument(&doc);
            }
            
            writer->optimize();
            writer->close();
            _CLLDELETE(writer);
        } catch (CLuceneError&) {
            _CLTHROWA(CL_ERR_Runtime, "can't build index");
        }

    }

public:
    void testPad() {

        const int tests[] = {
            -9999999, -99560, -100, -3, -1, 0, 3, 9, 10, 1000, 999999999
        };
        const size_t testsLen = 11;

        for (int i = 0; i < testsLen - 1; i++) {
            const int a = tests[i];
            const int b = tests[i+1];
            const TCHAR* aa = pad(a).c_str();
            const TCHAR* bb = pad(b).c_str();

            StringBuffer label(50);
            label << a << _T(':') << aa << _T(" vs ") << b << _T(':') << bb;

            std::tstring tmp(_T("length of "));
            tmp += label.getBuffer();
            assertEqualsMsg(tmp.c_str(), _tcslen(aa), _tcslen(bb));

            tmp = _T("compare less than ");
            tmp += label.getBuffer();
            assertTrueMsg(tmp.c_str(), _tcscmp(aa, bb) < 0);
        }
    }
};

class TestRangeFilter : BaseTestRangeFilter {
public:
    TestRangeFilter(CuTest* _tc)
        : BaseTestRangeFilter(_tc)
    {
    }

    void testRangeFilterId() {

        IndexReader* reader = IndexReader::open(index);
        IndexSearcher* search = new IndexSearcher(reader);

        int medId = ((maxId - minId) / 2);

        std::tstring minIPstr = pad(minId);
        const TCHAR* minIP = minIPstr.c_str();

        std::tstring maxIPstr = pad(maxId);
        const TCHAR* maxIP = maxIPstr.c_str();

        std::tstring medIPstr = pad(medId);
        const TCHAR* medIP = medIPstr.c_str();

        size_t numDocs = static_cast<size_t>(reader->numDocs());

        assertEqualsMsg(_T("num of docs"), numDocs, static_cast<size_t>(1+ maxId - minId));

        Hits* result;
        Term* term = _CLNEW Term(_T("body"),_T("body"));
        Query* q = _CLNEW TermQuery(term);
        _CLDECDELETE(term);

        // test id, bounded on both ends

        Filter* f = _CLNEW RangeFilter(_T("id"),minIP,maxIP,T,T);
        result = search->search(q, f);
        assertEqualsMsg(_T("find all"), numDocs, result->length());
        _CLLDELETE(result);
        _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("id"),minIP,maxIP,T,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("all but last"), numDocs-1, result->length());
        _CLLDELETE(result);
        _CLLDELETE(f);

        f =_CLNEW RangeFilter(_T("id"),minIP,maxIP,F,T);
        result = search->search(q,f);
        assertEqualsMsg(_T("all but first"), numDocs-1, result->length());
        _CLLDELETE(result);
        _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("id"),minIP,maxIP,F,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("all but ends"), numDocs-2, result->length());
        _CLLDELETE(result);
        _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("id"),medIP,maxIP,T,T);
        result = search->search(q,f);
        assertEqualsMsg(_T("med and up"), 1+ maxId-medId, result->length());
        _CLLDELETE(result);
        _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("id"),minIP,medIP,T,T);
        result = search->search(q,f);
        assertEqualsMsg(_T("up to med"), 1+ medId-minId, result->length());
        _CLLDELETE(result);
        _CLLDELETE(f);

        // unbounded id

        f=_CLNEW RangeFilter(_T("id"),minIP,NULL,T,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("min and up"), numDocs, result->length());
        _CLLDELETE(result);
        _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("id"),NULL,maxIP,F,T);
        result = search->search(q,f);
        assertEqualsMsg(_T("max and down"), numDocs, result->length());
        _CLLDELETE(result);
        _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("id"),minIP,NULL,F,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("not min, but up"), numDocs-1, result->length());
        _CLLDELETE(result);
        _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("id"),NULL,maxIP,F,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("not max, but down"), numDocs-1, result->length());
        _CLLDELETE(result);
        _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("id"),medIP,maxIP,T,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("med and up, not max"), maxId-medId, result->length());
        _CLLDELETE(result);
        _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("id"),minIP,medIP,F,T);
        result = search->search(q,f);
        assertEqualsMsg(_T("not min, up to med"), medId-minId, result->length());
        _CLLDELETE(result);
        _CLLDELETE(f);

        // very small sets

        f=_CLNEW RangeFilter(_T("id"),minIP,minIP,F,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("min,min,F,F"), 0, result->length());
        _CLLDELETE(result); _CLLDELETE(f);
        f=_CLNEW RangeFilter(_T("id"),medIP,medIP,F,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("med,med,F,F"), 0, result->length());
        _CLLDELETE(result); _CLLDELETE(f);
        f=_CLNEW RangeFilter(_T("id"),maxIP,maxIP,F,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("max,max,F,F"), 0, result->length());
        _CLLDELETE(result); _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("id"),minIP,minIP,T,T);
        result = search->search(q,f);
        assertEqualsMsg(_T("min,min,T,T"), 1, result->length());
        _CLLDELETE(result); _CLLDELETE(f);
        f=_CLNEW RangeFilter(_T("id"),NULL,minIP,F,T);
        result = search->search(q,f);
        assertEqualsMsg(_T("nul,min,F,T"), 1, result->length());
        _CLLDELETE(result); _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("id"),maxIP,maxIP,T,T);
        result = search->search(q,f);
        assertEqualsMsg(_T("max,max,T,T"), 1, result->length());
        _CLLDELETE(result); _CLLDELETE(f);
        f=_CLNEW RangeFilter(_T("id"),maxIP,NULL,T,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("max,nul,T,T"), 1, result->length());
        _CLLDELETE(result); _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("id"),medIP,medIP,T,T);
        result = search->search(q,f);
        assertEqualsMsg(_T("med,med,T,T"), 1, result->length());
        _CLLDELETE(result); _CLLDELETE(f);

        search->close();
        _CLLDELETE(search);

        reader->close();
        _CLLDELETE(reader);

        _CLLDELETE(q);
    }

    void testRangeFilterRand()
    {
        IndexReader* reader = IndexReader::open(index);
        IndexSearcher* search = _CLNEW IndexSearcher(reader);

        std::tstring minRPstr = pad(minR);
        const TCHAR* minRP = minRPstr.c_str();
        
        std::tstring maxRPstr = pad(maxR);
        const TCHAR* maxRP = maxRPstr.c_str();

        size_t numDocs = static_cast<size_t>(reader->numDocs());

        assertEqualsMsg(_T("num of docs"), numDocs, 1+ maxId - minId);

        Hits* result;
        Term* term = _CLNEW Term(_T("body"),_T("body"));
        Query* q = _CLNEW TermQuery(term);
        _CLDECDELETE(term);

        // test extremes, bounded on both ends

        Filter* f = _CLNEW RangeFilter(_T("rand"),minRP,maxRP,T,T);
        result = search->search(q,f);
        assertEqualsMsg(_T("find all"), numDocs, result->length());
        _CLLDELETE(result); _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("rand"),minRP,maxRP,T,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("all but biggest"), numDocs-1, result->length());
        _CLLDELETE(result); _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("rand"),minRP,maxRP,F,T);
        result = search->search(q,f);
        assertEqualsMsg(_T("all but smallest"), numDocs-1, result->length());
        _CLLDELETE(result); _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("rand"),minRP,maxRP,F,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("all but extremes"), numDocs-2, result->length());
        _CLLDELETE(result); _CLLDELETE(f);

        // unbounded

        f=_CLNEW RangeFilter(_T("rand"),minRP,NULL,T,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("smallest and up"), numDocs, result->length());
        _CLLDELETE(result); _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("rand"),NULL,maxRP,F,T);
        result = search->search(q,f);
        assertEqualsMsg(_T("biggest and down"), numDocs, result->length());
        _CLLDELETE(result); _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("rand"),minRP,NULL,F,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("not smallest, but up"), numDocs-1, result->length());
        _CLLDELETE(result); _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("rand"),NULL,maxRP,F,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("not biggest, but down"), numDocs-1, result->length());
        _CLLDELETE(result); _CLLDELETE(f);

        // very small sets

        f=_CLNEW RangeFilter(_T("rand"),minRP,minRP,F,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("min,min,F,F"), 0, result->length());
        _CLLDELETE(result); _CLLDELETE(f);
        f=_CLNEW RangeFilter(_T("rand"),maxRP,maxRP,F,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("max,max,F,F"), 0, result->length());
        _CLLDELETE(result); _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("rand"),minRP,minRP,T,T);
        result = search->search(q,f);
        assertEqualsMsg(_T("min,min,T,T"), 1, result->length());
        _CLLDELETE(result); _CLLDELETE(f);
        f=_CLNEW RangeFilter(_T("rand"),NULL,minRP,F,T);
        result = search->search(q,f);
        assertEqualsMsg(_T("nul,min,F,T"), 1, result->length());
        _CLLDELETE(result); _CLLDELETE(f);

        f=_CLNEW RangeFilter(_T("rand"),maxRP,maxRP,T,T);
        result = search->search(q,f);
        assertEqualsMsg(_T("max,max,T,T"), 1, result->length());
        _CLLDELETE(result); _CLLDELETE(f);
        f=_CLNEW RangeFilter(_T("rand"),maxRP,NULL,T,F);
        result = search->search(q,f);
        assertEqualsMsg(_T("max,nul,T,T"), 1, result->length());
        _CLLDELETE(result); _CLLDELETE(f);

        search->close();
        _CLLDELETE(search);

        reader->close();
        _CLLDELETE(reader);

        _CLLDELETE(q);
    }
};

void testRangeFilterTrigger(CuTest* tc)
{
    TestRangeFilter trf(tc);
    trf.testRangeFilterId();
    trf.testRangeFilterRand();
}

void testIncludeLowerTrue(CuTest* tc)
{
    WhitespaceAnalyzer a;
    RAMDirectory* index = _CLNEW RAMDirectory();
    IndexWriter* writer = _CLNEW IndexWriter(index,
        &a, true);

    Document doc;
    doc.add(*_CLNEW Field(_T("Category"), _T("a 1"), Field::STORE_YES | Field::INDEX_TOKENIZED));
    writer->addDocument(&doc); doc.clear();

    doc.add(*_CLNEW Field(_T("Category"), _T("a 2"), Field::STORE_YES | Field::INDEX_TOKENIZED));
    writer->addDocument(&doc); doc.clear();

    doc.add(*_CLNEW Field(_T("Category"), _T("a 3"), Field::STORE_YES | Field::INDEX_TOKENIZED));
    writer->addDocument(&doc); doc.clear();

    writer->close();
    _CLLDELETE(writer);

    IndexSearcher* s = _CLNEW IndexSearcher(index);
    Filter* f = _CLNEW RangeFilter(_T("Category"), _T("3"), _T("3"), true, true);

    Term* t = _CLNEW Term(_T("Category"), _T("a"));
    Query* q1 = _CLNEW TermQuery(t);
    _CLLDECDELETE(t);

    t = _CLNEW Term(_T("Category"), _T("3"));
    Query* q2 = _CLNEW TermQuery(t);
    _CLLDECDELETE(t);

    Hits* h = s->search(q1);
    assertTrue(h->length() == 3);
    _CLLDELETE(h);

    h = s->search(q2);
    assertTrue(h->length() == 1);
    _CLLDELETE(h);

    h = s->search(q1, f);
    assertTrue(h->length() == 1);
    _CLLDELETE(h);

    s->close();
    _CLLDELETE(s);
    _CLLDELETE(q1);
    _CLLDELETE(q2);
    _CLLDELETE(f);

    index->close();
    _CLLDECDELETE(index);
}

CuSuite *testRangeFilter(void)
{
    CuSuite *suite = CuSuiteNew(_T("CLucene RangeFilter Test"));

    SUITE_ADD_TEST(suite, testRangeFilterTrigger);
    SUITE_ADD_TEST(suite, testIncludeLowerTrue);

    return suite;
}



/*------------------------------------------------------------------------------
 * Test based on JLucene-2.3.2 (06/08/2010 - Jiri Splichal)
 *------------------------------------------------------------------------------*/
class TestConstantScoreRangeQuery : public BaseTestRangeFilter
{
private:
    Directory * m_pSmall;


public:
    TestConstantScoreRangeQuery( CuTest * _tc ) : BaseTestRangeFilter( _tc ), m_pSmall( NULL ) {}
	~TestConstantScoreRangeQuery()  
    {
        if( m_pSmall )
        {
            m_pSmall->close();
            _CLLDECDELETE( m_pSmall );
        }
    }


    /////////////////////////////////////////////////////////////////////////////
    void setUp()
    {
        TCHAR       tbuffer[16];
        TCHAR*      data[] = 
        {   
            _T( "A 1 2 3 4 5 6" ),
            _T( "Z       4 5 6" ),
            NULL,
            _T( "B   2   4 5 6" ),
            _T( "Y     3   5 6" ),
            NULL,
            _T( "C     3     6" ),
            _T( "X       4 5 6" )
        };
        
        m_pSmall = _CLNEW RAMDirectory();
        Analyzer * pAnalyzer = _CLNEW WhitespaceAnalyzer();
        IndexWriter * pWriter = _CLNEW IndexWriter( m_pSmall, pAnalyzer, true );
        
        for( int i = 0; i < sizeof( data ) / sizeof( data[0] ); i++ )
        {
            _itot( i, tbuffer, 10 ); 
            Document doc;
            doc.add( * _CLNEW Field( _T( "id" ), tbuffer, Field::STORE_YES | Field::INDEX_UNTOKENIZED ));
            doc.add( * _CLNEW Field( _T( "all" ), _T( "all" ), Field::STORE_YES | Field::INDEX_UNTOKENIZED ));
            if( data[ i ] )
                doc.add( * _CLNEW Field( _T( "data" ), data[ i ], Field::STORE_YES | Field::INDEX_TOKENIZED ));

            pWriter->addDocument( &doc );
        }
        
        pWriter->optimize();
        pWriter->close();
        _CLDELETE( pWriter );
        _CLDELETE( pAnalyzer );
    }


    /////////////////////////////////////////////////////////////////////////////
    Query * csrq(const TCHAR* f, const TCHAR* l, const TCHAR* h, bool il, bool ih) 
    {
        return _CLNEW ConstantScoreRangeQuery( f, l, h, il, ih );
    }


    /////////////////////////////////////////////////////////////////////////////
//     void testBasics() 
//     {
//         QueryUtils::check(csrq("data","1","6",T,T));
//         QueryUtils::check(csrq("data","A","Z",T,T));
//         QueryUtils::checkUnequal(csrq("data","1","6",T,T), csrq("data","A","Z",T,T));
//     }

    /////////////////////////////////////////////////////////////////////////////
    void testEqualScores() 
    {
        // NOTE: uses index build in *this* setUp
        
        IndexReader * pReader = IndexReader::open( m_pSmall );
	    IndexSearcher * pSearch = _CLNEW IndexSearcher( pReader );

	    Hits * pResult;

        // some hits match more terms then others, score should be the same
        Query * q = csrq( _T( "data" ), _T( "1" ), _T( "6" ), true, true );
        pResult = pSearch->search( q );
        int numHits = pResult->length();
        assertEqualsMsg( _T( "wrong number of results" ), 6, numHits );
        float_t score = pResult->score( 0 );
        for( int i = 1; i < numHits; i++ )
        {
            assertTrueMsg( _T( "score was not the same" ), score == pResult->score( i ));
        }
        _CLDELETE( pResult );
        _CLDELETE( q );

        pSearch->close();
        _CLDELETE( pSearch );

        pReader->close();
        _CLDELETE( pReader );
    }

    
    /////////////////////////////////////////////////////////////////////////////
    void testBoost()
    {
        // NOTE: uses index build in *this* setUp

        IndexReader * pReader = IndexReader::open( m_pSmall );
	    IndexSearcher * pSearch = _CLNEW IndexSearcher( pReader );
	    Hits * pResult;

        // test for correct application of query normalization
        // must use a non score normalizing method for this.
        Query * q = csrq( _T( "data" ), _T( "1" ), _T( "6" ), true, true );
        q->setBoost( 100 );
        pResult = pSearch->search( q );
        for( unsigned int i = 1; i < pResult->length(); i++ )
        {
            assertTrueMsg( _T( "score was not was not correct" ), 1.0f == pResult->score( i ));
        }
        _CLDELETE( pResult );
        _CLDELETE( q );


        //
        // Ensure that boosting works to score one clause of a query higher
        // than another.
        //
        Query * q1 = csrq( _T( "data" ), _T( "A" ), _T( "A" ), true, true );  // matches document #0
        q1->setBoost( .1f );
        Query * q2 = csrq( _T( "data" ), _T( "Z" ), _T( "Z" ), true, true );  // matches document #1
        BooleanQuery * bq = _CLNEW BooleanQuery( true );
        bq->add( q1, true, BooleanClause::SHOULD );
        bq->add( q2, true, BooleanClause::SHOULD );

        pResult = pSearch->search( bq );
        assertEquals( 1, pResult->id( 0 ));
        assertEquals( 0, pResult->id( 1 ));
        assertTrue( pResult->score( 0 ) > pResult->score( 1 ));
        _CLDELETE( pResult );
        _CLDELETE( bq );

        q1 = csrq( _T( "data" ), _T( "A" ), _T( "A" ), true, true );  // matches document #0
        q1->setBoost( 10.0f );
        q2 = csrq( _T( "data" ), _T( "Z" ), _T( "Z" ), true, true );  // matches document #1
        bq = _CLNEW BooleanQuery( true );
        bq->add( q1, true, BooleanClause::SHOULD );
        bq->add( q2, true, BooleanClause::SHOULD );

        pResult = pSearch->search( bq );
        assertEquals( 0, pResult->id( 0 ));
        assertEquals( 1, pResult->id( 1 ));
        assertTrue( pResult->score( 0 ) > pResult->score( 1 ));
        _CLDELETE( pResult );
        _CLDELETE( bq );

        pSearch->close();
        _CLDELETE( pSearch );

        pReader->close();
        _CLDELETE( pReader );
    }

    
    /////////////////////////////////////////////////////////////////////////////
    void testBooleanOrderUnAffected()
    {
        // NOTE: uses index build in *this* setUp

        IndexReader * pReader = IndexReader::open( m_pSmall );
	    IndexSearcher * pSearch = _CLNEW IndexSearcher( pReader );

        // first do a regular RangeQuery which uses term expansion so
        // docs with more terms in range get higher scores
        Term * pLower = _CLNEW Term( _T( "data" ), _T( "1" ));
        Term * pUpper = _CLNEW Term( _T( "data" ), _T( "4" ));
        Query * rq = _CLNEW RangeQuery( pLower, pUpper, true );
        _CLLDECDELETE( pUpper );
        _CLLDECDELETE( pLower );

        Hits * pExpected = pSearch->search( rq );
        int numHits = pExpected->length();
 
        // now do a boolean where which also contains a
        // ConstantScoreRangeQuery and make sure the order is the same
        
        BooleanQuery * q = _CLNEW BooleanQuery();
        q->add( rq, true, BooleanClause::MUST );
        q->add( csrq( _T( "data" ), _T( "1" ), _T( "6" ), true, true ), true, BooleanClause::MUST );
 
        Hits * pActual = pSearch->search( q );
        assertEqualsMsg( _T( "wrong number of hits" ), numHits, pActual->length() );
        for( int i = 0; i < numHits; i++ )
        {
            assertEqualsMsg( _T( "mismatch in docid for a hit" ), pExpected->id( i ), pActual->id( i ));
        }
        _CLDELETE( pActual );
        _CLDELETE( pExpected );
        _CLDELETE( q );

        pSearch->close();
        _CLDELETE( pSearch );

        pReader->close();
        _CLDELETE( pReader );
    }

    /////////////////////////////////////////////////////////////////////////////
    void testRangeQueryId()
    {
        // NOTE: uses index build in *super* setUp

        IndexReader * pReader = IndexReader::open( index );
	    IndexSearcher * pSearch = _CLNEW IndexSearcher( pReader );

        int medId = ((maxId - minId) / 2);
        
        std::tstring sMinIP = pad(minId);
        std::tstring sMaxIP = pad(maxId);
        std::tstring sMedIP = pad(medId);
        const TCHAR* minIP = sMinIP.c_str();
        const TCHAR* maxIP = sMaxIP.c_str();
        const TCHAR* medIP = sMedIP.c_str();
    
        size_t numDocs = static_cast<size_t>( pReader->numDocs() );
        assertEqualsMsg( _T("num of docs"), numDocs, static_cast<size_t>(1+ maxId - minId));
        
	    Hits * pResult;
        Query * q;
        // test id, bounded on both ends
        
        q = csrq( _T( "id" ), minIP, maxIP, true, true );
	    pResult = pSearch->search( q );
	    assertEqualsMsg( _T( "find all" ), numDocs, pResult->length() );
        _CLDELETE( pResult ); _CLDELETE( q );

	    q = csrq( _T( "id" ), minIP, maxIP, true, false );
	    pResult = pSearch->search( q );
	    assertEqualsMsg( _T( "all but last" ), numDocs-1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

	    q = csrq( _T( "id" ), minIP, maxIP, false, true );
	    pResult = pSearch->search( q );
	    assertEqualsMsg( _T( "all but first" ), numDocs-1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );
            
	    q = csrq( _T( "id" ), minIP, maxIP, false,false );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "all but ends" ), numDocs-2, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );
    
        q = csrq( _T( "id" ), medIP, maxIP, true, true );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "med and up" ), 1+maxId-medId, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );
        
        q = csrq( _T( "id" ), minIP, medIP, true, true );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "up to med" ), 1+medId-minId, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

        // unbounded id

	    q = csrq( _T( "id" ), minIP, NULL, true, false );
	    pResult = pSearch->search( q );
	    assertEqualsMsg( _T( "min and up" ), numDocs, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

	    q = csrq( _T( "id" ), NULL, maxIP, false, true );
	    pResult = pSearch->search( q );
	    assertEqualsMsg( _T( "max and down" ), numDocs, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

	    q = csrq( _T( "id" ), minIP, NULL, false, false );
	    pResult = pSearch->search( q );
	    assertEqualsMsg( _T( "not min, but up" ), numDocs-1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );
            
	    q = csrq( _T( "id" ), NULL, maxIP, false, false );
	    pResult = pSearch->search( q );
	    assertEqualsMsg( _T( "not max, but down" ), numDocs-1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );
            
        q = csrq( _T( "id" ), medIP, maxIP, true, false );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "med and up, not max" ), maxId-medId, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );
        
        q = csrq( _T( "id" ), minIP, medIP, false,true );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "not min, up to med" ), medId-minId, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

        // very small sets

	    q = csrq( _T( "id" ), minIP, minIP, false, false );
	    pResult = pSearch->search( q );
	    assertEqualsMsg( _T( "min,min,F,F" ), 0, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

        q = csrq( _T( "id" ), medIP, medIP, false, false );
	    pResult = pSearch->search( q );
	    assertEqualsMsg( _T( "med,med,F,F" ), 0, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

        q = csrq( _T( "id") , maxIP, maxIP, false, false );
	    pResult = pSearch->search( q );
	    assertEqualsMsg( _T( "max,max,F,F" ), 0, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );
                         
	    q = csrq( _T( "id" ), minIP, minIP, true, true );
	    pResult = pSearch->search( q );
	    assertEqualsMsg( _T( "min,min,T,T" ), 1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

        q = csrq( _T( "id" ), NULL, minIP, false, true );
	    pResult = pSearch->search( q );
	    assertEqualsMsg( _T( "nul,min,F,T" ), 1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

	    q = csrq( _T( "id" ), maxIP, maxIP, true, true );
	    pResult = pSearch->search( q );
	    assertEqualsMsg( _T( "max,max,T,T" ), 1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

        q = csrq( _T( "id" ), maxIP, NULL, true, false );
	    pResult = pSearch->search( q );
	    assertEqualsMsg( _T( "max,nul,T,T" ), 1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

	    q = csrq( _T( "id" ), medIP, medIP, true, true );
	    pResult = pSearch->search( q );
	    assertEqualsMsg( _T( "med,med,T,T" ), 1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );
            
        pSearch->close();
        _CLDELETE( pSearch );

        pReader->close();
        _CLDELETE( pReader );
    }

    
    /////////////////////////////////////////////////////////////////////////////
    void testRangeQueryRand()
    {
        // NOTE: uses index build in *super* setUp

        IndexReader * pReader = IndexReader::open( index );
	    IndexSearcher * pSearch = _CLNEW IndexSearcher( pReader );

        std::tstring sMinRP = pad(minR);
        std::tstring sMaxRP = pad(maxR);
        const TCHAR* minRP = sMinRP.c_str();
        const TCHAR* maxRP = sMaxRP.c_str();
    
        size_t numDocs = static_cast<size_t>( pReader->numDocs() );
        assertEqualsMsg( _T("num of docs"), numDocs, static_cast<size_t>(1+ maxId - minId));
        
    	Hits * pResult;
        Query * q;

        // test extremes, bounded on both ends
        
        q = csrq( _T( "rand" ), minRP, maxRP, true, true );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "find all" ), numDocs, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

        q = csrq( _T( "rand" ), minRP, maxRP, true, false );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "all but biggest" ), numDocs-1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

        q = csrq( _T( "rand" ), minRP, maxRP, false, true );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "all but smallest" ), numDocs-1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

        q = csrq( _T( "rand" ), minRP, maxRP, false, false );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "all but extremes" ), numDocs-2, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );
    
        // unbounded

        q = csrq( _T( "rand" ), minRP, NULL, true, false );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "smallest and up" ), numDocs, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

        q = csrq( _T( "rand" ), NULL, maxRP, false, true );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "biggest and down" ), numDocs, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

        q = csrq( _T( "rand" ), minRP, NULL, false, false );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "not smallest, but up" ), numDocs-1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );
            
        q = csrq( _T( "rand" ), NULL, maxRP, false, false );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "not biggest, but down" ), numDocs-1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );
        
        // very small sets

        q = csrq( _T( "rand" ), minRP, minRP, false, false );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "min,min,F,F" ), 0, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

        q = csrq( _T( "rand" ), maxRP, maxRP, false, false );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "max,max,F,F" ), 0, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );
                         
        q = csrq( _T( "rand" ), minRP, minRP, true, true );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "min,min,T,T" ), 1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

        q = csrq( _T( "rand" ), NULL, minRP, false, true );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "nul,min,F,T" ), 1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

        q = csrq( _T( "rand" ), maxRP, maxRP, true, true );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "max,max,T,T" ), 1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

        q = csrq( _T( "rand" ), maxRP, NULL, true, false );
	    pResult = pSearch->search( q );
        assertEqualsMsg( _T( "max,nul,T,T" ), 1, pResult->length());
        _CLDELETE( pResult ); _CLDELETE( q );

        pSearch->close();
        _CLDELETE( pSearch );

        pReader->close();
        _CLDELETE( pReader );
    }


    /////////////////////////////////////////////////////////////////////////////
    // CLucene specific 
    // Visual Studio 2005 shows memory leaks for this test, but some other 
    // tools do not detect any memory leaks. So what is right?
    // IN VC80 shows memory leaks ONLY if both subqueries are added as 
    // MUST BooleanClauses. 
    void testBooleanMemLeaks()
    {
        IndexReader * pReader = IndexReader::open( m_pSmall );
	    IndexSearcher * pSearch = _CLNEW IndexSearcher( pReader );
 
        Query * q1 = csrq( _T( "data" ), _T( "A" ), _T( "A" ), true, true );  // matches document #0
        Query * q2 = csrq( _T( "data" ), _T( "Z" ), _T( "Z" ), true, true );  // matches document #1
        BooleanQuery * bq = _CLNEW BooleanQuery( true );
        bq->add( q1, true, BooleanClause::MUST );
        bq->add( q2, true, BooleanClause::MUST );

        Hits * pResult = pSearch->search( bq );

        _CLDELETE( pResult );
        _CLDELETE( bq );

        pSearch->close();
        _CLDELETE( pSearch );

        pReader->close();
        _CLDELETE( pReader );
    }


    /////////////////////////////////////////////////////////////////////////////
    void runTests()
    {
        setUp();
        testEqualScores(); 
        testBoost();
        testBooleanOrderUnAffected();
        testRangeQueryId();
        testRangeQueryRand();
//        testBooleanMemLeaks();
    }
};


/////////////////////////////////////////////////////////////////////////////
void testConstantScoreRangeQuery( CuTest * pTc )
{
	/// Run Java Lucene tests
	TestConstantScoreRangeQuery tester( pTc );
	tester.runTests();
}


/////////////////////////////////////////////////////////////////////////////
CuSuite *testConstantScoreQueries(void)
{
	CuSuite *suite = CuSuiteNew( _T( "CLucene ConstScoreQuery Test" ));

	SUITE_ADD_TEST(suite, testConstantScoreRangeQuery);

	return suite; 
}


// EOF
