#ifndef STREAMREADER_H
#define STREAMREADER_H

#include "Reader.h"

namespace jstreams {

class StringReader : public Reader {
private:
    TCHAR* data;
    uint32_t markpt;
    uint32_t pt;
    size_t len;
    bool ownData;
public:
    StringReader ( const TCHAR* value );
    StringReader ( const TCHAR* value, const size_t length );
    StringReader ( TCHAR* value, const size_t length, bool copyData );
    ~StringReader();
    void close();
    StreamStatus read(TCHAR&);
    StreamStatus read(const TCHAR*& start, size_t& nread, size_t max=0);
    StreamStatus mark(size_t readlimit);
    StreamStatus reset();
};

} // end of namespace jstreams

#endif
