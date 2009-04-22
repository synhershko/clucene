/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_index_MultiSegmentReader
#define _lucene_index_MultiSegmentReader


//#include "SegmentHeader.h"
#include "IndexReader.h"
CL_CLASS_DEF(document,Document)
//#include "Terms.h"
//#include "SegmentMergeQueue.h"

CL_NS_DEF(index)

class MultiSegmentReader:public DirectoryIndexReader{
private:
  class Internal;
  Internal* internal;
	int32_t readerIndex(const int32_t n, int32_t* starts, int32_t numSubReaders) const;
  private int readerIndex(int32_t n) const {    // find reader for doc n:
    return readerIndex(n, this.starts, this.subReaders.length);
  }
	bool hasNorms(const TCHAR* field);
	uint8_t* fakeNorms();


  void startCommit() {
    super.startCommit();
    for (int i = 0; i < subReaders.length; i++) {
      subReaders[i].startCommit();
    }
  }

  void rollbackCommit() {
    super.rollbackCommit();
    for (int i = 0; i < subReaders.length; i++) {
      subReaders[i].rollbackCommit();
    }
  }

protected:
	CL_NS(util)::ObjectArray<IndexReader>* subReaders;
	int32_t* starts;			  // 1st docno for each segment

	void doSetNorm(int32_t n, const TCHAR* field, uint8_t value);
	void doUndeleteAll();
	void commitChanges();
	// synchronized
	void doClose();
	
	// synchronized
	void doDelete(const int32_t n);
public:
  /** Construct reading the named set of readers. */
   MultiSegmentReader(CL_NS(store)::Directory* directory, SegmentInfos* sis, boolean closeDirectory){
    super(directory, sis, closeDirectory);
    // To reduce the chance of hitting FileNotFound
    // (and having to retry), we open segments in
    // reverse because IndexWriter merges & deletes
    // the newest segments first.

    SegmentReader[] readers = new SegmentReader[sis.size()];
    for (int i = sis.size()-1; i >= 0; i--) {
      try {
        readers[i] = SegmentReader.get(sis.info(i));
      } catch (IOException e) {
        // Close all readers we had opened:
        for(i++;i<sis.size();i++) {
          try {
            readers[i].close();
          } catch (IOException ignore) {
            // keep going - we want to clean up as much as possible
          }
        }
        throw e;
      }
    }

    initialize(readers);
  }

