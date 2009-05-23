/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "stdafx.h"

#include "CLucene.h"
#include "CLucene/config/repl_tchar.h"
#include "CLucene/config/repl_wchar.h"
#include "CLucene/util/Misc.h"
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
	TCHAR* buf;

  IndexReader* reader = IndexReader::open(index);
  while (true) {
		printf("Enter query string: ");
		//char* tmp = fgets(line,80,stdin);
		//if ( tmp == NULL ) continue;
		//line[strlen(line)-1]=0;
    strcpy(line,"test");

    IndexReader* newreader = reader->reopen();
    if ( newreader != reader ){
      _CLDELETE(reader);
      reader = newreader;
    }
    IndexSearcher s(reader);

		if ( strlen(line) == 0 )
			break;
	  STRCPY_AtoT(tline,line,80);
		Query* q = QueryParser::parse(tline,_T("contents"),&analyzer);

		buf = q->toString(_T("contents"));
		_tprintf(_T("Searching for: %s\n\n"), buf);
		_CLDELETE_CARRAY(buf);

		uint64_t str = Misc::currentTimeMillis();
		Hits* h = s.search(q);
		uint32_t srch = (int32_t)(Misc::currentTimeMillis() - str);
		str = Misc::currentTimeMillis();

		for ( int32_t i=0;i<h->length();i++ ){
			Document* doc = &h->doc(i);
			//const TCHAR* buf = doc.get(_T("contents"));
			_tprintf(_T("%d. %s - %f\n"), i, doc->get(_T("path")), h->score(i));
			//delete doc;
		}

		printf("\n\nSearch took: %d ms.\n", srch);
		printf("Screen dump took: %d ms.\n\n", (int32_t)(Misc::currentTimeMillis() - str));

		_CLDELETE(h);
		_CLDELETE(q);

	  s.close();
break;
	}
  _CLDELETE(reader);
	//delete line;
}	

