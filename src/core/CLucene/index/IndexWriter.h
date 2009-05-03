/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_index_IndexWriter_
#define _lucene_index_IndexWriter_


//#include "CLucene/analysis/AnalysisHeader.h"
#include "CLucene/util/VoidMapSetDefinitions.h"
CL_CLASS_DEF(search,Similarity)
CL_CLASS_DEF(store,Lock)
CL_CLASS_DEF(store,TransactionalRAMDirectory)
CL_CLASS_DEF(analysis,Analyzer)
CL_CLASS_DEF(store,Directory)
CL_CLASS_DEF(store,LuceneLock)
CL_CLASS_DEF(document,Document)

//#include "CLucene/store/TransactionalRAMDirectory.h"
//#include "SegmentHeader.h"
#include "CLucene/LuceneThreads.h"

CL_NS_DEF(index)
class SegmentInfo;
class SegmentInfos;
class MergePolicy;
class IndexReader;
class SegmentReader;
class SegmentInfos;
class MergeScheduler;
class DocumentsWriter;
class IndexFileDeleter;

/**
  An <code>IndexWriter</code> creates and maintains an index.

  <p>The <code>create</code> argument to the
  <a href="#IndexWriter(org.apache.lucene.store.Directory, org.apache.lucene.analysis.Analyzer, boolean)"><b>constructor</b></a>
  determines whether a new index is created, or whether an existing index is
  opened.  Note that you
  can open an index with <code>create=true</code> even while readers are
  using the index.  The old readers will continue to search
  the "point in time" snapshot they had opened, and won't
  see the newly created index until they re-open.  There are
  also <a href="#IndexWriter(org.apache.lucene.store.Directory, org.apache.lucene.analysis.Analyzer)"><b>constructors</b></a>
  with no <code>create</code> argument which
  will create a new index if there is not already an index at the
  provided path and otherwise open the existing index.</p>

  <p>In either case, documents are added with <a
  href="#addDocument(org.apache.lucene.document.Document)"><b>addDocument</b></a>
  and removed with <a
  href="#deleteDocuments(org.apache.lucene.index.Term)"><b>deleteDocuments</b></a>.
  A document can be updated with <a href="#updateDocument(org.apache.lucene.index.Term, org.apache.lucene.document.Document)"><b>updateDocument</b></a>
  (which just deletes and then adds the entire document).
  When finished adding, deleting and updating documents, <a href="#close()"><b>close</b></a> should be called.</p>

  <p>These changes are buffered in memory and periodically
  flushed to the {@link Directory} (during the above method
  calls).  A flush is triggered when there are enough
  buffered deletes (see {@link #setMaxBufferedDeleteTerms})
  or enough added documents since the last flush, whichever
  is sooner.  For the added documents, flushing is triggered
  either by RAM usage of the documents (see {@link
  #setRAMBufferSizeMB}) or the number of added documents.
  The default is to flush when RAM usage hits 16 MB.  For
  best indexing speed you should flush by RAM usage with a
  large RAM buffer.  You can also force a flush by calling
  {@link #flush}.  When a flush occurs, both pending deletes
  and added documents are flushed to the index.  A flush may
  also trigger one or more segment merges which by default
  run with a background thread so as not to block the
  addDocument calls (see <a href="#mergePolicy">below</a>
  for changing the {@link MergeScheduler}).</p>

  <a name="autoCommit"></a>
  <p>The optional <code>autoCommit</code> argument to the
  <a href="#IndexWriter(org.apache.lucene.store.Directory, boolean, org.apache.lucene.analysis.Analyzer)"><b>constructors</b></a>
  controls visibility of the changes to {@link IndexReader} instances reading the same index.
  When this is <code>false</code>, changes are not
  visible until {@link #close()} is called.
  Note that changes will still be flushed to the
  {@link org.apache.lucene.store.Directory} as new files,
  but are not committed (no new <code>segments_N</code> file
  is written referencing the new files) until {@link #close} is
  called.  If something goes terribly wrong (for example the
  JVM crashes) before {@link #close()}, then
  the index will reflect none of the changes made (it will
  remain in its starting state).
  You can also call {@link #abort()}, which closes the writer without committing any
  changes, and removes any index
  files that had been flushed but are now unreferenced.
  This mode is useful for preventing readers from refreshing
  at a bad time (for example after you've done all your
  deletes but before you've done your adds).
  It can also be used to implement simple single-writer
  transactional semantics ("all or none").</p>

  <p>When <code>autoCommit</code> is <code>true</code> then
  every flush is also a commit ({@link IndexReader}
  instances will see each flush as changes to the index).
  This is the default, to match the behavior before 2.2.
  When running in this mode, be careful not to refresh your
  readers while optimize or segment merges are taking place
  as this can tie up substantial disk space.</p>

  <p>Regardless of <code>autoCommit</code>, an {@link
  IndexReader} or {@link org.apache.lucene.search.IndexSearcher} will only see the
  index as of the "point in time" that it was opened.  Any
  changes committed to the index after the reader was opened
  are not visible until the reader is re-opened.</p>

  <p>If an index will not have more documents added for a while and optimal search
  performance is desired, then the <a href="#optimize()"><b>optimize</b></a>
  method should be called before the index is closed.</p>

  <p>Opening an <code>IndexWriter</code> creates a lock file for the directory in use. Trying to open
  another <code>IndexWriter</code> on the same directory will lead to a
  {@link LockObtainFailedException}. The {@link LockObtainFailedException}
  is also thrown if an IndexReader on the same directory is used to delete documents
  from the index.</p>

  <a name="deletionPolicy"></a>
  <p>Expert: <code>IndexWriter</code> allows an optional
  {@link IndexDeletionPolicy} implementation to be
  specified.  You can use this to control when prior commits
  are deleted from the index.  The default policy is {@link
  KeepOnlyLastCommitDeletionPolicy} which removes all prior
  commits as soon as a new commit is done (this matches
  behavior before 2.2).  Creating your own policy can allow
  you to explicitly keep previous "point in time" commits
  alive in the index for some time, to allow readers to
  refresh to the new commit without having the old commit
  deleted out from under them.  This is necessary on
  filesystems like NFS that do not support "delete on last
  close" semantics, which Lucene's "point in time" search
  normally relies on. </p>

  <a name="mergePolicy"></a> <p>Expert:
  <code>IndexWriter</code> allows you to separately change
  the {@link MergePolicy} and the {@link MergeScheduler}.
  The {@link MergePolicy} is invoked whenever there are
  changes to the segments in the index.  Its role is to
  select which merges to do, if any, and return a {@link
  MergePolicy.MergeSpecification} describing the merges.  It
  also selects merges to do for optimize().  (The default is
  {@link LogByteSizeMergePolicy}.  Then, the {@link
  MergeScheduler} is invoked with the requested merges and
  it decides when and how to run the merges.  The default is
  {@link ConcurrentMergeScheduler}. </p>
*/
/*
 * Clarification: Check Points (and commits)
 * Being able to set autoCommit=false allows IndexWriter to flush and
 * write new index files to the directory without writing a new segments_N
 * file which references these new files. It also means that the state of
 * the in memory SegmentInfos object is different than the most recent
 * segments_N file written to the directory.
 *
 * Each time the SegmentInfos is changed, and matches the (possibly
 * modified) directory files, we have a new "check point".
 * If the modified/new SegmentInfos is written to disk - as a new
 * (generation of) segments_N file - this check point is also an
 * IndexCommitPoint.
 *
 * With autoCommit=true, every checkPoint is also a CommitPoint.
 * With autoCommit=false, some checkPoints may not be commits.
 *
 * A new checkpoint always replaces the previous checkpoint and
 * becomes the new "front" of the index. This allows the IndexFileDeleter
 * to delete files that are referenced only by stale checkpoints.
 * (files that were created since the last commit, but are no longer
 * referenced by the "front" of the index). For this, IndexFileDeleter
 * keeps track of the last non commit checkpoint.
 */
