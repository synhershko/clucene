/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Jos van den Oever
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef SUBSTREAMPROVIDER
#define SUBSTREAMPROVIDER

#include <string>
#include "streambase.h"

namespace jstreams {

struct EntryInfo {
    std::string filename;
    int32_t size;
    unsigned int mtime;
    enum Type {Unknown=0, Dir=1, File=2};
    Type type;
    EntryInfo() :size(-1), mtime(0), type(Unknown) {}
};

class SubStreamProvider {
protected:
    StreamStatus status;
    std::string error;
    StreamBase<char> *input;
    StreamBase<char> *entrystream;
    EntryInfo entryinfo;
public:
    SubStreamProvider(StreamBase<char> *i) :status(Ok), input(i), entrystream(0)
        {}
    virtual ~SubStreamProvider() { if (entrystream) delete entrystream; }
    StreamStatus getStatus() const { return status; }
    virtual StreamBase<char>* nextEntry() = 0;
    StreamBase<char>* currentEntry() { return entrystream; }
    const EntryInfo &getEntryInfo() const {
        return entryinfo;
    }
    const char* getError() const { return error.c_str(); }
};

} // end namespace jstreams

#endif
