/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_index_SegmentHeader_
#define _lucene_index_SegmentHeader_

#include "_SegmentInfos.h"
#include "CLucene/util/BitSet.h"
//#include "CLucene/util/VoidMap.h"
#include "CLucene/store/IndexInput.h"
#include "CLucene/store/IndexOutput.h"
#include "CLucene/index/IndexReader.h"
#include "Term.h"
#include "Terms.h"
#include "_TermInfo.h"
//#include "FieldInfos.h"
#include "_FieldsReader.h"
#include "_TermVector.h"
//#include "IndexReader.h"
#include "_TermInfosReader.h"
#include "_CompoundFile.h"
#include "DefaultSkipListReader.h"
#include "CLucene/util/_ThreadLocal.h"

CL_NS_DEF(index)
class SegmentReader;

class SegmentTermDocs:public virtual TermDocs {
protected:
	const SegmentReader* parent;
	CL_NS(store)::IndexInput* freqStream;
	int32_t count;
	int32_t df;
	CL_NS(util)::BitSet* deletedDocs;
	int32_t _doc;
	int32_t _freq;
	
private:
	int32_t skipInterval;
	int32_t maxSkipLevels;
	DefaultSkipListReader* skipListReader;
	
	int64_t freqBasePointer;
	int64_t proxBasePointer;

	int64_t skipPointer;
	bool haveSkipped;

protected:
	bool currentFieldStoresPayloads;

public:
	///\param Parent must be a segment reader
	SegmentTermDocs( const SegmentReader* Parent);
    virtual ~SegmentTermDocs();

	virtual void seek(Term* term);
    virtual void seek(TermEnum* termEnum);
	virtual void seek(const TermInfo* ti,Term* term);

	virtual void close();
	virtual int32_t doc()const;
	virtual int32_t freq()const;

	virtual bool next();

	/** Optimized implementation. */
	virtual int32_t read(int32_t* docs, int32_t* freqs, int32_t length);

	/** Optimized implementation. */
	virtual bool skipTo(const int32_t target);
	
	virtual TermPositions* __asTermPositions();

protected:
	virtual void skippingDoc(){}
	virtual void skipProx(const int64_t proxPointer, const int32_t payloadLength){}
};


class SegmentTermPositions: public SegmentTermDocs, public TermPositions {
private:
	CL_NS(store)::IndexInput* proxStream;
	int32_t proxCount;
	int32_t position;

	// the current payload length
	int32_t payloadLength;
	// indicates whether the payload of the currend position has
	// been read from the proxStream yet
	bool needToLoadPayload;
	
	// these variables are being used to remember information
	// for a lazy skip
	int64_t lazySkipPointer;
	int32_t lazySkipProxCount;

public:
	///\param Parent must be a segment reader
	SegmentTermPositions(const SegmentReader* Parent);
	~SegmentTermPositions();

private:
	void seek(const TermInfo* ti, Term* term);

public:
	void close();

	int32_t nextPosition();
private:
	int32_t readDeltaPosition();

protected:
	void skippingDoc();

public:
	bool next();
	int32_t read(int32_t* docs, int32_t* freqs, int32_t length);

protected:
	/** Called by super.skipTo(). */
	void skipProx(const int64_t proxPointer, const int32_t _payloadLength);

private:
	void skipPositions( int32_t n );
	void skipPayload();

	// It is not always neccessary to move the prox pointer
	// to a new document after the freq pointer has been moved.
	// Consider for example a phrase query with two terms:
	// the freq pointer for term 1 has to move to document x
	// to answer the question if the term occurs in that document. But
	// only if term 2 also matches document x, the positions have to be
	// read to figure out if term 1 and term 2 appear next
	// to each other in document x and thus satisfy the query.
	// So we move the prox pointer lazily to the document
	// as soon as positions are requested.
	void lazySkip();
public:
	int32_t getPayloadLength() const;

	uint8_t* getPayload(uint8_t* data, const int32_t offset);

	bool isPayloadAvailable() const;

private:	
	virtual TermDocs* __asTermDocs();
	virtual TermPositions* __asTermPositions();

