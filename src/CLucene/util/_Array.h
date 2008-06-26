/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_util_Array_
#define _lucene_util_Array_


template<typename T>
class CLUCENE_EXPORT Array: LUCENE_BASE{
public:
	T* values;
	size_t length;

	void deleteAll(){
		for (size_t i=0;i<length;i++)
			_CLDELETE(values[i]);
		_CLDELETE_ARRAY(values);
	}
	void deleteArray(){
		_CLDELETE_ARRAY(values);
	}

	Array(){
		values = NULL;
		length = 0;
	}
	Array(T* values, size_t length){
		this->values = values;
		this->length = length;
	}
	Array(size_t length){
		this->values = _CL_NEWARRAY(T,length);
		this->length = length;
	}
	~Array(){}

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

#endif
