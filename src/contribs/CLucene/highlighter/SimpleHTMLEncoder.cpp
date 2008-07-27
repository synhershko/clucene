#include "CLucene/_ApiHeader.h"
#include "SimpleHTMLEncoder.h"
#include "Formatter.h"
#include "CLucene/util/StringBuffer.h"

CL_NS_DEF2(search,highlight)

SimpleHTMLEncoder::SimpleHTMLEncoder(void)
{
}

SimpleHTMLEncoder::~SimpleHTMLEncoder(void)
{
}

TCHAR* SimpleHTMLEncoder::encodeText(TCHAR* originalText)
{
	return htmlEncode(originalText);
}

TCHAR* SimpleHTMLEncoder::htmlEncode(TCHAR* plainText) 
{
	size_t plainTextLen = _tcslen(plainText);
	if (plainText == NULL || plainTextLen == 0)
	{
		return STRDUP_TtoT(_T(""));
	}

	CL_NS(util)::StringBuffer result(plainTextLen);

	for (int32_t index=0; index<plainTextLen; index++) 
	{
		TCHAR ch = plainText[index];

		switch (ch) 
		{
		case '"':
			result.append(_T("&quot;"));
			break;

		case '&':
			result.append(_T("&amp;"));
			break;

		case '<':
			result.append(_T("&lt;"));
			break;

		case '>':
			result.append(_T("&gt;"));
			break;

		default:
			if (ch < 128)
				result.appendChar(ch);
			else{
  	            result.append(_T("&#"));
				result.appendInt(ch);
				result.append(_T(";"));
			}
		}
	}

	return result.toString();
}

CL_NS_END2
