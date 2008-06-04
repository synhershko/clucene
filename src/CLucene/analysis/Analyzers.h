/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_analysis_Analyzers_
#define _lucene_analysis_Analyzers_

#if defined(_LUCENE_PRAGMA_ONCE)
# pragma once
#endif

#include "CLucene/util/Reader.h"
#include "AnalysisHeader.h"
#include "CLucene/util/Misc.h"

CL_NS_DEF(analysis)

/** An abstract base class for simple, character-oriented tokenizers.*/
class CharTokenizer:public Tokenizer {
private:
	int32_t offset, bufferIndex, dataLen;
	TCHAR buffer[LUCENE_MAX_WORD_LEN+1];
	const TCHAR* ioBuffer;
protected:
    
    /** Returns true iff a character should be included in a token.  This
    * tokenizer generates as tokens adjacent sequences of characters which
    * satisfy this predicate.  Characters for which this is false are used to
    * define token boundaries and are not included in tokens. */
	virtual bool isTokenChar(const TCHAR c) const = 0;

    /** Called on each token character to normalize it before it is added to the
    * token.  The default implementation does nothing.  Subclasses may use this
    * to, e.g., lowercase tokens. */
   	virtual TCHAR normalize(const TCHAR c) const;

public:
	CharTokenizer(CL_NS(util)::Reader* in);
	virtual ~CharTokenizer(){
	}
	bool next(Token* token);
};


/** A LetterTokenizer is a tokenizer that divides text at non-letters.  That's
to say, it defines tokens as maximal strings of adjacent letters, as defined
by java.lang.Character.isLetter() predicate.

Note: this does a decent job for most European languages, but does a terrible
job for some Asian languages, where words are not separated by spaces. */
class LetterTokenizer:public CharTokenizer {
public:
	// Construct a new LetterTokenizer. 
	LetterTokenizer(CL_NS(util)::Reader* in):
	CharTokenizer(in) {}

    ~LetterTokenizer(){}
protected:
    /** Collects only characters which satisfy _istalpha.*/
	bool isTokenChar(const TCHAR c) const;
};



/**
* LowerCaseTokenizer performs the function of LetterTokenizer
* and LowerCaseFilter together.  It divides text at non-letters and converts
* them to lower case.  While it is functionally equivalent to the combination
* of LetterTokenizer and LowerCaseFilter, there is a performance advantage
* to doing the two tasks at once, hence this (redundant) implementation.
* <P>
* Note: this does a decent job for most European languages, but does a terrible
* job for some Asian languages, where words are not separated by spaces.
*/
class LowerCaseTokenizer:public LetterTokenizer {
public:
	/** Construct a new LowerCaseTokenizer. */
	LowerCaseTokenizer(CL_NS(util)::Reader* in):
	LetterTokenizer(in) {}

    ~LowerCaseTokenizer(){}
protected:
	/** Collects only characters which satisfy _totlower. */
	TCHAR normalize(const TCHAR chr) const;
};


/** A WhitespaceTokenizer is a tokenizer that divides text at whitespace.
 * Adjacent sequences of non-Whitespace characters form tokens. */
class WhitespaceTokenizer: public CharTokenizer {
public:
	/** Construct a new WhitespaceTokenizer. */ 
	WhitespaceTokenizer(CL_NS(util)::Reader* in):CharTokenizer(in) {}
	~WhitespaceTokenizer(){}
protected:
	/** Collects only characters which do not satisfy _istspace.
	*/
	bool isTokenChar(const TCHAR c) const;
};


/** An Analyzer that uses WhitespaceTokenizer. */
class WhitespaceAnalyzer: public Analyzer {
 public:
  TokenStream* tokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader);
  ~WhitespaceAnalyzer(){}
};

/** An Analyzer that filters LetterTokenizer with LowerCaseFilter. */
class SimpleAnalyzer: public Analyzer {
public:
	TokenStream* tokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader);
	~SimpleAnalyzer(){}
};



/**
* Normalizes token text to lower case.
*/
class LowerCaseFilter: public TokenFilter {
public:
	LowerCaseFilter(TokenStream* in, bool deleteTokenStream):TokenFilter(in,deleteTokenStream) {}
	~LowerCaseFilter(){}
	bool next(Token* token);
};


/**
 * Removes stop words from a token stream.
 */
class StopFilter: public TokenFilter {
private:
	//bvk: i found this to work faster with a non-hash table. the number of items
	//in the stop table is not like to make it worth having hashing.
	//ish: implement a radix/patricia tree for this?
	CL_NS(util)::CLSetList<const TCHAR*> stopWords;

	bool enablePositionIncrements;
	const bool ignoreCase;
public:
	static bool ENABLE_POSITION_INCREMENTS_DEFAULT;

	// Constructs a filter which removes words from the input
	//	TokenStream that are named in the array of words. 
	StopFilter(TokenStream* in, bool deleteTokenStream, const TCHAR** _stopWords, const bool _ignoreCase = false);

	~StopFilter(){}