    //resolve SegmentTermDocs/TermPositions ambiguity
	void seek(Term* term){ SegmentTermDocs::seek(term); }
    void seek(TermEnum* termEnum){ SegmentTermDocs::seek(termEnum); }
    int32_t doc() const{ return SegmentTermDocs::doc(); }
	int32_t freq() const{ return SegmentTermDocs::freq(); }
	bool skipTo(const int32_t target){ return SegmentTermDocs::skipTo(target); }
};




/**
* An IndexReader responsible for reading 1 segment of an index
*/
class SegmentReader: public DirectoryIndexReader {
	/**
	* The class Norm represents the normalizations for a field.
	* These normalizations are read from an IndexInput in into an array of bytes called bytes
	*/
	class Norm :LUCENE_BASE{
    volatile int refCount;
    boolean useSingleNormStream;

    public synchronized void incRef() {
      assert refCount > 0;
      refCount++;
    }

    public synchronized void decRef() throws IOException {
      assert refCount > 0;
      if (refCount == 1) {
        close();
      }
      refCount--;

    }
    
		int32_t number;
		int64_t normSeek;
		SegmentReader* reader;
		const char* segment; ///< pointer to segment name
    private long normSeek;
    private boolean rollbackDirty;
	public:
		CL_NS(store)::IndexInput* in;
		uint8_t* bytes;
		bool dirty;
		//Constructor
		Norm(CL_NS(store)::IndexInput* instrm, bool useSingleNormStream, int number, int64_t normSeek, SegmentReader* reader, const char* segment);
		//Destructor
		~Norm();

		void reWrite(SegmentInfo* si);


    /** Closes the underlying IndexInput for this norm.
     * It is still valid to access all other norm properties after close is called.
     * @throws IOException
     */
    private synchronized void close() throws IOException {
      if (in != null && !useSingleNormStream) {
        in.close();
      }
      in = null;
    }
	};
	friend class SegmentReader::Norm;

	//Holds the name of the segment that is being read
	const char* segment;
  private SegmentInfo si;
  private int readBufferSize;
	
	//Indicates if there are documents marked as deleted
	bool deletedDocsDirty;
	bool normsDirty;
	bool undeleteAll;

  private boolean rollbackDeletedDocsDirty = false;
  private boolean rollbackNormsDirty = false;
  private boolean rollbackUndeleteAll = false;


	//Holds all norms for all fields in the segment
	typedef CL_NS(util)::CLHashtable<const TCHAR*,Norm*,CL_NS(util)::Compare::TChar, CL_NS(util)::Equals::TChar> NormsType;
    NormsType _norms; 
	DEFINE_MUTEX(_norms_LOCK)
    
	uint8_t* ones;
	uint8_t* fakeNorms();

	uint8_t hasSingleNorm;
	CL_NS(store)::IndexInput* singleNormStream;
	
	// Compound File Reader when based on a compound file segment
	CompoundFileReader* cfsReader;
  CompoundFileReader storeCFSReader = null;

  // indicates the SegmentReader with which the resources are being shared,
  // in case this is a re-opened reader
  private SegmentReader referencedSegmentReader = null;

	///Reads the Field Info file
	FieldsReader* fieldsReader;
	TermVectorsReader* termVectorsReaderOrig;
	CL_NS(util)::ThreadLocal<TermVectorsReader*,
		CL_NS(util)::Deletor::Object<TermVectorsReader> >termVectorsLocal;

	void initialize(SegmentInfo* si);

	/**
	* Create a clone from the initial TermVectorsReader and store it in the ThreadLocal.
	* @return TermVectorsReader
	*/
	TermVectorsReader* getTermVectorsReader();

  FieldsReader* getFieldsReader() {
    return fieldsReader;
  }

  FieldInfos getFieldInfos() {
    return fieldInfos;
  }

protected:
	///Marks document docNum as deleted
	void doDelete(const int32_t docNum);
	void doUndeleteAll();
	void commitChanges();
	void doSetNorm(int32_t doc, const TCHAR* field, uint8_t value);

