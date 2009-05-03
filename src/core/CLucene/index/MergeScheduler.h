/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_index_MergeScheduler_
#define _lucene_index_MergeScheduler_

CL_NS_DEF(index)

/** Expert: {@link IndexWriter} uses an instance
 *  implementing this interface to execute the merges
 *  selected by a {@link MergePolicy}.  The default
 *  MergeScheduler is {@link ConcurrentMergeScheduler}.
 * <p><b>NOTE:</b> This API is new and still experimental
 * (subject to change suddenly in the next release)</p>
*/
class MergeScheduler {
public:
  /** Run the merges provided by {@link IndexWriter#getNextMerge()}. */
  void merge(IndexWriter* writer) = 0;

  /** Close this MergeScheduler. */
  void close() = 0;
}

/** A {@link MergeScheduler} that simply does each merge
 *  sequentially, using the current thread. */
class SerialMergeScheduler: public MergeScheduler {
public:
  DEFINE_MUTEX(THIS_LOCK)

  /** Just do the merges in sequence. We do this
   * "synchronized" so that even if the application is using
   * multiple threads, only one merge may run at a time. */
  void merge(IndexWriter* writer){
    SCOPED_LOCK_MUTEX(THIS_LOCK)
    while(true) {
      MergePolicy::OneMerge* merge = writer->getNextMerge();
      if (merge == NULL)
        break;
      writer->merge(merge);
    }
  }

  void close() {}
}


CL_NS_END
#endif