  /** This contructor is only used for {@link #reopen()} */
  MultiSegmentReader(CL_NS(store)::Directory* directory, SegmentInfos* sis, ObjectArray<SegmentReader>* subReaders, boolean closeDirectory, SegmentReader[] oldReaders, int[] oldStarts, Map oldNormsCache)  {
    super(directory, infos, closeDirectory);

    // we put the old SegmentReaders in a map, that allows us
    // to lookup a reader using its segment name
    Map segmentReaders = new HashMap();

    if (oldReaders != null) {
      // create a Map SegmentName->SegmentReader
      for (int i = 0; i < oldReaders.length; i++) {
        segmentReaders.put(oldReaders[i].getSegmentName(), new Integer(i));
      }
    }

    SegmentReader[] newReaders = new SegmentReader[infos.size()];

    // remember which readers are shared between the old and the re-opened
    // MultiSegmentReader - we have to incRef those readers
    boolean[] readerShared = new boolean[infos.size()];

    for (int i = infos.size() - 1; i>=0; i--) {
      // find SegmentReader for this segment
      Integer oldReaderIndex = (Integer) segmentReaders.get(infos.info(i).name);
      if (oldReaderIndex == null) {
        // this is a new segment, no old SegmentReader can be reused
        newReaders[i] = null;
      } else {
        // there is an old reader for this segment - we'll try to reopen it
        newReaders[i] = oldReaders[oldReaderIndex.intValue()];
      }

      boolean success = false;
      try {
        SegmentReader newReader;
        if (newReaders[i] == null || infos.info(i).getUseCompoundFile() != newReaders[i].getSegmentInfo().getUseCompoundFile()) {
          // this is a new reader; in case we hit an exception we can close it safely
          newReader = SegmentReader.get(infos.info(i));
        } else {
          newReader = (SegmentReader) newReaders[i].reopenSegment(infos.info(i));
        }
        if (newReader == newReaders[i]) {
          // this reader will be shared between the old and the new one,
          // so we must incRef it
          readerShared[i] = true;
          newReader.incRef();
        } else {
          readerShared[i] = false;
          newReaders[i] = newReader;
        }
        success = true;
      } finally {
        if (!success) {
          for (i++; i < infos.size(); i++) {
            if (newReaders[i] != null) {
              try {
                if (!readerShared[i]) {
                  // this is a new subReader that is not used by the old one,
                  // we can close it
                  newReaders[i].close();
                } else {
                  // this subReader is also used by the old reader, so instead
                  // closing we must decRef it
                  newReaders[i].decRef();
                }
              } catch (IOException ignore) {
                // keep going - we want to clean up as much as possible
              }
            }
          }
        }
      }
    }

    // initialize the readers to calculate maxDoc before we try to reuse the old normsCache
    initialize(newReaders);

    // try to copy unchanged norms from the old normsCache to the new one
    if (oldNormsCache != null) {
      Iterator it = oldNormsCache.keySet().iterator();
      while (it.hasNext()) {
        String field = (String) it.next();
        if (!hasNorms(field)) {
          continue;
        }

        byte[] oldBytes = (byte[]) oldNormsCache.get(field);

        byte[] bytes = new byte[maxDoc()];

        for (int i = 0; i < subReaders.length; i++) {
          Integer oldReaderIndex = ((Integer) segmentReaders.get(subReaders[i].getSegmentName()));

          // this SegmentReader was not re-opened, we can copy all of its norms
          if (oldReaderIndex != null &&
               (oldReaders[oldReaderIndex.intValue()] == subReaders[i]
                 || oldReaders[oldReaderIndex.intValue()].norms.get(field) == subReaders[i].norms.get(field))) {
            // we don't have to synchronize here: either this constructor is called from a SegmentReader,
            // in which case no old norms cache is present, or it is called from MultiReader.reopen(),
            // which is synchronized
            System.arraycopy(oldBytes, oldStarts[oldReaderIndex.intValue()], bytes, starts[i], starts[i+1] - starts[i]);
          } else {
            subReaders[i].norms(field, bytes, starts[i]);
          }
        }

        normsCache.put(field, bytes);      // update cache
      }
    }
  }

  private void initialize(SegmentReader[] subReaders) {
    this.subReaders = subReaders;
    starts = new int[subReaders.length + 1];    // build starts array
    for (int i = 0; i < subReaders.length; i++) {
      starts[i] = maxDoc;
      maxDoc += subReaders[i].maxDoc();      // compute maxDocs

      if (subReaders[i].hasDeletions())
        hasDeletions = true;
    }
    starts[subReaders.length] = maxDoc;
  }

  protected synchronized DirectoryIndexReader doReopen(SegmentInfos infos) throws CorruptIndexException, IOException {
    if (infos.size() == 1) {
      // The index has only one segment now, so we can't refresh the MultiSegmentReader.
      // Return a new SegmentReader instead
      SegmentReader newReader = SegmentReader.get(infos, infos.info(0), false);
      return newReader;
    } else {
      return new MultiSegmentReader(directory, infos, closeDirectory, subReaders, starts, normsCache);
    }
  }

	~MultiSegmentReader();

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
  

	// synchronized
	int32_t numDocs();

	int32_t maxDoc() const;

	bool document(int32_t n, CL_NS(document)::Document& doc, FieldSelector* fieldSelector);

	bool isDeleted(const int32_t n);
	bool hasDeletions() const;

	// synchronized
	uint8_t* norms(const TCHAR* field);
	void norms(const TCHAR* field, uint8_t* result);