	// can return null if norms aren't stored
	uint8_t* getNorms(const TCHAR* field);
  
public:
/**
   * @throws CorruptIndexException if the index is corrupt
   * @throws IOException if there is a low-level IO error
   */
  public static SegmentReader get(SegmentInfo si) throws CorruptIndexException, IOException {
    return get(si.dir, si, null, false, false, BufferedIndexInput.BUFFER_SIZE, true);
  }

  /**
   * @throws CorruptIndexException if the index is corrupt
   * @throws IOException if there is a low-level IO error
   */
  static SegmentReader get(SegmentInfo si, boolean doOpenStores) throws CorruptIndexException, IOException {
    return get(si.dir, si, null, false, false, BufferedIndexInput.BUFFER_SIZE, doOpenStores);
  }

  /**
   * @throws CorruptIndexException if the index is corrupt
   * @throws IOException if there is a low-level IO error
   */
  public static SegmentReader get(SegmentInfo si, int readBufferSize) throws CorruptIndexException, IOException {
    return get(si.dir, si, null, false, false, readBufferSize, true);
  }

  /**
   * @throws CorruptIndexException if the index is corrupt
   * @throws IOException if there is a low-level IO error
   */
  static SegmentReader get(SegmentInfo si, int readBufferSize, boolean doOpenStores) throws CorruptIndexException, IOException {
    return get(si.dir, si, null, false, false, readBufferSize, doOpenStores);
  }

  /**
   * @throws CorruptIndexException if the index is corrupt
   * @throws IOException if there is a low-level IO error
   */
  public static SegmentReader get(SegmentInfos sis, SegmentInfo si,
                                  boolean closeDir) throws CorruptIndexException, IOException {
    return get(si.dir, si, sis, closeDir, true, BufferedIndexInput.BUFFER_SIZE, true);
  }

  /**
   * @throws CorruptIndexException if the index is corrupt
   * @throws IOException if there is a low-level IO error
   */
  public static SegmentReader get(Directory dir, SegmentInfo si,
                                  SegmentInfos sis,
                                  boolean closeDir, boolean ownDir,
                                  int readBufferSize)
    throws CorruptIndexException, IOException {
    return get(dir, si, sis, closeDir, ownDir, readBufferSize, true);
  }

  /**
   * @throws CorruptIndexException if the index is corrupt
   * @throws IOException if there is a low-level IO error
   */
  public static SegmentReader get(Directory dir, SegmentInfo si,
                                  SegmentInfos sis,
                                  boolean closeDir, boolean ownDir,
                                  int readBufferSize,
                                  boolean doOpenStores)
    throws CorruptIndexException, IOException {
    SegmentReader instance;
    try {
      instance = (SegmentReader)IMPL.newInstance();
    } catch (Exception e) {
      throw new RuntimeException("cannot load SegmentReader class: " + e, e);
    }
    instance.init(dir, sis, closeDir);
    instance.initialize(si, readBufferSize, doOpenStores);
    return instance;
  }






	///Destructor.
	virtual ~SegmentReader();

	///Closes all streams to the files of a single segment
	void doClose();

	///Checks if a segment managed by SegmentInfo si has deletions
	static bool hasDeletions(const SegmentInfo* si);
    bool hasDeletions() const;
	bool hasNorms(const TCHAR* field) const;

	///Returns all file names managed by this SegmentReader
	void files(AStringArrayWithDeletor& retarray);
	///Returns an enumeration of all the Terms and TermInfos in the set.
	TermEnum* terms() const;
	///Returns an enumeration of terms starting at or after the named term t
	TermEnum* terms(const Term* t) const;

	///Gets the document identified by n
	bool document(int32_t n, CL_NS(document)::Document& doc, FieldSelector* fieldSelector);

	///Checks if the n-th document has been marked deleted
	bool isDeleted(const int32_t n);

	///Returns an unpositioned TermDocs enumerator.
	TermDocs* termDocs() const;
	///Returns an unpositioned TermPositions enumerator.
	TermPositions* termPositions() const;

