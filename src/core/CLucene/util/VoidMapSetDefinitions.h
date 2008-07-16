/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#if !defined(_lucene_util_MapSets_) && !defined(_lucene_util_VoidMap_)
#define _lucene_util_MapSets_

#include "VoidMap.h"
#include "VoidList.h"

//todo: only make these public...
//this is the interfaces for lists and maps. 
//we want to deprecate all public use of this, but can't do that until
//StringArray is deprecated for a while...

/*
#include "Equators.h"

CL_NS_DEF(util)

///////////////
//LISTS
///////////////
template<typename _kt,typename _base,typename _valueDeletor> 
class __CLList;

template<typename _kt, typename _valueDeletor=CL_NS(util)::Deletor::Dummy> 
class CLVector;

#define CLArrayList CLVector 
#define CLHashSet CLHashList

template<typename _kt,
	typename _Comparator=CL_NS(util)::Compare::TChar,
	typename _valueDeletor=CL_NS(util)::Deletor::Dummy> 
class CLHashList;

template<typename _kt, typename _valueDeletor=CL_NS(util)::Deletor::Dummy> 
class CLLinkedList;
template<typename _kt,
	typename _Comparator=CL_NS(util)::Compare::TChar,
	typename _valueDeletor=CL_NS(util)::Deletor::Dummy> 
class CLSetList;



///////////////
//MAPS
///////////////
template<typename _kt, typename _vt, 
	typename _base,
	typename _KeyDeletor=CL_NS(util)::Deletor::Dummy,
	typename _ValueDeletor=CL_NS(util)::Deletor::Dummy>
class __CLMap;

// makes no guarantees as to the order of the map
// cannot contain duplicate keys; each key can map to at most one value
#define CLHashtable CLHashMap

 //a CLSet with CLHashMap traits
template<typename _kt, typename _vt, 
	typename _Compare,
	typename _EqualDummy,
	typename _KeyDeletor=CL_NS(util)::Deletor::Dummy,
	typename _ValueDeletor=CL_NS(util)::Deletor::Dummy>
class CLHashMap;


//A collection that contains no duplicates
//does not guarantee that the order will remain constant over time
template<typename _kt, typename _vt, 
	typename _Compare,
	typename _KeyDeletor=CL_NS(util)::Deletor::Dummy,
	typename _ValueDeletor=CL_NS(util)::Deletor::Dummy>
class CLSet;


//A collection that can contains duplicates
template<typename _kt, typename _vt,
	typename _Compare,
	typename _KeyDeletor=CL_NS(util)::Deletor::Dummy,
	typename _ValueDeletor=CL_NS(util)::Deletor::Dummy>
class CLMultiMap;

CL_NS_END*/
#endif
