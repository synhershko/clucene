/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#include <assert.h>
////#include "CLucene/util/VoidMap.h"
#include "CLucene/util/Misc.h"
#include "CLucene/util/subinputstream.h"
#include "CLucene/store/Directory.h"
#include "CLucene/store/IndexInput.h"
#include "CLucene/document/Document.h"
#include "CLucene/document/_FieldSelector.h"
#include "_FieldInfos.h"
#include "_FieldsWriter.h"
#include "_FieldsReader.h"

CL_NS_USE(store)
CL_NS_USE(document)
CL_NS_USE(util)
CL_NS_DEF(index)


class FieldsReader::FieldsStreamHolder: public jstreams::StreamBase<char>{
    CL_NS(store)::IndexInput* indexInput;
    CL_NS(store)::IndexInputStream* indexInputStream;
    jstreams::SubInputStream<char>* subStream;
public:
    FieldsStreamHolder(CL_NS(store)::IndexInput* indexInput, int32_t subLength);
    ~FieldsStreamHolder();
    int32_t read(const char*& start, int32_t _min, int32_t _max);
    int64_t skip(int64_t ntoskip);
    int64_t reset(int64_t pos);
};

FieldsReader::FieldsReader(Directory* d, const char* segment, FieldInfos* fn, int32_t _readBufferSize, int32_t _docStoreOffset, int32_t size):
	fieldInfos(fn), closed(false)
{
//Func - Constructor
//Pre  - d contains a valid reference to a Directory
//       segment != NULL
//       fn contains a valid reference to a FieldInfos
//Post - The instance has been created

	CND_PRECONDITION(segment != NULL, "segment != NULL");

	bool success = false;

	try {
		const char* buf = Misc::segmentname(segment,".fdt");
		cloneableFieldsStream = NULL;
		fieldsStream = d->openInput( buf, _readBufferSize );
		//fieldsStream = cloneableFieldsStream->clone();
		_CLDELETE_LCaARRAY( buf );

		buf = Misc::segmentname(segment,".fdx");
		indexStream = d->openInput( buf, _readBufferSize );
		_CLDELETE_CaARRAY( buf );

		if (_docStoreOffset != -1) {
			// We read only a slice out of this shared fields file
			this->docStoreOffset = _docStoreOffset;
			this->_size = size;

			// Verify the file is long enough to hold all of our
			// docs
			CND_CONDITION(((int32_t) (indexStream.length() / 8)) >= size + this->docStoreOffset,
				"the file is not long enough to hold all of our docs");
		} else {
			this->docStoreOffset = 0;
			this->_size = (int32_t) (indexStream->length() >> 3);
		}

		//_size = (int32_t)indexStream->length()/8;

		numTotalDocs = (int32_t) (indexStream->length() >> 3);
		success = true;
	} _CLFINALLY ({
		// With lock-less commits, it's entirely possible (and
		// fine) to hit a FileNotFound exception above. In
		// this case, we want to explicitly close any subset
		// of things that were opened so that we don't have to
		// wait for a GC to do so.
		if (!success) {
			close();
		}
	});
}

FieldsReader::~FieldsReader(){
//Func - Destructor
//Pre  - true
//Post - The instance has been destroyed

	close();
}

void FieldsReader::ensureOpen() {
	if (closed) {
		_CLTHROWA(CL_ERR_IllegalState, "this FieldsReader is closed");
	}
}

void FieldsReader::close() {
	if (!closed) {
		if (fieldsStream){
			fieldsStream->close();
			_CLDELETE(fieldsStream);
		}
		if (cloneableFieldsStream){
			cloneableFieldsStream->close();
			_CLDELETE(cloneableFieldsStream);
		}
		if(indexStream){
			indexStream->close();
			_CLDELETE(indexStream);
		}
		/*
		CL_NS(store)::IndexInput* localFieldsStream = fieldsStreamTL.get();
		if (localFieldsStream != NULL) {
			localFieldsStream->close();
			fieldsStreamTL->set(NULL);
		}*/
		closed = true;
	}
}

int32_t FieldsReader::size() const{
	return _size;
}

