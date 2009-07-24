/*------------------------------------------------------------------------------
* Copyright (C) 2009 Isidor Zeuner
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "byteinputstream.h"

CL_NS_DEF(util)

ByteInputStream::ByteInputStream(ArrayBase<signed char> const* data) :
data(data),
current_position(0) {
}

int32_t ByteInputStream::read(const signed char*& start, int32_t min, int32_t max) {
	int32_t to_read = min;
	int32_t readable = data->length - current_position;
	if (readable < to_read) {
		to_read = readable;
	}
	start = data->values + current_position;
	current_position += to_read;
	return to_read;
}
	
int64_t ByteInputStream::skip(int64_t ntoskip) {
	int64_t to_skip = ntoskip;
	int64_t skippable = data->length - current_position;
	if (skippable < to_skip) {
		to_skip = skippable;
	}
	current_position += to_skip;
	return to_skip;
}

int64_t ByteInputStream::position() {
	return current_position;
}
	
size_t ByteInputStream::size() {
	return data->length;
}
CL_NS_END
