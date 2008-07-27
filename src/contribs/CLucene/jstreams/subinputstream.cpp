/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Jos van den Oever
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/jstreams/jstreamsconfig.h"
#include "subinputstream.h"
#include <cassert>
using namespace jstreams;

SubInputStream::SubInputStream(StreamBase<char> *i, int64_t length)
        : offset(i->getPosition()), input(i) {
    assert(length >= -1);
//    printf("substream offset: %lli\n", offset);
    size = length;
}
int32_t
SubInputStream::read(const char*& start, int32_t min, int32_t max) {
    if (size != -1) {
        const int64_t left = size - position;
        if (left == 0) {
            return -1;
        }
        // restrict the amount of data that can be read
        if (max <= 0 || max > left) {
            max = (int32_t)left;
        }
        if (min > max) min = max;
        if (left < min) min = (int32_t)left;
    }
    int32_t nread = input->read(start, min, max);
    if (nread < -1) {
        fprintf(stderr, "substream too short.\n");
        status = Error;
        error = input->getError();
    } else if (nread < min) {
        if (size == -1) {
            status = Eof;
            if (nread > 0) {
                position += nread;
                size = position;
            }
        } else {
//            fprintf(stderr, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! nread %i min %i max %i size %lli\n", nread, min, max, size);
//            fprintf(stderr, "pos %lli parentpos %lli\n", position, input->getPosition());
//            fprintf(stderr, "status: %i error: %s\n", input->getStatus(), input->getError());
            // we expected data but didn't get enough so that's an error
            status = Error;
            error = "Premature end of stream\n";
            nread = -2;
        }
    } else {
        position += nread;
        if (position == size) {
            status = Eof;
        }
    }
    return nread;
}
int64_t
SubInputStream::reset(int64_t newpos) {
    assert(newpos >= 0);
//    fprintf(stderr, "subreset pos: %lli newpos: %lli offset: %lli\n", position,
//        newpos, offset);
    position = input->reset(newpos + offset);
    if (position < offset) {
        fprintf(stderr, "########### position %lli newpos %lli\n", position, newpos);
        status = Error;
        error = input->getError();
    } else {
        position -= offset;
        status = input->getStatus();
    }
    return position;
}
int64_t
SubInputStream::skip(int64_t ntoskip) {
//    printf("subskip pos: %lli ntoskip: %lli offset: %lli\n", position, ntoskip, offset);
    if (size == position) {
        status = Eof;
        return -1;
    }
    if (size != -1) {
        const int64_t left = size - position;
        // restrict the amount of data that can be skipped
        if (ntoskip > left) {
            ntoskip = left;
        }
    }
    int64_t skipped = input->skip(ntoskip);
    if (input->getStatus() == Error) {
        status = Error;
        error = input->getError();
    } else {
        position += skipped;
        if (position == size) {
            status = Eof;
        }
    }
    return skipped;
}
