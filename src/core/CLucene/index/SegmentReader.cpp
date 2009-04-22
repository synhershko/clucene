/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "CLucene/util/Misc.h"
#include "_SegmentHeader.h"

#include "_FieldInfos.h"
#include "_FieldsReader.h"
#include "IndexReader.h"
#include "_TermInfosReader.h"
#include "Terms.h"
#include "CLucene/search/Similarity.h"
#include "CLucene/store/FSDirectory.h"

CL_NS_USE(util)
CL_NS_USE(store)
CL_NS_USE(document)
CL_NS_USE(search)
CL_NS_DEF(index)

 SegmentReader::Norm::Norm(IndexInput* instrm, int32_t n, int64_t ns, SegmentReader* r, const char* seg): 
	number(n), 
	normSeek(ns),
	reader(r), 
	segment(seg), 
	in(instrm),
	bytes(NULL), 
	dirty(false){
  //Func - Constructor
  //Pre  - instrm is a valid reference to an IndexInput
  //Post - A Norm instance has been created with an empty bytes array

	 bytes = NULL;
     dirty = false;
  }

  SegmentReader::Norm::~Norm() {
  //Func - Destructor
  //Pre  - true
  //Post - The IndexInput in has been deleted (and closed by its destructor) 
  //       and the array too.

      //Close and destroy the inputstream in-> The inputstream will be closed
      // by its destructor. Note that the IndexInput 'in' actually is a pointer!!!!!  
      if ( in != reader->singleNormStream )
    	  _CLDELETE(in);

	  //Delete the bytes array
      _CLDELETE_ARRAY(bytes);

  }

  void SegmentReader::Norm::reWrite(SegmentInfo* si){
      // NOTE: norms are re-written in regular directory, not cfs
      si.advanceNormGen(this.number);
      IndexOutput* out = reader->getDirectory()->createOutput(si.getNormFileName(this.number));
      try {
        out->writeBytes(bytes, reader->maxDoc());
      }_CLFINALLY(
        out->close();
        _CLDELETE(out)
      );
      this->dirty = false;
    }

  SegmentReader::SegmentReader(SegmentInfo* si) : 
      //Init the superclass IndexReader
      IndexReader(si->getDir()),
	  _norms(false,false)
  { 
      initialize(si);
  }

  SegmentReader::SegmentReader(SegmentInfos* sis, SegmentInfo* si) : 
      //Init the superclass IndexReader
      IndexReader(si->getDir(),sis,false),
	  _norms(false,false)
  { 
      initialize(si);
  }

  void SegmentReader::initialize(SegmentInfo* si){
      //Pre  - si-> is a valid reference to SegmentInfo instance
      //       identified by si->
      //Post - All files of the segment have been read

      deletedDocs      = NULL;
	  ones			   = NULL;
	  //There are no documents yet marked as deleted
      deletedDocsDirty = false;
      
      normsDirty=false;
      undeleteAll=false;

	  //Duplicate the name of the segment from SegmentInfo to segment
      segment          = STRDUP_AtoA(si->name);
      
	  // make sure that all index files have been read or are kept open
      // so that if an index update removes them we'll still have them
      freqStream       = NULL;
      proxStream       = NULL;

    segment = si.name;
    this.si = si;
    this.readBufferSize = readBufferSize;

    boolean success = false;

    try {
      // Use compound file directory for some files, if it exists
      Directory cfsDir = directory();
      if (si.getUseCompoundFile()) {
        cfsReader = new CompoundFileReader(directory(), segment + "." + IndexFileNames.COMPOUND_FILE_EXTENSION, readBufferSize);
        cfsDir = cfsReader;
      }

      final Directory storeDir;

      if (doOpenStores) {
        if (si.getDocStoreOffset() != -1) {
          if (si.getDocStoreIsCompoundFile()) {
            storeCFSReader = new CompoundFileReader(directory(), si.getDocStoreSegment() + "." + IndexFileNames.COMPOUND_FILE_STORE_EXTENSION, readBufferSize);
            storeDir = storeCFSReader;
          } else {
            storeDir = directory();
          }
        } else {
          storeDir = cfsDir;
        }
      } else
        storeDir = null;

      // No compound file exists - use the multi-file format
      fieldInfos = new FieldInfos(cfsDir, segment + ".fnm");

      final String fieldsSegment;

      if (si.getDocStoreOffset() != -1)
        fieldsSegment = si.getDocStoreSegment();
      else
        fieldsSegment = segment;

      if (doOpenStores) {
        fieldsReader = new FieldsReader(storeDir, fieldsSegment, fieldInfos, readBufferSize,
                                        si.getDocStoreOffset(), si.docCount);

        // Verify two sources of "maxDoc" agree:
        if (si.getDocStoreOffset() == -1 && fieldsReader.size() != si.docCount) {
          throw new CorruptIndexException("doc counts differ for segment " + si.name + ": fieldsReader shows " + fieldsReader.size() + " but segmentInfo shows " + si.docCount);
        }
      }

      tis = new TermInfosReader(cfsDir, segment, fieldInfos, readBufferSize);

      loadDeletedDocs();

      // make sure that all index files have been read or are kept open
      // so that if an index update removes them we'll still have them
      freqStream = cfsDir.openInput(segment + ".frq", readBufferSize);
      proxStream = cfsDir.openInput(segment + ".prx", readBufferSize);
      openNorms(cfsDir, readBufferSize);

      if (doOpenStores && fieldInfos.hasVectors()) { // open term vector files only as needed
        final String vectorsSegment;
        if (si.getDocStoreOffset() != -1)
          vectorsSegment = si.getDocStoreSegment();
        else
          vectorsSegment = segment;
        termVectorsReaderOrig = new TermVectorsReader(storeDir, vectorsSegment, fieldInfos, readBufferSize, si.getDocStoreOffset(), si.docCount);
      }
      success = true;
    } finally {

      // With lock-less commits, it's entirely possible (and
      // fine) to hit a FileNotFound exception above.  In
      // this case, we want to explicitly close any subset
      // of things that were opened so that we don't have to
      // wait for a GC to do so.
      if (!success) {
        doClose();
      }
    }
  }

  SegmentReader::~SegmentReader(){
  //Func - Destructor.
  //Pre  - doClose has been invoked!
  //Post - the instance has been destroyed

      doClose(); //this means that index reader doesn't need to be closed manually

      _CLDELETE(fieldInfos);
	  _CLDELETE(fieldsReader);
      _CLDELETE(tis);	      
 	  _CLDELETE(freqStream);
	  _CLDELETE(proxStream);
	  _CLDELETE_CaARRAY(segment);
	  _CLDELETE(deletedDocs);
	  _CLDELETE_ARRAY(ones);
     _CLDELETE(termVectorsReaderOrig)
     _CLDECDELETE(cfsReader);
    //termVectorsLocal->unregister(this);
  }

  void SegmentReader::commitChanges(){
    if (deletedDocsDirty) {               // re-write deleted
      si.advanceDelGen();

      // We can write directly to the actual name (vs to a
      // .tmp & renaming it) because the file is not live
      // until segments file is written:
      deletedDocs.write(directory(), si.getDelFileName());
    }
    if (undeleteAll && si.hasDeletions()) {
      si.clearDelGen();
    }
    if (normsDirty) {               // re-write norms
      si.setNumFields(fieldInfos.size());
      Iterator it = norms.values().iterator();
      while (it.hasNext()) {
        Norm norm = (Norm) it.next();
        if (norm.dirty) {
          norm.reWrite(si);
        }
      }
    }
    deletedDocsDirty = false;
    normsDirty = false;
    undeleteAll = false;
  }
  
  void SegmentReader::doClose() {
  //Func - Closes all streams to the files of a single segment
  //Pre  - fieldsReader != NULL
  //       tis != NULL
  //Post - All streams to files have been closed

      CND_PRECONDITION(fieldsReader != NULL, "fieldsReader is NULL");
      CND_PRECONDITION(tis != NULL, "tis is NULL");

      boolean hasReferencedReader = (referencedSegmentReader != null);

      if (hasReferencedReader) {
        referencedSegmentReader.decRefReaderNotNorms();
        referencedSegmentReader = null;
      }

      deletedDocs = null;

      // close the single norms stream
      if (singleNormStream != null) {
        // we can close this stream, even if the norms
        // are shared, because every reader has it's own
        // singleNormStream
        singleNormStream.close();
        singleNormStream = null;
      }

      // re-opened SegmentReaders have their own instance of FieldsReader
      if (fieldsReader != null) {
        fieldsReader->close();
      }

      if (!hasReferencedReader) {
        // close everything, nothing is shared anymore with other readers
        if (tis != null) {
          tis->close();
        }

        //Close the frequency stream
        if (freqStream != NULL){
              freqStream->close();
        }
        //Close the prox stream
        if (proxStream != NULL){
            proxStream->close();
        }

        if (termVectorsReaderOrig != NULL)
            termVectorsReaderOrig->close();

        if (cfsReader != NULL)
          cfsReader->close();

        if (storeCFSReader != null)
          storeCFSReader.close();

        // maybe close directory
        super.doClose();
      }
  }

  bool SegmentReader::hasDeletions()  const{
    // Don't call ensureOpen() here (it could affect performance)
      return deletedDocs != NULL;
  }

  //static 
  bool SegmentReader::usesCompoundFile(SegmentInfo* si) {
    return si.getUseCompoundFile();
  }
  
  //static
  bool SegmentReader::hasSeparateNorms(SegmentInfo* si) {
    return si.hasSeparateNorms();
  }

  bool SegmentReader::hasDeletions(const SegmentInfo* si) {
  //Func - Static method
  //       Checks if a segment managed by SegmentInfo si-> has deletions
  //Pre  - si-> holds a valid reference to an SegmentInfo instance
  //Post - if the segement contains deleteions true is returned otherwise flas

    // Don't call ensureOpen() here (it could affect performance)
    return si.hasDeletions();
  }

	//synchronized
  void SegmentReader::doDelete(const int32_t docNum){
  //Func - Marks document docNum as deleted
  //Pre  - docNum >=0 and DocNum < maxDoc() 
  //       docNum contains the number of the document that must be 
  //       marked deleted
  //Post - The document identified by docNum has been marked deleted

      SCOPED_LOCK_MUTEX(THIS_LOCK)
      
     CND_PRECONDITION(docNum >= 0, "docNum is a negative number");
     CND_PRECONDITION(docNum < maxDoc(), "docNum is bigger than the total number of documents");

	  //Check if deletedDocs exists
	  if (deletedDocs == NULL){
          deletedDocs = _CLNEW BitSet(maxDoc());

          //Condition check to see if deletedDocs points to a valid instance
          CND_CONDITION(deletedDocs != NULL,"No memory could be allocated for deletedDocs");
	  }
      //Flag that there are documents marked deleted
      deletedDocsDirty = true;
      undeleteAll = false;
      //Mark document identified by docNum as deleted
      deletedDocs->set(docNum);

  }

  void SegmentReader::doUndeleteAll(){
      _CLDELETE(deletedDocs);
      deletedDocsDirty = false;
      undeleteAll = true;
  }

  void SegmentReader::files(AStringArrayWithDeletor& retarray) {
  //Func - Returns all file names managed by this SegmentReader
  //Pre  - segment != NULL
  //Post - All filenames managed by this SegmentRead have been returned

    return new Vector(si.files());
  }

  TermEnum* SegmentReader::terms() const {
  //Func - Returns an enumeration of all the Terms and TermInfos in the set. 
  //Pre  - tis != NULL
  //Post - An enumeration of all the Terms and TermInfos in the set has been returned

      CND_PRECONDITION(tis != NULL, "tis is NULL");

      ensureOpen();
      return tis->terms();
  }

  TermEnum* SegmentReader::terms(const Term* t) const {
  //Func - Returns an enumeration of terms starting at or after the named term t 
  //Pre  - t != NULL
  //       tis != NULL
  //Post - An enumeration of terms starting at or after the named term t 

      CND_PRECONDITION(t   != NULL, "t is NULL");
      CND_PRECONDITION(tis != NULL, "tis is NULL");

      ensureOpen();
      return tis->terms(t);
  }

  bool SegmentReader::document(int32_t n, Document& doc, FieldSelector* fieldSelector) {
  //Func - writes the fields of document n into doc
  //Pre  - n >=0 and identifies the document n
  //Post - if the document has been deleted then an exception has been thrown
  //       otherwise a reference to the found document has been returned

      SCOPED_LOCK_MUTEX(THIS_LOCK)

      ensureOpen();
    
      CND_PRECONDITION(n >= 0, "n is a negative number");

	  //Check if the n-th document has been marked deleted
       if (isDeleted(n)){
          _CLTHROWA( CL_ERR_InvalidState,"attempt to access a deleted document" );
       }

	   //Retrieve the n-th document
       return fieldsReader->doc(n, doc, fieldSelector);
  }


  bool SegmentReader::isDeleted(const int32_t n){
  //Func - Checks if the n-th document has been marked deleted
  //Pre  - n >=0 and identifies the document n
  //Post - true has been returned if document n has been deleted otherwise fralse

      SCOPED_LOCK_MUTEX(THIS_LOCK)
      
      CND_PRECONDITION(n >= 0, "n is a negative number");

	  //Is document n deleted
      bool ret = (deletedDocs != NULL && deletedDocs->get(n));

      return ret;
  }

  TermDocs* SegmentReader::termDocs() const {
  //Func - Returns an unpositioned TermDocs enumerator. 
  //Pre  - true
  //Post - An unpositioned TermDocs enumerator has been returned

        ensureOpen();
       return _CLNEW SegmentTermDocs(this);
  }

  TermPositions* SegmentReader::termPositions() const {
  //Func - Returns an unpositioned TermPositions enumerator. 
  //Pre  - true
  //Post - An unpositioned TermPositions enumerator has been returned

        ensureOpen();
      return _CLNEW SegmentTermPositions(this);
  }

  int32_t SegmentReader::docFreq(const Term* t) const {
  //Func - Returns the number of documents which contain the term t
  //Pre  - t holds a valid reference to a Term
  //Post - The number of documents which contain term t has been returned

        ensureOpen();

      //Get the TermInfo ti for Term  t in the set
      TermInfo* ti = tis->get(t);
      //Check if an TermInfo has been returned
      if (ti){
		  //Get the frequency of the term
          int32_t ret = ti->docFreq;
		  //TermInfo ti is not needed anymore so delete it
          _CLDELETE( ti );
		  //return the number of documents which containt term t
          return ret;
          }
	  else
		  //No TermInfo returned so return 0
          return 0;
  }

  int32_t SegmentReader::numDocs() {
  //Func - Returns the actual number of documents in the segment
  //Pre  - true
  //Post - The actual number of documents in the segments

        ensureOpen();

	  //Get the number of all the documents in the segment including the ones that have 
	  //been marked deleted
      int32_t n = maxDoc();

	  //Check if there any deleted docs
      if (deletedDocs != NULL)
		  //Substract the number of deleted docs from the number returned by maxDoc
          n -= deletedDocs->count();

	  //return the actual number of documents in the segment
      return n;
  }

  int32_t SegmentReader::maxDoc() const {
  //Func - Returns the number of  all the documents in the segment including
  //       the ones that have been marked deleted
  //Pre  - true
  //Post - The total number of documents in the segment has been returned

    // Don't call ensureOpen() here (it could affect performance)

      return si.docCount;
  }

