/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "MultipleTermPositions.h"

#include "IndexReader.h"

#include "CLucene/util/Array.h"
#include "CLucene/util/PriorityQueue.h"

CL_NS_USE(util)

CL_NS_DEF(index)

class MultipleTermPositions::TermPositionsQueue : public CL_NS(util)::PriorityQueue<TermPositions*,
	CL_NS(util)::Deletor::Object<TermPositions> > {
public:
		TermPositionsQueue(TermPositions** termPositions, size_t termPositionsSize) {
			initialize(termPositionsSize, false);

			size_t i=0;
			while (termPositions[i]!=NULL) {
				if (termPositions[i]->next())
					put(termPositions[i]);
				++i;
			}
		}
		virtual ~TermPositionsQueue(){
		}

		TermPositions* peek() {
			return top();
		}

		bool lessThan(TermPositions* a, TermPositions* b) {
			return a->doc() < b->doc();
		}
};

class MultipleTermPositions::IntQueue {
private:
	ValueArray<int32_t>* _array;
	int32_t _index;
	int32_t _lastIndex;

public:
	IntQueue():_array(_CLNEW ValueArray<int32_t>(16)), _index(0), _lastIndex(0){
	}
	virtual ~IntQueue(){
		_CLLDELETE(_array);
	}

	void add(const int32_t i) {
		if (_lastIndex == _array->length)
			_array->resize(_array->length*2);

		_array->values[_lastIndex++] = i;
	}

	int32_t next() {
		return _array->values[_index++];
	}

	void sort() {
		//TODO: Arrays.sort(_array, _index, _lastIndex);
	}

	void clear() {
		_index = 0;
		_lastIndex = 0;
	}

	int32_t size() {
		return (_lastIndex - _index);
	}
};

MultipleTermPositions::MultipleTermPositions(IndexReader* indexReader, Term** terms) : _posList(_CLNEW IntQueue()){
	CLLinkedList<TermPositions*> termPositions;
	size_t i = 0;
	while (terms[i] != NULL) {
		termPositions.push_back(indexReader->termPositions(terms[i]));
		++i;
	}

	TermPositions** tps = _CL_NEWARRAY(TermPositions*, i+1); // i == tpsSize
	termPositions.toArray(tps);

	_termPositionsQueue = _CLNEW TermPositionsQueue(tps,i);
}

bool MultipleTermPositions::next() {
	if (_termPositionsQueue->size() == 0)
		return false;

	_posList->clear();
	_doc = _termPositionsQueue->peek()->doc();

	TermPositions* tp;
	do {
		tp = _termPositionsQueue->peek();

		for (int32_t i = 0; i < tp->freq(); i++)
			_posList->add(tp->nextPosition());

		if (tp->next())
			_termPositionsQueue->adjustTop();
		else {
			_termPositionsQueue->pop();
			tp->close();
		}
	} while (_termPositionsQueue->size() > 0 && _termPositionsQueue->peek()->doc() == _doc);

	_posList->sort();
	_freq = _posList->size();

	return true;
}

int32_t MultipleTermPositions::nextPosition() {
	return _posList->next();
}

bool MultipleTermPositions::skipTo(int32_t target) {
	while (_termPositionsQueue->peek() != NULL && target > _termPositionsQueue->peek()->doc()) {
		TermPositions* tp = _termPositionsQueue->pop();
		if (tp->skipTo(target))
			_termPositionsQueue->put(tp);
		else
			tp->close();
	}
	return next();
}

int32_t MultipleTermPositions::doc() const {
	return _doc;
}

int32_t MultipleTermPositions::freq() const {
	return _freq;
}

void MultipleTermPositions::close() {
	while (_termPositionsQueue->size() > 0)
		_termPositionsQueue->pop()->close();
}

CL_NS_END
