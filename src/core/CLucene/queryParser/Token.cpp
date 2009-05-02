/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "Token.h"

CL_NS_DEF(queryParser)


Token::Token() : image(NULL),kind(0),beginLine(0),beginColumn(0),endLine(0),endColumn(0),next(NULL),specialToken(NULL)
{
}

Token::~Token()
{
#ifndef LUCENE_TOKEN_WORD_LENGTH
	_CLDELETE_LCARRAY( image );
#endif
}

TCHAR* Token::toString() const
{
#ifndef LUCENE_TOKEN_WORD_LENGTH
	return image;
#else
	return STRDUP_TtoT(image);
#endif
}

/*static*/
Token* Token::newToken(const int32_t ofKind)
{
	//switch(ofKind)
	//{
	//default : return _CLNEW Token();
	//}
	return _CLNEW Token();
}

CL_NS_END
