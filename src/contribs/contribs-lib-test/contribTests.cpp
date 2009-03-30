#include "test.h"

unittest tests[] = {
    {"analysis", testanalysis},
    {"snowball", testsnowball},
    {"highlighter",testhighlighter},
    {"streams",teststreams},
    {"utf8",testutf8},
    {"LastTest", NULL}
};
