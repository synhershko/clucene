#ifndef FILEREADER_H
#define FILEREADER_H

#include "streambase.h"

namespace jstreams {

class FileInputStream;
class InputStreamReader;
class FileReader : public StreamBase<wchar_t> {
    FileInputStream* input;
    InputStreamReader* reader;
public:
    explicit FileReader(const char* fname, const char* encoding_scheme=NULL,
        const int32_t cachelen = 13,
        const int32_t cachebuff = 14 );
    ~FileReader();
    int32_t read(const wchar_t*& start, int32_t min, int32_t max);
    int64_t reset(int64_t);
};

} // end namespace jstreams

#endif
