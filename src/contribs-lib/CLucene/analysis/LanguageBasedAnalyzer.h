/**
 * Copyright 2002-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _lucene_analysis_languagebasedanalyzer_
#define _lucene_analysis_languagebasedanalyzer_

#include "CLucene/analysis/AnalysisHeader.h"

CL_NS_DEF(analysis)

class CLUCENE_CONTRIBS_EXPORT LanguageBasedAnalyzer: public CL_NS(analysis)::Analyzer{
	TCHAR lang[100];
	bool stem;
public:
	LanguageBasedAnalyzer(const TCHAR* language=LUCENE_BLANK_STRING, bool stem=true);
	~LanguageBasedAnalyzer();
	void setLanguage(const TCHAR* language);
	void setStem(bool stem);
	TokenStream* tokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader);
  };

CL_NS_END
#endif
