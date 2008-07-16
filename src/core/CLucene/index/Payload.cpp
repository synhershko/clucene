/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "Payload.h"

CL_NS_USE(util)
CL_NS_DEF(index)

Payload::Payload() : data(NULL), dataLen(0), offset(0), length(0) {
  // nothing to do
}
Payload::~Payload() { 
    _CLDELETE_LARRAY(data); 
}

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

void Payload::setData(uint8_t* _data, const int32_t _dataLen) {
	setData(_data, _dataLen, 0, _dataLen);
}
void Payload::setData(uint8_t* _data, const int32_t _dataLen, const int32_t _offset, const int32_t _length) {
	_CLDELETE_ARRAY(this->data);
	this->data = _data;
	this->dataLen = _dataLen;
	this->offset = _offset;
	this->length = _length;
}

uint8_t Payload::byteAt(int index) const {
	if (0 <= index && index < this->length) {
		return this->data[this->offset + index];    
	}
	_CLTHROWA(CL_ERR_IndexOutOfBounds,"Array index out of bounds at Payload::byteAt");
}

uint8_t* Payload::toByteArray(int32_t &_dataLen) {
	uint8_t* retArray = _CL_NEWARRAY(uint8_t,this->length);
	memcpy((void*)retArray, (void*)(this->data + this->offset), this->length * sizeof(uint8_t));
	_dataLen = this->length;

	return retArray;
}

void Payload::copyTo(uint8_t* target, const int32_t targetLen, const int32_t targetOffset) {
	if (this->length > targetLen + targetOffset) {
		_CLTHROWA(CL_ERR_IndexOutOfBounds,"Array index out of bounds at Payload::byteAt");
	}
	memcpy((void*)(target + targetOffset), (void*)(this->data + this->offset), this->length * sizeof(uint8_t));
}

Payload* Payload::clone() {
	int32_t i;
	Payload* clone = _CLNEW Payload(this->toByteArray(i), this->length, 0, this->length);
	return clone;
}

CL_NS_END
