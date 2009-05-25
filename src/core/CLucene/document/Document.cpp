/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "Document.h"
#include "Field.h"
#include "CLucene/util/StringBuffer.h"
#include <assert.h>

CL_NS_USE(util)
CL_NS_DEF(document)


  DocumentFieldEnumeration::DocumentFieldList::DocumentFieldList(Field* f, DocumentFieldList* n ) {
    field = f;
    next  = n;
  }
  DocumentFieldEnumeration::DocumentFieldList::~DocumentFieldList(){
    if (!field) {
        return; // nothing to do; deleted by different invocation of dtor
    }

    DocumentFieldList* cur = next;
    while (cur != NULL)
    {
      DocumentFieldList* temp = cur->next;
      cur->next = NULL;

      _CLDELETE(cur);
      cur = temp;
    }
    field = NULL;
  }
  DocumentFieldEnumeration::DocumentFieldEnumeration(const DocumentFieldList* fl){
    //Func - Constructor
  //Pre  - fl may be NULL
  //Post - Instance has been created

    fields = fl;
  }

  bool DocumentFieldEnumeration::hasMoreElements() const {
    return fields == NULL ? false : true;
  }

  Field* DocumentFieldEnumeration::nextElement() {
    //Func - Return the next element in the enumeration
  //Pre  - true
  //Post - The next element is returned or NULL


    Field* result = NULL;
    //Check if fields is still valid
    if (fields){
      result = fields->field;
      fields = fields->next;
    }
    return result;
  }

  DocumentFieldEnumeration::~DocumentFieldEnumeration(){
    //Func - Destructor
  //Pre  - true
  //Post - Instance has been destroyed
  }
  DocumentFieldEnumeration* Document::fields() {
    if ( fieldListCache == NULL ){
      for ( FieldsType::const_iterator itr = _fields->begin();
        itr != _fields->end(); itr ++ ){

        fieldListCache = _CLNEW DocumentFieldEnumeration::DocumentFieldList(*itr, fieldListCache);
      }
    }
    return _CLNEW DocumentFieldEnumeration(fieldListCache);
  }

  /** Constructs a new document with no fields-> */
  Document::Document():
    _fields(_CLNEW FieldsType(true) )
  {
    //Func - Constructor
	//Pre  - true
	//Post - Instance has been created
    boost = 1.0f;
    fieldListCache = NULL;
	}

	Document::~Document(){
    //Func - Destructor
	//Pre  - true
	//Post - Instance has been destroyed
    boost = 1.0f;
    _CLDELETE(_fields);
	}

	void Document::clear(){
		_fields->clear();
    _CLDELETE(fieldListCache);
	}

	void Document::add(Field& field) {
		_fields->push_back(&field);
    _CLDELETE(fieldListCache);
	}

   void Document::setBoost(const float_t boost) {
      this->boost = boost;
   }

   float_t Document::getBoost() const {
      return boost;
   }


	 Field* Document::getField(const TCHAR* name) const{
    CND_PRECONDITION(name != NULL, "name is NULL");
    for ( FieldsType::const_iterator itr = _fields->begin();
      itr != _fields->end(); itr ++ ){
      if ( _tcscmp( (*itr)->name(),name)==0 )
        return *itr;
    }
	  return NULL;
	 }
	const TCHAR* Document::get(const TCHAR* field) const {
	  CND_PRECONDITION(field != NULL, "field is NULL");
		Field* f = getField(field);
		if (f!=NULL)
			return f->stringValue(); //this returns null it is a binary(reader)
		else
			return NULL;
	}

	const Document::FieldsType* Document::getFields() const  {
    return _fields;
  }


	TCHAR* Document::toString() const {
		StringBuffer ret(_T("Document<"));
    for (FieldsType::const_iterator itr = _fields->begin();
         itr != _fields->end(); itr++ ) {
  		TCHAR* tmp = (*itr)->toString();
      if ( ret.len > 0 )
  		    ret.append(_T(" "));
		  ret.append( tmp );
			_CLDELETE_ARRAY( tmp );
		}
		ret.append(_T(">"));
		return ret.toString();
	}



   void Document::removeField(const TCHAR* name) {
	  CND_PRECONDITION(name != NULL, "name is NULL");
    for ( FieldsType::iterator itr = _fields->begin();
      itr != _fields->end(); itr++ ){

      if ( _tcscmp( (*itr)->name(), name) == 0 ){
        _fields->remove(itr);
        return;
      }
    }
    _CLDELETE(fieldListCache);
   }

   void Document::removeFields(const TCHAR* name) {
	  CND_PRECONDITION(name != NULL, "name is NULL");
    for ( FieldsType::iterator itr = _fields->begin();
      itr != _fields->end(); itr++ ){

      if ( _tcscmp( (*itr)->name(), name) == 0 ){
        _fields->remove(itr);
        itr--;
      }
    }
    _CLDELETE(fieldListCache);
   }

   TCHAR** Document::getValues(const TCHAR* name) {
    int count = 0;
    for ( FieldsType::iterator itr = _fields->begin();
      itr != _fields->end(); itr++ ){
      if ( _tcscmp( (*itr)->name(),name) == 0 && (*itr)->stringValue() != NULL )
        count++;
    }

    //todo: there must be a better way of doing this, we are doing two iterations of the fields
    TCHAR** ret = NULL;
	  if ( count > 0 ){
      ret = _CL_NEWARRAY(TCHAR*,count+1);
      int32_t i=0;
      for ( FieldsType::iterator itr = _fields->begin();
        itr != _fields->end(); itr++ ){

        if ( _tcscmp( (*itr)->name(),name) == 0 && (*itr)->stringValue() != NULL ){
           ret[i] = stringDuplicate((*itr)->stringValue());
           i++;
        }
      }
      ret[count]=NULL;
    }
    return ret;
   }
CL_NS_END
