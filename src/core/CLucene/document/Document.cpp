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


  /** Constructs a new document with no fields-> */
  Document::Document():
    fields(_CLNEW FieldsType(true) )
  {
    //Func - Constructor
	//Pre  - true
	//Post - Instance has been created
        boost = 1.0f;
	}

	Document::~Document(){
    //Func - Destructor
	//Pre  - true
	//Post - Instance has been destroyed
    boost = 1.0f;
    _CLDELETE(fields);
	}
	
	void Document::clear(){
		fields->clear();
	}

	void Document::add(Fieldable& field) {
		fields->push_back(&field);
	}

   void Document::setBoost(const float_t boost) {
      this->boost = boost;
   }

   float_t Document::getBoost() const {
      return boost;
   }


	 Fieldable* Document::getFieldable(const TCHAR* name) const{
    CND_PRECONDITION(name != NULL, "name is NULL");
    for ( FieldsType::const_iterator itr = fields->begin();
      itr != fields->end(); itr ++ ){
      if ( _tcscmp( (*itr)->name(),name)==0 )
        return *itr;
    }
	  return NULL;
	 }
   Field* Document::getField(const TCHAR* name) const{
     return (Field*)getFieldable(name);
   }

	const TCHAR* Document::get(const TCHAR* field) const {
	  CND_PRECONDITION(field != NULL, "field is NULL");
		Fieldable* f = getFieldable(field);
		if (f!=NULL)
			return f->stringValue(); //this returns null it is a binary(reader)
		else
			return NULL;
	}

	const Document::FieldsType* Document::getFields() const  {
    return fields;
  }


	TCHAR* Document::toString() const {
		StringBuffer ret(_T("Document<"));
    for (FieldsType::const_iterator itr = fields->begin();
         itr != fields->end(); itr++ ) {
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
    for ( FieldsType::iterator itr = fields->begin();
      itr != fields->end(); itr++ ){

      if ( _tcscmp( (*itr)->name(), name) == 0 ){
        fields->remove(itr);
        return;
      }
    }
   }
   
   void Document::removeFields(const TCHAR* name) {
	  CND_PRECONDITION(name != NULL, "name is NULL");
    for ( FieldsType::iterator itr = fields->begin();
      itr != fields->end(); itr++ ){

      if ( _tcscmp( (*itr)->name(), name) == 0 ){
        fields->remove(itr);
        itr--;
        assert(false);//do we have to go back?
      }
    }
   }

   TCHAR** Document::getValues(const TCHAR* name) {
    int count = 0;
    for ( FieldsType::iterator itr = fields->begin();
      itr != fields->end(); itr++ ){
      if ( _tcscmp( (*itr)->name(),name) == 0 && (*itr)->stringValue() != NULL )
        count++;
    }
    
    //todo: there must be a better way of doing this, we are doing two iterations of the fields
    TCHAR** ret = NULL;
	  if ( count > 0 ){
      ret = _CL_NEWARRAY(TCHAR*,count+1);
      int32_t i=0;
      for ( FieldsType::iterator itr = fields->begin();
        itr != fields->end(); itr++ ){

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
