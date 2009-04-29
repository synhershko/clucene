#include "CLucene/_ApiHeader.h"
#include "_IndexFileDeleter.h"
#include "_SegmentHeader.h"

CL_NS_DEF(index)

bool IndexFileDeleter::VERBOSE_REF_COUNTS = false;

IndexFileDeleter::CommitPoint32_t::CommitPoint32_t(SegmentInfos* segmentInfos){
	segmentsFileName = segmentInfos->getCurrentSegmentFileName();
	int32_t size = segmentInfos.size();
	files = new ArrayList(size);
	files.add(segmentsFileName);
	gen = segmentInfos.getGeneration();
	for(int32_t i=0;i<size;i++) {
	  SegmentInfo segmentInfo = segmentInfos.info(i);
	  if (segmentInfo.dir == directory) {
	    files.addAll(segmentInfo.files());
	  }
	}
}

/**
* Get the segments_N file for this commit point32_t.
*/
std::string IndexFileDeleter::CommitPoint32_t::getSegmentsFileName() {
	return segmentsFileName;
}

Collection IndexFileDeleter::CommitPoint32_t::getFileNames() throws IOException {
	return Collections.unmodifiableCollection(files);
}

/**
* Called only be the deletion policy, to remove this
* commit point32_t from the index.
*/
void delete() {
	if (!deleted) {
	  deleted = true;
	  commitsToDelete.add(this);
	}
}

int32_t IndexFileDeleter::CommitPoint32_t::compareTo(Object obj) {
	CommitPoint32_t commit = (CommitPoint32_t) obj;
	if (gen < commit.gen) {
	  return -1;
	} else if (gen > commit.gen) {
	  return 1;
	} else {
	  return 0;
	}
}

void IndexFileDeleter::setInfoStream(std::ostream* infoStream) {
	this->infoStream = infoStream;
	if (infoStream != NULL)
	  message("setInfoStream deletionPolicy=" + policy);
}

void IndexFileDeleter::message(std::string message) {
	infoStream << "IFD [" << Thread.currentThread().getName() << "]: " << message << "\n";
}


