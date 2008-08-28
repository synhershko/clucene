/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#include "_FieldSelector.h"

CL_NS_DEF(document)

FieldSelector::~FieldSelector(){
}

LoadFirstFieldSelector::~LoadFirstFieldSelector(){
}

FieldSelector::FieldSelectorResult LoadFirstFieldSelector::accept(const TCHAR* fieldName) {
	return LOAD_AND_BREAK;
}
CL_NS_END
