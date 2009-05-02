/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"

  void assertAnalyzersTo(CuTest *tc,Analyzer* a, const TCHAR* input, const TCHAR* output){
   Reader* reader = _CLNEW StringReader(input);
	TokenStream* ts = a->tokenStream(_T("dummy"), reader );

	const TCHAR* pos = output;
	TCHAR buffer[80];
	const TCHAR* last = output;
	CL_NS(analysis)::Token* t = NULL;
	while( (pos = _tcsstr(pos+1, _T(";"))) != NULL ) {
		int32_t len = (int32_t)(pos-last);
		_tcsncpy(buffer,last,len);
		buffer[len]=0;

	  CLUCENE_ASSERT((t = ts->next(t)) != NULL);
	  CLUCENE_ASSERT(_tcscmp( t->termBuffer(),buffer) == 0 );
		
    last = pos+1;
  }
  CLUCENE_ASSERT(ts->next(t)==NULL); //Test failed, more fields than expected.
  
  _CLDELETE(t);

	 ts->close();
    _CLDELETE(reader);
    _CLDELETE(ts);
  }

  void testSimpleAnalyzer(CuTest *tc){
    Analyzer* a = _CLNEW SimpleAnalyzer();
	assertAnalyzersTo(tc,a, _T("foo bar FOO BAR"), _T("foo;bar;foo;bar;") );
    assertAnalyzersTo(tc,a, _T("foo      bar .  FOO <> BAR"), _T("foo;bar;foo;bar;"));
    assertAnalyzersTo(tc,a, _T("foo.bar.FOO.BAR"), _T("foo;bar;foo;bar;"));
    assertAnalyzersTo(tc,a, _T("U.S.A."), _T("u;s;a;") );
    assertAnalyzersTo(tc,a, _T("C++"), _T("c;") );
    assertAnalyzersTo(tc,a, _T("B2B"), _T("b;b;"));
    assertAnalyzersTo(tc,a, _T("2B"), _T("b;"));
    assertAnalyzersTo(tc,a, _T("\"QUOTED\" word"), _T("quoted;word;"));
    
    _CLDELETE(a);
  }
  
   void testStandardAnalyzer(CuTest *tc){
    Analyzer* a = _CLNEW StandardAnalyzer();
    
    //todo: check this
	 assertAnalyzersTo(tc,a, _T("[050-070]"), _T("050;-070;") );
    
    _CLDELETE(a);
   }

   
   void testPerFieldAnalzyerWrapper(CuTest *tc){
        const TCHAR* text = _T("Qwerty");
        PerFieldAnalyzerWrapper analyzer(_CLNEW WhitespaceAnalyzer());

        analyzer.addAnalyzer(_T("special"), _CLNEW SimpleAnalyzer());

        StringReader reader(text);
        TokenStream* tokenStream = analyzer.tokenStream( _T("field"), &reader);
        CL_NS(analysis)::Token* token = NULL;

        CLUCENE_ASSERT( tokenStream->next(token) != NULL );
        CuAssertStrEquals(tc,_T("token.termBuffer()"), _T("Qwerty"),
                    token->termBuffer());
		_CLDELETE(token);
        _CLDELETE(tokenStream);

        StringReader reader2(text);
        tokenStream = analyzer.tokenStream(_T("special"), &reader2);
        CLUCENE_ASSERT( tokenStream->next(token) != NULL );
        CuAssertStrEquals(tc, _T("token.termBuffer()"), _T("qwerty"),
                    token->termBuffer());
		_CLDELETE(token);
        _CLDELETE(tokenStream);
   }

  void testNullAnalyzer(CuTest *tc){
    Analyzer* a = _CLNEW WhitespaceAnalyzer();
    assertAnalyzersTo(tc,a, _T("foo bar FOO BAR"), _T("foo;bar;FOO;BAR;"));
    assertAnalyzersTo(tc,a, _T("foo      bar .  FOO <> BAR"), _T("foo;bar;.;FOO;<>;BAR;"));
    assertAnalyzersTo(tc,a, _T("foo.bar.FOO.BAR"), _T("foo.bar.FOO.BAR;"));
    assertAnalyzersTo(tc,a, _T("U.S.A."), _T("U.S.A.;"));
    assertAnalyzersTo(tc,a, _T("C++"), _T("C++;"));
    assertAnalyzersTo(tc,a, _T("B2B"), _T("B2B;"));
    assertAnalyzersTo(tc,a, _T("2B"), _T("2B;"));
    assertAnalyzersTo(tc,a, _T("\"QUOTED\" word"), _T("\"QUOTED\";word;") );
    
    _CLDELETE(a);
  }

  void testStopAnalyzer(CuTest *tc){
    Analyzer* a = _CLNEW StopAnalyzer();
    assertAnalyzersTo(tc,a, _T("foo bar FOO BAR"), _T("foo;bar;foo;bar;"));
    assertAnalyzersTo(tc,a, _T("foo a bar such FOO THESE BAR"), _T("foo;bar;foo;bar;"));
    
    _CLDELETE(a);
  }

  void testISOLatin1AccentFilter(CuTest *tc){
	  TCHAR str[200];
	  _tcscpy(str, _T("Des mot cl\xe9s \xc0 LA CHA\xceNE \xc0 \xc1 \xc2 ") //Des mot cl�s � LA CHA�NE � � � 
						_T("\xc3 \xc4 \xc5 \xc6 \xc7 \xc8 \xc9 \xca \xcb \xcc \xcd \xce \xcf") //� � � � � � � � � � � � � 
						_T(" \xd0 \xd1 \xd2 \xd3 \xd4 \xd5 \xd6 \xd8 \xde \xd9 \xda \xdb") //� � � � � � � � � � � � � 
						_T(" \xdc \xdd \xe0 \xe1 \xe2 \xe3 \xe4 \xe5 \xe6 \xe7 \xe8 \xe9 ") //� � � � � � � � � � � 
						_T("\xea \xeb \xec \xed \xee \xef \xf0 \xf1 \xf2 \xf3 \xf4 \xf5 \xf6 ") //� � � � � � � � � � � � � 
						_T("\xf8 \xdf \xfe \xf9 \xfa \xfb \xfc \xfd \xff") //� � � � � � � � �
						_T("      ") ); //room for extra latin stuff
	#ifdef _UCS2
		int p = _tcslen(str)-6;
		str[p+1] = 0x152;// �
		str[p+3] = 0x153;// � 
		str[p+5] = 0x178;//� 
	#endif
	
	StringReader reader(str);
	WhitespaceTokenizer ws(&reader);
	ISOLatin1AccentFilter filter(&ws,false);
	CL_NS(analysis)::Token* token = NULL;

	
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("Des"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("mot"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("cles"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("A"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("LA"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("CHAINE"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("A"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("A"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("A"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("A"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("A"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("A"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("AE"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("C"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("E"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("E"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("E"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("E"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("I"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("I"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("I"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("I"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("D"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("N"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("O"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("O"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("O"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("O"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("O"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("O"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("TH"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("U"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("U"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("U"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("U"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("Y"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("a"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("a"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("a"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("a"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("a"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("a"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("ae"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("c"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("e"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("e"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("e"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("e"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("i"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("i"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("i"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("i"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("d"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("n"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("o"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("o"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("o"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("o"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("o"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("o"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("ss"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("th"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("u"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("u"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("u"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("u"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("y"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("y"), token->termBuffer());
	
	#ifdef _UCS2
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("OE"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("oe"), token->termBuffer());
	CLUCENE_ASSERT(filter.next(token) != NULL); CuAssertStrEquals(tc, _T("Token compare"), _T("Y"), token->termBuffer());
	#endif
	
	
	CLUCENE_ASSERT(filter.next(token)==NULL);

	_CLDELETE(token);
  }

  void testWordlistLoader(CuTest *tc){
	  char stopwordsfile[1024];
	  strcpy(stopwordsfile, clucene_data_location);
	  strcat(stopwordsfile, "/StopWords.test");
	  Analyzer* a = _CLNEW StopAnalyzer(stopwordsfile);
	  assertAnalyzersTo(tc,a, _T("foo bar FOO BAR"), _T("foo;bar;foo;bar;"));
	  assertAnalyzersTo(tc,a, _T("foo a bar such FOO THESE BAR"), _T("foo;bar;foo;bar;"));
    
	  _CLDELETE(a);

	  TCHAR* testString = new TCHAR[10];
	  _tcscpy(testString, _T("test"));
	  CuAssertStrEquals(tc, _T("stringTrim compare"), CL_NS(util)::Misc::wordTrim(testString), _T("test"));
	  _tcscpy(testString, _T("test "));
	  CuAssertStrEquals(tc, _T("stringTrim compare"), CL_NS(util)::Misc::wordTrim(testString), _T("test"));
	  _tcscpy(testString, _T("test  "));
	  CuAssertStrEquals(tc, _T("stringTrim compare"), CL_NS(util)::Misc::wordTrim(testString), _T("test"));
	  _tcscpy(testString, _T(" test"));
	  CuAssertStrEquals(tc, _T("stringTrim compare"), CL_NS(util)::Misc::wordTrim(testString), _T("test"));
	  _tcscpy(testString, _T("  test"));
	  CuAssertStrEquals(tc, _T("stringTrim compare"), CL_NS(util)::Misc::wordTrim(testString), _T("test"));
	  _tcscpy(testString, _T(" test "));
	  CuAssertStrEquals(tc, _T("stringTrim compare"), CL_NS(util)::Misc::wordTrim(testString), _T("test"));
	  _tcscpy(testString, _T(" test  "));
	  CuAssertStrEquals(tc, _T("stringTrim compare"), CL_NS(util)::Misc::wordTrim(testString), _T("test"));
	  _tcscpy(testString, _T("  test  "));
	  CuAssertStrEquals(tc, _T("stringTrim compare"), CL_NS(util)::Misc::wordTrim(testString), _T("test"));
	  _tcscpy(testString, _T("  te st  "));
	  CuAssertStrEquals(tc, _T("stringTrim compare"), CL_NS(util)::Misc::wordTrim(testString), _T("te"));
	  _tcscpy(testString, _T("tes t"));
	  CuAssertStrEquals(tc, _T("stringTrim compare"), CL_NS(util)::Misc::wordTrim(testString), _T("tes"));
	  _tcscpy(testString, _T(" t est "));
	  CuAssertStrEquals(tc, _T("stringTrim compare"), CL_NS(util)::Misc::wordTrim(testString), _T("t"));
	  delete[] testString;
  }
  
CuSuite *testanalyzers(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene Analyzers Test"));

    SUITE_ADD_TEST(suite, testISOLatin1AccentFilter);
    SUITE_ADD_TEST(suite, testStopAnalyzer);
    SUITE_ADD_TEST(suite, testNullAnalyzer);
    SUITE_ADD_TEST(suite, testSimpleAnalyzer);
    SUITE_ADD_TEST(suite, testPerFieldAnalzyerWrapper);
	SUITE_ADD_TEST(suite, testWordlistLoader);
    return suite; 
}
// EOF