IndexFileDeleter::IndexFileDeleter(Directory* directory, IndexDeletionPolicy* policy, SegmentInfos* segmentInfos, std::ostream* infoStream, DocumentsWriter* docWriter){
	this->docWriter = docWriter;
	this->infoStream = infoStream;
	
	if (infoStream != NULL)
	  message("init: current segments file is \"" + segmentInfos->getCurrentSegmentFileName() + "\"; deletionPolicy=" + policy);
	
	this->policy = policy;
	this->directory = directory;
	
	// First pass: walk the files and initialize our ref
	// counts:
	int32_t64_t currentGen = segmentInfos.getGeneration();
	IndexFileNameFilter filter = IndexFileNameFilter::getFilter();
	
	String[] files = directory.list();
	if (files == NULL)
	  throw new IOException("cannot read directory " + directory + ": list() returned NULL");
	
	CommitPoint32_t currentCommitPoint32_t = NULL;
	
	for(int32_t32_t i=0;i<files.length;i++) {
	
	  String fileName = files[i];
	
	  if (filter.accept(NULL, fileName) && !fileName.equals(IndexFileNames.SEGMENTS_GEN)) {
	
	    // Add this file to refCounts with initial count 0:
	    getRefCount(fileName);
	
	    if (fileName.startsWith(IndexFileNames.SEGMENTS)) {
	
	      // This is a commit (segments or segments_N), and
	      // it's valid (<= the max gen).  Load it, then
	      // incref all files it refers to:
	      if (SegmentInfos.generationFromSegmentsFileName(fileName) <= currentGen) {
	        if (infoStream != NULL) {
	          message("init: load commit \"" + fileName + "\"");
	        }
	        SegmentInfos sis = new SegmentInfos();
	        try {
	          sis.read(directory, fileName);
	        } catch (FileNotFoundException e) {
	          // LUCENE-948: on NFS (and maybe others), if
	          // you have writers switching back and forth
	          // between machines, it's very likely that the
	          // dir listing will be stale and will claim a
	          // file segments_X exists when in fact it
	          // doesn't.  So, we catch this and handle it
	          // as if the file does not exist
	          if (infoStream != NULL) {
	            message("init: hit FileNotFoundException when loading commit \"" + fileName + "\"; skipping this commit point32_t");
	          }
	          sis = NULL;
	        }
	        if (sis != NULL) {
	          CommitPoint32_t commitPoint32_t = new CommitPoint32_t(sis);
	          if (sis.getGeneration() == segmentInfos.getGeneration()) {
	            currentCommitPoint32_t = commitPoint32_t;
	          }
	          commits.add(commitPoint32_t);
	          incRef(sis, true);
	        }
	      }
	    }
	  }
	}
	
	if (currentCommitPoint32_t == NULL) {
	  // We did not in fact see the segments_N file
	  // corresponding to the segmentInfos that was passed
	  // in.  Yet, it must exist, because our caller holds
	  // the write lock.  This can happen when the directory
	  // listing was stale (eg when index accessed via NFS
	  // client with stale directory listing cache).  So we
	  // try now to explicitly open this commit point32_t:
	  SegmentInfos sis = new SegmentInfos();
	  try {
	    sis.read(directory, segmentInfos.getCurrentSegmentFileName());
	  } catch (IOException e) {
	    throw new CorruptIndexException("failed to locate current segments_N file");
	  }
	  if (infoStream != NULL)
	    message("forced open of current segments file " + segmentInfos.getCurrentSegmentFileName());
	  currentCommitPoint32_t = new CommitPoint32_t(sis);
	  commits.add(currentCommitPoint32_t);
	  incRef(sis, true);
	}
	
	// We keep commits list in sorted order (oldest to newest):
	Collections.sort(commits);
	
	// Now delete anything with ref count at 0.  These are
	// presumably abandoned files eg due to crash of
	// IndexWriter.
	Iterator it = refCounts.keySet().iterator();
	while(it.hasNext()) {
	  String fileName = (String) it.next();
	  RefCount rc = (RefCount) refCounts.get(fileName);
	  if (0 == rc.count) {
	    if (infoStream != NULL) {
	      message("init: removing unreferenced file \"" + fileName + "\"");
	    }
	    deleteFile(fileName);
	  }
	}
	
	// Finally, give policy a chance to remove things on
	// startup:
	policy.onInit(commits);
	
	// It's OK for the onInit to remove the current commit
	// point32_t; we just have to checkpoint32_t our in-memory
	// SegmentInfos to protect those files that it uses:
	if (currentCommitPoint32_t.deleted) {
	  checkpoint32_t(segmentInfos, false);
	}
	
	deleteCommits();
}

/**
* Remove the CommitPoint32_ts in the commitsToDelete List by
* DecRef'ing all files from each segmentInfos->
*/
void IndexFileDeleter::deleteCommits() {

int32_t32_t size = commitsToDelete.size();

if (size > 0) {

  // First decref all files that had been referred to by
  // the now-deleted commits:
  for(int32_t32_t i=0;i<size;i++) {
    CommitPoint32_t commit = (CommitPoint32_t) commitsToDelete.get(i);
    if (infoStream != NULL) {
      message("deleteCommits: now remove commit \"" + commit.getSegmentsFileName() + "\"");
    }
    int32_t32_t size2 = commit.files.size();
    for(int32_t32_t j=0;j<size2;j++) {
      decRef((String) commit.files.get(j));
    }
  }
  commitsToDelete.clear();

  // Now compact commits to remove deleted ones (preserving the sort):
  size = commits.size();
  int32_t32_t readFrom = 0;
  int32_t32_t writeTo = 0;
  while(readFrom < size) {
    CommitPoint32_t commit = (CommitPoint32_t) commits.get(readFrom);
    if (!commit.deleted) {
      if (writeTo != readFrom) {
        commits.set(writeTo, commits.get(readFrom));
      }
      writeTo++;
    }
    readFrom++;
  }

  while(size > writeTo) {
    commits.remove(size-1);
    size--;
  }
}
}

