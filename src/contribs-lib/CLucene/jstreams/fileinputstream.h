/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Jos van den Oever
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef FILEINPUTSTREAM_H
#define FILEINPUTSTREAM_H

#include "bufferedstream.h"

namespace jstreams {

class FileInputStream : public BufferedInputStream<char> {
private:
    FILE *file;
    std::string filepath;

public:
    static const int32_t defaultBufferSize;
    explicit FileInputStream(const char *filepath,
        int32_t buffersize=defaultBufferSize);
    ~FileInputStream();
    int32_t fillBuffer(char* start, int32_t space);
};

} // end namespace jstreams

#endif

