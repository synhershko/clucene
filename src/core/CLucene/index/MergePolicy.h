/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_index_MergeScheduler_
#define _lucene_index_MergeScheduler_

CL_CLASS_DEF(store,Directory)
CL_NS_DEF(index)

/**
 * <p>Expert: a MergePolicy determines the sequence of
 * primitive merge operations to be used for overall merge
 * and optimize operations.</p>
 *
 * <p>Whenever the segments in an index have been altered by
 * {@link IndexWriter}, either the addition of a newly
 * flushed segment, addition of many segments from
 * addIndexes* calls, or a previous merge that may now need
 * to cascade, {@link IndexWriter} invokes {@link
 * #findMerges} to give the MergePolicy a chance to pick
 * merges that are now required.  This method returns a
 * {@link MergeSpecification} instance describing the set of
 * merges that should be done, or null if no merges are
 * necessary.  When IndexWriter.optimize is called, it calls
 * {@link #findMergesForOptimize} and the MergePolicy should
 * then return the necessary merges.</p>
 *
 * <p>Note that the policy can return more than one merge at
 * a time.  In this case, if the writer is using {@link
 * SerialMergeScheduler}, the merges will be run
 * sequentially but if it is using {@link
 * ConcurrentMergeScheduler} they will be run concurrently.</p>
 *
 * <p>The default MergePolicy is {@link
 * LogByteSizeMergePolicy}.</p>
 * <p><b>NOTE:</b> This API is new and still experimental
 * (subject to change suddenly in the next release)</p>
 */
class MergePolicy {
public:
  /** OneMerge provides the information necessary to perform
   *  an individual primitive merge operation, resulting in
   *  a single new segment.  The merge spec includes the
   *  subset of segments to be merged as well as whether the
   *  new segment should use the compound file format. */
  class OneMerge {
  public:
    DEFINE_MUTEX(THIS_LOCK)
    SegmentInfo* info;               // used by IndexWriter
    bool mergeDocStores;         // used by IndexWriter
    bool optimize;               // used by IndexWriter
    SegmentInfos* segmentsClone;     // used by IndexWriter
    bool increfDone;             // used by IndexWriter
    bool registerDone;           // used by IndexWriter
    int64_t mergeGen;                  // used by IndexWriter
    bool isExternal;             // used by IndexWriter
    int32_t maxNumSegmentsOptimize;     // used by IndexWriter

    const SegmentInfos* segments;
    const bool useCompoundFile;
    bool aborted;
    CLuceneError error;

    OneMerge(SegmentInfos* segments, bool _useCompoundFile):
      useCompoundFile(_useCompoundFile)
    {
      if (0 == segments->size())
        _CLTHROWA(CL_ERR_Runtime,"segments must include at least one segment");
      this->segments = segments;
    }

    /** Record that an exception occurred while executing
     *  this merge */
    void setException(CLuceneError& error) {
      SCOPED_LOCK_MUTEX(THIS_LOCK)
      this->error.set(error.number(),error.what());
    }

    /** Retrieve previous exception set by {@link
     *  #setException}. */
    const CLuceneError& getException() {
      SCOPED_LOCK_MUTEX(THIS_LOCK)
      return error;
    }

    /** Mark this merge as aborted.  If this is called
     *  before the merge is committed then the merge will
     *  not be committed. */
    void abort() {
      SCOPED_LOCK_MUTEX(THIS_LOCK)
      aborted = true;
    }

    /** Returns true if this merge was aborted. */
    bool isAborted() {
      SCOPED_LOCK_MUTEX(THIS_LOCK)
      return aborted;
    }

    void checkAborted(CL_NS(store)::Directory* dir){
      SCOPED_LOCK_MUTEX(THIS_LOCK)
      if (aborted)
        _CLTHROWA(CL_ERR_MergeAborted, (string("merge is aborted: ") + segString(dir)).c_str() );
    }

    std::string segString(CL_NS(store)::Directory* dir) {
      std::string b;
      const int32_t numSegments = segments->size();
      for(int32_t i=0;i<numSegments;i++) {
        if (i > 0) b.append(" ");
        b.append(segments->info(i)->segString(dir));
      }
      if (info != NULL)
        b.append(" into ").append(info->name);
      if (optimize)
        b.append(" [optimize]");
      return b;
    }
  };

