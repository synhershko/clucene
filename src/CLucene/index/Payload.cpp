/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/StdHeader.h"
#include "Payload.h"

CL_NS_USE(util)
CL_NS_DEF(index)

Payload::Payload(uint8_t* _data, const int32_t _dataLen, const int32_t _offset, int32_t _length)
{
	if (_length < 1) _length=_dataLen;

	if (_offset < 0 || _offset + _length > _dataLen) {
		_CLTHROWA(CL_ERR_IllegalArgument,"offset < 0 || offset + length > data.length");
	}
	_CLDELETE_ARRAY(this->data);
	this->data = _data;
	this->dataLen = _dataLen;
	this->offset = _offset;
	this->length = _length;
}

void Payload::setData(uint8_t* _data, const int32_t _dataLen, const int32_t _offset, const int32_t _length) {
	_CLDELETE_ARRAY(this->data);
	this->data = _data;
	this->dataLen = _dataLen;
	this->offset = _offset;
	this->length = _length;
}

CL_NS_END