	///Returns the number of documents which contain the term t
	int32_t docFreq(const Term* t) const;

	///Returns the actual number of documents in the segment
	int32_t numDocs();
	///Returns the number of  all the documents in the segment including the ones that have
	///been marked deleted
	int32_t maxDoc() const;


  public void setTermInfosIndexDivisor(int indexDivisor) throws IllegalStateException {
    tis.setIndexDivisor(indexDivisor);
  }

  public int getTermInfosIndexDivisor() {
    return tis.getIndexDivisor();
  }

    ///Returns the bytes array that holds the norms of a named field.
	///Returns fake norms if norms aren't available
    uint8_t* norms(const TCHAR* field);
	
    ///Reads the Norms for field from disk
	void norms(const TCHAR* field, uint8_t* bytes);
	
	///concatenating segment with ext and x
	char* SegmentName(const char* ext, const int32_t x=-1);
    ///Creates a filename in buffer by concatenating segment with ext and x
	void SegmentName(char* buffer,int32_t bufferLen,const char* ext, const int32_t x=-1 );

	/**
	* @see IndexReader#getFieldNames(IndexReader.FieldOption fldOption)
	*/
	void getFieldNames(FieldOption fldOption, StringArrayWithDeletor& retarray);
    
    static bool usesCompoundFile(SegmentInfo* si);

	/** Return a term frequency vector for the specified document and field. The
	*  vector returned contains term numbers and frequencies for all terms in
	*  the specified field of this document, if the field had storeTermVector
	*  flag set.  If the flag was not set, the method returns null.
	* @throws IOException
	*/
    TermFreqVector* getTermFreqVector(int32_t docNumber, const TCHAR* field=NULL);

	/** Return an array of term frequency vectors for the specified document.
	*  The array contains a vector for each vectorized field in the document.
	*  Each vector vector contains term numbers and frequencies for all terms
	*  in a given vectorized field.
	*  If no such fields existed, the method returns null.
	* @throws IOException
	*/
	//bool getTermFreqVectors(int32_t docNumber, CL_NS(util)::ObjectArray<TermFreqVector>& result);
	CL_NS(util)::ObjectArray<TermFreqVector>* getTermFreqVectors(int32_t docNumber);


  public void getTermFreqVector(int docNumber, String field, TermVectorMapper mapper) throws IOException {
    ensureOpen();
    FieldInfo fi = fieldInfos.fieldInfo(field);
    if (fi == null || !fi.storeTermVector || termVectorsReaderOrig == null)
      return;

    TermVectorsReader termVectorsReader = getTermVectorsReader();
    if (termVectorsReader == null)
    {
      return;
    }


    termVectorsReader.get(docNumber, field, mapper);
  }


  public void getTermFreqVector(int docNumber, TermVectorMapper mapper) throws IOException {
    ensureOpen();
    if (termVectorsReaderOrig == null)
      return;

    TermVectorsReader termVectorsReader = getTermVectorsReader();
    if (termVectorsReader == null)
      return;

    termVectorsReader.get(docNumber, mapper);
  }
private:
	//Open all norms files for all fields
	void openNorms(CL_NS(store)::Directory* cfsDir, int32_t readBufferSize);
	
	///a bitVector that manages which documents have been deleted
	CL_NS(util)::BitSet* deletedDocs;
	///an IndexInput to the frequency file
	CL_NS(store)::IndexInput* freqStream;
	///For reading the fieldInfos file
	FieldInfos* fieldInfos;
    ///For reading the Term Dictionary .tis file
	TermInfosReader* tis;
	///an IndexInput to the prox file
	CL_NS(store)::IndexInput* proxStream;

  // optionally used for the .nrm file shared by multiple norms
  private IndexInput singleNormStream;


    static bool hasSeparateNorms(SegmentInfo* si);
	static uint8_t* createFakeNorms(int32_t size);



