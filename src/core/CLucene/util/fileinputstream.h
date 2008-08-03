/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Jos van den Oever
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
/* This file is part of Strigi Desktop Search
 *
 * Copyright (C) 2006 Jos van den Oever <jos@vandenoever.info>
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
#ifndef FILEINPUTSTREAM_H
#define FILEINPUTSTREAM_H

#include <stdio.h>
#include "bufferedstream.h"

namespace jstreams {

class CLUCENE_EXPORT FileInputStream : public BufferedInputStream<char> {
private:
    FILE *file;
    std::string filepath;

public:
    LUCENE_STATIC_CONSTANT(int32_t, defaultBufferSize=1048576);
    FileInputStream(const char *filepath, int32_t buffersize=defaultBufferSize);
    ~FileInputStream();
    int32_t fillBuffer(char* start, int32_t space);
};

} // end namespace jstreams

#endif

