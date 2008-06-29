/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_document_DateTools_
#define _lucene_document_DateTools_

#if defined(_LUCENE_PRAGMA_ONCE)
# pragma once
#endif

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

CL_NS_DEF(document)

#define DATETOOLS_BUFFER_SIZE 20

class DateTools :LUCENE_BASE {
public:

	enum Resolution {
		YEAR_FORMAT,		// yyyy
		MONTH_FORMAT,		// yyyyMM
		DAY_FORMAT,			// yyyyMMdd
		HOUR_FORMAT,		// yyyyMMddHH
		MINUTE_FORMAT,		// yyyyMMddHHmm
		SECOND_FORMAT,		// yyyyMMddHHmmss
		MILLISECOND_FORMAT	// yyyyMMddHHmmssSSS
	};
	
	/**
	* Converts a millisecond time to a string suitable for indexing.
	* 
	* @param time the date expressed as milliseconds since January 1, 1970, 00:00:00 GMT
	* @param resolution the desired resolution, see {@link #Resolution}
	* @return a string in format <code>yyyyMMddHHmmssSSS</code> or shorter,
	*  depeding on <code>resolution</code>; using UTC as timezone
	*/
	static TCHAR* timeToString(const int64_t time, Resolution resolution = MILLISECOND_FORMAT) {
		TCHAR* buf = _CL_NEWARRAY(TCHAR, DATETOOLS_BUFFER_SIZE);
		timeToString(time, resolution, buf);
		return buf;
	}

	static void timeToString(const int64_t time, Resolution resolution, TCHAR* buf) {
		time_t secs = time / 1000;
		tm *ptm = gmtime(&secs);
		size_t len = _tcsftime(buf, DATETOOLS_BUFFER_SIZE, _T("%Y%m%d%H%M%S"), ptm);

		if (resolution == MILLISECOND_FORMAT) {
			size_t len = _tcsftime(buf, DATETOOLS_BUFFER_SIZE, _T("%Y%m%d%H%M%S"), ptm);
			uint32_t ms = time % 1000;
			_stprintf(buf + len , _T("%03u"), ms);
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

	/**
	* Converts a string produced by <code>timeToString</code> or
	* <code>dateToString</code> back to a time, represented as the
	* number of milliseconds since January 1, 1970, 00:00:00 GMT.
	* 
	* @param dateString the date string to be converted
	* @return the number of milliseconds since January 1, 1970, 00:00:00 GMT
	* @throws ParseException if <code>dateString</code> is not in the 
	*  expected format 
	*/
	static int64_t stringToTime(const TCHAR* dateString) {
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
				TCHAR* tmpDate = _tcsdup(dateString);
				s_time.tm_mon = _ttoi(&tmpDate[4]) - 1;
				tmpDate[4] = 0;
				s_time.tm_year = _ttoi( tmpDate ) - 1900;
				delete[] tmpDate;
				break;
			}
		case 8: // DAY_FORMAT
			{
				TCHAR* tmpDate = _tcsdup(dateString);
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
				TCHAR* tmpDate = _tcsdup(dateString);
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
				TCHAR* tmpDate = _tcsdup(dateString);
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
				TCHAR* tmpDate = _tcsdup(dateString);
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
				TCHAR* tmpDate = _tcsdup(dateString);
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

	~DateTools() {};
	
};
CL_NS_END
#endif