  /**
   * Increments the RC of this reader, as well as
   * of all norms this reader is using
   */
  protected synchronized void incRef() {
    super.incRef();
    Iterator it = norms.values().iterator();
    while (it.hasNext()) {
      Norm norm = (Norm) it.next();
      norm.incRef();
    }
  }

  /**
   * only increments the RC of this reader, not tof
   * he norms. This is important whenever a reopen()
   * creates a new SegmentReader that doesn't share
   * the norms with this one
   */
  private synchronized void incRefReaderNotNorms() {
    super.incRef();
  }

  protected synchronized void decRef() throws IOException {
    super.decRef();
    Iterator it = norms.values().iterator();
    while (it.hasNext()) {
      Norm norm = (Norm) it.next();
      norm.decRef();
    }
  }

  private synchronized void decRefReaderNotNorms() throws IOException {
    super.decRef();
  }




  private void loadDeletedDocs() throws IOException {
    // NOTE: the bitvector is stored using the regular directory, not cfs
    if (hasDeletions(si)) {
      deletedDocs = new BitVector(directory(), si.getDelFileName());

      // Verify # deletes does not exceed maxDoc for this segment:
      if (deletedDocs.count() > maxDoc()) {
        throw new CorruptIndexException("number of deletes (" + deletedDocs.count() + ") exceeds max doc (" + maxDoc() + ") for segment " + si.name);
      }
    }
  }

  protected synchronized DirectoryIndexReader doReopen(SegmentInfos infos) throws CorruptIndexException, IOException {
    DirectoryIndexReader newReader;

    if (infos.size() == 1) {
      SegmentInfo si = infos.info(0);
      if (segment.equals(si.name) && si.getUseCompoundFile() == SegmentReader.this.si.getUseCompoundFile()) {
        newReader = reopenSegment(si);
      } else {
        // segment not referenced anymore, reopen not possible
        // or segment format changed
        newReader = SegmentReader.get(infos, infos.info(0), false);
      }
    } else {
      return new MultiSegmentReader(directory, infos, closeDirectory, new SegmentReader[] {this}, null, null);
    }

    return newReader;
  }

