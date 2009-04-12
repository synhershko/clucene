/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "MultiLevelSkipListReader.h"

CL_NS_USE(store)
CL_NS_DEF(index)

MultiLevelSkipListReader::MultiLevelSkipListReader(IndexInput* _skipStream, const int32_t maxSkipLevels,
												   const int32_t _skipInterval):
		numberOfLevelsToBuffer(1),skipStream(NULL),skipPointer(NULL),skipInterval(NULL),
		numSkipped(NULL),skipDoc(_CL_NEWARRAY(int32_t,maxSkipLevels)),childPointer(NULL)
{
	this->skipStream = _CL_NEWARRAY(IndexInput*,maxSkipLevels);
	this->skipPointer = _CL_NEWARRAY(int64_t,maxSkipLevels);
	this->childPointer = _CL_NEWARRAY(int64_t,maxSkipLevels);
	this->numSkipped = _CL_NEWARRAY(int32_t,maxSkipLevels);
	this->maxNumberOfSkipLevels = maxSkipLevels;
	this->skipInterval = _CL_NEWARRAY(int32_t,maxSkipLevels);
	this->skipStream[0] = _skipStream;
	this->inputIsBuffered = (strcmp(_skipStream->getObjectName(),"BufferedIndexInput") == 0);
	this->skipInterval[0] = _skipInterval;
	for (int32_t i = 1; i < maxSkipLevels; i++) {
		// cache skip intervals
		this->skipInterval[i] = this->skipInterval[i - 1] * _skipInterval;
	}
	memset(skipDoc,0,maxSkipLevels*sizeof(int32_t)); // TODO: artificial init
}
MultiLevelSkipListReader::~MultiLevelSkipListReader(){
	close();
	_CLDELETE_LARRAY(skipStream);
	_CLDELETE_LARRAY(skipPointer);
	_CLDELETE_LARRAY(childPointer);
	_CLDELETE_LARRAY(numSkipped);
	_CLDELETE_LARRAY(skipInterval);
	_CLDELETE_LARRAY(skipDoc);
}

int32_t MultiLevelSkipListReader::getDoc() const {
	return lastDoc;
}

int32_t MultiLevelSkipListReader::skipTo(const int32_t target) {
	if (!haveSkipped) {
		// first time, load skip levels
		loadSkipLevels();
		haveSkipped = true;
	}

	// walk up the levels until highest level is found that has a skip
	// for this target
	int32_t level = 0;
	while (level < numberOfSkipLevels - 1 && target > skipDoc[level + 1]) {
		level++;
	}    

	while (level >= 0) {
		if (target > skipDoc[level]) {
			if (!loadNextSkip(level)) {
				continue;
			}
		} else {
			// no more skips on this level, go down one level
			if (level > 0 && lastChildPointer > skipStream[level - 1]->getFilePointer()) {
				seekChild(level - 1);
			} 
			level--;
		}
	}

	return numSkipped[0] - skipInterval[0] - 1;
}

bool MultiLevelSkipListReader::loadNextSkip(const int32_t level) {
	// we have to skip, the target document is greater than the current
	// skip list entry        
	setLastSkipData(level);

	numSkipped[level] += skipInterval[level];

	if (numSkipped[level] > docCount) {
		// this skip list is exhausted
		skipDoc[level] = LUCENE_INT32_MAX_SHOULDBE;
		if (numberOfSkipLevels > level) numberOfSkipLevels = level; 
		return false;
	}

	// read next skip entry
	skipDoc[level] += readSkipData(level, skipStream[level]);

	if (level != 0) {
		// read the child pointer if we are not on the leaf level
		childPointer[level] = skipStream[level]->readVLong() + skipPointer[level - 1];
	}
	return true;
}

void MultiLevelSkipListReader::seekChild(const int32_t level) {
	skipStream[level]->seek(lastChildPointer);
	numSkipped[level] = numSkipped[level + 1] - skipInterval[level + 1];
	skipDoc[level] = lastDoc;
	if (level > 0) {
		childPointer[level] = skipStream[level]->readVLong() + skipPointer[level - 1];
	}
}

void MultiLevelSkipListReader::close() {
	for (int32_t i = 1; i < maxNumberOfSkipLevels; i++) {
		if (skipStream[i] != NULL) {
			//skipStream[i]->close();
			_CLLDELETE(skipStream[i]);
		}
	}
}

