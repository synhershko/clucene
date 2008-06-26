/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_index_Payload_
#define _lucene_index_Payload_

CL_NS_DEF(index)

/**
*  A Payload is metadata that can be stored together with each occurrence 
*  of a term. This metadata is stored inline in the posting list of the
*  specific term.  
*  <p>
*  To store payloads in the index a {@link TokenStream} has to be used that
*  produces {@link Token}s containing payload data.
*  <p>
*  Use {@link TermPositions#getPayloadLength()} and {@link TermPositions#getPayload(byte[], int)}
*  to retrieve the payloads from the index.<br>
*
*/
class Payload:LUCENE_REFBASE {
protected:
	/** the byte array containing the payload data */
    uint8_t*	data;
	int32_t		dataLen; // CLucene specific, to keep track of the data array length

	/** the offset within the byte array */
	int32_t		offset;

	/** the length of the payload data */
	int32_t		length;

public:

	/** Creates an empty payload and does not allocate a byte array. */
	Payload();

	/**
	* Creates a new payload with the the given array as data. 
	* A reference to the passed-in array is held, i. e. no 
	* copy is made.
	* 
	* @param _data the data of this payload
	* @param _dataLen the length of _data
	* @param _offset the offset in the data byte array
	* @param _length the length of the data
	*/
	Payload(uint8_t* _data, const int32_t _dataLen, const int32_t _offset=0, int32_t _length=0);

	/* Desctructor - auto-delete the data container */
	~Payload();

	/**
	* Sets this payloads data. 
	* A reference to the passed-in array is held, i. e. no 
	* copy is made.
	*/
	void setData(uint8_t* _data, const int32_t _dataLen);

	/**
	* Sets this payloads data. 
	* A reference to the passed-in array is held, i. e. no 
	* copy is made.
	*/
	void setData(uint8_t* _data, const int32_t _dataLen, const int32_t _offset, const int32_t _length);

	/**
	* Returns a reference to the underlying byte array
	* that holds this payloads data.
	*/
    uint8_t* getData(int32_t &_dataLen) { _dataLen = dataLen; return data; }

    /**
     * Returns the offset in the underlying byte array 
     */
	int getOffset() const { return offset; };

    /**
     * Returns the length of the payload data. 
     */
    int getLength() const { return length; }

    /**
     * Returns the byte at the given index.
     */
	uint8_t byteAt(int index) const;

	/**
	* Allocates a new byte array, copies the payload data into it and returns it. Caller is responsible
	* for deleting it later.
	*/
	uint8_t* toByteArray(int32_t &_dataLen);

	/**
	* Copies the payload data to a byte array.
	* 
	* @param target the target byte array
	* @param targetOffset the offset in the target byte array
	*/
	void copyTo(uint8_t* target, const int32_t targetLen, const int32_t targetOffset);

	/**
	* Clones this payload by creating a copy of the underlying
	* byte array.
	*/
	Payload* clone();

};

CL_NS_END
#endif