void SegmentReader::getFieldNames(FieldOption fldOption, StringArrayWithDeletor& retarray){
  ensureOpen();

	size_t len = fieldInfos->size();
	for (size_t i = 0; i < len; i++) {
		FieldInfo* fi = fieldInfos->fieldInfo(i);
		bool v=false;
		if (fldOption & IndexReader::ALL) {
			v=true;
		}else {
			if (!fi->isIndexed && (fldOption & IndexReader::UNINDEXED) )
				v=true;
			else if (fi->isIndexed && (fldOption & IndexReader::INDEXED) )
				v=true;
      else if (fi->storePayloads && (fieldOption & IndexReader::STORES_PAYLOADS) )
        v=true;
			else if (fi->isIndexed && fi->storeTermVector == false && ( fldOption & IndexReader::INDEXED_NO_TERMVECTOR) )
				v=true;
			else if ( (fldOption & IndexReader::TERMVECTOR) &&
				    fi->storeTermVector == true &&
					fi->storePositionWithTermVector == false &&
					fi->storeOffsetWithTermVector == false )
				v=true;
			else if (fi->isIndexed && fi->storeTermVector && (fldOption & IndexReader::INDEXED_WITH_TERMVECTOR) )
				v=true;
			else if (fi->storePositionWithTermVector && fi->storeOffsetWithTermVector == false && 
					(fldOption & IndexReader::TERMVECTOR_WITH_POSITION))
				v=true;
			else if (fi->storeOffsetWithTermVector && fi->storePositionWithTermVector == false && 
					(fldOption & IndexReader::TERMVECTOR_WITH_OFFSET) )
				v=true;
			else if ((fi->storeOffsetWithTermVector && fi->storePositionWithTermVector) &&
					(fldOption & IndexReader::TERMVECTOR_WITH_POSITION_OFFSET) )
				v=true;
		}
		if ( v )
			retarray.push_back(STRDUP_TtoT(fi->name));
	}
}

