/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "Field.h"
#include "CLucene/util/_StringIntern.h"
#include "CLucene/util/StringBuffer.h"
#include "CLucene/util/Reader.h"

CL_NS_USE(util)
CL_NS_DEF(document) 

struct Field::Internal{
	const TCHAR* _name;
	TCHAR* _stringValue;
	CL_NS(util)::Reader* _readerValue;
    jstreams::StreamBase<char>* _streamValue;

	uint32_t config;
	float_t boost;
};

Field::Field(const TCHAR* Name, Reader* reader, int config):
	internal(new Internal)
{
	CND_PRECONDITION(Name != NULL, "Name cannot be NULL");
	CND_PRECONDITION(reader != NULL, "reader cannot be NULL");

	internal->_name        = CLStringIntern::intern( Name );
	internal->_stringValue = NULL;
	internal->_readerValue = reader;
	internal->_streamValue = NULL;
	internal->boost=1.0f;

	setConfig(config);
}


Field::Field(const TCHAR* Name, const TCHAR* Value, int _config):
	internal(new Internal)
{
	CND_PRECONDITION(Name != NULL, "Name cannot be NULL");
	CND_PRECONDITION(Value != NULL, "value cannot be NULL");
	CND_PRECONDITION(_tcslen(Value)>0 && _tcslen(Name)>0, "name and value cannot both be empty");

	/*
	if (_config & INDEX_NO && _config & STORE_NO)
		_CLTHROWA(CL_ERR_IllegalArgument,"it doesn't make sense to have a field that is neither indexed nor stored");
	if (_config & INDEX_NO && _config & TERMVECTOR_YES)
		_CLTHROWA(CL_ERR_IllegalArgument,"cannot store term vector information for a field that is not indexed");
	*/

	internal->_name        = CLStringIntern::intern( Name );
	internal->_stringValue = stringDuplicate( Value );
	internal->_readerValue = NULL;
	internal->_streamValue = NULL;
	internal->boost=1.0f;

	//config = INDEX_TOKENIZED; // default Field is tokenized and indexed

	setConfig(_config);
}

Field::Field(const TCHAR* Name, jstreams::StreamBase<char>* Value, int config):
	internal(new Internal)
{
	CND_PRECONDITION(Name != NULL, "Name cannot be NULL");
	CND_PRECONDITION(Value != NULL, "value cannot be NULL");

	internal->_name        = CLStringIntern::intern( Name );
	internal->_stringValue = NULL;
	internal->_readerValue = NULL;
	internal->_streamValue = Value;
	internal->boost=1.0f;

	setConfig(config);
}

Field::~Field(){
//Func - Destructor
//Pre  - true
//Post - Instance has been destroyed

	CLStringIntern::unintern(internal->_name);
	_resetValue();
	delete internal;
}


/*===============FIELDS=======================*/
const TCHAR* Field::name() const	{ return internal->_name; } ///<returns reference
TCHAR* Field::stringValue() const	{ return internal->_stringValue; } ///<returns reference
Reader* Field::readerValue() const	{ return internal->_readerValue; } ///<returns reference
jstreams::StreamBase<char>* Field::streamValue() const	{ return internal->_streamValue; } ///<returns reference
CL_NS(analysis)::TokenStream* Field::tokenStreamValue() const { return NULL; }
	    
bool	Field::isStored() const 	{ return (internal->config & STORE_YES) != 0; }
bool 	Field::isIndexed() const	{ return (internal->config & INDEX_TOKENIZED)!=0 || (internal->config & INDEX_UNTOKENIZED)!=0; }
bool 	Field::isTokenized() const	{ return (internal->config & INDEX_TOKENIZED) != 0; }
bool 	Field::isCompressed() const	{ return (internal->config & STORE_COMPRESS) != 0; }
bool 	Field::isBinary() const		{ return internal->_streamValue!=NULL; }

bool	Field::isTermVectorStored() const			{ return (internal->config & TERMVECTOR_YES) != 0; }
bool	Field::isStoreOffsetWithTermVector() const	{ return (internal->config & TERMVECTOR_YES) != 0 && (internal->config & TERMVECTOR_WITH_OFFSETS) != 0; }
bool	Field::isStorePositionWithTermVector() const{ return (internal->config & TERMVECTOR_YES) != 0 && (internal->config & TERMVECTOR_WITH_POSITIONS) != 0; }

bool Field::getOmitNorms() const { return (internal->config & INDEX_NONORMS) != 0; }
void Field::setOmitNorms(const bool omitNorms) { internal->config |= INDEX_NONORMS; }
    
bool Field::isLazy() const { return (internal->config & LAZY_YES) != 0; }

void Field::setValue(const TCHAR* value) {
	_resetValue();
	internal->_stringValue = stringDuplicate( value );
}

void Field::setValue(CL_NS(util)::Reader* value) {
	_resetValue();
	internal->_readerValue = value;
}
void Field::setValue(jstreams::StreamBase<char>* value) {
	_resetValue();
	internal->_streamValue = value;
}

