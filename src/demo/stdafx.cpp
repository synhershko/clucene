/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
// stdafx.cpp : source file that includes just the standard includes
// demo.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#if defined(_CL_HAVE_SYS_TIME_H)
# include <sys/time.h>
#else
# include <time.h>
#endif

uint64_t currentTimeMillis() {
#ifndef _CL_HAVE_GETTIMEOFDAY
    struct _timeb tstruct;
    _ftime(&tstruct);

    return (((uint64_t) tstruct.time) * 1000) + tstruct.millitm;
#else
    struct timeval tstruct;
    if (gettimeofday(&tstruct, NULL) < 0) {
		_CLTHROWA(CL_ERR_Runtime,"Error in gettimeofday call.");
    }

    return (((uint64_t) tstruct.tv_sec) * 1000) + tstruct.tv_usec / 1000;
#endif
}

void strcpy_a2t(wchar_t* to, const char* from, size_t len){
	for ( size_t i=0;i<len && from[i];i++)
		to[i] = from[i];
}
