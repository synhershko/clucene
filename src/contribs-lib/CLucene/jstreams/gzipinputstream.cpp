/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Jos van den Oever
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/jstreams/jstreamsconfig.h"
#include "gzipinputstream.h"
#include <zlib.h>
using namespace jstreams;

GZipInputStream::GZipInputStream(StreamBase<char>* input, ZipFormat format) {
    // initialize values that signal state
    status = Ok;
    zstream = 0;

    this->input = input;

    // check first bytes of stream before allocating buffer
    if (format == GZIPFORMAT && !checkMagic()) {
        error = "Magic bytes are wrong.";
        status = Error;
        return;
    }

    // initialize the z_stream
    zstream = (z_stream_s*)malloc(sizeof(z_stream_s));
    zstream->zalloc = Z_NULL;
    zstream->zfree = Z_NULL;
    zstream->opaque = Z_NULL;
    zstream->avail_in = 0;
    zstream->next_in = Z_NULL;
    // initialize for reading gzip streams
    // for reading libz streams, you need inflateInit(zstream)
    int r;
    switch(format) {
    case ZLIBFORMAT:
        r = inflateInit(zstream);
        break;
    case GZIPFORMAT:
        r = inflateInit2(zstream, 15+16);
        break;
    case ZIPFORMAT:
    default:
        r = inflateInit2(zstream, -MAX_WBITS);
        break;
    }
    if (r != Z_OK) {
        error = "Error initializing GZipInputStream.";
        dealloc();
        status = Error;
        return;
    }

    // signal that we need to read into the buffer
    zstream->avail_out = 1;

    // initialize the buffer
    mark(262144);
}
GZipInputStream::~GZipInputStream() {
    dealloc();
}
void
GZipInputStream::dealloc() {
    if (zstream) {
        inflateEnd(zstream);
        free(zstream);
        zstream = 0;
    }
}
bool
GZipInputStream::checkMagic() {
    const unsigned char* buf;
    const char* begin;
    int32_t nread;

    int64_t pos = input->getPosition();
    nread = input->read(begin, 2, 2);
    input->reset(pos);
    if (nread != 2) {
        return false;
    }

    buf = (const unsigned char*)begin;
    return buf[0] == 0x1f && buf[1] == 0x8b;
}
void
GZipInputStream::readFromStream() {
    // read data from the input stream
    const char* inStart;
    int32_t nread;
    nread = input->read(inStart, 1, 0);
    if (nread < -1) {
        status = Error;
        error = input->getError();
    } else if (nread < 1) {
        status = Error;
        error = "unexpected end of stream";
    } else {
        zstream->next_in = (Bytef*)inStart;
        zstream->avail_in = nread;
    }
}
int32_t
GZipInputStream::fillBuffer(char* start, int32_t space) {
    if (zstream == 0) return -1;
    // make sure there is data to decompress
    if (zstream->avail_out) {
        readFromStream();
        if (status == Error) {
            // no data was read
            return -1;
        }
    }
    // make sure we can write into the buffer
    zstream->avail_out = space;
    zstream->next_out = (Bytef*)start;
    // decompress
    int r = inflate(zstream, Z_SYNC_FLUSH);
    // inform the buffer of the number of bytes that was read
    int32_t nwritten = space - zstream->avail_out;
    switch (r) {
    case Z_NEED_DICT:
        error = "Z_NEED_DICT while inflating stream.";
        status = Error;
        break;
    case Z_DATA_ERROR:
        error = "Z_DATA_ERROR while inflating stream.";
        status = Error;
        break;
    case Z_MEM_ERROR:
        error = "Z_MEM_ERROR while inflating stream.";
        status = Error;
        break;
    case Z_STREAM_END:
        if (zstream->avail_in) {
            input->reset(input->getPosition()-zstream->avail_in);
        }
        // we are finished decompressing,
        // (but this stream is not yet finished)
        dealloc();
    }
    return nwritten;
}
