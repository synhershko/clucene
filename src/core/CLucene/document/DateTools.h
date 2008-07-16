/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_document_DateTools_
#define _lucene_document_DateTools_

CL_NS_DEF(document)


class CLUCENE_EXPORT DateTools :LUCENE_BASE {
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
	static TCHAR* timeToString(const int64_t time, Resolution resolution = MILLISECOND_FORMAT);

	static void timeToString(const int64_t time, Resolution resolution, TCHAR* buf, size_t bufLength);

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
	static int64_t stringToTime(const TCHAR* dateString);

	~DateTools();
	
};
CL_NS_END
#endif
