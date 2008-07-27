#include "CLucene/jstreams/jstreamsconfig.h"
#include "filereader.h"
#include "fileinputstream.h"
#include "inputstreamreader.h"
using namespace jstreams;

FileReader::FileReader(const char* fname, const char* encoding_scheme,
        int32_t cachelen, int32_t /*cachebuff*/) {
    input = new FileInputStream(fname, cachelen);
    reader = new InputStreamReader(input, encoding_scheme);
}
FileReader::~FileReader() {
    if (reader) delete reader;
    if (input) delete input;
}
int32_t
FileReader::read(const wchar_t*& start, int32_t min, int32_t max) {
    int32_t nread = reader->read(start, min, max);
    if (nread < -1) {
        error = reader->getError();
        status = Error;
        return nread;
    } else if (nread == -1) {
        status = Eof;
    }
    return nread;
}
int64_t
FileReader::reset(int64_t newpos) {
    position = reader->reset(newpos);
    if (position < -1) {
        status = Error;
        error = reader->getError();
    }
    return position;
}

