/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
//note: you may be bound by the google license if you choose
//to compile your source code with the google sparsemap library
#ifndef _lucene_util_GoogleSparseMaps_H
#define _lucene_util_GoogleSparseMaps_H

#if defined(_LUCENE_PRAGMA_ONCE)
# pragma once
#endif

#ifdef _CL_HAVE_GOOGLE_DENSE_HASH_MAP

#include <google/dense_hash_map>

CL_NS_DEF(util)


template<typename _Type, typename _Equals>
class SparseMapEquals{
	_Equals equals;
public:
	bool operator()( _Type val1, _Type val2 ) const{
	   if ( val1==val2 )
	      return true;
	   else if ( val1 == NULL || val2 == NULL )
	      return false;
	   else if ( val1 == (_Type)0x02 || val2 == (_Type)0x02 )
	      return false;
		return equals(val1,val2);
	}
};

template<typename _kt, typename _vt,
	typename _Hasher,
	typename _Equals,
	typename _KeyDeletor=CL_NS(util)::Deletor::Dummy,
	typename _ValueDeletor=CL_NS(util)::Deletor::Dummy >
class CLHashMap:public __CLMap<_kt,_vt,
	GOOGLE_NAMESPACE::dense_hash_map<_kt,_vt, _Hasher, SparseMapEquals<_kt,_Equals> >,
	_KeyDeletor,_ValueDeletor>
{
	typedef __CLMap<_kt,_vt,
		GOOGLE_NAMESPACE::dense_hash_map<_kt,_vt, _Hasher, SparseMapEquals<_kt,_Equals> >,
		_KeyDeletor,_ValueDeletor> _this;
public:
	CLHashMap ( bool deleteKey=false, bool deleteValue=false )
	{
		GOOGLE_NAMESPACE::dense_hash_map<_kt,_vt, _Hasher, SparseMapEquals<_kt,_Equals> >::set_empty_key(NULL);
		GOOGLE_NAMESPACE::dense_hash_map<_kt,_vt, _Hasher, SparseMapEquals<_kt,_Equals> >::set_deleted_key((_kt)0x02);
		_this::setDeleteKey(deleteKey);
		_this::setDeleteValue(deleteValue);
	}
	~CLHashMap(){

	}
};


CL_NS_END
#endif //LUCENE_USE_GOOGLEMAPS
#endif
