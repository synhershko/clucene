#include "CLucene/_ApiHeader.h"
#include "SimpleHTMLFormatter.h"
#include "TokenGroup.h"
#include "CLucene/util/StringBuffer.h"


CL_NS_DEF2(search,highlight)
CL_NS_USE(util)

SimpleHTMLFormatter::SimpleHTMLFormatter(const TCHAR* preTag, const TCHAR* postTag):
	_preTag(stringDuplicate(preTag)),
	_postTag(stringDuplicate(postTag))
{
}

SimpleHTMLFormatter::SimpleHTMLFormatter()
{
	_preTag = stringDuplicate(_T("<B>"));
	_postTag = stringDuplicate(_T("</B>"));
}

SimpleHTMLFormatter::~SimpleHTMLFormatter() 
{
	_CLDELETE_CARRAY(_preTag);
	_CLDELETE_CARRAY(_postTag);
}

TCHAR* SimpleHTMLFormatter::highlightTerm(const TCHAR* originalText, const TokenGroup* tokenGroup)
{
	if(tokenGroup->getTotalScore()>0){
		StringBuffer sb;
		sb.append(_preTag);
		sb.append(originalText);
		sb.append(_postTag);
		return sb.toString();
	}
	return stringDuplicate(originalText);
}

CL_NS_END2