bool FieldsReader::doc(int32_t n, Document* doc, CL_NS(document)::FieldSelector* fieldSelector) {
    if ( (n + docStoreOffset) * 8L > indexStream->length() )
        return false;
	indexStream->seek((n + docStoreOffset) * 8L);
	int64_t position = indexStream->readLong();
	fieldsStream->seek(position);
    
	int32_t numFields = fieldsStream->readVInt();
	for (int32_t i = 0; i < numFields; i++) {
		int32_t fieldNumber = fieldsStream->readVInt();
		FieldInfo* fi = fieldInfos->fieldInfo(fieldNumber);
        if ( fi == NULL ) _CLTHROWA(CL_ERR_IO, "Field stream is invalid");

		FieldSelectorResult acceptField = (fieldSelector == NULL) ? CL_NS(document)::LOAD : fieldSelector->accept(fi->name);

		uint8_t bits = fieldsStream->readByte();
		CND_CONDITION(bits <= FieldsWriter::FIELD_IS_COMPRESSED + FieldsWriter::FIELD_IS_TOKENIZED + FieldsWriter::FIELD_IS_BINARY,
			"invalid field bits");

		if ((bits & FieldsWriter::FIELD_IS_BINARY) != 0) {
			int32_t fieldLen = fieldsStream->readVInt();
            FieldsReader::FieldsStreamHolder* subStream = new FieldsReader::FieldsStreamHolder(fieldsStream, fieldLen);
			uint8_t bits = Field::STORE_YES;
			Field* f = _CLNEW Field(
				fi->name,     // name
                subStream, // read value
                bits);

          	doc->add(*f);

			//now skip over the rest of the field
			if ( fieldsStream->getFilePointer() + fieldLen == fieldsStream->length() ){
				fieldsStream->seek(fieldsStream->getFilePointer() + fieldLen - 1); //set to eof
				fieldsStream->readByte();
			}else
				fieldsStream->seek(fieldsStream->getFilePointer() + fieldLen);
		}else{
			uint8_t bits = Field::STORE_YES;
			
			if (fi->isIndexed && (bits & FieldsWriter::FIELD_IS_TOKENIZED)!=0 )
				bits |= Field::INDEX_TOKENIZED;
			else if (fi->isIndexed && (bits & FieldsWriter::FIELD_IS_TOKENIZED)==0 )
				bits |= Field::INDEX_UNTOKENIZED;
			else
				bits |= Field::INDEX_NO;
			
			if (fi->storeTermVector) {
				if (fi->storeOffsetWithTermVector) {
					if (fi->storePositionWithTermVector) {
						bits |= Field::TERMVECTOR_WITH_OFFSETS;
						bits |= Field::TERMVECTOR_WITH_POSITIONS;
					}else {
						bits |= Field::TERMVECTOR_WITH_OFFSETS;
					}
				}else if (fi->storePositionWithTermVector) {
					bits |= Field::TERMVECTOR_WITH_POSITIONS;
				}else {
					bits |= Field::TERMVECTOR_YES;
				}
			}else {
				bits |= Field::TERMVECTOR_NO;
			}
			if ( (bits & FieldsWriter::FIELD_IS_COMPRESSED) != 0 ) {
				bits |= Field::STORE_COMPRESS;
				int32_t fieldLen = fieldsStream->readVInt();
                FieldsStreamHolder* subStream = new FieldsStreamHolder(fieldsStream, fieldLen);

                //todo: we dont have gzip inputstream available, must alert user
                //to somehow use a gzip inputstream
                Field* f = _CLNEW Field(
					fi->name,     // name
	                subStream, // read value
	                bits);
	                
	            f->setOmitNorms(fi->omitNorms);
	          	doc->add(*f);
					
				//now skip over the rest of the field
				if ( fieldsStream->getFilePointer() + fieldLen == fieldsStream->length() ){
					fieldsStream->seek(fieldsStream->getFilePointer() + fieldLen - 1); //set to eof
					fieldsStream->readByte();
				}else
					fieldsStream->seek(fieldsStream->getFilePointer() + fieldLen);
	        }else {
				TCHAR* fvalue = fieldsStream->readString();
				Field* f = _CLNEW Field(
					fi->name,     // name
	                fvalue, // read value
	                bits);
				_CLDELETE_CARRAY(fvalue); //todo: could optimise this
	          	f->setOmitNorms(fi->omitNorms);
	          	doc->add(*f);
	        }
			
		}
	}
	return true;
}

CL_NS(store)::IndexInput* FieldsReader::rawDocs(int32_t* lengths, const int32_t startDocID, const int32_t numDocs) {
	indexStream->seek((docStoreOffset+startDocID) * 8L);
	int64_t startOffset = indexStream->readLong();
	int64_t lastOffset = startOffset;
	int32_t count = 0;
	while (count < numDocs) {
		int64_t offset;
		const int32_t docID = docStoreOffset + startDocID + count + 1;
		CND_CONDITION( docID <= numTotalDocs, "invalid docID");
		if (docID < numTotalDocs) 
			offset = indexStream->readLong();
		else
			offset = fieldsStream->length();
		lengths[count++] = static_cast<int32_t>(offset-lastOffset);
		lastOffset = offset;
	}

	fieldsStream->seek(startOffset);

	return fieldsStream;
}

void FieldsReader::skipField(const bool binary, const bool compressed) {
	skipField(binary, compressed, fieldsStream->readVInt());
}

