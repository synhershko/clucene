/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "DirectoryIndexReader.h"
#include "CLucene/store/Directory.h"

CL_NS_USE(store)
CL_NS_USE(util)

CL_NS_DEF(index)


  void DirectoryIndexReader::doClose() {
    if(closeDirectory)
      _directory->close();
  }

  void DirectoryIndexReader::doCommit() {
    if(hasChanges){
      if (segmentInfos != NULL) {

        // Default deleter (for backwards compatibility) is
        // KeepOnlyLastCommitDeleter:
        IndexFileDeleter* deleter =  _CLNEW IndexFileDeleter(directory,
                                                         deletionPolicy == NULL ? _CLNEW KeepOnlyLastCommitDeletionPolicy() : deletionPolicy,
                                                         segmentInfos, NULL, NULL);

        // Checkpoint the state we are about to change, in
        // case we have to roll back:
        startCommit();

        bool success = false;
        try {
          commitChanges();
          segmentInfos.write(directory);
          success = true;
        } _CLFINALLY (

          if (!success) {

            // Rollback changes that were made to
            // SegmentInfos but failed to get [fully]
            // committed.  This way this reader instance
            // remains consistent (matched to what's
            // actually in the index):
            rollbackCommit();

            // Recompute deletable files & remove them (so
            // partially written .del files, etc, are
            // removed):
            deleter.refresh();
          }
        )

        // Have the deleter remove any now unreferenced
        // files due to this commit:
        deleter.checkpoint(segmentInfos, true);

        if (writeLock != NULL) {
          writeLock.release();  // release write lock
          writeLock = NULL;
        }
      }
      else
        commitChanges();
    }
    hasChanges = false;
  }

  void DirectoryIndexReader::acquireWriteLock() {
    if (segmentInfos != NULL) {
      ensureOpen();
      if (stale)
        throw _CLNEW StaleReaderException("IndexReader out of date and no longer valid for delete, undelete, or setNorm operations");

      if (writeLock == NULL) {
        LuceneLock* writeLock = _directory->makeLock(IndexWriter::WRITE_LOCK_NAME);
        if (!writeLock.obtain(IndexWriter::WRITE_LOCK_TIMEOUT)) // obtain write lock
          throw _CLNEW LockObtainFailedException("Index locked for write: " + writeLock);
        this.writeLock = writeLock;

        // we have to check whether index has changed since this reader was opened.
        // if so, this reader is no longer valid for deletion
        if (SegmentInfos::readCurrentVersion(directory) > segmentInfos.getVersion()) {
          stale = true;
          this.writeLock.release();
          this.writeLock = NULL;
          throw _CLNEW StaleReaderException("IndexReader out of date and no longer valid for delete, undelete, or setNorm operations");
        }
      }
    }
  }

  void DirectoryIndexReader::finalize() {
    try {
      if (writeLock != NULL) {
        writeLock.release();                        // release write lock
        writeLock = NULL;
      }
    } _CLFINALLY (
      super.finalize();
    )
  }

  void DirectoryIndexReader::init(Directory* directory, SegmentInfos* segmentInfos, bool closeDirectory) {
    this.directory = directory;
    this.segmentInfos = segmentInfos;
    this.closeDirectory = closeDirectory;
  }

  DirectoryIndexReader::DirectoryIndexReader(){}
  DirectoryIndexReader::DirectoryIndexReader(Directory* directory, SegmentInfos* segmentInfos,
      bool closeDirectory) {
    super();
    init(directory, segmentInfos, closeDirectory);
  }

  DirectoryIndexReader::DirectoryIndexReader open(Directory* directory, bool closeDirectory, IndexDeletionPolicy* deletionPolicy) {

    return (DirectoryIndexReader) _CLNEW SegmentInfos::FindSegmentsFile(directory) {

      protected Object doBody(String segmentFileName) {

        SegmentInfos infos = _CLNEW SegmentInfos();
        infos.read(directory, segmentFileName);

        DirectoryIndexReader reader;

        if (infos.size() == 1) {          // index is optimized
          reader = SegmentReader.get(infos, infos.info(0), closeDirectory);
        } else {
          reader = _CLNEW MultiSegmentReader(directory, infos, closeDirectory);
        }
        reader.setDeletionPolicy(deletionPolicy);
        return reader;
      }
    }.run();
  }


  IndexReader* DirectoryIndexReader::reopen(){
    LUCENE_SCOPED_LOCK(THIS_LOCK)
    ensureOpen();

    if (this.hasChanges || this.isCurrent()) {
      // the index hasn't changed - nothing to do here
      return this;
    }

    return (DirectoryIndexReader*) _CLNEW SegmentInfos::FindSegmentsFile(directory) {

      protected Object doBody(String segmentFileName){
        SegmentInfos infos = _CLNEW SegmentInfos();
        infos.read(directory, segmentFileName);

        DirectoryIndexReader _CLNEWReader = doReopen(infos);

        if (DirectoryIndexReader.this != _CLNEWReader) {
          _CLNEWReader.init(directory, infos, closeDirectory);
          _CLNEWReader.deletionPolicy = deletionPolicy;
        }

        return _CLNEWReader;
      }
    }.run();
  }

  void DirectoryIndexReader::setDeletionPolicy(IndexDeletionPolicy* deletionPolicy) {
    this.deletionPolicy = deletionPolicy;
  }

  /** Returns the directory this index resides in.
   */
  Directory* DirectoryIndexReader::directory() {
    ensureOpen();
    return directory;
  }

  /**
   * Version number when this IndexReader was opened.
   */
  int64_t DirectoryIndexReader::getVersion() {
    ensureOpen();
    return segmentInfos.getVersion();
  }

  /**
   * Check whether this IndexReader is still using the
   * current (i.e., most recently committed) version of the
   * index.  If a writer has committed any changes to the
   * index since this reader was opened, this will return
   * <code>false</code>, in which case you must open a _CLNEW
   * IndexReader in order to see the changes.  See the
   * description of the <a href="IndexWriter.html#autoCommit"><code>autoCommit</code></a>
   * flag which controls when the {@link IndexWriter}
   * actually commits changes to the index.
   *
   * @throws CorruptIndexException if the index is corrupt
   * @throws IOException if there is a low-level IO error
   */
  bool DirectoryIndexReader::isCurrent(){
    ensureOpen();
    return SegmentInfos::readCurrentVersion(directory) == segmentInfos.getVersion();
  }

  /**
   * Checks is the index is optimized (if it has a single segment and no deletions)
   * @return <code>true</code> if the index is optimized; <code>false</code> otherwise
   */
  bool DirectoryIndexReader::isOptimized() {
    ensureOpen();
    return segmentInfos.size() == 1 && hasDeletions() == false;
  }

  /**
   * Should internally checkpoint state that will change
   * during commit so that we can rollback if necessary.
   */
  void DirectoryIndexReader::startCommit() {
    if (segmentInfos != NULL) {
      rollbackSegmentInfos = segmentInfos.clone();
    }
    rollbackHasChanges = hasChanges;
  }

  /**
   * Rolls back state to just before the commit (this is
   * called by commit() if there is some exception while
   * committing).
   */
  void DirectoryIndexReader::rollbackCommit() {
    if (segmentInfos != NULL) {
      for(int32_t i=0;i<segmentInfos.size();i++) {
        // Rollback each segmentInfo.  Because the
        // SegmentReader holds a reference to the
        // SegmentInfo we can't [easily] just replace
        // segmentInfos, so we reset it in place instead:
        segmentInfos.info(i).reset(rollbackSegmentInfos::info(i));
      }
      rollbackSegmentInfos = NULL;
    }

    hasChanges = rollbackHasChanges;
  }

CL_NS_END