/** Expert: change the value of this field.  See <a href="#setValue(java.lang.String)">setValue(String)</a>. */
void Field::setValue(CL_NS(analysis)::TokenStream* value) {
	_resetValue();
	//fieldsData = value;
}

void Field::setBoost(const float_t boost)	{ this->internal->boost = boost; }
float_t Field::getBoost() const				{ return internal->boost; }

void Field::setConfig(const uint32_t x){
	uint32_t newConfig=0;

	//set storage settings
	if ( (x & STORE_YES) || (x & STORE_COMPRESS) ){
		newConfig |= STORE_YES;
		if ( x & STORE_COMPRESS )
			newConfig |= STORE_COMPRESS;
	} else
		newConfig |= STORE_NO;

	if ( (x & INDEX_NO)==0 ){
		bool index=false;

		if ( x & INDEX_TOKENIZED && x & INDEX_UNTOKENIZED )
			_CLTHROWA(CL_ERR_IllegalArgument,"it doesn't make sense to have an untokenised and tokenised field");

		if ( x & INDEX_NONORMS ){
			newConfig |= INDEX_UNTOKENIZED;
			newConfig |= INDEX_NONORMS;
			index = true;
		}
		 else if ( x & INDEX_TOKENIZED ){
			newConfig |= INDEX_TOKENIZED;
			index = true;
		}
		else if ( x & INDEX_UNTOKENIZED ){
			newConfig |= INDEX_UNTOKENIZED;
			index = true;
		}

		if ( !index )
			newConfig |= INDEX_NO;
	}else
		newConfig |= INDEX_NO;

	if ( newConfig & INDEX_NO && newConfig & STORE_NO )
		_CLTHROWA(CL_ERR_IllegalArgument,"it doesn't make sense to have a field that is neither indexed nor stored");

	//set termvector settings
	if ( (x & TERMVECTOR_NO) == 0 ){
		bool termVector=false;
		if ( x & TERMVECTOR_YES ){
			termVector=true;
		}
		if ( x & TERMVECTOR_WITH_POSITIONS ){
			newConfig |= TERMVECTOR_WITH_POSITIONS;
			termVector=true;
		}
		if ( x & TERMVECTOR_WITH_OFFSETS ){
			newConfig |= TERMVECTOR_WITH_OFFSETS;
			termVector=true;
		}
		// TERMVECTOR_WITH_POSITIONS_OFFSETS is being automatically handled here

		if ( termVector ){
			if ( newConfig & INDEX_NO )
				_CLTHROWA(CL_ERR_IllegalArgument,"cannot store a term vector for fields that are not indexed.");

			newConfig |= TERMVECTOR_YES;
		}else
			newConfig |= TERMVECTOR_NO;
	}else
		newConfig |= TERMVECTOR_NO;

	internal->config = newConfig;
}

TCHAR* Field::toString() {
    CL_NS(util)::StringBuffer result;
	if (isStored()) {
      result.append( _T("stored") );
	  if (isCompressed())
		  result.append( _T("/compressed"));
	  else
		  result.append( _T("/uncompressed") );
    }
    if (isIndexed()) {
      if (result.length() > 0)
        result.append( _T(",") );
      result.append( _T("indexed") );
    }
    if (isTokenized()) {
      if (result.length() > 0)
        result.append( _T(",") );
      result.append( _T("tokenized") );
    }
    if (isTermVectorStored()) {
      if (result.length() > 0)
        result.append( _T(",") );
      result.append( _T("termVector") );
    }
    if (isStoreOffsetWithTermVector()) { 
      if (result.length() > 0) 
        result.appendChar( ',' ); 
      result.append( _T("termVectorOffsets") ); 
    } 
    if (isStorePositionWithTermVector()) { 
      if (result.length() > 0) 
        result.appendChar( ',' ); 
      result.append( _T("termVectorPosition") ); 
    } 
    if (isBinary()) {
      if (result.length() > 0)
        result.appendChar( ',' );
      result.append( _T("binary") );
    }
    if (getOmitNorms()) {
      result.append( _T(",omitNorms") );
    }
	if (isLazy()){
      result.append( _T(",lazy") );
    }
    result.appendChar('<');
    result.append(name());
    result.appendChar(':');
    
	if (! isLazy()) {
		if (internal->_stringValue != NULL)
			result.append(internal->_stringValue);
		else if ( internal->_readerValue != NULL )
			result.append( _T("Reader") );
		else if ( internal->_streamValue != NULL )
			result.append( _T("Stream") );
		else
			result.append( _T("NULL") );
	}
    
    result.appendChar('>');
    return result.toString();
}


void Field::_resetValue() {
	_CLDELETE_CARRAY(internal->_stringValue);
	_CLDELETE(internal->_readerValue);
	_CLVDELETE( internal->_streamValue );
}

CL_NS_END
