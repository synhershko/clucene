/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"

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

    BaseTestRangeFilter(CuTest* _tc);
    virtual ~BaseTestRangeFilter();

    /**
     * a simple padding function that should work with any int
     */
    std::tstring pad(int n);
    
private:
    void build();

public:
    void testPad();
};


// EOF
