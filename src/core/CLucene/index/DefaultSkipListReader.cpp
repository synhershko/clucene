/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/

#include "CLucene/_ApiHeader.h"
#include "DefaultSkipListReader.h"

CL_NS_DEF(index)

DefaultSkipListReader::DefaultSkipListReader(CL_NS(store)::IndexInput* _skipStream, const int32_t maxSkipLevels, const int32_t _skipInterval)
		: MultiLevelSkipListReader(_skipStream, maxSkipLevels, _skipInterval)
{
	freqPointer = _CL_NEWARRAY(int64_t,maxSkipLevels);
	proxPointer = _CL_NEWARRAY(int64_t,maxSkipLevels);
	payloadLength = _CL_NEWARRAY(int32_t,maxSkipLevels);
}

DefaultSkipListReader::~DefaultSkipListReader(){
	_CLDELETE_LARRAY(freqPointer);
	_CLDELETE_LARRAY(proxPointer);
	_CLDELETE_LARRAY(payloadLength);
}

void DefaultSkipListReader::init(const int64_t _skipPointer, const int64_t freqBasePointer, const int64_t proxBasePointer, const int32_t df, const bool storesPayloads) {
	MultiLevelSkipListReader::init(_skipPointer, df);
	this->currentFieldStoresPayloads = storesPayloads;
	lastFreqPointer = freqBasePointer;
	lastProxPointer = proxBasePointer;

	for (int32_t j=0; j<numberOfSkipLevels; j++){
		freqPointer[j] = freqBasePointer;
		proxPointer[j] = proxBasePointer;
		payloadLength[j] = 0;
	}
}

int64_t DefaultSkipListReader::getFreqPointer() const {
	return lastFreqPointer;
}
int64_t DefaultSkipListReader::getProxPointer() const {
	return lastProxPointer;
}
int32_t DefaultSkipListReader::getPayloadLength() const {
	return lastPayloadLength;
}

void DefaultSkipListReader::seekChild(const int32_t level) {
	MultiLevelSkipListReader::seekChild(level);
	freqPointer[level] = lastFreqPointer;
	proxPointer[level] = lastProxPointer;
	payloadLength[level] = lastPayloadLength;
}

void DefaultSkipListReader::setLastSkipData(const int32_t level) {
	MultiLevelSkipListReader::setLastSkipData(level);
	lastFreqPointer = freqPointer[level];
	lastProxPointer = proxPointer[level];
	lastPayloadLength = payloadLength[level];
}

int32_t DefaultSkipListReader::readSkipData(const int32_t level, CL_NS(store)::IndexInput* _skipStream) {
	int32_t delta;
	if (currentFieldStoresPayloads) {
		// the current field stores payloads.
		// if the doc delta is odd then we have
		// to read the current payload length
		// because it differs from the length of the
		// previous payload
		delta = _skipStream->readVInt();
		if ((delta & 1) != 0) {
			payloadLength[level] = _skipStream->readVInt();
		}
		delta = (int32_t)(((uint32_t)delta) >> (uint32_t)1);
	} else {
		delta = _skipStream->readVInt();
	}
	freqPointer[level] += _skipStream->readVInt();
	proxPointer[level] += _skipStream->readVInt();

	return delta;
}

CL_NS_END
