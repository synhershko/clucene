/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Jos van den Oever
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/jstreams/jstreamsconfig.h"
#include "inputstreamreader.h"
#include "CLucene/jstreams/jstreamsconfig.h"
#include <cerrno>
using namespace jstreams;

#ifndef ICONV_CONST
     //we try to guess whether the iconv function requires
     //a const char. We have no way of automatically figuring
     //this out if we did not use autoconf, so we guess based
     //on certain parameters:
     #ifdef _LIBICONV_H
          #define ICONV_CONST const
     #else
          #define ICONV_CONST
     #endif
#endif

InputStreamReader::InputStreamReader(StreamBase<char>* i, const char* enc) {
    status = Ok;
    finishedDecoding = false;
    input = i;
    if (enc == 0) enc = "UTF-8";
#ifdef _LIBICONV_H
    if (sizeof(wchar_t) == 4) {
        converter = iconv_open("UCS-4-INTERNAL", enc);
    } if (sizeof(wchar_t) == 2) {
        converter = iconv_open("UCS-2-INTERNAL", enc);
#else
    if (sizeof(wchar_t) > 1) {
        converter = iconv_open("WCHAR_T", enc);
#endif
    } else {
        converter = iconv_open("ASCII", enc);
    }

    // check if the converter is valid
    if (converter == (iconv_t) -1) {
        error = "conversion from '";
        error += enc;
        error += "' not available.";
        status = Error;
        return;
    }
    charbuf.setSize(262);
    //mark(262);
    charsLeft = 0;
}
InputStreamReader::~InputStreamReader() {
    if (converter != (iconv_t) -1) {
        iconv_close(converter);
    }
}
int32_t
InputStreamReader::decode(wchar_t* start, int32_t space) {
    // decode from charbuf
    ICONV_CONST char *inbuf = charbuf.readPos;
    size_t inbytesleft = charbuf.avail;
    size_t outbytesleft = sizeof(wchar_t)*space;
    char *outbuf = (char*)start;
    size_t r = iconv(converter, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    int32_t nwritten;
    if (r == (size_t)-1) {
        switch (errno) {
        case EILSEQ: //invalid multibyte sequence
            error = "Invalid multibyte sequence.";
            status = Error;
            return -1;
        case EINVAL: // last character is incomplete
            // move from inbuf to the end to the start of
            // the buffer
            memmove(charbuf.start, inbuf, inbytesleft);
            charbuf.readPos = charbuf.start;
            charbuf.avail = inbytesleft;
            nwritten = ((wchar_t*)outbuf) - start;
            break;
        case E2BIG: // output buffer is full
            charbuf.readPos += charbuf.avail - inbytesleft;
            charbuf.avail = inbytesleft;
            nwritten = space;
            break;
        default:
            printf("InputStreamReader::error %d\n", errno);
        }
    } else { //input sequence was completely converted
        charbuf.readPos = charbuf.start;
        charbuf.avail = 0;
        nwritten = ((wchar_t*)outbuf) - start;
        if (input == 0) {
            finishedDecoding = true;
        }
    }
    return nwritten;
}
int32_t
InputStreamReader::fillBuffer(wchar_t* start, int32_t space) {
    // fill up charbuf
    if (input && charbuf.readPos == charbuf.start) {
        const char *begin;
        int32_t numRead;
        numRead = input->read(begin, 1, charbuf.size - charbuf.avail);
        //printf("filled up charbuf\n");
        if (numRead < -1) {
            error = input->getError();
            status = Error;
            input = 0;
            return numRead;
        }
        if (numRead < 1) {
            // signal end of input buffer
            input = 0;
            if (charbuf.avail) {
                error = "stream ends on incomplete character";
                status = Error;
            }
            return -1;
        }
        // copy data into other buffer
        memmove(charbuf.start + charbuf.avail, begin, numRead);
        charbuf.avail = numRead + charbuf.avail;
    }
    // decode
    int32_t n = decode(start, space);
    //printf("decoded %i\n", n);
    return n;
}
