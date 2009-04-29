/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_util_Array_
#define _lucene_util_Array_

CL_NS_DEF(util)

template<typename T>
class CLUCENE_EXPORT ArrayBase: LUCENE_BASE{
public:
	T* values;
	size_t length;
    
    /**
    * Delete's the values in the array and then calls deleteArray(). 
    * This won't do anything if deleteArray or takeArray is called first.
    * This is overridden in various implementations to provide the appropriate deletor function.
    */
	virtual void deleteValues() = 0;
	
	_CL_DEPRECATED(deleteValues) void deleteAll(){ this->deleteValues(); }
	
	/**
	* Deletes the array holding the values. Do this if you want to take
	* ownership of the array's values, but not the array containing the values.
	*/
	void deleteArray(){
		_CLDELETE_ARRAY(values);
	}
	/**
	* Empties the array. Do this if you want to take ownership of the array
	* and the array's values.
	*/
	T* takeArray(){
	    T* ret = values;
	    values = NULL;
	    return ret;
	} 

	ArrayBase(){
		values = NULL;
		length = 0;
	}
	ArrayBase(T* values, size_t length){
		this->values = values;
		this->length = length;
	}
	ArrayBase(size_t length){
		this->values = _CL_NEWARRAY(T,length);
		this->length = length;
    memset(this->values,0,sizeof(T));
	}
	virtual ~ArrayBase(){
	}

	const T operator[](size_t _Pos) const
	{
		if (length <= _Pos){
			_CLTHROWA(CL_ERR_IllegalArgument,"vector subscript out of range");
		}
		return (*(values + _Pos));
	}
	T operator[](size_t _Pos)
	{
		if (length <= _Pos){
			_CLTHROWA(CL_ERR_IllegalArgument,"vector subscript out of range");
		}
		return (*(values + _Pos));
	}
};

/**
* An array of objects. _CLDELETE is called on every contained object.
*/
template<typename T>
class CLUCENE_EXPORT ObjectArray: public ArrayBase<T*>{
public:
    ObjectArray():ArrayBase<T*>(){}
	ObjectArray(T** values, size_t length):ArrayBase<T*>(values,length){}
	ObjectArray(size_t length):ArrayBase<T*>(length){}
	
    void deleteValues(){
        if ( this->values == NULL )
            return;
		for (size_t i=0;i<this->length;i++){
			_CLLDELETE(this->values[i]);
		}
	    this->deleteArray();
	}
	virtual ~ObjectArray(){
	    deleteValues();
	}

	/* Initializes all cells in the array with a NULL value */
	void initArray(){
		for (size_t i=0;i<this->length;i++){
			this->values[i]=NULL;
		}
	}
};

/**
* Legacy code... don't use, remove all instances of this!
*/
template<typename T>
class CLUCENE_EXPORT Array: public ArrayBase<T>{
public:
    _CL_DEPRECATED(ObjectArray or ValueArray) Array():ArrayBase<T>(){}
	_CL_DEPRECATED(ObjectArray or ValueArray) Array(T* values, size_t length):ArrayBase<T>(values,length){}
	_CL_DEPRECATED(ObjectArray or ValueArray) Array(size_t length):ArrayBase<T>(length){}
    void deleteValues(){
        if ( this->values == NULL )
            return;
	    this->deleteArray();
	}
	virtual ~Array(){
	}
};

/**
* An array where the values do not need to be deleted
*/
template<typename T>
class CLUCENE_EXPORT ValueArray: public ArrayBase<T>{
public:
    ValueArray():ArrayBase<T>(){}
	ValueArray(T* values, size_t length):ArrayBase<T>(values,length){}
	ValueArray(size_t length):ArrayBase<T>(length){}

    void deleteValues(){
        if ( this->values == NULL )
            return;
	    this->deleteArray();
	}
	virtual ~ValueArray(){
	    deleteValues();
	}
};


/**
* An array of TCHAR strings
*/
class CLUCENE_EXPORT TCharArray: public ArrayBase<TCHAR*>{
public:
    TCharArray():ArrayBase<TCHAR*>(){}
	TCharArray(TCHAR** values, size_t length):ArrayBase<TCHAR*>(values,length){}
	TCharArray(size_t length):ArrayBase<TCHAR*>(length){}

    void deleteValues(){
        if ( this->values == NULL )
            return;
		for (size_t i=0;i<this->length;i++)
			_CLDELETE_CARRAY(this->values[i]);
	    this->deleteArray();
	}
	virtual ~TCharArray(){
	    deleteValues();
	}
};

/**
* An array of char strings
*/
class CLUCENE_EXPORT CharArray: public ArrayBase<char*>{
public:
    CharArray():ArrayBase<char*>(){}
	CharArray(char** values, size_t length):ArrayBase<char*>(values,length){}
	CharArray(size_t length):ArrayBase<char*>(length){}

    void deleteValues(){
        if ( this->values == NULL )
            return;
		for (size_t i=0;i<this->length;i++)
			_CLDELETE_CaARRAY(this->values[i]);
	    this->deleteArray();
	}
	virtual ~CharArray(){
	    deleteValues();
	}
};

/**
* An array of const TCHAR strings
*/
class CLUCENE_EXPORT TCharConstArray: public ArrayBase<const TCHAR*>{
public:
    TCharConstArray():ArrayBase<const TCHAR*>(){}
	TCharConstArray(const TCHAR** values, size_t length):ArrayBase<const TCHAR*>(values,length){}
	TCharConstArray(size_t length):ArrayBase<const TCHAR*>(length){}

    void deleteValues(){
        if ( this->values == NULL )
            return;
		for (size_t i=0;i<this->length;i++)
			_CLDELETE_CARRAY(this->values[i]);
	    this->deleteArray();
	}
	virtual ~TCharConstArray(){
	    deleteValues();
	}
};

/**
* An array of const char strings
*/
class CLUCENE_EXPORT CharConstArray: public ArrayBase<const char*>{
public:
    CharConstArray():ArrayBase<const char*>(){}
	CharConstArray(const char** values, size_t length):ArrayBase<const char*>(values,length){}
	CharConstArray(size_t length):ArrayBase<const char*>(length){}

    void deleteValues(){
        if ( this->values == NULL )
            return;
		for (size_t i=0;i<this->length;i++)
			_CLDELETE_CaARRAY(this->values[i]);
	    this->deleteArray();
	}
	virtual ~CharConstArray(){
	    deleteValues();
	}
};


CL_NS_END
#endif
