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
CL_CLASS_DEF(store,Directory)
CL_CLASS_DEF(store,IndexInput)

CL_NS_DEF(index)

	class SegmentInfo :LUCENE_BASE{
	public:
		
		LUCENE_STATIC_CONSTANT(int32_t, NO = -1);
		LUCENE_STATIC_CONSTANT(int32_t, YES = 1);
		LUCENE_STATIC_CONSTANT(int32_t, CHECK_DIR = 0);
		LUCENE_STATIC_CONSTANT(int32_t, WITHOUT_GEN = 0);

		char *name;									// unique name in dir
		const int32_t docCount;						// number of docs in seg
		CL_NS(store)::Directory* dir;				// where segment resides

	private:
		const bool preLockless;                    // true if this is a segments file written before
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

		const uint8_t isCompoundFile;             // NO if it is not; YES if it is; CHECK_DIR if it's
                                                  // pre-2.1 (ie, must check file system to see
                                                  // if <name>.cfs and <name>.nrm exist)         
	
	public: // todo: privatize later. needed for SegmentReader::initialize
		const bool hasSingleNormFile;			  // true if this segment maintains norms in a single file; 
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
		bool docStoreIsCompoundFile;			  // whether doc store files are stored in compound file (*.cfx)

	public:
		//SegmentInfo(const char* Name, const int32_t DocCount, CL_NS(store)::Directory* Dir);
		SegmentInfo(const char* Name, const int32_t DocCount, CL_NS(store)::Directory* Dir, const int64_t DelGen = 0, int64_t* NormGen = NULL, const size_t _normGenLen = 0, const uint8_t IsCompoundFile = 0, const bool HasSingleNormFile = false, const bool PreLockless = true );

		~SegmentInfo();

		SegmentInfo* clone () {
			SegmentInfo* si = _CLNEW SegmentInfo(name, docCount, dir, delGen, NULL, 0,
													isCompoundFile, hasSingleNormFile, preLockless);
			//si->isCompoundFile = isCompoundFile;
			//si->delGen = delGen;
			//si->preLockless = preLockless;
			//si->hasSingleNormFile = hasSingleNormFile;
			if (normGen != NULL) {
				memcpy(si->normGen, this->normGen, normGenLen);
			}
			si->docStoreOffset = docStoreOffset;
			si->docStoreSegment = STRDUP_AtoA(docStoreSegment);
			si->docStoreIsCompoundFile = docStoreIsCompoundFile;
			return si;
		}

		///Gets the Directory where the segment resides
		CL_NS(store)::Directory* getDir() const{ return dir; } 
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
		
		LUCENE_STATIC_CONSTANT(int32_t,defaultGenFileRetryCount=10);
		LUCENE_STATIC_CONSTANT(int32_t,defaultGenFileRetryPauseMsec=50);
		LUCENE_STATIC_CONSTANT(int32_t,defaultGenLookaheadCount=10);
		
		/**
		* counts how often the index has been changed by adding or deleting docs.
		* starting with the current time in milliseconds forces to create unique version numbers.
		*/
		int64_t version;

		segmentInfosType infos;
		
        int32_t counter;  // used to name new segments
		friend class IndexWriter; //allow IndexWriter to use counter
		
		int64_t generation;
		int64_t lastGeneration;
		
    public:
        SegmentInfos(bool deleteMembers=true, int32_t reserveCount=0);
        ~SegmentInfos();

		
		//delete and clears objects 'from' from to 'to'
		void clearto(size_t to, size_t end);
		
		//count of segment infos
		int32_t size() const;
		//add a segment info
		void add(SegmentInfo* info);
		//Returns a reference to the i-th SegmentInfo in the list.
		SegmentInfo* info(int32_t i);
		
		/**
		* version number when this SegmentInfos was generated.
		*/
		int64_t getVersion() { return version; }
		
		static int64_t readCurrentVersion(CL_NS(store)::Directory* directory);

		//Reads segments file that resides in directory
		void read(CL_NS(store)::Directory* directory);
		void read(CL_NS(store)::Directory* directory, const char* segmentFileName);

	    //Writes a new segments file based upon the SegmentInfo instances it manages
        void write(CL_NS(store)::Directory* directory);
        
        static int64_t getCurrentSegmentGeneration( char** files );        
        static int64_t getCurrentSegmentGeneration( CL_NS(store)::Directory* directory );
        
        static const char* getCurrentSegmentFileName( char** files );        
        static const char* getCurrentSegmentFileName( CL_NS(store)::Directory* directory );
        
        const char* getCurrentSegmentFileName();
        
        static int64_t generationFromSegmentsFileName( const char* fileName );
        
        const char* getNextSegmentFileName();

		/* public vector operations */
		SegmentInfo* elementAt(int32_t pos);
		void setElementAt(SegmentInfo* si, int32_t pos);

		/**
		* Returns a copy of this instance, also copying each
		* SegmentInfo.
		*/
		SegmentInfos* clone();
        
    	class FindSegmentsFile: LUCENE_BASE {
    	protected:
    		CL_NS(store)::Directory* directory;    		    	
    		
    	public:
    		FindSegmentsFile( CL_NS(store)::Directory* dir );    		
    		~FindSegmentsFile();    			
    		
    		void* run();    		
    		virtual void* doBody( const char* segmentFileName ) = 0;
    		
    	};
    	friend class SegmentInfos::FindSegmentsFile;
    	
    	class FindSegmentsReader: public FindSegmentsFile {
    	public:
    		FindSegmentsReader( CL_NS(store)::Directory* dir );
    		void* doBody( const char* segmentFileName );
    	};
    	friend class SegmentInfos::FindSegmentsReader;
    	
    	class FindSegmentsVersion: public FindSegmentsFile {
    	public:
    		FindSegmentsVersion( CL_NS(store)::Directory* dir );
    		void* doBody( const char* segmentFileName );
    	};
    	friend class SegmentInfos::FindSegmentsVersion;
    	
  };
CL_NS_END
#endif
