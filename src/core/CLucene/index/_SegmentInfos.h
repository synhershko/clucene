/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_index_SegmentInfos_
#define _lucene_index_SegmentInfos_


//#include "IndexReader.h"
//#include "CLucene/util/VoidList.h"
#include "_IndexFileNames.h"
CL_CLASS_DEF(store,Directory)
CL_CLASS_DEF(store,IndexInput)
CL_CLASS_DEF(store,IndexOutput)

CL_NS_DEF(index)

	class SegmentInfo :LUCENE_BASE{
	public:

		LUCENE_STATIC_CONSTANT(int32_t, NO = -1);			// e.g. no norms; no deletes;
		LUCENE_STATIC_CONSTANT(int32_t, YES = 1);			// e.g. have norms; have deletes;
		LUCENE_STATIC_CONSTANT(int32_t, CHECK_DIR = 0);		// e.g. must check dir to see if there are norms/deletions
		LUCENE_STATIC_CONSTANT(int32_t, WITHOUT_GEN = 0);	// a file name that has no GEN in it.

		char *name;									// unique name in dir
		int32_t docCount;							// number of docs in seg
		CL_NS(store)::Directory* dir;				// where segment resides

	private:
		bool preLockless;						  // true if this is a segments file written before
                                                  // lock-less commits (2.1)

		int64_t delGen;                            // current generation of del file; NO if there
                                                  // are no deletes; CHECK_DIR if it's a pre-2.1 segment
                                                  // (and we must check filesystem); YES or higher if
                                                  // there are deletes at generation N

		int64_t *normGen;                         // current generation of each field's norm file.
                                                  // If this array is null, for lockLess this means no
                                                  // separate norms.  For preLockLess this means we must
                                                  // check filesystem. If this array is not null, its
                                                  // values mean: NO says this field has no separate
                                                  // norms; CHECK_DIR says it is a preLockLess segment and
                                                  // filesystem must be checked; >= YES says this field
                                                  // has separate norms with the specified generation

		size_t normGenLen;						  // To keep the length of array normGen

		int8_t isCompoundFile;					  // NO if it is not; YES if it is; CHECK_DIR if it's
                                                  // pre-2.1 (ie, must check file system to see
                                                  // if <name>.cfs and <name>.nrm exist)

	public: // todo: privatize later. needed for SegmentReader::initialize
		bool hasSingleNormFile;					  // true if this segment maintains norms in a single file;
                                                  // false otherwise
                                                  // this is currently false for segments populated by DocumentWriter
                                                  // and true for newly created merged segments (both
                                                  // compound and non compound).

	private:

//		List files;                               // cached list of files that this segment uses
                                                  // in the Directory

		int64_t sizeInBytes;					  // total byte size of all of our files (computed on demand)

		int32_t docStoreOffset;					  // if this segment shares stored fields & vectors, this
                                                  // offset is where in that file this segment's docs begin
		char* docStoreSegment;					  // name used to derive fields/vectors file we share with
                                                  // other segments
												  // This string is being interned. There might be a way around this,
												  // and if found, this would greatly improve perfomance.

		bool docStoreIsCompoundFile;			  // whether doc store files are stored in compound file (*.cfx)

		/* Called whenever any change is made that affects which
		* files this segment has. */
		void clearFiles();

	public:
		SegmentInfo(const char* _name, const int32_t _docCount, CL_NS(store)::Directory* _dir);

		SegmentInfo(const char* _name, const int32_t _docCount, CL_NS(store)::Directory* _dir,
			bool _isCompoundFile, bool _hasSingleNormFile,
			int32_t _docStoreOffset = -1, const char* _docStoreSegment = NULL, bool _docStoreIsCompoundFile = false);

		/**
		* Construct a new SegmentInfo instance by reading a
		* previously saved SegmentInfo from input.
		*
		* @param dir directory to load from
		* @param format format of the segments info file
		* @param input input handle to read segment info from
		*/
		SegmentInfo(CL_NS(store)::Directory* dir, int32_t format, CL_NS(store)::IndexInput* input);

		~SegmentInfo();

		void setNumFields(const int32_t numFields);
		bool hasDeletions() const;

		void advanceDelGen();
		void clearDelGen();

		SegmentInfo* clone ();

		char* getDelFileName() const;

		/**
		* Returns true if this field for this segment has saved a separate norms file (_<segment>_N.sX).
		*
		* @param fieldNumber the field index to check
		*/
		bool hasSeparateNorms(const int32_t fieldNumber) const;

		/**
		* Returns true if any fields in this segment have separate norms.
		*/
		bool hasSeparateNorms() const;

		/**
		* Get the file name for the norms file for this field.
		*
		* @param number field index
		*/
		char* getNormFileName(const int32_t number) const;

		/**
		* Increment the generation count for the norms file for
		* this field.
		*
		* @param fieldIndex field whose norm file will be rewritten
		*/
		void advanceNormGen(const int32_t fieldIndex);

		/**
		* Mark whether this segment is stored as a compound file.
		*
		* @param isCompoundFile true if this is a compound file;
		* else, false
		*/
		void setUseCompoundFile(const bool isCompoundFile);

		/**
		* Returns true if this segment is stored as a compound
		* file; else, false.
		*/
		bool getUseCompoundFile() const;

		/**
		* Copy everything from src SegmentInfo into our instance.
		*/
		void reset(const SegmentInfo* src);

		/**
		* Save this segment's info.
		*/
		void write(CL_NS(store)::IndexOutput* output);

		int32_t getDocStoreOffset() const;

		bool getDocStoreIsCompoundFile() const;

		void setDocStoreIsCompoundFile(const bool v);

		/**
		* Returns a reference to docStoreSegment
		*/
		char* getDocStoreSegment() const;

		void setDocStoreOffset(const int32_t offset);

		/** We consider another SegmentInfo instance equal if it
		*  has the same dir and same name. */
		bool equals(SegmentInfo* obj);

		///Gets the Directory where the segment resides
		CL_NS(store)::Directory* getDir() const{ return dir; } //todo: since dir is public, consider removing this function
	};

	typedef CL_NS(util)::CLVector<SegmentInfo*,CL_NS(util)::Deletor::Object<SegmentInfo> > segmentInfosType;
  //SegmentInfos manages a list of SegmentInfo instances
  //Each SegmentInfo contains information about a segment in a directory.
  //
  //The active segments in the index are stored in the segment info file.
  //An index only has a single file in this format, and it is named "segments".
  //This lists each segment by name, and also contains the size of each segment.
  //The format of the file segments is defined as follows:
  //
  //                                        SegCount
  //Segments --> SegCount, <SegName, SegSize>
  //
  //SegCount, SegSize --> UInt32
  //
  //SegName --> String
  //
  //SegName is the name of the segment, and is used as the file name prefix
  //for all of the files that compose the segment's index.
  //
  //SegSize is the number of documents contained in the segment index.
  //
  //Note:
  //At http://jakarta.apache.org/lucene/docs/fileformats.html the definition
  //of all file formats can be found. Note that java lucene currently
  //defines Segments as follows:
  //
  //Segments --> Format, Version, SegCount, <SegName, SegSize>SegCount
  //
  //Format, SegCount, SegSize --> UInt32
  //
  //Format and Version have not been implemented yet

	class IndexReader;

	class SegmentInfos: LUCENE_BASE {
	public:
		/** The file format version, a negative number. */
		/* Works since counter, the old 1st entry, is always >= 0 */
		LUCENE_STATIC_CONSTANT(int32_t,FORMAT=-1);

		/** This format adds details used for lockless commits.  It differs
		* slightly from the previous format in that file names
		* are never re-used (write once).  Instead, each file is
		* written to the next generation.  For example,
		* segments_1, segments_2, etc.  This allows us to not use
		* a commit lock.  See <a
		* href="http://lucene.apache.org/java/docs/fileformats.html">file
		* formats</a> for details.
		*/
		LUCENE_STATIC_CONSTANT(int32_t,FORMAT_LOCKLESS=-2);

		/** This format adds a "hasSingleNormFile" flag into each segment info.
		* See <a href="http://issues.apache.org/jira/browse/LUCENE-756">LUCENE-756</a>
		* for details.
		*/
		LUCENE_STATIC_CONSTANT(int32_t,FORMAT_SINGLE_NORM_FILE=-3);

		/** This format allows multiple segments to share a single
		* vectors and stored fields file. */
		LUCENE_STATIC_CONSTANT(int32_t,FORMAT_SHARED_DOC_STORE=-4);

	private:
		/* This must always point to the most recent file format. */
		LUCENE_STATIC_CONSTANT(int32_t,CURRENT_FORMAT=FORMAT_SHARED_DOC_STORE);

	public:
		int32_t counter;  // used to name new segments

		/**
		* counts how often the index has been changed by adding or deleting docs.
		* starting with the current time in milliseconds forces to create unique version numbers.
		*/
		int64_t version;

	private:
		int64_t generation;					// generation of the "segments_N" for the next commit
		int64_t lastGeneration;				// generation of the "segments_N" file we last successfully read
											// or wrote; this is normally the same as generation except if
											// there was an IOException that had interrupted a commit

		/**
		* If non-null, information about loading segments_N files
		* will be printed here.  @see #setInfoStream.
		*/
		static std::ostream* infoStream;

		LUCENE_STATIC_CONSTANT(int32_t,defaultGenFileRetryCount=10);
		LUCENE_STATIC_CONSTANT(int32_t,defaultGenFileRetryPauseMsec=50);
		LUCENE_STATIC_CONSTANT(int32_t,defaultGenLookaheadCount=10);

		segmentInfosType infos;

		friend class IndexWriter; //allow IndexWriter to use counter

    static void message(const char* _message, ...);

    public:
        SegmentInfos(bool deleteMembers=true, int32_t reserveCount=0);
        ~SegmentInfos();

		//Returns a reference to the i-th SegmentInfo in the list.
		SegmentInfo* info(int32_t i);

		/**
		* Get the generation (N) of the current segments_N file
		* from a list of files.
		*
		* @param files -- array of file names to check
		*/
    static int64_t getCurrentSegmentGeneration( std::vector<std::string>& files );

		/**
		* Get the generation (N) of the current segments_N file
		* in the directory.
		*
		* @param directory -- directory to search for the latest segments_N file
		*/
		static int64_t getCurrentSegmentGeneration( const CL_NS(store)::Directory* directory );

		/**
		* Get the filename of the current segments_N file
		* from a list of files.
		*
		* @param files -- array of file names to check
		*/
    static const char* getCurrentSegmentFileName( std::vector<std::string>& files );

		/**
		* Get the filename of the current segments_N file
		* in the directory.
		*
		* @param directory -- directory to search for the latest segments_N file
		*/
		static const char* getCurrentSegmentFileName( CL_NS(store)::Directory* directory );

		/**
		* Get the segments_N filename in use by this segment infos.
		*/
		const char* getCurrentSegmentFileName();

		/**
		* Parse the generation off the segments file name and
		* return it.
		*/
		static int64_t generationFromSegmentsFileName( const char* fileName );

		/**
		* Get the next segments_N filename that will be written.
		*/
		const char* getNextSegmentFileName();

		/* public vector-like operations */
		//delete and clears objects 'from' from to 'to'
		void clearto(size_t to, size_t end);
		//count of segment infos
		int32_t size() const;
		//add a segment info
		void add(SegmentInfo* info);
		SegmentInfo* elementAt(int32_t pos);
		void setElementAt(SegmentInfo* si, int32_t pos);
		inline void clear();

		/**
		* Read a particular segmentFileName.  Note that this may
		* throw an IOException if a commit is in process.
		*
		* @param directory -- directory containing the segments file
		* @param segmentFileName -- segment file to load
		* @throws CorruptIndexException if the index is corrupt
		* @throws IOException if there is a low-level IO error
		*/
		void read(CL_NS(store)::Directory* directory, const char* segmentFileName);

		/**
		* This version of read uses the retry logic (for lock-less
		* commits) to find the right segments file to load.
		* @throws CorruptIndexException if the index is corrupt
		* @throws IOException if there is a low-level IO error
		*/
		void read(CL_NS(store)::Directory* directory);

		//Writes a new segments file based upon the SegmentInfo instances it manages
		//note: still does not support lock-less writes (still pre-2.1 format)
        void write(CL_NS(store)::Directory* directory);

		/**
		* Returns a copy of this instance, also copying each
		* SegmentInfo.
		*/
		SegmentInfos* clone();

		/**
		* version number when this SegmentInfos was generated.
		*/
		int64_t getVersion() const;
		int64_t getGeneration() const;
		int64_t getLastGeneration() const;

		/**
		* Current version number from segments file.
		* @throws CorruptIndexException if the index is corrupt
		* @throws IOException if there is a low-level IO error
		*/
		static int64_t readCurrentVersion(CL_NS(store)::Directory* directory);


    /** If non-null, information about retries when loading
    * the segments file will be printed to this.
    */
    static void setInfoStream(std::ostream* infoStream);

    /**
    * @see #setInfoStream
    */
    static std::ostream* getInfoStream();

		/**
		* Advanced: set how many times to try loading the
		* segments.gen file contents to determine current segment
		* generation.  This file is only referenced when the
		* primary method (listing the directory) fails.
		*/
		//static void setDefaultGenFileRetryCount(const int32_t count);
		/**
		* @see #setDefaultGenFileRetryCount
		*/
		static int32_t getDefaultGenFileRetryCount();

		/**
		* Advanced: set how many milliseconds to pause in between
		* attempts to load the segments.gen file.
		*/
		//static void setDefaultGenFileRetryPauseMsec(const int32_t msec);
		/**
		* @see #setDefaultGenFileRetryPauseMsec
		*/
		static int32_t getDefaultGenFileRetryPauseMsec();

		/**
		* Advanced: set how many times to try incrementing the
		* gen when loading the segments file.  This only runs if
		* the primary (listing directory) and secondary (opening
		* segments.gen file) methods fail to find the segments
		* file.
		*/
		//static void setDefaultGenLookaheadCount(const int32_t count);
		/**
		* @see #setDefaultGenLookaheadCount
		*/
		static int32_t getDefaultGenLookahedCount();

		/**
		* Utility class for executing code that needs to do
		* something with the current segments file.  This is
		* necessary with lock-less commits because from the time
		* you locate the current segments file name, until you
		* actually open it, read its contents, or check modified
		* time, etc., it could have been deleted due to a writer
		* commit finishing.
		*/
    template<typename RET>
		class FindSegmentsFile: LUCENE_BASE {
    	protected:
    		CL_NS(store)::Directory* directory;
        const char* fileDirectory;
    	public:
    		FindSegmentsFile( CL_NS(store)::Directory* dir ){
	        this->directory = dir;
          this->fileDirectory = NULL;
        }
    		FindSegmentsFile( const char* dir ){
	        this->directory = NULL;
          this->fileDirectory = dir;
        }
        ~FindSegmentsFile(){
        }

    		RET run(){
	        char* segmentFileName = NULL;
	        int64_t lastGen = -1;
	        int64_t gen = 0;
	        int32_t genLookaheadCount = 0;
	        bool retry = false;

	        int32_t exc_num = 0;
	        TCHAR* exc_txt = NULL;

	        int32_t method = 0;

	        // Loop until we succeed in calling doBody() without
	        // hitting an IOException.  An IOException most likely
	        // means a commit was in process and has finished, in
	        // the time it took us to load the now-old infos files
	        // (and segments files).  It's also possible it's a
	        // true error (corrupt index).  To distinguish these,
	        // on each retry we must see "forward progress" on
	        // which generation we are trying to load.  If we
	        // don't, then the original error is real and we throw
	        // it.

	        // We have three methods for determining the current
	        // generation.  We try the first two in parallel, and
	        // fall back to the third when necessary. 

	        while( true ) {

		        if ( 0 == method ) {
			        // Method 1: list the directory and use the highest
			        // segments_N file.  This method works well as long
			        // as there is no stale caching on the directory
			        // contents (NOTE: NFS clients often have such stale
			        // caching):
			        vector<string> files;

			        int64_t genA = -1;

			        if (directory != NULL)
				        directory->list(&files);
			        //else
			        //todo: files = fileDirectory.list();

			        if (!files.empty()) {
				        genA = getCurrentSegmentGeneration( files );
                files.clear();
			        }

              if ( infoStream ){
                (*infoStream) << "[SIS]: directory listing genA=" << genA << "\n";
              }

			        // Method 2: open segments.gen and read its
			        // contents.  Then we take the larger of the two
			        // gen's.  This way, if either approach is hitting
			        // a stale cache (NFS) we have a better chance of
			        // getting the right generation.
			        int64_t genB = -1;
			        if (directory != NULL) {
				        CLuceneError e;
				        for(int32_t i=0;i<defaultGenFileRetryCount;i++) {
					        IndexInput* genInput = NULL;
					        if ( ! directory->openInput(IndexFileNames::SEGMENTS_GEN, genInput, e) ){
                    if (e.number() == CL_ERR_IO ) {
							        if ( infoStream ){
                        (*infoStream) << "[SIS]: segments.gen open: IOException " << e.what() << "\n";
                      }
                      break;
						        } else {
							        _CLLDELETE(genInput);
							        _CLDELETE_LARRAY(exc_txt);
							        throw e;
						        }
					        }

					        if (genInput != NULL) {
						        try {
							        int32_t version = genInput->readInt();
							        if (version == FORMAT_LOCKLESS) {
								        int64_t gen0 = genInput->readLong();
								        int64_t gen1 = genInput->readLong();
								        //CL_TRACE("fallback check: %d; %d", gen0, gen1);
								        if (gen0 == gen1) {
									        // The file is consistent.
									        genB = gen0;
									        _CLDELETE(genInput);
									        break;
								        }
							        }
						        } catch (CLuceneError &err2) {
							        if (err2.number() != CL_ERR_IO) {
								        _CLLDELETE(genInput);
								        _CLDELETE_LARRAY(exc_txt);
								        throw err2; // retry only for IOException
							        }
						        } _CLFINALLY({
							        genInput->close();
							        _CLDELETE(genInput);
						        });
					        }

					        _LUCENE_SLEEP(defaultGenFileRetryPauseMsec);
					        /*
					        //todo: Wrap the LUCENE_SLEEP call above with the following try/catch block if
					        //	  InterruptedException is implemented
					        try {
					        } catch (CLuceneError &e) {
					        //if (err2.number != CL_ERR_Interrupted) // retry only for InterruptedException
					        // todo: see if CL_ERR_Interrupted needs to be added...
					        _CLDELETE_LARRAY(exc_txt);
					        throw e;
					        }*/

				        }
			        }

			        //CL_TRACE("%s check: genB=%d", IndexFileNames::SEGMENTS_GEN, genB);

			        // Pick the larger of the two gen's:
			        if (genA > genB)
				        gen = genA;
			        else
				        gen = genB;

			        if (gen == -1) {
				        // Neither approach found a generation
				        _CLDELETE_LARRAY(exc_txt);
				        _CLTHROWA(CL_ERR_IO, "No segments* file found"); //todo: add folder name (directory->toString())
			        } 
		        }

		        // Third method (fallback if first & second methods
		        // are not reliable): since both directory cache and
		        // file contents cache seem to be stale, just
		        // advance the generation.
		        if ( 1 == method || ( 0 == method && lastGen == gen && retry )) {

			        method = 1;

			        if (genLookaheadCount < defaultGenLookaheadCount) {
				        gen++;
				        genLookaheadCount++;
				        //CL_TRACE("look ahead increment gen to %d", gen);
			        }
		        }

		        if (lastGen == gen) {

			        // This means we're about to try the same
			        // segments_N last tried.  This is allowed,
			        // exactly once, because writer could have been in
			        // the process of writing segments_N last time.

			        if (retry) {
				        // OK, we've tried the same segments_N file
				        // twice in a row, so this must be a real
				        // error.  We throw the original exception we
				        // got.
				        _CLTHROWT_DEL(exc_num, exc_txt);
			        } else {
				        retry = true;
			        }

		        } else {
			        // Segment file has advanced since our last loop, so
			        // reset retry:
			        retry = false;
		        }

		        lastGen = gen;

		        segmentFileName = IndexFileNames::fileNameFromGeneration(IndexFileNames::SEGMENTS, "", gen);

		        try {
			        RET v = doBody(segmentFileName);
			        _CLDELETE_LARRAY( segmentFileName );
			        _CLDELETE_LARRAY(exc_txt);
			        //if (exc != NULL) {
			        //CL_TRACE("success on %s", segmentFileName);
			        //}
			        return v;
		        } catch (CLuceneError& err) {

			        _CLDELETE_LARRAY( segmentFileName );

			        if (err.number() != CL_ERR_IO) {
				        _CLDELETE_LARRAY(exc_txt);
				        throw err;
			        }

			        // Save the original root cause:
			        if (exc_num == 0) {
				        exc_num = err.number();
				        CND_CONDITION( exc_num > 0, _T("Unsupported error code"));
				        exc_txt = STRDUP_TtoT(err.twhat());
			        }

			        //CL_TRACE("primary Exception on '" + segmentFileName + "': " + err + "'; will retry: retry=" + retry + "; gen = " + gen);

			        if (!retry && gen > 1) {

				        // This is our first time trying this segments
				        // file (because retry is false), and, there is
				        // possibly a segments_(N-1) (because gen > 1).
				        // So, check if the segments_(N-1) exists and
				        // try it if so:
				        const char* prevSegmentFileName = IndexFileNames::fileNameFromGeneration( IndexFileNames::SEGMENTS, "", gen-1 );

				        bool prevExists=false;
				        if (directory != NULL)
					        prevExists = directory->fileExists(prevSegmentFileName);
				        //todo: File implementation below
				        //else
				        //  prevExists = new File(fileDirectory, prevSegmentFileName).exists();

				        if (prevExists) {
					        //CL_TRACE("fallback to prior segment file '%s'", prevSegmentFileName);
					        try {
						        RET v = doBody(prevSegmentFileName);
						        _CLDELETE_CaARRAY( prevSegmentFileName );
						        _CLDELETE_CARRAY(exc_txt);
						        //if (exc != NULL) {
						        //CL_TRACE("success on fallback %s", prevSegmentFileName);
						        //}
						        return v;
					        } catch (CLuceneError& err2) {
						        _CLDELETE_CaARRAY( prevSegmentFileName );
						        if (err2.number()!=CL_ERR_IO) {
							        _CLDELETE_LARRAY(exc_txt);
							        throw err2;
						        }
						        //CL_TRACE("secondary Exception on '" + prevSegmentFileName + "': " + err2 + "'; will retry");
					        }
				        }
			        }
		        }
	        }
        }
    		virtual RET doBody( const char* segmentFileName ) = 0;
    	};
    	//friend class SegmentInfos::FindSegmentsFile;

    	class FindSegmentsReader: public FindSegmentsFile<IndexReader*> {
    	public:
    		FindSegmentsReader( CL_NS(store)::Directory* dir );
    		FindSegmentsReader( const char* dir );
    		IndexReader* doBody( const char* segmentFileName );
    	};
    	friend class SegmentInfos::FindSegmentsReader;

    	class FindSegmentsVersion: public FindSegmentsFile<int64_t> {
    	public:
    		FindSegmentsVersion( CL_NS(store)::Directory* dir );
    		FindSegmentsVersion( const char* dir );
    		int64_t doBody( const char* segmentFileName );
    	};
    	friend class SegmentInfos::FindSegmentsVersion;

		class FindSegmentsRead: public FindSegmentsFile<bool> {
    	public:
			  FindSegmentsRead( CL_NS(store)::Directory* dir );
    	  FindSegmentsRead( const char* dir );
    		bool doBody( const char* segmentFileName );
    	};
    	friend class SegmentInfos::FindSegmentsRead;
  };
CL_NS_END
#endif