class CLUCENE_EXPORT IndexWriter:LUCENE_BASE {
	bool isOpen; //indicates if the writers is open - this way close can be called multiple times

	// how to analyze text
	CL_NS(analysis)::Analyzer* analyzer;

	CL_NS(search)::Similarity* similarity; // how to normalize

	bool closeDir;
  bool closed;
  bool closing;

  // Holds all SegmentInfo instances currently involved in
  // merges
  CL_NS(util)::CLHashSet<void*> mergingSegments;
  MergePolicy* mergePolicy;
  MergeScheduler* mergeScheduler;
  CL_NS(util)::CLLinkedList<void*> pendingMerges;
  CL_NS(util)::CLHashSet<void*,int> runningMerges;
  CL_NS(util)::CLArrayList<void*> mergeExceptions;
  int64_t mergeGen;
  bool stopMerges;


  bool commitPending; // true if segmentInfos has changes not yet committed
  SegmentInfos* rollbackSegmentInfos;      // segmentInfos we will fallback to if the commit fails

  SegmentInfos* localRollbackSegmentInfos;      // segmentInfos we will fallback to if the commit fails
  bool localAutoCommit;                // saved autoCommit during local transaction
  bool autoCommit;              // false if we should commit only on close

  DocumentsWriter* docWriter;
  IndexFileDeleter* deleter;