  /**
   * A MergeSpecification instance provides the information
   * necessary to perform multiple merges.  It simply
   * contains a list of {@link OneMerge} instances.
   */

  class MergeSpecification {
  public:
    /**
     * The subset of segments to be included in the primitive merge.
     */

    CL_NS(util)::CLArrayList<OneMerge*> merges;

    void add(OneMerge* merge) {
      merges.push_back(merge);
    }

    std::string segString(CL_NS(store)::Directory* dir) {
      std::string b = "MergeSpec:\n";
      int32_t count = merges.size();
      for(int32_t i=0;i<count;i++){
        b.append("  ").append(1 + i).append(": ").append(merges[i]->segString(dir));
      }
      return b.toString();
    }
  };


  /**
   * Determine what set of merge operations are now
   * necessary on the index.  The IndexWriter calls this
   * whenever there is a change to the segments.  This call
   * is always synchronized on the IndexWriter instance so
   * only one thread at a time will call this method.
   *
   * @param segmentInfos the total set of segments in the index
   * @param writer IndexWriter instance
   */
  virtual MergeSpecification* findMerges(SegmentInfos* segmentInfos,
                                         IndexWriter* writer) = 0;

  /**
   * Determine what set of merge operations are necessary in
   * order to optimize the index.  The IndexWriter calls
   * this when its optimize() method is called.  This call
   * is always synchronized on the IndexWriter instance so
   * only one thread at a time will call this method.
   *
   * @param segmentInfos the total set of segments in the index
   * @param writer IndexWriter instance
   * @param maxSegmentCount requested maximum number of
   *   segments in the index (currently this is always 1)
   * @param segmentsToOptimize contains the specific
   *   SegmentInfo instances that must be merged away.  This
   *   may be a subset of all SegmentInfos.
   */
  virtual MergeSpecification* findMergesForOptimize(SegmentInfos* segmentInfos,
                                                    IndexWriter* writer,
                                                    int32_t maxSegmentCount,
                                                    std::vector<string>& segmentsToOptimize) = 0;

  /**
   * Release all resources for the policy.
   */
  virtual void close() = 0;

  /**
   * Returns true if a newly flushed (not from merge)
   * segment should use the compound file format.
   */
  virtual bool useCompoundFile(SegmentInfos* segments, SegmentInfo* newSegment) = 0;

  /**
   * Returns true if the doc store files should use the
   * compound file format.
   */
  virtual bool useCompoundDocStore(SegmentInfos* segments) = 0;
};













/** <p>This class implements a {@link MergePolicy} that tries
 *  to merge segments into levels of exponentially
 *  increasing size, where each level has < mergeFactor
 *  segments in it.  Whenever a given levle has mergeFactor
 *  segments or more in it, they will be merged.</p>
 *
 * <p>This class is abstract and requires a subclass to
 * define the {@link #size} method which specifies how a
 * segment's size is determined.  {@link LogDocMergePolicy}
 * is one subclass that measures size by document count in
 * the segment.  {@link LogByteSizeMergePolicy} is another
 * subclass that measures size as the total byte size of the
 * file(s) for the segment.</p>
 */
class LogMergePolicy: public MergePolicy {

  int32_t mergeFactor;

  int32_t maxMergeDocs;

  bool useCompoundFile;
  bool useCompoundDocStore;
  IndexWriter* writer;

  void message(const char* message) {
    if (writer != NULL)
      writer->message( (string("LMP: ") + message).c_str());
  }

  bool isOptimized(SegmentInfos* infos, IndexWriter* writer, int32_t maxNumSegments, Set segmentsToOptimize){
    const int32_t numSegments = infos->size();
    int32_t numToOptimize = 0;
    SegmentInfo* optimizeInfo = NULL;
    for(int32_t i=0;i<numSegments && numToOptimize <= maxNumSegments;i++) {
      SegmentInfo* info = infos->info(i);
      if (segmentsToOptimize.contains(info)) {
        numToOptimize++;
        optimizeInfo = info;
      }
    }

    return numToOptimize <= maxNumSegments &&
      (numToOptimize != 1 || isOptimized(writer, optimizeInfo));
  }

