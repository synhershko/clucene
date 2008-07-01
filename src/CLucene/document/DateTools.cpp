/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#ifdef _CL_TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if defined(_CL_HAVE_SYS_TIME_H)
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef _CL_HAVE_SYS_TIMEB_H
# include <sys/timeb.h>
#endif

// Defining function macros missing in specific enviroments
#ifndef _tcsftime
	#ifdef _UCS2
		#define _tcsftime wcsftime
	#else
		#define _tcsftime strftime
	#endif
#endif
#ifndef _ttoi
	#define _ttoi(x) (int)_tcstoi64(x,NULL,10)
#endif

#include "DateTools.h"
#include "CLucene/util/Misc.h"
CL_NS_USE(util)
CL_NS_DEF(document)

TCHAR* DateTools::timeToString(const int64_t time, Resolution resolution /*= MILLISECOND_FORMAT*/) {
	TCHAR* buf = _CL_NEWARRAY(TCHAR, DATETOOLS_BUFFER_SIZE);
	timeToString(time, resolution, buf);
	return buf;
}

void DateTools::timeToString(const int64_t time, Resolution resolution, TCHAR* buf) {
	time_t secs = time / 1000;
	tm *ptm = gmtime(&secs);

	if (resolution == MILLISECOND_FORMAT) {
		size_t len = _tcsftime(buf, DATETOOLS_BUFFER_SIZE, _T("%Y%m%d%H%M%S"), ptm);
		uint32_t ms = time % 1000;
		_sntprintf(buf + len, 3, _T("%03u"), ms);
	} else if (resolution == SECOND_FORMAT) {
		_tcsftime(buf, DATETOOLS_BUFFER_SIZE, _T("%Y%m%d%H%M%S"), ptm);
	} else if (resolution == MINUTE_FORMAT) {
		_tcsftime(buf, DATETOOLS_BUFFER_SIZE, _T("%Y%m%d%H%M"), ptm);
	} else if (resolution == YEAR_FORMAT) {
		_tcsftime(buf, DATETOOLS_BUFFER_SIZE, _T("%Y"), ptm);
	} else if (resolution == MONTH_FORMAT) {
		_tcsftime(buf, DATETOOLS_BUFFER_SIZE, _T("%Y%m"), ptm);
	} else if (resolution == DAY_FORMAT) {
		_tcsftime(buf, DATETOOLS_BUFFER_SIZE, _T("%Y%m%d"), ptm);
	} else if (resolution == HOUR_FORMAT) {
		_tcsftime(buf, DATETOOLS_BUFFER_SIZE, _T("%Y%m%d%H"), ptm);
	}
}


int64_t DateTools::stringToTime(const TCHAR* dateString) {
	int64_t ret = 0;
	tm s_time;
	memset(&s_time, 0, sizeof(s_time));
	s_time.tm_mday=1;
	int32_t ms = 0;

	switch (_tcslen(dateString)) {
		case 4: // YEAR_FORMAT
			{
				s_time.tm_year = _ttoi( dateString ) - 1900;
				break;
			}
		case 6: // MONTH_FORMAT
			{
				TCHAR* tmpDate = STRDUP_TtoT(dateString);
				s_time.tm_mon = _ttoi(&tmpDate[4]) - 1;
				tmpDate[4] = 0;
				s_time.tm_year = _ttoi( tmpDate ) - 1900;
				delete[] tmpDate;
				break;
			}
		case 8: // DAY_FORMAT
			{
				TCHAR* tmpDate = STRDUP_TtoT(dateString);
				s_time.tm_mday = _ttoi(&tmpDate[6]);
				tmpDate[6] = 0;
				s_time.tm_mon = _ttoi(&tmpDate[4]) - 1;
				tmpDate[4] = 0;
				s_time.tm_year = _ttoi( tmpDate ) - 1900;
				delete[] tmpDate;
				break;
			}
		case 10: // HOUR_FORMAT
			{
				TCHAR* tmpDate = STRDUP_TtoT(dateString);
				s_time.tm_hour = _ttoi(&tmpDate[8]);
				tmpDate[8] = 0;
				s_time.tm_mday = _ttoi(&tmpDate[6]);
				tmpDate[6] = 0;
				s_time.tm_mon = _ttoi(&tmpDate[4]) - 1;
				tmpDate[4] = 0;
				s_time.tm_year = _ttoi( tmpDate ) - 1900;
				delete[] tmpDate;
				break;
			}
		case 12: // MINUTE_FORMAT
			{
				TCHAR* tmpDate = STRDUP_TtoT(dateString);
				s_time.tm_min = _ttoi(&tmpDate[10]);
				tmpDate[10] = 0;
				s_time.tm_hour = _ttoi(&tmpDate[8]);
				tmpDate[8] = 0;
				s_time.tm_mday = _ttoi(&tmpDate[6]);
				tmpDate[6] = 0;
				s_time.tm_mon = _ttoi(&tmpDate[4]) - 1;
				tmpDate[4] = 0;
				s_time.tm_year = _ttoi( tmpDate ) - 1900;
				delete[] tmpDate;
				break;
			}
		case 14: // SECOND_FORMAT
			{
				TCHAR* tmpDate = STRDUP_TtoT(dateString);
				s_time.tm_sec = _ttoi(&tmpDate[12]);
				tmpDate[12] = 0;
				s_time.tm_min = _ttoi(&tmpDate[10]);
				tmpDate[10] = 0;
				s_time.tm_hour = _ttoi(&tmpDate[8]);
				tmpDate[8] = 0;
				s_time.tm_mday = _ttoi(&tmpDate[6]);
				tmpDate[6] = 0;
				s_time.tm_mon = _ttoi(&tmpDate[4]) - 1;
				tmpDate[4] = 0;
				s_time.tm_year = _ttoi( tmpDate ) - 1900;
				delete[] tmpDate;
				break;
			}
		case 17: // MILLISECOND_FORMAT
			{
				TCHAR* tmpDate = STRDUP_TtoT(dateString);
				ms = _ttoi(&tmpDate[14]);
				tmpDate[14] = 0;
				s_time.tm_sec = _ttoi(&tmpDate[12]);
				tmpDate[12] = 0;
				s_time.tm_min = _ttoi(&tmpDate[10]);
				tmpDate[10] = 0;
				s_time.tm_hour = _ttoi(&tmpDate[8]);
				tmpDate[8] = 0;
				s_time.tm_mday = _ttoi(&tmpDate[6]);
				tmpDate[6] = 0;
				s_time.tm_mon = _ttoi(&tmpDate[4]) - 1;
				tmpDate[4] = 0;
				s_time.tm_year = _ttoi( tmpDate ) - 1900;
				delete[] tmpDate;
				break;
			}
		default:
			{
				_CLTHROWA(CL_ERR_Parse, "Input is not valid date string");
				break;
			}
	}

	time_t t = mktime(&s_time);
	if (t == -1)
		_CLTHROWA(CL_ERR_Parse, "Input is not valid date string");

	ret = (t * 1000) + ms;

	return ret;
} 

DateTools::~DateTools() {};

CL_NS_END