void FieldsReader::skipField(const bool binary, const bool compressed, const int32_t toRead) {
	if (binary || compressed) {
		int64_t pointer = fieldsStream->getFilePointer();
		fieldsStream->seek(pointer + toRead);
	} else {
		//We need to skip chars.  This will slow us down, but still better
		fieldsStream->skipChars(toRead);
	}
}

void FieldsReader::addFieldLazy(CL_NS(document)::Document* doc, FieldInfo* fi, const bool binary,
								const bool compressed, const bool tokenize) {
	if (binary) {
		int32_t toRead = fieldsStream->readVInt();
		int64_t pointer = fieldsStream->getFilePointer();
		if (compressed) {
			//was: doc.add(new Fieldable(fi.name, uncompress(b), Fieldable.Store.COMPRESS));
			doc->add(*_CLNEW LazyField(this, fi->name, Field::STORE_COMPRESS, toRead, pointer));
		} else {
			//was: doc.add(new Fieldable(fi.name, b, Fieldable.Store.YES));
			doc->add(*_CLNEW LazyField(this, fi->name, Field::STORE_YES, toRead, pointer));
		}
		//Need to move the pointer ahead by toRead positions
		fieldsStream->seek(pointer + toRead);
	} else {
		//Field.Store store = Field.Store.YES;
		//Field.Index index = getIndexType(fi, tokenize);
		//Field.TermVector termVector = getTermVectorType(fi);

		LazyField* f = NULL;
		if (compressed) {
			int32_t toRead = fieldsStream->readVInt();
			int64_t pointer = fieldsStream->getFilePointer();
			f = _CLNEW LazyField(this, fi->name, Field::STORE_COMPRESS, toRead, pointer);
			//skip over the part that we aren't loading
			fieldsStream->seek(pointer + toRead);
			f->setOmitNorms(fi->omitNorms);
		} else {
			int32_t length = fieldsStream->readVInt();
			int64_t pointer = fieldsStream->getFilePointer();
			//Skip ahead of where we are by the length of what is stored
			fieldsStream->skipChars(length);
			f = _CLNEW LazyField(this, fi->name, Field::STORE_YES | getIndexType(fi, tokenize) | getTermVectorType(fi), length, pointer);
			f->setOmitNorms(fi->omitNorms);
		}
		doc->add(*f);
	}
}

/*
// Add the size of field as a byte[] containing the 4 bytes of the integer byte size (high order byte first; char = 2 bytes)
// Read just the size -- caller must skip the field content to continue reading fields
// Return the size in bytes or chars, depending on field type
int32_t FieldsReader::addFieldSize(const CL_NS(document)::Document* doc, const FieldInfo* fi, const bool binary, const bool compressed) {
	const int32_t size = fieldsStream->readVInt();
	const int32_t bytesize = binary || compressed ? size : 2*size;
	uint8_t* sizebytes = _CL_NEWARRAY(byte, 4);
	sizebytes[0] = (byte) (bytesize>>>24);
	sizebytes[1] = (byte) (bytesize>>>16);
	sizebytes[2] = (byte) (bytesize>>> 8);
	sizebytes[3] = (byte)  bytesize      ;
	doc->add(*_CLNEW Field(fi->name, sizebytes, Field::STORE_YES));
	return size;
}*/

CL_NS(document)::Field::TermVector FieldsReader::getTermVectorType(const FieldInfo* fi) {
	if (fi->storeTermVector) {
		if (fi->storeOffsetWithTermVector) {
			if (fi->storePositionWithTermVector) {
				return Field::TERMVECTOR_WITH_POSITIONS_OFFSETS;
			} else {
				return Field::TERMVECTOR_WITH_OFFSETS;
			}
		} else if (fi->storePositionWithTermVector) {
			return Field::TERMVECTOR_WITH_POSITIONS;
		} else {
			return Field::TERMVECTOR_YES;
		}
	} else {
		return Field::TERMVECTOR_NO ;
	}
}

CL_NS(document)::Field::Index FieldsReader::getIndexType(const FieldInfo* fi, const bool tokenize) {
	if (fi->isIndexed && tokenize)
		return Field::INDEX_TOKENIZED;
	else if (fi->isIndexed && !tokenize)
		return Field::INDEX_UNTOKENIZED;
	else
		return Field::INDEX_NO;
}