  synchronized SegmentReader reopenSegment(SegmentInfo si) throws CorruptIndexException, IOException {
    boolean deletionsUpToDate = (this.si.hasDeletions() == si.hasDeletions())
                                  && (!si.hasDeletions() || this.si.getDelFileName().equals(si.getDelFileName()));
    boolean normsUpToDate = true;


    boolean[] fieldNormsChanged = new boolean[fieldInfos.size()];
    if (normsUpToDate) {
      for (int i = 0; i < fieldInfos.size(); i++) {
        if (!this.si.getNormFileName(i).equals(si.getNormFileName(i))) {
          normsUpToDate = false;
          fieldNormsChanged[i] = true;
        }
      }
    }

    if (normsUpToDate && deletionsUpToDate) {
      return this;
    }


      // clone reader
    SegmentReader clone = new SegmentReader();
    boolean success = false;
    try {
      clone.directory = directory;
      clone.si = si;
      clone.segment = segment;
      clone.readBufferSize = readBufferSize;
      clone.cfsReader = cfsReader;
      clone.storeCFSReader = storeCFSReader;

      clone.fieldInfos = fieldInfos;
      clone.tis = tis;
      clone.freqStream = freqStream;
      clone.proxStream = proxStream;
      clone.termVectorsReaderOrig = termVectorsReaderOrig;


      // we have to open a new FieldsReader, because it is not thread-safe
      // and can thus not be shared among multiple SegmentReaders
      // TODO: Change this in case FieldsReader becomes thread-safe in the future
      final String fieldsSegment;

      Directory storeDir = directory();

      if (si.getDocStoreOffset() != -1) {
        fieldsSegment = si.getDocStoreSegment();
        if (storeCFSReader != null) {
          storeDir = storeCFSReader;
        }
      } else {
        fieldsSegment = segment;
        if (cfsReader != null) {
          storeDir = cfsReader;
        }
      }

      if (fieldsReader != null) {
        clone.fieldsReader = new FieldsReader(storeDir, fieldsSegment, fieldInfos, readBufferSize,
                                        si.getDocStoreOffset(), si.docCount);
      }


      if (!deletionsUpToDate) {
        // load deleted docs
        clone.deletedDocs = null;
        clone.loadDeletedDocs();
      } else {
        clone.deletedDocs = this.deletedDocs;
      }

      clone.norms = new HashMap();
      if (!normsUpToDate) {
        // load norms
        for (int i = 0; i < fieldNormsChanged.length; i++) {
          // copy unchanged norms to the cloned reader and incRef those norms
          if (!fieldNormsChanged[i]) {
            String curField = fieldInfos.fieldInfo(i).name;
            Norm norm = (Norm) this.norms.get(curField);
            norm.incRef();
            clone.norms.put(curField, norm);
          }
        }

        clone.openNorms(si.getUseCompoundFile() ? cfsReader : directory(), readBufferSize);
      } else {
        Iterator it = norms.keySet().iterator();
        while (it.hasNext()) {
          String field = (String) it.next();
          Norm norm = (Norm) norms.get(field);
          norm.incRef();
          clone.norms.put(field, norm);
        }
      }

      if (clone.singleNormStream == null) {
        for (int i = 0; i < fieldInfos.size(); i++) {
          FieldInfo fi = fieldInfos.fieldInfo(i);
          if (fi.isIndexed && !fi.omitNorms) {
            Directory d = si.getUseCompoundFile() ? cfsReader : directory();
            String fileName = si.getNormFileName(fi.number);
            if (si.hasSeparateNorms(fi.number)) {
              continue;
            }

            if (fileName.endsWith("." + IndexFileNames.NORMS_EXTENSION)) {
              clone.singleNormStream = d.openInput(fileName, readBufferSize);
              break;
            }
          }
        }
      }

      success = true;
    } finally {
      if (this.referencedSegmentReader != null) {
        // this reader shares resources with another SegmentReader,
        // so we increment the other readers refCount. We don't
        // increment the refCount of the norms because we did
        // that already for the shared norms
        clone.referencedSegmentReader = this.referencedSegmentReader;
        referencedSegmentReader.incRefReaderNotNorms();
      } else {
        // this reader wasn't reopened, so we increment this
        // readers refCount
        clone.referencedSegmentReader = this;
        incRefReaderNotNorms();
      }

      if (!success) {
        // An exception occured during reopen, we have to decRef the norms
        // that we incRef'ed already and close singleNormsStream and FieldsReader
        clone.decRef();
      }
    }

    return clone;
  }



  /** Returns the field infos of this segment */
  FieldInfos fieldInfos() {
    return fieldInfos;
  }

  /**
   * Return the name of the segment this reader is reading.
   */
  String getSegmentName() {
    return segment;
  }

  /**
   * Return the SegmentInfo of the segment this reader is reading.
   */
  SegmentInfo getSegmentInfo() {
    return si;
  }

  void setSegmentInfo(SegmentInfo info) {
    si = info;
  }

  void startCommit() {
    super.startCommit();
    rollbackDeletedDocsDirty = deletedDocsDirty;
    rollbackNormsDirty = normsDirty;
    rollbackUndeleteAll = undeleteAll;
    Iterator it = norms.values().iterator();
    while (it.hasNext()) {
      Norm norm = (Norm) it.next();
      norm.rollbackDirty = norm.dirty;
    }
  }

  void rollbackCommit() {
    super.rollbackCommit();
    deletedDocsDirty = rollbackDeletedDocsDirty;
    normsDirty = rollbackNormsDirty;
    undeleteAll = rollbackUndeleteAll;
    Iterator it = norms.values().iterator();
    while (it.hasNext()) {
      Norm norm = (Norm) it.next();
      norm.dirty = norm.rollbackDirty;
    }
  }

    //allow various classes to access the internals of this. this allows us to have
    //a more tight idea of the package
    friend class IndexReader;
    friend class IndexWriter;
    friend class SegmentTermDocs;
    friend class SegmentTermPositions;
    friend class MultiReader;
};

CL_NS_END
#endif