bool SegmentReader::hasNorms(const TCHAR* field) const{
    ensureOpen();
	return _norms.find(field) != _norms.end();
}


  void SegmentReader::norms(const TCHAR* field, uint8_t* bytes) {
  //Func - Reads the Norms for field from disk starting at offset in the inputstream
  //Pre  - field != NULL
  //       bytes != NULL is an array of bytes which is to be used to read the norms into.
  //       it is advisable to have bytes initalized by zeroes!
  //Post - The if an inputstream to the norm file could be retrieved the bytes have been read
  //       You are never sure whether or not the norms have been read into bytes properly!!!!!!!!!!!!!!!!!

    CND_PRECONDITION(field != NULL, "field is NULL");
    CND_PRECONDITION(bytes != NULL, "field is NULL");

    SCOPED_LOCK_MUTEX(THIS_LOCK)

    ensureOpen();
    Norm* norm = _norms.get(field);
    if ( norm == NULL ){
      memcpy(bytes, fakeNorms(), maxDoc());
      return;
    }


    {SCOPED_MUTEX_LOCK(norm->THIS_LOCK)
      if (norm->bytes != NULL) { // can copy from cache
      memcpy(bytes, norm->bytes, maxDoc());
        return;
      }

      // Read from disk.  norm.in may be shared across  multiple norms and
      // should only be used in a synchronized context.
      IndexInput normStream;
      if (norm.useSingleNormStream) {
        normStream = singleNormStream;
      } else {
        normStream = norm.in;
      }
      normStream.seek(norm.normSeek);
      normStream.readBytes(bytes, offset, maxDoc());
    }
  }

  uint8_t* SegmentReader::createFakeNorms(int32_t size) {
    uint8_t* ones = _CL_NEWARRAY(uint8_t,size);
    memset(ones, DefaultSimilarity::encodeNorm(1.0f), size);
    return ones;
  }

  uint8_t* SegmentReader::fakeNorms() {
    if (ones==NULL) 
		ones=createFakeNorms(maxDoc());
    return ones;
  }
  // can return null if norms aren't stored
  uint8_t* SegmentReader::getNorms(const TCHAR* field) {
    SCOPED_LOCK_MUTEX(THIS_LOCK)
    Norm* norm = _norms.get(field);
    if (norm == NULL)  return NULL;  // not indexed, or norms not stored

    {SCOPED_MUTEX_LOCK(norm->THIS_LOCK)
      if (norm->bytes == NULL) {                     // value not yet read
        uint8_t* bytes = _CL_NEWARRAY(uint8_t, maxDoc());
        norms(field, bytes);
        norm->bytes = bytes;                         // cache it
        // it's OK to close the underlying IndexInput as we have cached the
        // norms and will never read them again.
        norm->close();
      }
      return norm->bytes;
    }
  }

  uint8_t* SegmentReader::norms(const TCHAR* field) {
  //Func - Returns the bytes array that holds the norms of a named field
  //Pre  - field != NULL and contains the name of the field for which the norms 
  //       must be retrieved
  //Post - If there was norm for the named field then a bytes array has been allocated 
  //       and returned containing the norms for that field. If the named field is unknown NULL is returned.

    CND_PRECONDITION(field != NULL, "field is NULL");
    SCOPED_LOCK_MUTEX(THIS_LOCK)
    ensureOpen();
    uint8_t* bytes = getNorms(field);
    if (bytes==NULL) 
		bytes=fakeNorms();
    return bytes;
  }

  void SegmentReader::doSetNorm(int32_t doc, const TCHAR* field, uint8_t value){
    Norm* norm = _norms.get(field);
    if (norm == NULL)                             // not an indexed field
      return;
    norm->dirty = true;                            // mark it dirty
    normsDirty = true;

    uint8_t* bits = norms(field);
    bits[doc] = value;                    // set the value
  }


  char* SegmentReader::SegmentName(const char* ext, const int32_t x){
  //Func - Returns an allocated buffer in which it creates a filename by 
  //       concatenating segment with ext and x
  //Pre    ext != NULL and holds the extension
  //       x contains a number
  //Post - A buffer has been instantiated an when x = -1 buffer contains the concatenation of 
  //       segment and ext otherwise buffer contains the contentation of segment, ext and x
      
	  CND_PRECONDITION(ext     != NULL, "ext is NULL");

	  //Create a buffer of length CL_MAX_PATH
	  char* buf = _CL_NEWARRAY(char,CL_MAX_PATH);
	  //Create the filename
      SegmentName(buf,CL_MAX_PATH,ext,x);
	  
      return buf ;
  }

  void SegmentReader::SegmentName(char* buffer,int32_t bufferLen, const char* ext, const int32_t x ){
  //Func - Creates a filename in buffer by concatenating segment with ext and x
  //Pre  - buffer != NULL
  //       ext    != NULL
  //       x contains a number
  //Post - When x = -1 buffer contains the concatenation of segment and ext otherwise
  //       buffer contains the contentation of segment, ext and x

      CND_PRECONDITION(buffer  != NULL, "buffer is NULL");
      CND_PRECONDITION(segment != NULL, "Segment is NULL");

      Misc::segmentname(buffer,bufferLen,segment,ext,x);
  }
  void SegmentReader::openNorms(Directory* cfsDir, int32_t readBufferSize) {
  //Func - Open all norms files for all fields
  //       Creates for each field a norm Instance with an open inputstream to 
  //       a corresponding norm file ready to be read
  //Pre  - true
  //Post - For each field a norm instance has been created with an open inputstream to
  //       a corresponding norm file ready to be read
    long nextNormSeek = SegmentMerger.NORMS_HEADER.length; //skip header (header unused for now)
    int maxDoc = maxDoc();
    for (int i = 0; i < fieldInfos.size(); i++) {
      FieldInfo fi = fieldInfos.fieldInfo(i);
      if (norms.containsKey(fi.name)) {
        // in case this SegmentReader is being re-opened, we might be able to
        // reuse some norm instances and skip loading them here
        continue;
      }
      if (fi.isIndexed && !fi.omitNorms) {
        Directory d = directory();
        String fileName = si.getNormFileName(fi.number);
        if (!si.hasSeparateNorms(fi.number)) {
          d = cfsDir;
        }

        // singleNormFile means multiple norms share this file
        boolean singleNormFile = fileName.endsWith("." + IndexFileNames.NORMS_EXTENSION);
        IndexInput normInput = null;
        long normSeek;

        if (singleNormFile) {
          normSeek = nextNormSeek;
          if (singleNormStream==null) {
            singleNormStream = d.openInput(fileName, readBufferSize);
          }
          // All norms in the .nrm file can share a single IndexInput since
          // they are only used in a synchronized context.
          // If this were to change in the future, a clone could be done here.
          normInput = singleNormStream;
        } else {
          normSeek = 0;
          normInput = d.openInput(fileName);
        }

        norms.put(fi.name, new Norm(normInput, singleNormFile, fi.number, normSeek));
        nextNormSeek += maxDoc; // increment also if some norms are separate
      }
    }
  }


	TermVectorsReader* SegmentReader::getTermVectorsReader() {
		TermVectorsReader* tvReader = termVectorsLocal.get();
		if (tvReader == NULL) {
		  tvReader = termVectorsReaderOrig->clone();
		  termVectorsLocal.set(tvReader);
		}
		return tvReader;
	}

   TermFreqVector* SegmentReader::getTermFreqVector(int32_t docNumber, const TCHAR* field){
    ensureOpen();
 		if ( field != NULL ){
			// Check if this field is invalid or has no stored term vector
			FieldInfo* fi = fieldInfos->fieldInfo(field);
			if (fi == NULL || !fi->storeTermVector || termVectorsReaderOrig == NULL ) 
				return NULL;
		}
		TermVectorsReader* termVectorsReader = getTermVectorsReader();
		if (termVectorsReader == NULL)
		  return NULL;
		return termVectorsReader->get(docNumber, field);
  }

  ObjectArray<TermFreqVector>* SegmentReader::getTermFreqVectors(int32_t docNumber) {
    ensureOpen();
    if (termVectorsReaderOrig == NULL)
      return NULL;
    
    TermVectorsReader* termVectorsReader = getTermVectorsReader();
    if (termVectorsReader == NULL)
      return NULL;
    
	return termVectorsReader->get(docNumber);
  }

CL_NS_END
