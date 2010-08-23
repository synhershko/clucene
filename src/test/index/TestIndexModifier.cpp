/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"

#include "CLucene/index/IndexModifier.h"

CL_NS_USE(store)
CL_NS_USE(index)
CL_NS_USE(document)
CL_NS_USE2(analysis,standard)

void IndexModifierExceptionTest(CuTest *tc)
{
    class LockedLock : public LuceneLock
    {
    public:
        LockedLock() : LuceneLock() {};
        virtual bool obtain() {return obtain(0);};
        virtual void release() {};
        virtual bool isLocked() {return true;};
        bool obtain(int64_t lockWaitTimeout) {return false;};
        virtual std::string toString() {return "LockedLock";};
        virtual const char* getObjectName() const {return "LockedLock";};
    };

    class LockedDirectory : public RAMDirectory
    {
    public:
        bool    errOn;

        LockedDirectory() : RAMDirectory(), errOn(false) {};

        // this simulates locking problem, only if errOn is true
        LuceneLock* makeLock(const char* name) {
            if (errOn)
                return _CLNEW LockedLock();
            else
                return RAMDirectory::makeLock(name);
        };
    };

    LockedDirectory     directory;
    StandardAnalyzer    analyzer;
    Document            doc;
    IndexModifier *     pIm = NULL;

    try
    {
        doc.add(* _CLNEW Field(_T("text"), _T("Document content"), Field::STORE_YES | Field::INDEX_TOKENIZED));
        pIm = _CLNEW IndexModifier(&directory, &analyzer, true);

        pIm->addDocument(&doc);
        pIm->deleteDocument(0);
    }
    catch (CLuceneError & err)
    {
        CuFail(tc, _T("Exception thrown upon startup"));
        return;
    }

    try
    {
        // switch on locking timeout simulation
        directory.errOn = true;

        // throws lock timeout exception
        pIm->addDocument(&doc);
        CuFail(tc, _T("Exception was not thrown during addDocument"));
    }
    catch (CLuceneError & err)
    {
    }

    // this produces Access Violation exception
    try {
        _CLLDELETE(pIm);
    } catch (...)
    {
        CuFail(tc, _T("Exception thrown upon deletion"));
    }
}

CuSuite *testIndexModifier(void)
{
    CuSuite *suite = CuSuiteNew(_T("CLucene IndexModifer Test"));
    SUITE_ADD_TEST(suite, IndexModifierExceptionTest);

  return suite;
}

