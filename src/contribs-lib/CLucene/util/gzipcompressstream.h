/* This file is part of Strigi Desktop Search
 *
 * Copyright (C) 2006 Ben van Klinken <ustramooner at users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef CLUENE_UTIL_GZIPCOMPRESSSTREAM_H
#define CLUENE_UTIL_GZIPCOMPRESSSTREAM_H

#include "CLucene/util/CLStreams.h"

struct z_stream_s;

CL_NS_DEF(util)

class CLUCENE_CONTRIBS_EXPORT GZipCompressInputStream : public InputStream{
private:
	 class Internal;
	 Internal* internal;
public:
	 LUCENE_STATIC_CONSTANT(int32_t, DEFAULT_BUFFER_SIZE=4096);
    explicit GZipCompressInputStream(InputStream* input, int level=-1);
    virtual ~GZipCompressInputStream();
	
	 int32_t read(const signed char*& start, int32_t min, int32_t max);
	 int64_t position();
	 int64_t reset(int64_t);
	 int64_t skip(int64_t ntoskip);
	 size_t size();
	 void setMinBufSize(int32_t minbufsize);
};

CL_NS_END
#endif