	/** Constructs a filter which removes words from the input
	*	TokenStream that are named in the CLSetList.
	*/
	StopFilter(TokenStream* in, bool deleteTokenStream, CL_NS(util)::CLSetList<const TCHAR*>* stopTable):
		TokenFilter(in, deleteTokenStream),
		stopWords (*stopTable),
		enablePositionIncrements(ENABLE_POSITION_INCREMENTS_DEFAULT),
		ignoreCase(false)
		{
		}
	  
	
	/**
	* Builds a Hashtable from an array of stop words, appropriate for passing
	* into the StopFilter constructor.  This permits this table construction to
	* be cached once when an Analyzer is constructed. 
	* Note: the stopWords list must be a static list because the strings are not copied
	*/
	static void fillStopTable(CL_NS(util)::CLSetList<const TCHAR*>* stopTable,
                                      const TCHAR** stopWords, const bool _ignoreCase = false);

	/**
	* Returns the next input Token whose termText() is not a stop word.
	*/ 
	bool next(Token* token);


	/**
	* @see #setEnablePositionIncrementsDefault(boolean). 
	*/
	static bool getEnablePositionIncrementsDefault() {
		return ENABLE_POSITION_INCREMENTS_DEFAULT;
	}

	/**
	* Set the default position increments behavior of every StopFilter created from now on.
	* <p>
	* Note: behavior of a single StopFilter instance can be modified 
	* with {@link #setEnablePositionIncrements(boolean)}.
	* This static method allows control over behavior of classes using StopFilters internally, 
	* for example {@link org.apache.lucene.analysis.standard.StandardAnalyzer StandardAnalyzer}. 
	* <p>
	* Default : false.
	* @see #setEnablePositionIncrements(boolean).
	*/
	static void setEnablePositionIncrementsDefault(const bool defaultValue) {
		ENABLE_POSITION_INCREMENTS_DEFAULT = defaultValue;
	}

	/**
	* @see #setEnablePositionIncrements(boolean). 
	*/
	bool getEnablePositionIncrements() const { return enablePositionIncrements; }

	/**
	* Set to <code>true</code> to make <b>this</b> StopFilter enable position increments to result tokens.
	* <p>
	* When set, when a token is stopped (omitted), the position increment of 
	* the following token is incremented.  
	* <p>
	* Default: see {@link #setEnablePositionIncrementsDefault(boolean)}.
	*/
	void setEnablePositionIncrements(const bool enable) { this->enablePositionIncrements = enable; }

};

/**
 * Loader for text files that represent a list of stopwords.
 *
 */
class WordlistLoader {
public:
	/**
	* Loads a text file and adds every line as an entry to a HashSet (omitting
	* leading and trailing whitespace). Every line of the file should contain only
	* one word. The words need to be in lowercase if you make use of an
	* Analyzer which uses LowerCaseFilter (like StandardAnalyzer).
	*
	* @param wordfile File containing the wordlist
	* @return A HashSet with the file's words
	*/
	static CL_NS(util)::CLSetList<const TCHAR*>* getWordSet(const char* wordfilePath, const char* enc = "ASCII", CL_NS(util)::CLSetList<const TCHAR*>* stopTable = NULL)
	{
		CL_NS(util)::FileReader* reader = NULL;
		try {
			reader = _CLNEW CL_NS(util)::FileReader(wordfilePath, enc, LUCENE_DEFAULT_TOKEN_BUFFER_SIZE);
			stopTable = getWordSet(reader, stopTable);
		}
		_CLFINALLY (
			if (reader != NULL) {
				//reader->close();
				_CLLDELETE(reader);
			}
		);
		return stopTable;
	}

	/**
	* Reads lines from a Reader and adds every line as an entry to a HashSet (omitting
	* leading and trailing whitespace). Every line of the Reader should contain only
	* one word. The words need to be in lowercase if you make use of an
	* Analyzer which uses LowerCaseFilter (like StandardAnalyzer).
	*
	* @param reader Reader containing the wordlist
	* @return A HashSet with the reader's words
	*/
	static CL_NS(util)::CLSetList<const TCHAR*>* getWordSet(CL_NS(util)::Reader* reader, CL_NS(util)::CLSetList<const TCHAR*>* stopTable = NULL, const bool bDeleteReader = false)
	{
		if (!stopTable)
			stopTable = _CLNEW CL_NS(util)::CLSetList<const TCHAR*>(false);

		TCHAR* word = NULL;
		try {
			word = _CL_NEWARRAY(TCHAR, LUCENE_DEFAULT_TOKEN_BUFFER_SIZE);
			while (reader->readLine(word) > 0) {
				stopTable->insert( STRDUP_TtoT(CL_NS(util)::Misc::stringTrim(word)));
			}
		}
		_CLFINALLY (
			if (bDeleteReader && reader != NULL) {
				//reader->close();
				_CLDELETE(reader);
			}
			_CLDELETE_ARRAY(word);
		);
		return stopTable;
	}
};


