/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_index_DefaultSkipListReader_
#define _lucene_index_DefaultSkipListReader_

#include "MultiLevelSkipListReader.h"

CL_NS_DEF(index)

/**
 * Implements the skip list reader for the default posting list format
 * that stores positions and payloads.
 *
 */
class DefaultSkipListReader: public MultiLevelSkipListReader {
private:
	bool currentFieldStoresPayloads;
	int64_t* freqPointer;
	int64_t* proxPointer;
	int32_t* payloadLength;
  
	int64_t lastFreqPointer;
	int64_t lastProxPointer;
	int32_t lastPayloadLength;
                           
public:
	DefaultSkipListReader(CL_NS(store)::IndexInput* _skipStream, const int32_t maxSkipLevels, const int32_t _skipInterval);
	virtual ~DefaultSkipListReader();

	void init(const int64_t _skipPointer, const int64_t freqBasePointer, const int64_t proxBasePointer, const int32_t df, const bool storesPayloads);

	/** Returns the freq pointer of the doc to which the last call of 
	* {@link MultiLevelSkipListReader#skipTo(int)} has skipped.  */
	int64_t getFreqPointer() const;

	/** Returns the prox pointer of the doc to which the last call of 
	* {@link MultiLevelSkipListReader#skipTo(int)} has skipped.  */
	int64_t getProxPointer() const;

	/** Returns the payload length of the payload stored just before 
	* the doc to which the last call of {@link MultiLevelSkipListReader#skipTo(int)} 
	* has skipped.  */
	int32_t getPayloadLength() const;

protected:
	void seekChild(const int32_t level);
  
	void setLastSkipData(const int32_t level);

	int32_t readSkipData(const int32_t level, CL_NS(store)::IndexInput* _skipStream);
};
CL_NS_END
#endif
