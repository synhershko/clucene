/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_search_FuzzyQuery_
#define _lucene_search_FuzzyQuery_


//#include "CLucene/index/IndexReader.h"
CL_CLASS_DEF(index,Term)
//#include "MultiTermQuery.h"
#include "MultiTermQuery.h"
#include "FilteredTermEnum.h"


CL_NS_DEF(search)

  // class FuzzyQuery implements the fuzzy search query
  class CLUCENE_EXPORT FuzzyQuery: public MultiTermQuery {
    private:
	  float_t minimumSimilarity;
	  size_t prefixLength;
  protected:
	  FuzzyQuery(const FuzzyQuery& clone);
   public:
	  static float_t defaultMinSimilarity;
	  static int32_t defaultPrefixLength;

     /**
	* Create a new FuzzyQuery that will match terms with a similarity 
	* of at least <code>minimumSimilarity</code> to <code>term</code>.
	* If a <code>prefixLength</code> &gt; 0 is specified, a common prefix
	* of that length is also required.
	* 
	* @param term the term to search for
	* @param minimumSimilarity a value between 0 and 1 to set the required similarity
	*  between the query term and the matching terms. For example, for a
	*  <code>minimumSimilarity</code> of <code>0.5</code> a term of the same length
	*  as the query term is considered similar to the query term if the edit distance
	*  between both terms is less than <code>length(term)*0.5</code>
	* @param prefixLength length of common (non-fuzzy) prefix
	* @throws IllegalArgumentException if minimumSimilarity is &gt; 1 or &lt; 0
	* or if prefixLength &lt; 0 or &gt; <code>term.text().length()</code>.
	*/
     FuzzyQuery(CL_NS(index)::Term* term, float_t minimumSimilarity=-1, size_t prefixLength=0);
	 //Destructor
     ~FuzzyQuery();

     TCHAR* toString(const TCHAR* field) const;

	 //Returns the name "FuzzyQuery"
	 static const char* getClassName();
     const char* getQueryName() const;

	  Query* clone() const;
	  bool equals(Query * other) const;
	  size_t hashCode() const;

	  /**
		* Returns the minimum similarity that is required for this query to match.
		* @return float value between 0.0 and 1.0
		*/
		float_t getMinSimilarity() const;

		/**
		* Returns the prefix length, i.e. the number of characters at the start
		* of a term that must be identical (not fuzzy) to the query term if the query
		* is to match that term. 
		*/
		size_t getPrefixLength() const;

		//Query* FuzzyQuery::rewrite(IndexReader* reader)

  protected:
	  FilteredTermEnum* getEnum(CL_NS(index)::IndexReader* reader);
  };

/** Subclass of FilteredTermEnum for enumerating all terms that are similiar
 * to the specified filter term.
 *
 * <p>Term enumerations are always ordered by Term.compareTo().  Each term in
 * the enumeration is greater than all that precede it.
 */
class CLUCENE_EXPORT FuzzyTermEnum: public FilteredTermEnum {
  private:
		float_t distance;
		bool _endEnum;

		CL_NS(index)::Term* searchTerm; 
		TCHAR* text;
		size_t textLen;
		TCHAR* prefix;
		size_t prefixLength;
		float_t minimumSimilarity;
		double scale_factor;

		
		/**
		* This static array saves us from the time required to create a new array
		* everytime editDistance is called.
		*/
		int32_t* e;
		int32_t eWidth;
		int32_t eHeight;

		/******************************
		* Compute Levenshtein distance
		******************************/
 
		/**
		Levenshtein distance also known as edit distance is a measure of similiarity
		between two strings where the distance is measured as the number of character 
		deletions, insertions or substitutions required to transform one string to 
		the other string. 
		<p>This method takes in four parameters; two strings and their respective 
		lengths to compute the Levenshtein distance between the two strings.
		The result is returned as an integer.
		*/ 
		int32_t editDistance(const TCHAR* s, const TCHAR* t, const int32_t n, const int32_t m) ;

    protected:
		/**
		* The termCompare method in FuzzyTermEnum uses Levenshtein distance to 
		* calculate the distance between the given term and the comparing term. 
		*/
		bool termCompare(CL_NS(index)::Term* term) ;
		
		///Returns the fact if the current term in the enumeration has reached the end
		bool endEnum();
    public:
		
		/**
		* Empty prefix and minSimilarity of 0.5f are used.
		* 
		* @param reader
		* @param term
		* @throws IOException
		* @see #FuzzyTermEnum(IndexReader, Term, float_t, int32_t)
		*/
		FuzzyTermEnum(const CL_NS(index)::IndexReader* reader, CL_NS(index)::Term* term, float_t minSimilarity=FuzzyQuery::defaultMinSimilarity, size_t prefixLength=0);
		/** Destructor */
		~FuzzyTermEnum();
		/** Close the enumeration */
		void close();
		
		/** Returns the difference between the distance and the fuzzy threshold
		*  multiplied by the scale factor
		*/
		float_t difference();

		
		const char* getObjectName(){ return FuzzyTermEnum::getClassName(); }
		static const char* getClassName(){ return "FuzzyTermEnum"; }
  };
CL_NS_END
#endif
