#ifndef JSTREAMS_JSTREAMCONFIG_H
#define JSTREAMS_JSTREAMCONFIG_H

#include "CLucene/_ApiHeader.h"

#if @ICONV_SECOND_ARGUMENT_IS_CONST@
    #define _CL_ICONV_CONST const
#else
    #define _CL_ICONV_CONST
#endif

#endif