void MultiLevelSkipListReader::init(const int64_t _skipPointer, const int32_t df) {
	this->skipPointer[0] = _skipPointer;
	this->docCount = df;
	for (int32_t j=0; j<numberOfSkipLevels; j++){
		skipDoc[j] = 0;
		numSkipped[j] = 0;
		childPointer[j] = 0;
	}

	haveSkipped = false;
	for (int32_t i = 1; i < numberOfSkipLevels; i++) {
		_CLDELETE(skipStream[i]);
	}
}

void MultiLevelSkipListReader::loadSkipLevels() {
	numberOfSkipLevels = (docCount == 0) ? 0 : (int32_t)floor(log((double)docCount) / log((double)skipInterval[0]));
	if (numberOfSkipLevels > maxNumberOfSkipLevels) {
		numberOfSkipLevels = maxNumberOfSkipLevels;
	}

	skipStream[0]->seek(skipPointer[0]);

	int32_t toBuffer = numberOfLevelsToBuffer;

	for (int32_t i = numberOfSkipLevels - 1; i > 0; i--) {
		// the length of the current level
		int64_t length = skipStream[0]->readVLong();

		// the start pointer of the current level
		skipPointer[i] = skipStream[0]->getFilePointer();
		if (toBuffer > 0) {
			// buffer this level
			skipStream[i] = static_cast<IndexInput*>(_CLNEW SkipBuffer(skipStream[0], (int32_t) length));
			toBuffer--;
		} else {
			// clone this stream, it is already at the start of the current level
			skipStream[i] = (IndexInput*) skipStream[0]->clone();
			if (inputIsBuffered && length < BufferedIndexInput::BUFFER_SIZE) {
				((BufferedIndexInput*) skipStream[i])->setBufferSize((int32_t) length);
			}

			// move base stream beyond the current level
			skipStream[0]->seek(skipStream[0]->getFilePointer() + length);
		}
	}

	// use base stream for the lowest level
	skipPointer[0] = skipStream[0]->getFilePointer();
}

void MultiLevelSkipListReader::setLastSkipData(const int32_t level) {
	lastDoc = skipDoc[level];
	lastChildPointer = childPointer[level];
}

MultiLevelSkipListReader::SkipBuffer::SkipBuffer(IndexInput* input, const int32_t _length):pos(0)
{
	data = _CL_NEWARRAY(uint8_t,_length);
	this->_datalength = _length;
	pointer = input->getFilePointer();
	input->readBytes(data, _length);
}
MultiLevelSkipListReader::SkipBuffer::~SkipBuffer()
{
	_CLLDELETE(data);
}

void MultiLevelSkipListReader::SkipBuffer::close() {
	_CLDELETE(data);
	_datalength=0;
}

int64_t MultiLevelSkipListReader::SkipBuffer::getFilePointer() const {
	return pointer + pos;
}

int64_t MultiLevelSkipListReader::SkipBuffer::length() const {
	return _datalength;
}

uint8_t MultiLevelSkipListReader::SkipBuffer::readByte() {
	return data[pos++];
}

void MultiLevelSkipListReader::SkipBuffer::readBytes(uint8_t* b, const int32_t len) {
	memcpy(b,data+pos,len);
	pos += len;
}

void MultiLevelSkipListReader::SkipBuffer::seek(const int64_t _pos) {
	this->pos = (int32_t) (_pos - pointer);
}

const char* MultiLevelSkipListReader::SkipBuffer::getObjectName(){ return "SkipBuffer"; }
const char* MultiLevelSkipListReader::SkipBuffer::getDirectoryType() const{ return "SKIP"; }
MultiLevelSkipListReader::SkipBuffer::SkipBuffer(const SkipBuffer& other): IndexInput(other){
	data = _CL_NEWARRAY(uint8_t,other._datalength);
	memcpy(data,other.data,other._datalength * sizeof(uint8_t));
	this->_datalength = other._datalength;
	this->pointer = other.pointer;
	this->pos = other.pos;
}
IndexInput* MultiLevelSkipListReader::SkipBuffer::clone() const{
	return _CLNEW SkipBuffer(*this);
}

CL_NS_END