  CL_NS(util)::CLHashSet<SegmentInfo*> segmentsToOptimize;           // used by optimize to note those needing optimization


	CL_NS(store)::LuceneLock* writeLock;

	void _IndexWriter(const bool create);

	void _finalize();

	// where this index resides
	CL_NS(store)::Directory* directory;		
		
		
	int32_t getSegmentsCounter();
	int32_t maxFieldLength;
	int32_t mergeFactor;
	int32_t minMergeDocs;
	int32_t maxMergeDocs;
	int32_t termIndexInterval;

	int64_t writeLockTimeout;
	int64_t commitLockTimeout;

  // The normal read buffer size defaults to 1024, but
  // increasing this during merging seems to yield
  // performance gains.  However we don't want to increase
  // it too much because there are quite a few
  // BufferedIndexInputs created during merging.  See
  // LUCENE-888 for details.
  static const int32_t MERGE_READ_BUFFER_SIZE;

  // Used for printing messages
  STATIC_DEFINE_MUTEX(MESSAGE_ID_LOCK)
  static int32_t MESSAGE_ID;
  int32_t messageID;
  mutable bool hitOOM;

public:
	DEFINE_MUTEX(THIS_LOCK)
	
	// Release the write lock, if needed. 
	SegmentInfos* segmentInfos;
  
	// Release the write lock, if needed.
	~IndexWriter();

	/**
	*  The Java implementation of Lucene silently truncates any tokenized
	*  field if the number of tokens exceeds a certain threshold.  Although
	*  that threshold is adjustable, it is easy for the client programmer
	*  to be unaware that such a threshold exists, and to become its
	*  unwitting victim.
	*  CLucene implements a less insidious truncation policy.  Up to
	*  DEFAULT_MAX_FIELD_LENGTH tokens, CLucene behaves just as JLucene
	*  does.  If the number of tokens exceeds that threshold without any
	*  indication of a truncation preference by the client programmer,
	*  CLucene raises an exception, prompting the client programmer to
	*  explicitly set a truncation policy by adjusting maxFieldLength.
	*/
	LUCENE_STATIC_CONSTANT(int32_t, DEFAULT_MAX_FIELD_LENGTH = 10000);
	LUCENE_STATIC_CONSTANT(int32_t, FIELD_TRUNC_POLICY__WARN = -1);
	int32_t getMaxFieldLength() const;
	void setMaxFieldLength(int32_t val);

	/** Determines the minimal number of documents required before the buffered
	* in-memory documents are merging and a new Segment is created.
	* Since Documents are merged in a {@link RAMDirectory},
	* large value gives faster indexing.  At the same time, mergeFactor limits
	* the number of files open in a FSDirectory.
	*
	* <p> The default value is DEFAULT_MAX_BUFFERED_DOCS.*/
	void setMaxBufferedDocs(int32_t val);
	/**
	* @see #setMaxBufferedDocs
	*/
	int32_t getMaxBufferedDocs();
	
	/**
	* Default value for the write lock timeout (1,000).
  * @see #setDefaultWriteLockTimeout
	*/
	static int64_t WRITE_LOCK_TIMEOUT;
	/**
	* Sets the maximum time to wait for a write lock (in milliseconds).
	*/
	void setWriteLockTimeout(int64_t writeLockTimeout);
	/**
	* @see #setWriteLockTimeout
	*/
	int64_t getWriteLockTimeout();
	
	/**
	* Sets the maximum time to wait for a commit lock (in milliseconds).
	*/
	void setCommitLockTimeout(int64_t commitLockTimeout);
	/**
	* @see #setCommitLockTimeout
	*/
	int64_t getCommitLockTimeout();

  /**
   * Name of the write lock in the index.
   */
	static const char* WRITE_LOCK_NAME; //"write.lock";

  /**
   * @deprecated
   * @see LogMergePolicy#DEFAULT_MERGE_FACTOR
   */
  static const int32_t DEFAULT_MERGE_FACTOR ;

  /**
   * Value to denote a flush trigger is disabled
   */
  static const int32_t DISABLE_AUTO_FLUSH;

  /**
   * Disabled by default (because IndexWriter flushes by RAM usage
   * by default). Change using {@link #setMaxBufferedDocs(int)}.
   */
  static const int32_t DEFAULT_MAX_BUFFERED_DOCS;

