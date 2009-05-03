/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "stdafx.h"
#include "CLucene.h"
#include "CLucene/util/Misc.h"

//test for memory leaks:
#ifdef _MSC_VER
#ifdef _DEBUG
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif
#endif

#include <iostream>
#include <string.h>

using namespace std;
using namespace lucene::util;

void DeleteFiles(const char* dir);
void IndexFiles(const char* path, const char* target, const bool clearIndex);
void SearchFiles(const char* index);
void getStats(const char* directory);

int main( int32_t argc, char** argv ){
	//Dumper Debug
	#ifdef _MSC_VER
	#ifdef _DEBUG
		_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_CHECK_CRT_DF );
		_crtBreakAlloc=-1;
	#endif
	#endif

	uint64_t str = Misc::currentTimeMillis();
	try{

		const char* ndx = "d:/git_lucene232/testindex";
    //IndexFiles("d:\\git_lucene232\\clucene\\src\\test\\data\\reuters-21578", ndx, true);
		//getStats(ndx);
		SearchFiles(ndx);

	}catch(CLuceneError& err){
		printf("Error: %s\n", err.what());
  }catch(...){
		printf("Unknown error\n");
	}

	_lucene_shutdown(); //clears all static memory
    //print lucenebase debug


	//Debuggin techniques:
	//For msvc, use this for breaking on memory leaks: 
	//	_crtBreakAlloc
	//for linux, use valgrind

	printf ("\n\nTime taken: %d\n\n", (int32_t)(Misc::currentTimeMillis() - str));
	return 0;
}