	TermEnum* terms() const;
	TermEnum* terms(const Term* term) const;
	
	//Returns the document frequency of the current term in the set
	int32_t docFreq(const Term* t=NULL) const;
	TermDocs* termDocs() const;
	TermPositions* termPositions() const;

  public Collection getFieldNames (IndexReader.FieldOption fieldNames) {
    ensureOpen();
    return getFieldNames(fieldNames, this.subReaders);
  }
	static void getFieldNames(FieldOption fldOption, StringArrayWithDeletor& retarray, ObjectArray<IndexReader>* subReaders);


  public void setTermInfosIndexDivisor(int indexDivisor) throws IllegalStateException {
    for (int i = 0; i < subReaders.length; i++)
      subReaders[i].setTermInfosIndexDivisor(indexDivisor);
  }

  public int getTermInfosIndexDivisor() throws IllegalStateException {
    if (subReaders.length > 0)
      return subReaders[0].getTermInfosIndexDivisor();
    else
      throw new IllegalStateException("no readers");
  }

};


class MultiTermDocs:public virtual TermDocs {
protected:
  CL_NS(util)::ObjectArray<TermDocs>* readerTermDocs;

  CL_NS(util)::ObjectArray<SegmentReader>* subReaders;
  const int32_t* starts;
  Term* term;

  int32_t base;
  int32_t pointer;

  TermDocs* current;              // == segTermDocs[pointer]
  TermDocs* termDocs(const int32_t i) const; //< internal use only
  virtual TermDocs* termDocs(const IndexReader* reader) const;
public:
  MultiTermDocs();
  MultiTermDocs(ObjectArray<SegmentReader>* subReaders, const int32_t* s);
  virtual ~MultiTermDocs();

  int32_t doc() const;
  int32_t freq() const;

    void seek(TermEnum* termEnum);
  void seek(Term* tterm);
  bool next();

  /** Optimized implementation. */
  int32_t read(int32_t* docs, int32_t* freqs, int32_t length);

   /* A Possible future optimization could skip entire segments */ 
  bool skipTo(const int32_t target);

  void close();

  virtual TermPositions* __asTermPositions();
};


//MultiTermEnum represents the enumeration of all terms of all readers
class MultiTermEnum:public TermEnum {
private:
  SegmentMergeQueue* queue;

  Term* _term;
  int32_t _docFreq;
public:
  //Constructor
  //Opens all enumerations of all readers
  MultiTermEnum(ObjectArray<SegmentReader>* subReaders, const int32_t* starts, const Term* t);

  //Destructor
  ~MultiTermEnum();

  //Move the current term to the next in the set of enumerations
  bool next();

  //Returns a pointer to the current term of the set of enumerations
  Term* term();
  Term* term(bool pointer);

  //Returns the document frequency of the current term in the set
  int32_t docFreq() const;

  //Closes the set of enumerations in the queue
  void close();


  const char* getObjectName(){ return MultiTermEnum::getClassName(); }
  static const char* getClassName(){ return "MultiTermEnum"; }
};


#if _MSC_VER
    #pragma warning(disable : 4250)
#endif
class MultiTermPositions:public MultiTermDocs,public TermPositions {
protected:
  TermDocs* termDocs(const IndexReader* reader) const;
public:
  MultiTermPositions(ObjectArray<SegmentReader>* subReaders, const int32_t* s);
  ~MultiTermPositions() {};
  int32_t nextPosition();

  /**
  * Not implemented.
  * @throws UnsupportedOperationException
  */
  int32_t getPayloadLength() const;

  /**
  * Not implemented.
  * @throws UnsupportedOperationException
  */
  uint8_t* getPayload(uint8_t* data, const int32_t offset);

  /**
  *
  * @return false
  */
  // TODO: Remove warning after API has been finalized
  bool isPayloadAvailable() const;

  virtual TermDocs* __asTermDocs();
  virtual TermPositions* __asTermPositions();
};

CL_NS_END
#endif