  /**
   * Default value is 16 MB (which means flush when buffered
   * docs consume 16 MB RAM).  Change using {@link #setRAMBufferSizeMB}.
   */
  static const float_t DEFAULT_RAM_BUFFER_SIZE_MB;

  /**
   * Disabled by default (because IndexWriter flushes by RAM usage
   * by default). Change using {@link #setMaxBufferedDeleteTerms(int)}.
   */
  static const int32_t DEFAULT_MAX_BUFFERED_DELETE_TERMS;

  /**
   * @deprecated
   * @see LogDocMergePolicy#DEFAULT_MAX_MERGE_DOCS
   */
  static const int32_t DEFAULT_MAX_MERGE_DOCS;

  /**
   * Absolute hard maximum length for a term.  If a term
   * arrives from the analyzer longer than this length, it
   * is skipped and a message is printed to infoStream, if
   * set (see {@link #setInfoStream}).
   */
  static const int32_t MAX_TERM_LENGTH;

	
	/* Determines how often segment indices are merged by addDocument().  With
	*  smaller values, less RAM is used while indexing, and searches on
	*  unoptimized indices are faster, but indexing speed is slower.  With larger
	*  values more RAM is used while indexing and searches on unoptimized indices
	*  are slower, but indexing is faster.  Thus larger values (> 10) are best
	*  for batched index creation, and smaller values (< 10) for indices that are
	*  interactively maintained.
	*
	* <p>This must never be less than 2.  The default value is 10.
	*/
	int32_t getMergeFactor() const;
	void setMergeFactor(int32_t val);

	
	/** Expert: The fraction of terms in the "dictionary" which should be stored
	*   in RAM.  Smaller values use more memory, but make searching slightly
	*   faster, while larger values use less memory and make searching slightly
	*   slower.  Searching is typically not dominated by dictionary lookup, so
	*   tweaking this is rarely useful.
	*/
	LUCENE_STATIC_CONSTANT(int32_t, DEFAULT_TERM_INDEX_INTERVAL = 128);
	/** Expert: Set the interval between indexed terms.  Large values cause less
	* memory to be used by IndexReader, but slow random-access to terms.  Small
	* values cause more memory to be used by an IndexReader, and speed
	* random-access to terms.
	*
	* This parameter determines the amount of computation required per query
	* term, regardless of the number of documents that contain that term.  In
	* particular, it is the maximum number of other terms that must be
	* scanned before a term is located and its frequency and position information
	* may be processed.  In a large index with user-entered query terms, query
	* processing time is likely to be dominated not by term lookup but rather
	* by the processing of frequency and positional data.  In a small index
	* or when many uncommon query terms are generated (e.g., by wildcard
	* queries) term lookup may become a dominant cost.
	*
	* In particular, <code>numUniqueTerms/interval</code> terms are read into
	* memory by an IndexReader, and, on average, <code>interval/2</code> terms
	* must be scanned for each random term access.
	*
	* @see #DEFAULT_TERM_INDEX_INTERVAL
	*/
	void setTermIndexInterval(int32_t interval);
	/** Expert: Return the interval between indexed terms.
	*
	* @see #setTermIndexInterval(int)
	*/
	int32_t getTermIndexInterval();
  
	/** Determines the minimal number of documents required before the buffered
	* in-memory documents are merging and a new Segment is created.
	* Since Documents are merged in a {@link RAMDirectory},
	* large value gives faster indexing.  At the same time, mergeFactor limits
	* the number of files open in a FSDirectory.
	*
	* <p> The default value is 10.*/
	int32_t getMinMergeDocs() const;
	void setMinMergeDocs(int32_t val);

	/**Determines the largest number of documents ever merged by addDocument().
	*  Small values (e.g., less than 10,000) are best for interactive indexing,
	*  as this limits the length of pauses while indexing to a few seconds.
	*  Larger values are best for batched indexing and speedier searches.
	*
	*  <p>The default value is {@link Integer#MAX_VALUE}.
	*/
	int32_t getMaxMergeDocs() const;
	void setMaxMergeDocs(int32_t val);

