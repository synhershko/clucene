/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"

#include "CLucene/search/RangeFilter.h"

#include <limits>

namespace std
{
#ifdef _UNICODE
    typedef wstring tstring;
#else
    typedef string tstring;
#endif
};

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
        , tc(tc)
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

// EOF
