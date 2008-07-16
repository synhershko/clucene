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
	#define CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif
#endif

#include <iostream>

using namespace std;
using namespace lucene::util;

void DeleteFiles(const char* dir);
void IndexFiles(char* path, char* target, const bool clearIndex);
void SearchFiles(const char* index);
void getStats(const char* directory);

int main( int32_t argc, char** argv ){
	//Dumper Debug
	#ifdef TR_LEAKS 
	#ifdef _CLCOMPILER_MSVC
	#ifdef _DEBUG
		_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );//| _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_CHECK_CRT_DF );
	#endif
	#endif
	#endif

	uint64_t str = Misc::currentTimeMillis();
	try{

		printf("Location of text files to be indexed: ");
		char files[250];
		fgets(files,250,stdin);
		files[strlen(files)-1] = 0;
		
		printf("Location to store the clucene index: ");
		char ndx[250];
		fgets(ndx,250,stdin);
		ndx[strlen(ndx)-1] = 0;

		IndexFiles(files,ndx,true);
		getStats(ndx);
		SearchFiles(ndx);
		DeleteFiles(ndx);

	}catch(CLuceneError& err){
		printf(err.what());
    }catch(...){
		printf("Unknown error");
	}

	_lucene_shutdown(); //clears all static memory
    //print lucenebase debug
#ifdef LUCENE_ENABLE_MEMLEAKTRACKING
	lucene::debug::LuceneBase::__cl_PrintUnclosedObjects();
   //clear memtracking memory (not the unclosed objects)
   lucene::debug::LuceneBase::__cl_ClearMemory();
#endif

	//Debuggin techniques:
	//For msvc, use this for breaking on memory leaks: 
	//	_crtBreakAlloc
	//to break at this clucene item:
	//	_lucene_counter_break
	//run a memory check before deleting objects:
	//	_lucene_run_objectcheck
	//if LUCENE_ENABLE_CONSTRUCTOR_LOG is on, dont do log if this is true:
	//	_lucene_disable_debuglogging

	printf ("\n\nTime taken: %d\n\n",Misc::currentTimeMillis() - str);
	return 0;
}
