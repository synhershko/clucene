/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Jos van den Oever
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef INPUTSTREAMREADER_H
#define INPUTSTREAMREADER_H

#include <string>
#include <iconv.h>
#include "bufferedstream.h"

namespace jstreams {

/**
 * Info on conversions:
http://www.gnu.org/software/libc/manual/html_node/iconv-Examples.html
http://tangentsoft.net/mysql++/doc/userman/html/unicode.html
 **/
class InputStreamReader : public BufferedInputStream<wchar_t> {
private:
    iconv_t converter;
    bool finishedDecoding;
    StreamBase<char>* input;
    int32_t charsLeft;

    InputStreamBuffer<char> charbuf;
    void readFromStream();
    int32_t decode(wchar_t* start, int32_t space);
public:
    explicit InputStreamReader(StreamBase<char> *i, const char *enc=0);
    ~InputStreamReader();
    int32_t fillBuffer(wchar_t* start, int32_t space);
};

} // end namespace jstreams

#endif
