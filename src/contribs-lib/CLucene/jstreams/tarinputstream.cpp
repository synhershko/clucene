/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Jos van den Oever
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/jstreams/jstreamsconfig.h"
#include "tarinputstream.h"
#include "subinputstream.h"
#include <cstring>
using namespace jstreams;

TarInputStream::TarInputStream(StreamBase<char>* input)
        : SubStreamProvider(input) {
}

TarInputStream::~TarInputStream() {
}
StreamBase<char>*
TarInputStream::nextEntry() {
    if (status) return 0;
    if (entrystream) {
        entrystream->skip(entrystream->getSize());
        input->skip(numPaddingBytes);
        delete entrystream;
        entrystream = 0;
    }
    parseHeader();
    if (status) return 0;
    entrystream = new SubInputStream(input, entryinfo.size);
    return entrystream;
}
const char*
TarInputStream::readHeader() {
    // read the first 500 characters
    const char *begin;
    int32_t nread = input->read(begin, 512, 512);
    if (nread != 512) {
        status = Error;
    }
    return begin;
}
bool
TarInputStream::checkHeader(const char* h, const int32_t hsize) {
    if (hsize < 257) {
        // header is too small to check
        return false;
    }
    // check for field values that should be '\0' for the header to be a
    // tar header. Two positions are also accepted if they are ' ' because they
    return !(h[107] || h[115] || h[123] || (h[135]&&h[135]!=' ')
            || (h[147] && h[147] != ' ') || h[256]);
}
void
TarInputStream::parseHeader() {
    const char *hb;
    hb = readHeader();
    if (status) return;

    // check for terminators ('\0') on the first couple of fields
    if (!checkHeader(hb, 257)) {
        error = "Invalid tar header.\n";
        status = Error;
        return;
    }

    int32_t len = strlen(hb);
    if (len == 0) {
        // ready
        status = Eof;
        return;
    }
    if (len > 100) len = 100;
    entryinfo.filename.resize(0);
    entryinfo.filename.append(hb, len);
    if (entryinfo.filename == "././@LongLink") {
        entryinfo.filename.resize(0);
        readLongLink(hb);
        if (status) return;
        hb = readHeader();
        if (status) return;
    }

    // read the file size which is in octal format
    entryinfo.size = readOctalField(hb, 124);
    if (status) return;
    entryinfo.mtime = readOctalField(hb, 136);
    if (status) return;

    numPaddingBytes = 512 - entryinfo.size%512;
    if (numPaddingBytes == 512) {
        numPaddingBytes = 0;
    }

    len = entryinfo.filename.length();
    if (entryinfo.filename[len-1] == '/') {
        entryinfo.filename.resize(len-1);
    }
    // read file type
    if (hb[156] == 0 || hb[156] == '0') {
        entryinfo.type = EntryInfo::File;
    } else if (hb[156] == '5') {
        entryinfo.type = EntryInfo::Dir;
    } else {
        entryinfo.type = EntryInfo::Unknown;
    }
//    printf("!%s %i\n", entryinfo.filename.c_str(), hb[156]);
}
int32_t
TarInputStream::readOctalField(const char *b, int32_t offset) {
    int32_t val;
    int r = sscanf(b+offset, "%o", &val);
    if (r != 1) {
        status = Error;
        error = "Error reading header: octal field is not a valid integer.";
        return 0;
    }
    return val;
}
void
TarInputStream::readLongLink(const char *b) {
    int32_t toread = readOctalField(b, 124);
    int32_t left = toread%512;
    if (left) {
        left = 512 - left;
    }
    const char *begin;
    if (status) return;
    int32_t nread = input->read(begin, toread, toread);
    if (nread != toread) {
            status = Error;
            error = "Error reading LongLink: ";
            if (nread == -1) {
                error += input->getError();
            } else {
                error += " premature end of file.";
            }
            return;
    }
    entryinfo.filename.append(begin, nread);

    int64_t skipped = input->skip(left);
    if (skipped != left) {
        status = Error;
        error = "Error reading LongLink: ";
        if (input->getStatus() == Error) {
            error += input->getError();
        } else {
            error += " premature end of file.";
        }
    }
}
