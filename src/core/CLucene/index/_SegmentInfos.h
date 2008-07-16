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
	private:
		//Directory where the segment resides
		CL_NS(store)::Directory* dir;
		
	public:
		
		LUCENE_STATIC_CONSTANT(int32_t, NO = -1);
		LUCENE_STATIC_CONSTANT(int32_t, YES = 1);
		LUCENE_STATIC_CONSTANT(int32_t, CHECK_DIR = 0);
		LUCENE_STATIC_CONSTANT(int32_t, WITHOUT_GEN = 0);
		
		///Gets the Directory where the segment resides
		CL_NS(store)::Directory* getDir() const{ return dir; } 

    	//Unique name in directory dir
		char* name;	
		//Number of docs in the segment
		const int32_t docCount;						  					

		// before lock-less commits
		const bool preLockless;
		// generation of each field's norm file
		int64_t *normGen;
		// generation count of the deletes file
		const int64_t delGen;
		// single norm file flag
		const bool hasSingleNorm;
		// compound file flag
		const uint8_t isCompoundFile;
		
		SegmentInfo(const char* Name, const int32_t DocCount, CL_NS(store)::Directory* Dir);
		SegmentInfo(const char* Name, const int32_t DocCount, CL_NS(store)::Directory* Dir, const int64_t DelGen, int64_t* NormGen, const uint8_t IsCompoundFile, const bool HasSingleNorm, const bool PreLockless );

		~SegmentInfo();
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
		/** The file format version, a negative number. */
		/* Works since counter, the old 1st entry, is always >= 0 */
		LUCENE_STATIC_CONSTANT(int32_t,FORMAT=-1);
		LUCENE_STATIC_CONSTANT(int32_t,FORMAT_LOCKLESS=-2);
		LUCENE_STATIC_CONSTANT(int32_t,FORMAT_SINGLE_NORM_FILE=-3);
		
		LUCENE_STATIC_CONSTANT(int32_t,CURRENT_FORMAT=FORMAT_SINGLE_NORM_FILE);
		
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
        SegmentInfos(bool deleteMembers=true);
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