  /** Returns true if this single nfo is optimized (has no
   *  pending norms or deletes, is in the same dir as the
   *  writer, and matches the current compound file setting */
  bool isOptimized(IndexWriter* writer, SegmentInfo* info){
    return !info->hasDeletions() &&
      !info->hasSeparateNorms() &&
      info->dir == writer->getDirectory() &&
      info->getUseCompoundFile() == useCompoundFile;
  }


protected:
  virtual int64_t size(const SegmentInfo* info) = 0;
  int64_t minMergeSize;
  int64_t maxMergeSize;

public:
  LogMergePolicy(){
    this->maxMergeDocs = DEFAULT_MAX_MERGE_DOCS;
    this->mergeFactor = DEFAULT_MERGE_FACTOR;
    this->useCompoundFile = true;
    this->useCompoundDocStore = true;
  }

  /** Defines the allowed range of log(size) for each
   *  level.  A level is computed by taking the max segment
   *  log size, minuse LEVEL_LOG_SPAN, and finding all
   *  segments falling within that range. */
  LUCENE_STATIC_DEFINE(float_t, LEVEL_LOG_SPAN = 0.75);

  /** Default merge factor, which is how many segments are
   *  merged at a time */
  LUCENE_STATIC_DEFINE(int32_t, DEFAULT_MERGE_FACTOR = 10);

  /** Default maximum segment size.  A segment of this size
   *  or larger will never be merged.  @see setMaxMergeDocs */
  LUCENE_STATIC_DEFINE(int32_t, DEFAULT_MAX_MERGE_DOCS = LUCENE_INT32_MAX_SHOULDBE);

  /** <p>Returns the number of segments that are merged at
   * once and also controls the total number of segments
   * allowed to accumulate in the index.</p> */
  int32_t getMergeFactor() {
    return mergeFactor;
  }

  /** Determines how often segment indices are merged by
   * addDocument().  With smaller values, less RAM is used
   * while indexing, and searches on unoptimized indices are
   * faster, but indexing speed is slower.  With larger
   * values, more RAM is used during indexing, and while
   * searches on unoptimized indices are slower, indexing is
   * faster.  Thus larger values (> 10) are best for batch
   * index creation, and smaller values (< 10) for indices
   * that are interactively maintained. */
  void setMergeFactor(int32_t mergeFactor) {
    if (mergeFactor < 2)
      _CLTHROWA(CL_ERR_IllegalArgument, "mergeFactor cannot be less than 2");
    this->mergeFactor = mergeFactor;
  }

  // Javadoc inherited
  bool useCompoundFile(SegmentInfos infos, SegmentInfo info) {
    return useCompoundFile;
  }

  /** Sets whether compound file format should be used for
   *  newly flushed and newly merged segments. */
  void setUseCompoundFile(bool useCompoundFile) {
    this->useCompoundFile = useCompoundFile;
  }

  /** Returns true if newly flushed and newly merge segments
   *  are written in compound file format. @see
   *  #setUseCompoundFile */
  bool getUseCompoundFile() {
    return useCompoundFile;
  }

  // Javadoc inherited
  bool useCompoundDocStore(SegmentInfos* infos) {
    return useCompoundDocStore;
  }

  /** Sets whether compound file format should be used for
   *  newly flushed and newly merged doc store
   *  segment files (term vectors and stored fields). */
  void setUseCompoundDocStore(bool useCompoundDocStore) {
    this->useCompoundDocStore = useCompoundDocStore;
  }

  /** Returns true if newly flushed and newly merge doc
   *  store segment files (term vectors and stored fields)
   *  are written in compound file format. @see
   *  #setUseCompoundDocStore */
  bool getUseCompoundDocStore() {
    return useCompoundDocStore;
  }

  void close() {}