/** Filters LetterTokenizer with LowerCaseFilter and StopFilter. */
class StopAnalyzer: public Analyzer {
    CL_NS(util)::CLSetList<const TCHAR*> stopTable;

public:
    /** Builds an analyzer which removes words in ENGLISH_STOP_WORDS. */
    StopAnalyzer();
    ~StopAnalyzer();
    
    /** Builds an analyzer which removes words in the provided array. */
    StopAnalyzer( const TCHAR** stopWords );

	/** Builds an analyzer with the stop words from the given file.
	* @see WordlistLoader#getWordSet(File)
	*/
	StopAnalyzer(const char* stopwordsFile, const char* enc = "ASCII") {
		WordlistLoader::getWordSet(stopwordsFile, enc, &stopTable);
	}

	/** Builds an analyzer with the stop words from the given reader.
	* @see WordlistLoader#getWordSet(Reader)
	*/
	StopAnalyzer(CL_NS(util)::Reader* stopwordsReader, const bool _bDeleteReader = false) {
		WordlistLoader::getWordSet(stopwordsReader, &stopTable, _bDeleteReader);
	}

    /** Filters LowerCaseTokenizer with StopFilter. */
    TokenStream* tokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader);
	
	/** An array containing some common English words that are not usually useful
    for searching. */
    static const TCHAR* ENGLISH_STOP_WORDS[];
};



/**
 * This analyzer is used to facilitate scenarios where different
 * fields require different analysis techniques.  Use {@link #addAnalyzer}
 * to add a non-default analyzer on a field name basis.
 * 
 * <p>Example usage:
 * 
 * <pre>
 *   PerFieldAnalyzerWrapper aWrapper =
 *      new PerFieldAnalyzerWrapper(new StandardAnalyzer());
 *   aWrapper.addAnalyzer("firstname", new KeywordAnalyzer());
 *   aWrapper.addAnalyzer("lastname", new KeywordAnalyzer());
 * </pre>
 * 
 * <p>In this example, StandardAnalyzer will be used for all fields except "firstname"
 * and "lastname", for which KeywordAnalyzer will be used.
 * 
 * <p>A PerFieldAnalyzerWrapper can be used like any other analyzer, for both indexing
 * and query parsing.
 */
class PerFieldAnalyzerWrapper : public Analyzer {
private:
    Analyzer* defaultAnalyzer;
    CL_NS(util)::CLHashMap<const TCHAR*, Analyzer*, CL_NS(util)::Compare::TChar,
    CL_NS(util)::Equals::TChar, CL_NS(util)::Deletor::tcArray,CL_NS(util)::Deletor::Void<Analyzer> > analyzerMap;
public:
    /**
    * Constructs with default analyzer.
    *
    * @param defaultAnalyzer Any fields not specifically
    * defined to use a different analyzer will use the one provided here.
    */
    PerFieldAnalyzerWrapper(Analyzer* defaultAnalyzer);
    ~PerFieldAnalyzerWrapper();
    
    /**
    * Defines an analyzer to use for the specified field.
    *
    * @param fieldName field name requiring a non-default analyzer
    * @param analyzer non-default analyzer to use for field
    */
    void addAnalyzer(const TCHAR* fieldName, Analyzer* analyzer);
    TokenStream* tokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader);
};


/**
 * A filter that replaces accented characters in the ISO Latin 1 character set 
 * (ISO-8859-1) by their unaccented equivalent. The case will not be altered.
 * <p>
 * For instance, '&agrave;' will be replaced by 'a'.
 * <p>
 */
class ISOLatin1AccentFilter: public TokenFilter {
public:
	ISOLatin1AccentFilter(TokenStream* input, bool deleteTs):
		TokenFilter(input,deleteTs)
	{
	}
	
	/**
	 * To replace accented characters in a String by unaccented equivalents.
	 */
	bool next(Token* token);
};


/**
 * Emits the entire input as a single token.
 */
class KeywordTokenizer: public Tokenizer {
private:
    LUCENE_STATIC_CONSTANT(int, DEFAULT_BUFFER_SIZE = 256);
    bool done;
    int bufferSize;
public:
    KeywordTokenizer(CL_NS(util)::Reader* input, int bufferSize=-1);
    virtual ~KeywordTokenizer();
    bool next(Token* token);
};

/**
 * "Tokenizes" the entire stream as a single token. This is useful
 * for data like zip codes, ids, and some product names.
 */
class KeywordAnalyzer: public Analyzer {
public:
    TokenStream* tokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader);
    virtual ~KeywordAnalyzer(){}
};

    
/**
 * Removes words that are too long and too short from the stream.
 *
 */
class LengthFilter: public TokenFilter {
private:
    size_t _min;
    size_t _max;
public:
    /**
    * Build a filter that removes words that are too long or too
    * short from the text.
    */
    LengthFilter(TokenStream* in, const size_t _min, const size_t _max);
    
    /**
    * Returns the next input Token whose termText() is the right len
    */
    bool next(Token* token);
};

CL_NS_END
#endif
