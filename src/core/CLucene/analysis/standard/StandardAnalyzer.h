/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_analysis_standard_StandardAnalyzer
#define _lucene_analysis_standard_StandardAnalyzer


//#include "CLucene/util/VoidMap.h"
CL_CLASS_DEF(util,Reader)
#include "CLucene/analysis/AnalysisHeader.h"
#include "CLucene/util/VoidMapSetDefinitions.h"
//#include "CLucene/analysis/Analyzers.h"
//#include "StandardFilter.h"
//#include "StandardTokenizer.h"


CL_NS_DEF2(analysis,standard)

/**
* Filters {@link StandardTokenizer} with {@link StandardFilter}, {@link
* LowerCaseFilter} and {@link StopFilter}, using a list of English stop words.
*
*/
	class CLUCENE_EXPORT StandardAnalyzer : public Analyzer 
	{
	private:
		CLTCSetList* stopSet;
	public:
		/** Builds an analyzer.*/
		StandardAnalyzer();

		/** Builds an analyzer with the given stop words. */
		StandardAnalyzer( const TCHAR** stopWords);

		/** Builds an analyzer with the stop words from the given file.
		* @see WordlistLoader#getWordSet(File)
		*/
		StandardAnalyzer(const char* stopwordsFile, const char* enc = NULL);

		/** Builds an analyzer with the stop words from the given reader.
		* @see WordlistLoader#getWordSet(Reader)
		*/
		StandardAnalyzer(CL_NS(util)::Reader* stopwordsReader, const bool _bDeleteReader = false);

		~StandardAnalyzer();

		/**
		* Constructs a StandardTokenizer filtered by a 
		* StandardFilter, a LowerCaseFilter and a StopFilter.
		*/
		TokenStream* tokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader);
	};
CL_NS_END2
#endif
