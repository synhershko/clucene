/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "stdafx.h"

#include "CLucene.h"
#include <iostream>

using namespace std;
using namespace lucene::analysis;
using namespace lucene::index;
using namespace lucene::util;
using namespace lucene::queryParser;
using namespace lucene::document;
using namespace lucene::search;


void SearchFiles(const char* index){
    //Searcher searcher(index);
	standard::StandardAnalyzer analyzer;
	char line[80];
	TCHAR tline[80];
	const TCHAR* buf;

	IndexSearcher s(index);
    while (true) {
		printf("Enter query string: ");
		fgets(line,80,stdin);
		line[strlen(line)-1]=0;

		if ( strlen(line) == 0 )
			break;
	   STRCPY_AtoT(tline,line,80);
		Query* q = QueryParser::parse(tline,_T("contents"),&analyzer);

		buf = q->toString(_T("contents"));
		_tprintf(_T("Searching for: %s\n\n"), buf);
		_CLDELETE_CARRAY(buf);

		uint64_t str = lucene::util::Misc::currentTimeMillis();
		Hits* h = s.search(q);
		uint64_t srch = lucene::util::Misc::currentTimeMillis() - str;
		str = lucene::util::Misc::currentTimeMillis();

		for ( int32_t i=0;i<h->length();i++ ){
			Document* doc = &h->doc(i);
			//const TCHAR* buf = doc.get(_T("contents"));
			_tprintf(_T("%d. %s - %f\n"), i, doc->get(_T("path")), h->score(i));
			//delete doc;
		}

		printf("\n\nSearch took: %d ms.\n", srch);
		printf("Screen dump took: %d ms.\n\n", lucene::util::Misc::currentTimeMillis() - str);

		_CLDELETE(h);
		_CLDELETE(q);
		
	}
	s.close();
	//delete line;
}	