  /** Returns the merges necessary to optimize the index.
   *  This merge policy defines "optimized" to mean only one
   *  segment in the index, where that segment has no
   *  deletions pending nor separate norms, and it is in
   *  compound file format if the current useCompoundFile
   *  setting is true.  This method returns multiple merges
   *  (mergeFactor at a time) so the {@link MergeScheduler}
   *  in use may make use of concurrency. */
  MergeSpecification* findMergesForOptimize(SegmentInfos* infos, IndexWriter* writer, int32_t maxNumSegments, Set segmentsToOptimize) {
    MergeSpecification* spec;

    assert (maxNumSegments > 0);

    if (!isOptimized(infos, writer, maxNumSegments, segmentsToOptimize)) {

      // Find the newest (rightmost) segment that needs to
      // be optimized (other segments may have been flushed
      // since optimize started):
      int32_t last = infos->size();
      while(last > 0) {
        const SegmentInfo* info = infos->info(--last);
        if (segmentsToOptimize.contains(info)) {
          last++;
          break;
        }
      }

      if (last > 0) {

        spec = _CLNEW MergeSpecification();

        // First, enroll all "full" merges (size
        // mergeFactor) to potentially be run concurrently:
        while (last - maxNumSegments + 1 >= mergeFactor) {
          spec->add(new OneMerge(infos->range(last-mergeFactor, last), useCompoundFile));
          last -= mergeFactor;
        }

        // Only if there are no full merges pending do we
        // add a final partial (< mergeFactor segments) merge:
        if (0 == spec->merges.size()) {
          if (maxNumSegments == 1) {

            // Since we must optimize down to 1 segment, the
            // choice is simple:
            if (last > 1 || !isOptimized(writer, infos->info(0)))
              spec->add(new OneMerge(infos->range(0, last), useCompoundFile));
          } else if (last > maxNumSegments) {

            // Take care to pick a partial merge that is
            // least cost, but does not make the index too
            // lopsided.  If we always just picked the
            // partial tail then we could produce a highly
            // lopsided index over time:

            // We must merge this many segments to leave
            // maxNumSegments in the index (from when
            // optimize was first kicked off):
            const int32_t finalMergeSize = last - maxNumSegments + 1;

            // Consider all possible starting points:
            int64_t bestSize = 0;
            int32_t bestStart = 0;

            for(int32_t i=0;i<last-finalMergeSize+1;i++) {
              int64_t sumSize = 0;
              for(int32_t j=0;j<finalMergeSize;j++)
                sumSize += size(infos->info(j+i));
              if (i == 0 || (sumSize < 2*size(infos->info(i-1)) && sumSize < bestSize)) {
                bestStart = i;
                bestSize = sumSize;
              }
            }

            spec->add(new OneMerge(infos->range(bestStart, bestStart+finalMergeSize), useCompoundFile));
          }
        }

      } else
        spec = NULL;
    } else
      spec = NULL;

    return spec;
  }

