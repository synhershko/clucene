
/*
 * This class keeps track of each SegmentInfos instance that
 * is still "live", either because it corresponds to a 
 * segments_N file in the Directory (a "commit", i.e. a 
 * committed SegmentInfos) or because it's the in-memory SegmentInfos 
 * that a writer is actively updating but has not yet committed 
 * (currently this only applies when autoCommit=false in IndexWriter).
 * This class uses simple reference counting to map the live
 * SegmentInfos instances to individual files in the Directory. 
 * 
 * The same directory file may be referenced by more than
 * one IndexCommitPoints, i.e. more than one SegmentInfos.
 * Therefore we count how many commits reference each file.
 * When all the commits referencing a certain file have been
 * deleted, the refcount for that file becomes zero, and the
 * file is deleted.
 *
 * A separate deletion policy interface
 * (IndexDeletionPolicy) is consulted on creation (onInit)
 * and once per commit (onCommit), to decide when a commit
 * should be removed.
 * 
 * It is the business of the IndexDeletionPolicy to choose
 * when to delete commit points.  The actual mechanics of
 * file deletion, retrying, etc, derived from the deletion
 * of commit points is the business of the IndexFileDeleter.
 * 
 * The current default deletion policy is {@link
 * KeepOnlyLastCommitDeletionPolicy}, which removes all
 * prior commits when a new commit has completed.  This
 * matches the behavior before 2.2.
 *
 * Note that you must hold the write.lock before
 * instantiating this class.  It opens segments_N file(s)
 * directly with no retry logic.
 */
class IndexFileDeleter {
private:
  /* Files that we tried to delete but failed (likely
   * because they are open and we are running on Windows),
   * so we will retry them again later: */
  List deletable;

  /* Reference count for all files in the index.  
   * Counts how many existing commits reference a file.
   * Maps String to RefCount (class below) instances: */
  Map refCounts = new HashMap();

  /* Holds all commits (segments_N) currently in the index.
   * This will have just 1 commit if you are using the
   * default delete policy (KeepOnlyLastCommitDeletionPolicy).
   * Other policies may leave commit points live for longer
   * in which case this list would be longer than 1: */
  List commits = new ArrayList();

  /* Holds files we had incref'd from the previous
   * non-commit checkpoint: */
  List lastFiles = new ArrayList();

  /* Commits that the IndexDeletionPolicy have decided to delete: */ 
  List commitsToDelete = new ArrayList();

  PrintStream infoStream;
  Directory directory;
  IndexDeletionPolicy policy;
  DocumentsWriter docWriter;
  void deletePendingFiles();

  void setInfoStream(PrintStream infoStream);
  void message(String message);
  void decRef(String fileName);
  RefCount getRefCount(String fileName);

  /**
   * Remove the CommitPoints in the commitsToDelete List by
   * DecRef'ing all files from each SegmentInfos.
   */
  void deleteCommits();

  /**
   * Holds details for each commit point.  This class is
   * also passed to the deletion policy.  Note: this class
   * has a natural ordering that is inconsistent with
   * equals.
   */

  final class CommitPoint implements Comparable, IndexCommitPoint {
    long gen;
    List files;
    String segmentsFileName;
    boolean deleted;

    public CommitPoint(SegmentInfos segmentInfos) throws IOException {
      segmentsFileName = segmentInfos.getCurrentSegmentFileName();
      int size = segmentInfos.size();
      files = new ArrayList(size);
      files.add(segmentsFileName);
      gen = segmentInfos.getGeneration();
      for(int i=0;i<size;i++) {
        SegmentInfo segmentInfo = segmentInfos.info(i);
        if (segmentInfo.dir == directory) {
          files.addAll(segmentInfo.files());
        }
      }
    }

    /**
     * Get the segments_N file for this commit point.
     */
    public String getSegmentsFileName() {
      return segmentsFileName;
    }

    public Collection getFileNames() throws IOException {
      return Collections.unmodifiableCollection(files);
    }

    /**
     * Called only be the deletion policy, to remove this
     * commit point from the index.
     */
    public void delete() {
      if (!deleted) {
        deleted = true;
        commitsToDelete.add(this);
      }
    }

    public int compareTo(Object obj) {
      CommitPoint commit = (CommitPoint) obj;
      if (gen < commit.gen) {
        return -1;
      } else if (gen > commit.gen) {
        return 1;
      } else {
        return 0;
      }
    }
  }
public:
  /** Change to true to see details of reference counts when
   *  infoStream != null */
  static boolean VERBOSE_REF_COUNTS = false;

  /**
   * Initialize the deleter: find all previous commits in
   * the Directory, incref the files they reference, call
   * the policy to let it delete commits.  The incoming
   * segmentInfos must have been loaded from a commit point
   * and not yet modified.  This will remove any files not
   * referenced by any of the commits.
   * @throws CorruptIndexException if the index is corrupt
   * @throws IOException if there is a low-level IO error
   */
  IndexFileDeleter(Directory directory, IndexDeletionPolicy policy, SegmentInfos segmentInfos, PrintStream infoStream, DocumentsWriter docWriter);

  /**
   * Writer calls this when it has hit an error and had to
   * roll back, to tell us that there may now be
   * unreferenced files in the filesystem.  So we re-list
   * the filesystem and delete such files.  If segmentName
   * is non-null, we will only delete files corresponding to
   * that segment.
   */
  void refresh(String segmentName);
  void refresh();
  void close();

  /**
   * For definition of "check point" see IndexWriter comments:
   * "Clarification: Check Points (and commits)".
   * 
   * Writer calls this when it has made a "consistent
   * change" to the index, meaning new files are written to
   * the index and the in-memory SegmentInfos have been
   * modified to point to those files.
   *
   * This may or may not be a commit (segments_N may or may
   * not have been written).
   *
   * We simply incref the files referenced by the new
   * SegmentInfos and decref the files we had previously
   * seen (if any).
   *
   * If this is a commit, we also call the policy to give it
   * a chance to remove other commits.  If any commits are
   * removed, we decref their files as well.
   */
  void checkpoint(SegmentInfos segmentInfos, boolean isCommit);


  void CLUCENE_LOCAL_DECL incRef(SegmentInfos segmentInfos, boolean isCommit);
  void CLUCENE_LOCAL_DECL incRef(List files);
  void CLUCENE_LOCAL_DECL decRef(List files) ;
  void CLUCENE_LOCAL_DECL decRef(SegmentInfos segmentInfos);
  void CLUCENE_LOCAL_DECL deleteFiles(List files);

  /** Delets the specified files, but only if they are new
   *  (have not yet been incref'd). */
  void CLUCENE_LOCAL_DECL deleteNewFiles(List files);
  void CLUCENE_LOCAL_DECL deleteFile(String fileName);
}