	/**
	* Constructs an IndexWriter for the index in <code>path</code>.
	* Text will be analyzed with <code>a</code>.  If <code>create</code>
	* is true, then a new, empty index will be created in
	* <code>path</code>, replacing the index already there, if any.
	*
	* @param path the path to the index directory
	* @param a the analyzer to use
	* @param create <code>true</code> to create the index or overwrite
	*  the existing one; <code>false</code> to append to the existing
	*  index
	* @throws IOException if the directory cannot be read/written to, or
	*  if it does not exist, and <code>create</code> is
	*  <code>false</code>
	*/
	IndexWriter(const char* path, CL_NS(analysis)::Analyzer* a, const bool create, const bool closeDir=true);
	
	
	/**Constructs an IndexWriter for the index in <code>d</code>.  Text will be
	*  analyzed with <code>a</code>.  If <code>create</code> is true, then a new,
	*  empty index will be created in <code>d</code>, replacing the index already
	*  there, if any.
	*/
	IndexWriter(CL_NS(store)::Directory* d, CL_NS(analysis)::Analyzer* a, const bool create, const bool closeDir=false);

	/**
	* Flushes all changes to an index, closes all associated files, and closes
	* the directory that the index is stored in.
	*/
	void close();

	/**Returns the number of documents currently in this index.
	*  synchronized
	*/
	int32_t docCount();


	/**
	* Adds a document to this index, using the provided analyzer instead of the
	* value of {@link #getAnalyzer()}.  If the document contains more than
	* {@link #setMaxFieldLength(int)} terms for a given field, the remainder are
	* discarded.
	*/
	void addDocument(CL_NS(document)::Document* doc, CL_NS(analysis)::Analyzer* analyzer=NULL);
  

	/**Merges all segments together into a single segment, optimizing an index
	*  for search.
	*@synchronized
	*/
	void optimize();


	/**Merges all segments from an array of indices into this index.
	*  
	*  <p>This may be used to parallelize batch indexing.  A large document
	*  collection can be broken into sub-collections.  Each sub-collection can be
	*  indexed in parallel, on a different thread, process or machine.  The
	*  complete index can then be created by merging sub-collection indices
	*  with this method.
	*
	*  <p>After this completes, the index is optimized.
	*@synchronized
	*/
	void addIndexes(CL_NS(store)::Directory** dirs);
		
	/** Merges the provided indexes into this index.
	* <p>After this completes, the index is optimized. </p>
	* <p>The provided IndexReaders are not closed.</p>
	*/
	void addIndexes(IndexReader** readers);


	/** Returns the directory this index resides in. */
	CL_NS(store)::Directory* getDirectory();

	/** Get the current setting of whether to use the compound file format.
	*  Note that this just returns the value you set with setUseCompoundFile(boolean)
	*  or the default. You cannot use this to query the status of an existing index.
	*  @see #setUseCompoundFile(boolean)
	*/
	bool getUseCompoundFile();

	/** Setting to turn on usage of a compound file. When on, multiple files
	*  for each segment are merged into a single file once the segment creation
	*  is finished. This is done regardless of what directory is in use.
	*/
	void setUseCompoundFile(bool value);


	/** Expert: Set the Similarity implementation used by this IndexWriter.
	*
	* @see Similarity#setDefault(Similarity)
	*/
	void setSimilarity(CL_NS(search)::Similarity* similarity);

	/** Expert: Return the Similarity implementation used by this IndexWriter.
	*
	* <p>This defaults to the current value of {@link Similarity#getDefault()}.
	*/
	CL_NS(search)::Similarity* getSimilarity();

	/** Returns the analyzer used by this index. */
	CL_NS(analysis)::Analyzer* getAnalyzer();

	// synchronized
	std::string newSegmentName();
private:
	class LockWith2;
	class LockWithCFS;
	friend class LockWith2;
	friend class LockWithCFS;

	/** Merges all RAM-resident segments. */
	void flushRamSegments();

	/** Incremental segment merger. */
	void maybeMergeSegments();

	/** Pops segments off of segmentInfos stack down to minSegment, merges them,
	* 	and pushes the merged index onto the top of the segmentInfos stack.
	*/
	void mergeSegments(const uint32_t minSegment);
	
	/** Merges the named range of segments, replacing them in the stack with a
	* single segment. */
	void mergeSegments(const uint32_t minSegment, const uint32_t end);

  void deleteFiles(std::vector<std::string>& files);
	void readDeleteableFiles(std::vector<std::string>& files);
	void writeDeleteableFiles(std::vector<std::string>& files);
    
	/*
	* Some operating systems (e.g. Windows) don't permit a file to be deleted
	* while it is opened for read (e.g. by another process or thread). So we
	* assume that when a delete fails it is because the file is open in another
	* process, and queue the file for subsequent deletion.
	*/
	void deleteSegments(CL_NS(util)::CLVector<SegmentReader*>* segments);
	void deleteFiles(std::vector<std::string>& files, CL_NS(store)::Directory* directory);
	void deleteFiles(std::vector<std::string>& files, std::vector<std::string>& deletable);
};

CL_NS_END
#endif
