/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "stdafx.h"
#include "CLucene.h"
#include "CLucene/util/CLStreams.h"
#include "CLucene/util/dirent.h"
#include "CLucene/config/repl_tchar.h"
#include "CLucene/util/Misc.h"
#include "CLucene/util/StringBuffer.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <cctype>
#include <string.h>
#include <algorithm>

using namespace std;
using namespace lucene::index;
using namespace lucene::analysis;
using namespace lucene::util;
using namespace lucene::store;
using namespace lucene::document;

void FileDocument(const char* f, Document* doc){

	// Add the path of the file as a field named "path".  Use a Tex t field, so
	// that the index stores the path, and so that the path is searchable
   TCHAR tf[CL_MAX_DIR];
   STRCPY_AtoT(tf,f,CL_MAX_DIR);
   doc->add( *_CLNEW Field(_T("path"), tf, Field::STORE_YES | Field::INDEX_UNTOKENIZED ) );

	// Add the last modified date of the file a field named "modified".  Use a
	// Keyword field, so that it's searchable, but so that no attempt is made
	// to tokenize the field into words.
	//doc->add( *Field.Keyword("modified", DateTools::timeToString(f->lastModified())));

	// Add the contents of the file a field named "contents".  Use a Text
	// field, specifying a Reader, so that the text of the file is tokenized.

    //read the data without any encoding. if you want to use special encoding
    //see the contrib/jstreams - they contain various types of stream readers
    FILE* fh = fopen(f,"r");
	if ( fh != NULL ){
		StringBuffer str;
		char abuf[1024];
		TCHAR tbuf[1024];
		size_t r;
		do{
			r = fread(abuf,1,1023,fh);
			abuf[r]=0;
			STRCPY_AtoT(tbuf,abuf,r);
			tbuf[r]=0;
			str.append(tbuf);
		}while(r>0);
		fclose(fh);

		doc->add( *_CLNEW Field(_T("contents"),str.getBuffer(), Field::STORE_YES | Field::INDEX_TOKENIZED) );
	}

}

void indexDocs(IndexWriter* writer, const char* directory) {
  vector<string> files;
  std::sort(files.begin(),files.end());
  Misc::listFiles(directory,files,true);
  vector<string>::iterator itr = files.begin();
	// use a single, empty document
	Document doc;
  int i=0;
	while ( itr != files.end() ){
    const char* path = itr->c_str();
	  printf( "adding file %d: %s\n", ++i, path );

    doc.clear();
		FileDocument( path, &doc );
		writer->addDocument( &doc );
    itr++;
	}
}
void IndexFiles(const char* path, const char* target, const bool clearIndex){
	IndexWriter* writer = NULL;
	//lucene::analysis::SimpleAnalyzer* an = *_CLNEW lucene::analysis::SimpleAnalyzer();
	lucene::analysis::WhitespaceAnalyzer an;
	
	if ( !clearIndex && IndexReader::indexExists(target) ){
		if ( IndexReader::isLocked(target) ){
			printf("Index was locked... unlocking it.\n");
			IndexReader::unlock(target);
		}

		writer = _CLNEW IndexWriter( target, &an, false);
	}else{
		writer = _CLNEW IndexWriter( target ,&an, true);
	}
  writer->setInfoStream(&std::cout);
  writer->setMaxBufferedDocs(3);
  //writer->setMaxMergeDocs(5);
	writer->setMaxFieldLength(10000);
  writer->setUseCompoundFile(false);
  //writer->setRAMBufferSizeMB(0.1);

	/*printf("Set MaxFieldLength: ");
	char mfl[250];
	fgets(mfl,250,stdin);
	mfl[strlen(mfl)-1] = 0;
	if ( mfl[0] != 0 )
		writer->setMaxFieldLength(atoi(mfl));*/
	//writer->infoStream = cout; //TODO: infoStream - unicode

	uint64_t str = Misc::currentTimeMillis();

	indexDocs(writer, path);
//	writer->optimize();
	writer->close();
	_CLDELETE(writer);

	printf("Indexing took: %d ms.\n\n", (int32_t)(Misc::currentTimeMillis() - str));
}
