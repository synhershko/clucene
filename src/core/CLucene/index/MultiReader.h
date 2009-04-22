/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_index_MultiReader
#define _lucene_index_MultiReader


//#include "SegmentHeader.h"
#include "IndexReader.h"
CL_CLASS_DEF(document,Document)
//#include "Terms.h"
//#include "SegmentMergeQueue.h"

CL_NS_DEF(index)

/** An IndexReader which reads multiple indexes, appending their content.
*/
class CLUCENE_EXPORT MultiReader:public IndexReader{
private:
  class Internal;
  Internal* internal;
	int32_t readerIndex(const int32_t n) const;
	bool hasNorms(const TCHAR* field);
	uint8_t* fakeNorms();
  CL_NS(util)::ValueArray<bool> decrefOnClose; //remember which subreaders to decRef on close


protected:
	CL_NS(util)::ObjectArray<IndexReader>* subReaders;
	CL_NS(util)::ValueArray<int32_t> starts;			  // 1st docno for each segment

	void doSetNorm(int32_t n, const TCHAR* field, uint8_t value);
	void doUndeleteAll();
	void doCommit();
	void doClose();
	void doDelete(const int32_t n);
public:
  /**
   * <p>Construct a MultiReader aggregating the named set of (sub)readers.
   * Directory locking for delete, undeleteAll, and setNorm operations is
   * left to the subreaders. </p>
   * @param closeSubReaders indicates whether the subreaders should be closed
   * when this MultiReader is closed
   * @param subReaders set of (sub)readers
   * @throws IOException
   */
	MultiReader(CL_NS(store)::Directory* directory, SegmentInfos* sis, IndexReader** subReaders);

	/**
	* <p>Construct a MultiReader aggregating the named set of (sub)readers.
	* Directory locking for delete, undeleteAll, and setNorm operations is
	* left to the subreaders. </p>
	* <p>Note that all subreaders are closed if this Multireader is closed.</p>
	* @param subReaders set of (sub)readers
	* @throws IOException
	*/
	MultiReader(CL_NS(util)::ObjectArray<IndexReader>* subReaders, bool closeSubReaders=true);

	~MultiReader();


  /**
   * Tries to reopen the subreaders.
   * <br>
   * If one or more subreaders could be re-opened (i. e. subReader.reopen()
   * returned a new instance != subReader), then a new MultiReader instance
   * is returned, otherwise this instance is returned.
   * <p>
   * A re-opened instance might share one or more subreaders with the old
   * instance. Index modification operations result in undefined behavior
   * when performed before the old instance is closed.
   * (see {@link IndexReader#reopen()}).
   * <p>
   * If subreaders are shared, then the reference count of those
   * readers is increased to ensure that the subreaders remain open
   * until the last referring reader is closed.
   *
   * @throws CorruptIndexException if the index is corrupt
   * @throws IOException if there is a low-level IO error
   */
  public IndexReader reopen() throws CorruptIndexException, IOException {
    ensureOpen();

    boolean reopened = false;
    IndexReader[] newSubReaders = new IndexReader[subReaders.length];
    boolean[] newDecrefOnClose = new boolean[subReaders.length];

    boolean success = false;
    try {
      for (int i = 0; i < subReaders.length; i++) {
        newSubReaders[i] = subReaders[i].reopen();
        // if at least one of the subreaders was updated we remember that
        // and return a new MultiReader
        if (newSubReaders[i] != subReaders[i]) {
          reopened = true;
          // this is a new subreader instance, so on close() we don't
          // decRef but close it
          newDecrefOnClose[i] = false;
        }
      }

      if (reopened) {
        for (int i = 0; i < subReaders.length; i++) {
          if (newSubReaders[i] == subReaders[i]) {
            newSubReaders[i].incRef();
            newDecrefOnClose[i] = true;
          }
        }

        MultiReader mr = new MultiReader(newSubReaders);
        mr.decrefOnClose = newDecrefOnClose;
        success = true;
        return mr;
      } else {
        success = true;
        return this;
      }
    } finally {
      if (!success && reopened) {
        for (int i = 0; i < newSubReaders.length; i++) {
          if (newSubReaders[i] != null) {
            try {
              if (newDecrefOnClose[i]) {
                newSubReaders[i].decRef();
              } else {
                newSubReaders[i].close();
              }
            } catch (IOException ignore) {
              // keep going - we want to clean up as much as possible
            }
          }
        }
      }
    }
  }

	/** Return an array of term frequency vectors for the specified document.
	*  The array contains a vector for each vectorized field in the document.
	*  Each vector vector contains term numbers and frequencies for all terms
	*  in a given vectorized field.
	*  If no such fields existed, the method returns null.
	*/
	CL_NS(util)::ObjectArray<TermFreqVector>* getTermFreqVectors(int32_t n);
	TermFreqVector* getTermFreqVector(int32_t n, const TCHAR* field);


  public void getTermFreqVector(int docNumber, String field, TermVectorMapper mapper) throws IOException {
    ensureOpen();
    int i = readerIndex(docNumber);        // find segment num
    subReaders[i].getTermFreqVector(docNumber - starts[i], field, mapper);
  }

  public void getTermFreqVector(int docNumber, TermVectorMapper mapper) throws IOException {
    ensureOpen();
    int i = readerIndex(docNumber);        // find segment num
    subReaders[i].getTermFreqVector(docNumber - starts[i], mapper);
  }

  public boolean isOptimized() {
    return false;
  }

	int32_t numDocs();
	int32_t maxDoc() const;
	bool document(int32_t n, CL_NS(document)::Document& doc, FieldSelector* fieldSelector);
	bool isDeleted(const int32_t n);
	bool hasDeletions() const;
	uint8_t* norms(const TCHAR* field);
	void norms(const TCHAR* field, uint8_t* result);
	TermEnum* terms() const;
	TermEnum* terms(const Term* term) const;
	
	//Returns the document frequency of the current term in the set
	int32_t docFreq(const Term* t=NULL) const;
	TermDocs* termDocs() const;
	TermPositions* termPositions() const;
	
	/**
	* @see IndexReader#getFieldNames(IndexReader.FieldOption fldOption)
	*/
	void getFieldNames(FieldOption fldOption, StringArrayWithDeletor& retarray);


  /**
   * Checks recursively if all subreaders are up to date.
   */
  public boolean isCurrent() throws CorruptIndexException, IOException {
    for (int i = 0; i < subReaders.length; i++) {
      if (!subReaders[i].isCurrent()) {
        return false;
      }
    }

    // all subreaders are up to date
    return true;
  }

  /** Not implemented.
   * @throws UnsupportedOperationException
   */
  public long getVersion() {
    throw new UnsupportedOperationException("MultiReader does not support this method.");
  }

  // for testing
  IndexReader[] getSubReaders() {
    return subReaders;
  }
};


CL_NS_END
#endif