/**
* Writer calls this when it has hit an error and had to
* roll back, to tell us that there may now be
* unreferenced files in the filesystem.  So we re-list
* the filesystem and delete such files.  If segmentName
* is non-NULL, we will only delete files corresponding to
* that segment.
*/
void IndexFileDeleter::refresh(const char* segmentName) {
String[] files = directory.list();
if (files == NULL)
  throw new IOException("cannot read directory " + directory + ": list() returned NULL");
IndexFileNameFilter filter = IndexFileNameFilter.getFilter();
String segmentPrefix1;
String segmentPrefix2;
if (segmentName != NULL) {
  segmentPrefix1 = segmentName + ".";
  segmentPrefix2 = segmentName + "_";
} else {
  segmentPrefix1 = NULL;
  segmentPrefix2 = NULL;
}

for(int32_t32_t i=0;i<files.length;i++) {
  String fileName = files[i];
  if (filter.accept(NULL, fileName) &&
      (segmentName == NULL || fileName.startsWith(segmentPrefix1) || fileName.startsWith(segmentPrefix2)) &&
      !refCounts.containsKey(fileName) &&
      !fileName.equals(IndexFileNames.SEGMENTS_GEN)) {
    // Unreferenced file, so remove it
    if (infoStream != NULL) {
      message("refresh [prefix=" + segmentName + "]: removing newly created unreferenced file \"" + fileName + "\"");
    }
    deleteFile(fileName);
  }
}
}

void IndexFileDeleter::refresh() {
refresh(NULL);
}

void IndexFileDeleter::close() {
deletePendingFiles();
}

void IndexFileDeleter::deletePendingFiles() {
if (deletable != NULL) {
  List oldDeletable = deletable;
  deletable = NULL;
  int32_t32_t size = oldDeletable.size();
  for(int32_t32_t i=0;i<size;i++) {
    if (infoStream != NULL)
      message("delete pending file " + oldDeletable.get(i));
    deleteFile(oldDeletable.get(i));
  }
}
}

