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
#include "CLucene/document/Field.h"
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

		/*
		if (docStoreOffset != -1) {
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
		}*/

		_size = (int32_t)indexStream->length()/8; //todo: remove when uncommenting block above

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

bool FieldsReader::doc(int32_t n, Document* doc) {
    if ( n * 8L > indexStream->length() )
        return false;
	indexStream->seek(n * 8L);
	int64_t position = indexStream->readLong();
	fieldsStream->seek(position);
    
	int32_t numFields = fieldsStream->readVInt();
	for (int32_t i = 0; i < numFields; i++) {
		int32_t fieldNumber = fieldsStream->readVInt();
		FieldInfo* fi = fieldInfos->fieldInfo(fieldNumber);

        if ( fi == NULL )
            _CLTHROWA(CL_ERR_IO, "Field stream is invalid");

		uint8_t bits = fieldsStream->readByte();
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

CL_NS_END
