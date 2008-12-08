/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Jos van den Oever
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/jstreams/jstreamsconfig.h"
#include "zipinputstream.h"
#include "gzipinputstream.h"
#include "subinputstream.h"

#include "dostime.h"

using namespace jstreams;

bool
ZipInputStream::checkHeader(const char* data, int32_t datasize) {
    static const char magic[] = {0x50, 0x4b, 0x03, 0x04};
    if (datasize < 4) return false;
    bool ok = memcmp(data, magic, 4) == 0 && datasize > 8;
    return ok;
}
ZipInputStream::ZipInputStream(StreamBase<char>* input)
        : SubStreamProvider(input) {
    compressedEntryStream = 0;
    uncompressionStream = 0;
}
ZipInputStream::~ZipInputStream() {
    if (compressedEntryStream) {
        delete compressedEntryStream;
    }
    if (uncompressionStream) {
        delete uncompressionStream;
    }
}
StreamBase<char>*
ZipInputStream::nextEntry() {
    if (status) return 0;
    // clean up the last stream(s)
    if (entrystream) {
	// if this entry is a compressed entry of know size, we can skip to
	// the end by skipping in the compressed stream, without decompressing
        if (compressedEntryStream) {
            compressedEntryStream->skip(compressedEntryStream->getSize());
            delete compressedEntryStream;
            compressedEntryStream = 0;
            delete uncompressionStream;
            uncompressionStream = 0;
        } else {
            int32_t size = entrystream->getSize();
            if (size < 1) {
                size = 1024;
            }
            while (entrystream->getStatus() == Ok) {
                entrystream->skip(size);
            }
            if (entryinfo.size < 0) {
                // skip the data descriptor that occurs after the data
                const char* c;
                int32_t n = input->read(c, 4, 4);
                if (n == 4) {
                    n = read4bytes((const unsigned char*)c);
                    if (n == 0x08074b50) { // sometimes this signature appears
                        n = input->read(c, 12, 12);
                        n -= 8;
                    } else {
                        n = input->read(c, 8, 8);
                        n -= 4;
                    }
                }
                if (n != 4) {
                    status = Error;
                    error = "No valid data descriptor after entry data.";
                    return 0;
                }
            }
        }
        delete entrystream;
        entrystream = 0;
    }
    readHeader();
    if (status) return 0;
    if (compressionMethod == 8) {
        if (entryinfo.size >= 0) {
            compressedEntryStream = new SubInputStream(input, entryCompressedSize);
            if (uncompressionStream) {
                delete uncompressionStream;
            }
            uncompressionStream = new GZipInputStream(compressedEntryStream,
                GZipInputStream::ZIPFORMAT);
            entrystream
                = new SubInputStream(uncompressionStream, entryinfo.size);
        } else {
            entrystream = new GZipInputStream(input,
                GZipInputStream::ZIPFORMAT);
        }
    } else {
        entrystream = new SubInputStream(input, entryinfo.size);
    }
    return entrystream;
}
int32_t
ZipInputStream::read2bytes(const unsigned char *b) {
    return b[0] + (b[1]<<8);
}
int32_t
ZipInputStream::read4bytes(const unsigned char *b) {
    return b[0] + (b[1]<<8) + (b[2]<<16) + (b[3]<<24);
}
void
ZipInputStream::readHeader() {
    const unsigned char *hb;
    const char *b;
    int32_t toread;
    int32_t nread;

    // read the first 30 characters
    toread = 30;
    nread = input->read(b, toread, toread);
    if (nread != toread) {
        error = "Error reading zip header: ";
        if (nread == -1) {
            error += input->getError();
        } else {
            error += " premature end of file.";
        }
        status = Error;
        fprintf(stderr, "%s\n", error.c_str());
        return;
    }
    hb = (const unsigned char*)b;
    // check the signature
    // check the first half of the signature
    if (hb[0] != 0x50 || hb[1] != 0x4b) {
        // signature is invalid
        status = Error;
        error = "Error: wrong zip signature.";
        return;
    }
    // check the second half of the signature
    if (hb[2] != 0x03 || hb[3] != 0x04) {
        // this may be the regular end of the file
        if (hb[2] != 0x01 || hb[3] != 0x02) {
            fprintf(stderr, "This is new: %x %x %x %x\n",
                hb[0], hb[1], hb[2], hb[3]);
        }
        status = Eof;
        return;
    }
    // read 2 bytes into the filename size
    int32_t filenamelen = read2bytes(hb + 26);
    int64_t extralen = read2bytes(hb + 28);
    // read 4 bytes into the length of the uncompressed size
    entryinfo.size = read4bytes(hb + 22);
    // read 4 bytes into the length of the compressed size
    entryCompressedSize = read4bytes(hb + 18);
    compressionMethod = read2bytes(hb + 8);
    int32_t generalBitFlags = read2bytes(hb+6);
    if (generalBitFlags & 8) { // is bit 3 set?
        // ohoh, the file size and compressed file size are unknown at this
        // point
	// if the file is compressed with method 8 we rely on the decompression
	// stream to signal the end of the stream properly
        if (compressionMethod != 8) {
        	status = Error;
        	error = "This particular zip file format is not supported for "
			"reading as a stream.";
	        return;
        }
        entryinfo.size = -1;
        entryCompressedSize = -1;
    }
    unsigned long dost = read4bytes(hb+10);
    entryinfo.mtime = dos2unixtime(dost);

    readFileName(filenamelen);
    if (status) {
        status = Error;
        error = "Error reading file name.";
        return;
    }
    // read 2 bytes into the length of the extra field
    int64_t skipped = input->skip(extralen);
    if (skipped != extralen) {
        status = Error;
        error = "Error skipping extra field.";
        return;
    }
}
void
ZipInputStream::readFileName(int32_t len) {
    entryinfo.filename.resize(0);
    const char *begin;
    int32_t nread = input->read(begin, len, len);
    if (nread != len) {
        error = "Error reading filename: ";
        if (nread == -1) {
            error += input->getError();
        } else {
            error += " premature end of file.";
        }
        return;
    }
    entryinfo.filename.append(begin, nread);

    // temporary hack for determining if this is a directory:
    // does the filename end in '/'?
    len = entryinfo.filename.length();
    if (entryinfo.filename[len-1] == '/') {
        entryinfo.filename.resize(len-1);
        entryinfo.type = EntryInfo::Dir;
    } else {
        entryinfo.type = EntryInfo::File;
    }
}