FieldsReader::FieldsStreamHolder::FieldsStreamHolder(IndexInput* indexInput, int32_t subLength){
    this->indexInput = indexInput->clone();
    this->indexInputStream = new IndexInputStream(this->indexInput);
    this->subStream = new jstreams::SubInputStream<char>(indexInputStream, subLength);

    this->size = subStream->getSize();
    this->position = subStream->getPosition();
    this->error = subStream->getError();
    this->status = subStream->getStatus();
}
FieldsReader::FieldsStreamHolder::~FieldsStreamHolder(){
    delete subStream;
    delete indexInputStream;

    indexInput->close();
    _CLDELETE(indexInput);
}
int32_t FieldsReader::FieldsStreamHolder::read(const char*& start, int32_t _min, int32_t _max){
    int32_t ret = subStream->read(start,_min,_max);
    this->position = subStream->getPosition();
    this->error = subStream->getError();
    this->status = subStream->getStatus();
    return ret;
}
int64_t FieldsReader::FieldsStreamHolder::skip(int64_t ntoskip){
    int64_t ret = subStream->skip(ntoskip);
    this->position = subStream->getPosition();
    this->error = subStream->getError();
    this->status = subStream->getStatus();
    return ret;
}
int64_t FieldsReader::FieldsStreamHolder::reset(int64_t pos){
    int64_t ret = subStream->reset(pos);
    this->position = subStream->getPosition();
    this->error = subStream->getError();
    this->status = subStream->getStatus();
    return ret;
}


FieldsReader::LazyField::LazyField(FieldsReader* _parent, const TCHAR* _name,
								   int config, const int32_t _toRead, const int64_t _pointer)
: Field(_name, config), parent(_parent) {
	// todo: need to allow for auto setting Field::INDEX_NO | Field::TERMVECTOR_NO so only Store is required
	this->toRead = _toRead;
	this->pointer = _pointer;
	lazy = true;
}

CL_NS(store)::IndexInput* FieldsReader::LazyField::getFieldStream() {
	CL_NS(store)::IndexInput* localFieldsStream = parent->fieldsStreamTL.get();
	if (localFieldsStream == NULL) {
		localFieldsStream = parent->cloneableFieldsStream->clone();
		parent->fieldsStreamTL.set(localFieldsStream);
	}
	return localFieldsStream;
}

uint8_t* FieldsReader::LazyField::binaryValue() {
	parent->ensureOpen();
	if (fieldsData == NULL) {
		uint8_t* b = _CL_NEWARRAY(uint8_t, toRead);
		CL_NS(store)::IndexInput* localFieldsStream = getFieldStream();

		localFieldsStream->seek(pointer);
		localFieldsStream->readBytes(b, toRead);
		if (isCompressed()) {
			//fieldsData = uncompress(b);
		} else {
			fieldsData = b;
		}
		valueType = VALUE_STREAM;
	}
	return static_cast<uint8_t*>(fieldsData); // instanceof byte[] ? (byte[]) fieldsData : null;
}

CL_NS(util)::Reader* FieldsReader::LazyField::readerValue() {
	parent->ensureOpen();
	return (valueType & VALUE_READER) ? static_cast<CL_NS(util)::Reader*>(fieldsData) : NULL;
}


CL_NS(analysis)::TokenStream* FieldsReader::LazyField::tokenStreamValue() {
	parent->ensureOpen();
	return (valueType & VALUE_TOKENSTREAM) ? static_cast<CL_NS(analysis)::TokenStream*>(fieldsData) : NULL;
}


/** The value of the field as a String, or null.  If null, the Reader value,
* binary value, or TokenStream value is used.  Exactly one of stringValue(), 
* readerValue(), binaryValue(), and tokenStreamValue() must be set. */
const TCHAR* FieldsReader::LazyField::stringValue() {
	parent->ensureOpen();
	if (fieldsData == NULL) {
		CL_NS(store)::IndexInput* localFieldsStream = getFieldStream();
		localFieldsStream->seek(pointer);
		if (isCompressed()) {
			uint8_t* b = _CL_NEWARRAY(uint8_t, toRead);
			localFieldsStream->readBytes(b, toRead);
			_resetValue();
			//fieldsData = new String(uncompress(b), "UTF-8");
		} else {
			//read in chars b/c we already know the length we need to read
			TCHAR* chars = _CL_NEWARRAY(TCHAR, toRead);
			localFieldsStream->readChars(chars, 0, toRead);
			_resetValue();
			fieldsData = chars;
		}
		valueType = VALUE_STRING;
	}
	return static_cast<const TCHAR*>(fieldsData); //instanceof String ? (String) fieldsData : null;
}

int64_t FieldsReader::LazyField::getPointer() const {
	parent->ensureOpen();
	return pointer;
}

void FieldsReader::LazyField::setPointer(const int64_t _pointer) {
	parent->ensureOpen();
	this->pointer = _pointer;
}

int32_t FieldsReader::LazyField::getToRead() const {
	parent->ensureOpen();
	return toRead;
}

void FieldsReader::LazyField::setToRead(const int32_t _toRead) {
	parent->ensureOpen();
	this->toRead = _toRead;
}

CL_NS_END
