/*------------------------------------------------------------------------------
* Copyright (C) 2009 Isidor Zeuner
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef CLUCENE_UTIL_BYTEINPUTSTREAM_H
#define CLUCENE_UTIL_BYTEINPUTSTREAM_H

#include "CLucene/_ApiHeader.h"
#include "CLucene/util/CLStreams.h"
#include "CLucene/util/Array.h"

CL_NS_DEF(util)

class CLUCENE_CONTRIBS_EXPORT ByteInputStream : public CL_NS(util)::InputStream {
public:
	ByteInputStream(ArrayBase<signed char> const* data);
	int32_t read(const signed char*& start, int32_t min, int32_t max);
	int64_t skip(int64_t ntoskip);
	int64_t position();
	size_t size();
private:
	ArrayBase<signed char> const* data;
	int64_t current_position;
};
CL_NS_END
#endif