  /** Checks if any merges are now necessary and returns a
   *  {@link MergePolicy.MergeSpecification} if so.  A merge
   *  is necessary when there are more than {@link
   *  #setMergeFactor} segments at a given level.  When
   *  multiple levels have too many segments, this method
   *  will return multiple merges, allowing the {@link
   *  MergeScheduler} to use concurrency. */
  MergeSpecification* findMerges(SegmentInfos* infos, IndexWriter* writer){

    const int32_t numSegments = infos->size();
    this->writer = writer;
    message("findMerges: " + numSegments + " segments");

    // Compute levels, which is just log (base mergeFactor)
    // of the size of each segment
    float_t* levels = _CL_NEWARRAY(float_t,numSegments);
    const float_t norm = log(mergeFactor);

    const Directory* directory = writer->getDirectory();

    for(int32_t i=0;i<numSegments;i++) {
      const SegmentInfo* info = infos->info(i);
      int64_t size = size(info);

      // Floor tiny segments
      if (size < 1)
        size = 1;
      levels[i] = log(size)/norm;
    }

    float_t levelFloor;
    if (minMergeSize <= 0)
      levelFloor = 0.0;
    else
      levelFloor = log(minMergeSize)/norm;

    // Now, we quantize the log values into levels.  The
    // first level is any segment whose log size is within
    // LEVEL_LOG_SPAN of the max size, or, who has such as
    // segment "to the right".  Then, we find the max of all
    // other segments and use that to define the next level
    // segment, etc.

    MergeSpecification* spec = NULL;

    int32_t start = 0;
    while(start < numSegments) {

      // Find max level of all segments not already
      // quantized.
      float_t maxLevel = levels[start];
      for(int32_t i=1+start;i<numSegments;i++) {
        const float_t level = levels[i];
        if (level > maxLevel)
          maxLevel = level;
      }

      // Now search backwards for the rightmost segment that
      // falls into this level:
      float_t levelBottom;
      if (maxLevel < levelFloor)
        // All remaining segments fall into the min level
        levelBottom = -1.0F;
      else {
        levelBottom = maxLevel - LEVEL_LOG_SPAN;

        // Force a boundary at the level floor
        if (levelBottom < levelFloor && maxLevel >= levelFloor)
          levelBottom = levelFloor;
      }

      int32_t upto = numSegments-1;
      while(upto >= start) {
        if (levels[upto] >= levelBottom) {
          break;
        }
        upto--;
      }
      message("  level " + levelBottom + " to " + maxLevel + ": " + (1+upto-start) + " segments");

      // Finally, record all merges that are viable at this level:
      int32_t end = start + mergeFactor;
      while(end <= 1+upto) {
        bool anyTooLarge = false;
        for(int32_t i=start;i<end;i++) {
          const SegmentInfo* info = infos->info(i);
          anyTooLarge |= (size(info) >= maxMergeSize || info->docCount >= maxMergeDocs);
        }

        if (!anyTooLarge) {
          if (spec == NULL)
            spec = _CLNEW MergeSpecification();
          message("    " + start + " to " + end + ": add this merge");
          spec->add(new OneMerge(infos->range(start, end), useCompoundFile));
        } else
          message("    " + start + " to " + end + ": contains segment over maxMergeSize or maxMergeDocs; skipping");

        start = end;
        end = start + mergeFactor;
      }

      start = 1+upto;
    }

    return spec;
  }

  /** <p>Determines the largest segment (measured by
   * document count) that may be merged with other segments.
   * Small values (e.g., less than 10,000) are best for
   * interactive indexing, as this limits the length of
   * pauses while indexing to a few seconds.  Larger values
   * are best for batched indexing and speedier
   * searches.</p>
   *
   * <p>The default value is {@link Integer#MAX_VALUE}.</p>
   *
   * <p>The default merge policy ({@link
   * LogByteSizeMergePolicy}) also allows you to set this
   * limit by net size (in MB) of the segment, using {@link
   * LogByteSizeMergePolicy#setMaxMergeMB}.</p>
   */
  void setMaxMergeDocs(int32_t maxMergeDocs) {
    this->maxMergeDocs = maxMergeDocs;
  }

  /** Returns the largest segment (measured by document
   *  count) that may be merged with other segments.
   *  @see #setMaxMergeDocs */
  int32_t getMaxMergeDocs() {
    return maxMergeDocs;
  }

};






/** This is a {@link LogMergePolicy} that measures size of a
 *  segment as the number of documents (not taking deletions
 *  into account). */
class LogDocMergePolicy: public LogMergePolicy {
public:

  /** Default minimum segment size.  @see setMinMergeDocs */
  LUCENE_STATIC_DEFINE(int32_t, DEFAULT_MIN_MERGE_DOCS = 1000);

  LogDocMergePolicy() {
    minMergeSize = DEFAULT_MIN_MERGE_DOCS;

    // maxMergeSize is never used by LogDocMergePolicy; set
    // it to Long.MAX_VALUE to disable it
    maxMergeSize = LUCENE_INT64_MAX;
  }

  /** Sets the minimum size for the lowest level segments.
   * Any segments below this size are considered to be on
   * the same level (even if they vary drastically in size)
   * and will be merged whenever there are mergeFactor of
   * them.  This effectively truncates the "int64_t tail" of
   * small segments that would otherwise be created into a
   * single level.  If you set this too large, it could
   * greatly increase the merging cost during indexing (if
   * you flush many small segments). */
  void setMinMergeDocs(int32_t minMergeDocs) {
    minMergeSize = minMergeDocs;
  }

  /** Get the minimum size for a segment to remain
   *  un-merged.
   *  @see #setMinMergeDocs **/
  int32_t getMinMergeDocs() {
    return minMergeSize;
  }
protected:
  int64_t size(const SegmentInfo* info) {
    return info->docCount;
  }
}

CL_NS_END
#endif

