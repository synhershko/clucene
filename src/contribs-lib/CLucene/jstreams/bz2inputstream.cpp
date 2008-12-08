/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Jos van den Oever
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/jstreams/jstreamsconfig.h"
#include "bz2inputstream.h"
using namespace jstreams;

bool
BZ2InputStream::checkHeader(const char* data, int32_t datasize) {
    static const char magic[] = {0x42, 0x5a, 0x68, 0x39, 0x31};
    if (datasize < 5) return false;
    return memcmp(data, magic, 5) == 0;
}
BZ2InputStream::BZ2InputStream(StreamBase<char>* input) {
    // initialize values that signal state
    this->input = input;

    // TODO: check first bytes of stream before allocating buffer
    // 0x42 0x5a 0x68 0x39 0x31
    if (!checkMagic()) {
        error = "Magic bytes are wrong.";
        status = Error;
        allocatedBz = false;
        return;
    }

    bzstream = (bz_stream*)malloc(sizeof(bz_stream));
    bzstream->bzalloc = NULL;
    bzstream->bzfree = NULL;
    bzstream->opaque = NULL;
    bzstream->avail_in = 0;
    bzstream->next_in = NULL;
    int r;
    r = BZ2_bzDecompressInit(bzstream, 1, 0);
    if (r != BZ_OK) {
        error = "Error initializing BZ2InputStream.";
        fprintf(stderr, "Error initializing BZ2InputStream.\n");
        dealloc();
        status = Error;
        return;
    }
    allocatedBz = true;
    // signal that we need to read into the buffer
    bzstream->avail_out = 1;

    // set the minimum size for the output buffer
    mark(262144);
}
BZ2InputStream::~BZ2InputStream() {
    dealloc();
}
void
BZ2InputStream::dealloc() {
    if (allocatedBz) {
        BZ2_bzDecompressEnd(bzstream);
        free(bzstream);
        bzstream = 0;
    }
}
bool
BZ2InputStream::checkMagic() {
    const char* begin;
    int32_t nread;

    int64_t pos = input->getPosition();
    nread = input->read(begin, 5, 5);
    input->reset(pos);
    if (nread != 5) {
        return false;
    }

    return checkHeader(begin, 5);
}
void
BZ2InputStream::readFromStream() {
    // read data from the input stream
    const char* inStart;
    int32_t nread;
    nread = input->read(inStart, 1, 0);
    if (nread <= -1) {
        status = Error;
        error = input->getError();
    } else if (nread < 1) {
        status = Error;
        error = "unexpected end of stream";
    } else {
        bzstream->next_in = (char*)inStart;
        bzstream->avail_in = nread;
    }
}
int32_t
BZ2InputStream::fillBuffer(char* start, int32_t space) {
    if (bzstream == 0) return -1;
    // make sure there is data to decompress
    if (bzstream->avail_out != 0) {
        readFromStream();
        if (status != Ok) {
            // no data was read
            return -1;
        }
    }
    // make sure we can write into the buffer
    bzstream->avail_out = space;
    bzstream->next_out = start;
    // decompress
    int r = BZ2_bzDecompress(bzstream);
    // inform the buffer of the number of bytes that was read
    int32_t nwritten = space - bzstream->avail_out;
    switch (r) {
    case BZ_PARAM_ERROR:
        error = "BZ_PARAM_ERROR";
        status = Error;
        return -1;
    case BZ_DATA_ERROR:
        error = "BZ_DATA_ERROR";
        status = Error;
        return -1;
    case BZ_DATA_ERROR_MAGIC:
        error = "BZ_DATA_ERROR_MAGIC";
        status = Error;
        return -1;
    case BZ_MEM_ERROR:
        error = "BZ_MEM_ERROR";
        status = Error;
        return -1;
    case BZ_STREAM_END:
        if (bzstream->avail_in) {
            input->reset(input->getPosition()-bzstream->avail_in);
        }
        // we are finished decompressing,
        // (but this stream is not yet finished)
        dealloc();
    }
    return nwritten;
}
