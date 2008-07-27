#include "CLucene/_ApiHeader.h"
#include "TextFragment.h"
#include "CLucene/util/StringBuffer.h"

CL_NS_DEF2(search,highlight)
CL_NS_USE(util)

	TextFragment::TextFragment(int32_t textStartPos, int32_t fragNum)
	{
		_textStartPos = textStartPos;
		_textEndPos=0;
		_fragNum = fragNum;
	}
	TextFragment::~TextFragment(){
	}

	void TextFragment::setScore(float_t score)
	{
		_score=score;
	}

	float_t TextFragment::getScore() const
	{
		return _score;
	}

	/**
	 * @param frag2 Fragment to be merged into this one
	 */
	void TextFragment::merge(const TextFragment * frag2)
	{
		_textEndPos = frag2->_textEndPos;
		_score=cl_max(_score,frag2->_score);
	}
	/**
	 * @param fragment 
	 * @return true if this fragment follows the one passed
	 */
	bool TextFragment::follows(const TextFragment * fragment) const
	{
		return _textStartPos == fragment->_textEndPos;
	}

	/**
	 * @return the fragment sequence number
	 */
	int32_t TextFragment::getFragNum() const
	{
		return _fragNum;
	}

	/* Returns the marked-up text for this text fragment 
	 */
	TCHAR* TextFragment::toString(StringBuffer* buffer) {
		TCHAR* ret = _CL_NEWARRAY(TCHAR,_textEndPos-_textStartPos+1);
		_tcsncpy(ret,buffer->getBuffer()+_textStartPos,_textEndPos-_textStartPos);
		ret[_textEndPos-_textStartPos]=_T('\0');
		
		return ret;
	}

CL_NS_END2