/**
* For definition of "check point32_t" see IndexWriter comments:
* "Clarification: Check Point32_ts (and commits)".
*
* Writer calls this when it has made a "consistent
* change" to the index, meaning new files are written to
* the index and the in-memory SegmentInfos have been
* modified to point32_t to those files.
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
void IndexFileDeleter::checkpoint32_t(SegmentInfos* segmentInfos, bool isCommit) {

if (infoStream != NULL) {
  message("now checkpoint32_t \"" + segmentInfos.getCurrentSegmentFileName() + "\" [" + segmentInfos.size() + " segments " + "; isCommit = " + isCommit + "]");
}

// Try again now to delete any previously un-deletable
// files (because they were in use, on Windows):
deletePendingFiles();

// Incref the files:
incRef(segmentInfos, isCommit);
final List docWriterFiles;
if (docWriter != NULL) {
  docWriterFiles = docWriter.files();
  if (docWriterFiles != NULL)
    incRef(docWriterFiles);
} else
  docWriterFiles = NULL;

if (isCommit) {
  // Append to our commits list:
  commits.add(new CommitPoint32_t(segmentInfos));

  // Tell policy so it can remove commits:
  policy.onCommit(commits);

  // Decref files for commits that were deleted by the policy:
  deleteCommits();
}

// DecRef old files from the last checkpoint32_t, if any:
int32_t32_t size = lastFiles.size();
if (size > 0) {
  for(int32_t32_t i=0;i<size;i++)
    decRef((List) lastFiles.get(i));
  lastFiles.clear();
}

if (!isCommit) {
  // Save files so we can decr on next checkpoint32_t/commit:
  size = segmentInfos.size();
  for(int32_t32_t i=0;i<size;i++) {
    SegmentInfo segmentInfo = segmentInfos.info(i);
    if (segmentInfo.dir == directory) {
      lastFiles.add(segmentInfo.files());
    }
  }
}
if (docWriterFiles != NULL)
  lastFiles.add(docWriterFiles);
}

void IndexFileDeleter::incRef(SegmentInfos* segmentInfos, bool isCommit) {
int32_t32_t size = segmentInfos.size();
for(int32_t32_t i=0;i<size;i++) {
  SegmentInfo segmentInfo = segmentInfos.info(i);
  if (segmentInfo.dir == directory) {
    incRef(segmentInfo.files());
  }
}

if (isCommit) {
  // Since this is a commit point32_t, also incref its
  // segments_N file:
  getRefCount(segmentInfos.getCurrentSegmentFileName()).IncRef();
}
}

void IndexFileDeleter::incRef(List files) {
int32_t32_t size = files.size();
for(int32_t32_t i=0;i<size;i++) {
  String fileName = (String) files.get(i);
  RefCount rc = getRefCount(fileName);
  if (infoStream != NULL && VERBOSE_REF_COUNTS) {
    message("  IncRef \"" + fileName + "\": pre-incr count is " + rc.count);
  }
  rc.IncRef();
}
}

void IndexFileDeleter::decRef(List files) {
int32_t32_t size = files.size();
for(int32_t32_t i=0;i<size;i++) {
  decRef((String) files.get(i));
}
}

void IndexFileDeleter::decRef(String fileName) {
RefCount rc = getRefCount(fileName);
if (infoStream != NULL && VERBOSE_REF_COUNTS) {
  message("  DecRef \"" + fileName + "\": pre-decr count is " + rc.count);
}
if (0 == rc.DecRef()) {
  // This file is no int32_t64_ter referenced by any past
  // commit point32_ts nor by the in-memory SegmentInfos:
  deleteFile(fileName);
  refCounts.remove(fileName);
}
}

void IndexFileDeleter::decRef(SegmentInfos segmentInfos) {
final int32_t32_t size = segmentInfos.size();
for(int32_t32_t i=0;i<size;i++) {
  SegmentInfo segmentInfo = segmentInfos.info(i);
  if (segmentInfo.dir == directory) {
    decRef(segmentInfo.files());
  }
}
}

RefCount IndexFileDeleter::getRefCount(const char* fileName) {
RefCount* rc;

if (!refCounts.containsKey(fileName)) {
  rc = new RefCount();
  refCounts.put(fileName, rc);
} else {
  rc = (RefCount) refCounts.get(fileName);
}
return rc;
}

void IndexFileDeleter::deleteFiles(List files) {
final int32_t32_t size = files.size();
for(int32_t32_t i=0;i<size;i++)
  deleteFile((String) files.get(i));
}

/** Delets the specified files, but only if they are new
*  (have not yet been incref'd). */
void IndexFileDeleter::deleteNewFiles(std::vector<std::string>& files) {
	final int32_t32_t size = files.size();
	for(int32_t32_t i=0;i<size;i++)
	  if (!refCounts.containsKey(files.get(i)))
	    deleteFile((String) files.get(i));
}

void IndexFileDeleter::deleteFile(const char* fileName)
{
	try {
	  if (infoStream != NULL) {
	    message("delete \"" + fileName + "\"");
	  }
	  directory.deleteFile(fileName);
	} catch (IOException e) {       // if delete fails
	  if (directory.fileExists(fileName)) {
	
	    // Some operating systems (e.g. Windows) don't
	    // permit a file to be deleted while it is opened
	    // for read (e.g. by another process or thread). So
	    // we assume that when a delete fails it is because
	    // the file is open in another process, and queue
	    // the file for subsequent deletion.
	
	    if (infoStream != NULL) {
	      message("IndexFileDeleter: unable to remove file \"" + fileName + "\": " + e.toString() + "; Will re-try later.");
	    }
	    if (deletable == NULL) {
	      deletable = new ArrayList();
	    }
	    deletable.add(fileName);                  // add to deletable
	  }
	}
}

CL_NS_END
