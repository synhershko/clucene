/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#include "_SegmentInfos.h"
#include "_IndexFileNames.h"
#include "_SegmentHeader.h"
#include "MultiReader.h"

#include "CLucene/store/Directory.h"
//#include "CLucene/util/VoidMap.h"
#include "CLucene/util/_Misc.h"

#ifdef _CL_HAVE_WINDEF_H
	#include <windef.h>
#endif


CL_NS_USE(store)
CL_NS_USE(util)
CL_NS_DEF(index)


   SegmentInfo::SegmentInfo(const char* Name, const int32_t DocCount, CL_NS(store)::Directory* Dir):
   docCount(DocCount),dir(Dir),delGen(0),normGen(NULL),isCompoundFile(0),hasSingleNorm(false),preLockless(true){
		name = STRDUP_AtoA(Name);
	   
   }

   SegmentInfo::SegmentInfo(const char* Name, const int32_t DocCount, CL_NS(store)::Directory* Dir, const int64_t DelGen, int64_t* NormGen, const uint8_t IsCompoundFile, const bool HasSingleNorm, const bool PreLockless ):
	docCount(DocCount),dir(Dir),delGen(DelGen),normGen(NormGen),isCompoundFile(IsCompoundFile),hasSingleNorm(HasSingleNorm),preLockless(PreLockless){
	//Func - Constructor. Initialises SegmentInfo.
	//Pre  - Name holds the unique name in the directory Dir
	//       DocCount holds the number of documents in the segment
	//       Dir holds the Directory where the segment resides
	//Post - The instance has been created. name contains the duplicated string Name.
	//       docCount = DocCount and dir references Dir
		name = STRDUP_AtoA(Name);
   }
     
   SegmentInfo::~SegmentInfo(){
	   	_CLDELETE_ARRAY(normGen);
		_CLDELETE_CaARRAY(name);
   }


  SegmentInfos::SegmentInfos(bool deleteMembers) :
      infos(deleteMembers),generation(0),lastGeneration(0) {
  //Func - Constructor
  //Pre  - deleteMembers indicates if the instance to be created must delete
  //       all SegmentInfo instances it manages when the instance is destroyed or not
  //       true -> must delete, false may not delete
  //Post - An instance of SegmentInfos has been created.
  
      //initialize counter to 0
      counter = 0;
      version = Misc::currentTimeMillis();
  }

  SegmentInfos::~SegmentInfos(){
  //Func - Destructor
  //Pre  - true
  //Post - The instance has been destroyed. Depending on the constructor used
  //       the SegmentInfo instances that this instance managed have been deleted or not.

	  //Clear the list of SegmentInfo instances - make sure everything is deleted
      infos.clear();
  }
  
  SegmentInfo* SegmentInfos::info(int32_t i) {
  //Func - Returns a reference to the i-th SegmentInfo in the list.
  //Pre  - i >= 0
  //Post - A reference to the i-th SegmentInfo instance has been returned

      CND_PRECONDITION(i >= 0, "i contains negative number");

	  //Get the i-th SegmentInfo instance
      SegmentInfo *ret = infos[i];
      //Condition check to see if the i-th SegmentInfo has been retrieved
      CND_CONDITION(ret != NULL,"No SegmentInfo instance found");

      return ret;
  }

  void SegmentInfos::clearto(size_t from, size_t end){
	size_t range = end - from;
	  if ( from >= 0 && (infos.size() - from) >= range) { // Make sure we actually need to remove
		segmentInfosType::iterator itr,bitr=infos.begin()+from,eitr=infos.end();
		size_t count = 0;	
		for(itr=bitr;itr!=eitr && count < range;++itr, count++) {
				_CLLDELETE((*itr));
			}
			infos.erase(bitr,bitr + count);
		}
  }
  void SegmentInfos::add(SegmentInfo* info){
	infos.push_back(info);
  }
  int32_t SegmentInfos::size() const{
	  return infos.size();
  }

  void SegmentInfos::read(Directory* directory) {
	  generation = lastGeneration = -1;
  }
  
  void SegmentInfos::read(Directory* directory, const char* segmentFileName){
  //Func - Reads segments file that resides in directory. 
  //Pre  - directory contains a valid reference
  //Post - The segments file has been read and for each segment found
  //       a SegmentsInfo intance has been created and stored.

	  bool success = false;
	  
	  IndexInput* input = directory->openInput( segmentFileName );
	  generation = generationFromSegmentsFileName( segmentFileName );
	  lastGeneration = generation;
	  
	  //Check if input is valid
	  if (input){
        try {
            int32_t format = input->readInt();

            if(format < 0){     // file contains explicit format info
               // check that it is a format we can understand
               if (format < CURRENT_FORMAT){
                  TCHAR err[30];
                  _sntprintf(err,30,_T("Unknown format version: %d"),format);
                  _CLTHROWT(CL_ERR_Runtime,err);
               }
               version = input->readLong(); // read version
               counter = input->readInt(); // read counter
            }
            else{     // file is in old format without explicit format info
               counter = format;
            }

            //Temporary variable for storing the name of the segment
            TCHAR tname[CL_MAX_PATH];
            char aname[CL_MAX_PATH];
            SegmentInfo* si  = NULL;

            int32_t size = 0;
            int64_t delGen = 0;
            int64_t *normGen = NULL;
            bool hasSingleNorm = false;
            uint8_t isCompoundFile = 0;
            bool preLockless = true;
            
            //read segmentInfos
            for (int32_t i = input->readInt(); i > 0; --i){
            	
                // read the name of the segment
                input->readString(tname, CL_MAX_PATH); 
				STRCPY_TtoA(aname,tname,CL_MAX_PATH);

				size = input->readInt();
				
				if ( format <= FORMAT_LOCKLESS ) {						
					delGen = input->readLong();
					if ( format <= FORMAT_SINGLE_NORM_FILE ) {
						hasSingleNorm = (1 == input->readByte());
					} else {
						hasSingleNorm = false;
					}
					int32_t numNormGen = input->readInt();
					if ( numNormGen == -1 ) {
						normGen = NULL;
					} else {
						normGen = _CL_NEWARRAY(int64_t, numNormGen);
						for ( int32_t j = 0; j < numNormGen; j++ ) {
							normGen[j] = input->readLong();
						}
					}
					isCompoundFile = input->readByte();
					preLockless = ( isCompoundFile == 0 );
					
					si = _CLNEW SegmentInfo(aname, size, directory, delGen, normGen, isCompoundFile, hasSingleNorm, preLockless );	
					
				} else {	
					
					si = _CLNEW SegmentInfo(aname, size, directory); 
					
				}

                //Condition check to see if si points to an instance
                CND_CONDITION(si != NULL, "Memory allocation for si failed")	;
                
                //store SegmentInfo si
                infos.push_back(si);
                
             } 

            if(format >= 0){ // in old format the version number may be at the end of the file
               if (input->getFilePointer() >= input->length())
                  version = Misc::currentTimeMillis(); // old file format without version number
               else
                  version = input->readLong(); // read version
            }
            
            success = true;
        } _CLFINALLY(
            //destroy the inputStream input. The destructor of IndexInput will 
		    //also close the Inputstream input
            _CLDELETE( input );
            if ( !success ) {
            	infos.clear();
            }
            );
	  }
  }

  void SegmentInfos::write(Directory* directory){
  //Func - Writes a new segments file based upon the SegmentInfo instances it manages
  //Pre  - directory is a valid reference to a Directory
  //Post - The new segment has been written to disk
    
	  //Open an IndexOutput to the segments file
	  IndexOutput* output = directory->createOutput("segments.new");
	   //Check if output is valid
	  if (output){
          try {
		   output->writeInt(FORMAT); // write FORMAT
           output->writeLong(++version); // every write changes the index
           output->writeInt(counter); //Write the counter

			  //Write the number of SegmentInfo Instances
			  //which is equal to the number of segments in directory as
			  //each SegmentInfo manages a single segment
			  output->writeInt(infos.size());			  

			  SegmentInfo *si = NULL;

			  //temporary value for wide segment name
			  TCHAR tname[CL_MAX_PATH];

			  //Iterate through all the SegmentInfo instances
           for (uint32_t i = 0; i < infos.size(); ++i) {
				  //Retrieve the SegmentInfo
               si = info(i);
               //Condition check to see if si has been retrieved
               CND_CONDITION(si != NULL,"No SegmentInfo instance found");

				  //Write the name of the current segment
              STRCPY_AtoT(tname,si->name,CL_MAX_PATH);
				  output->writeString(tname,_tcslen(tname));

				  //Write the number of documents in the segment 
              output->writeInt(si->docCount);
           }
         } _CLFINALLY(
              output->close();
              _CLDELETE( output );
         );

          // install new segment info
          directory->renameFile("segments.new","segments");
	  }
  }

  
  int64_t SegmentInfos::readCurrentVersion(Directory* directory){

	  FindSegmentsVersion find(directory);
	  
	  int64_t* tmp = (int64_t*)find.run();
	  int64_t version = *tmp;
	  delete tmp;
	  
	  return version;
  }

  int64_t SegmentInfos::getCurrentSegmentGeneration( char** files ) {
  	if ( files == NULL ) {
  		return -1;
  	}
  	
  	int64_t max = -1;
  	int32_t i = 0;
  	
  	while ( files[i] != NULL ) {
  		char* file = files[i++];
  		if ( strncmp( file, IndexFileNames::SEGMENTS, strlen(IndexFileNames::SEGMENTS) ) == 0 && strcmp( file, IndexFileNames::SEGMENTS_GEN ) != 0 ) {
  			int64_t gen = generationFromSegmentsFileName( file );
  			if ( gen > max ) {
  				max = gen;
  			}
  		}
  	}
  	
  	return max;
  }
  
  int64_t SegmentInfos::getCurrentSegmentGeneration( CL_NS(store)::Directory* directory ) {
  	char** files = directory->list();
	if ( files == NULL ){
  		;//throw new exception
	}
  	int64_t gen = getCurrentSegmentGeneration( files );
  	_CLDELETE_ARRAY( files );
  	return gen;
  }
  
  const char* SegmentInfos::getCurrentSegmentFileName( char** files ) {
	return IndexFileNames::fileNameFromGeneration( IndexFileNames::SEGMENTS, "", getCurrentSegmentGeneration( files ));
  }
  
  const char* SegmentInfos::getCurrentSegmentFileName( CL_NS(store)::Directory* directory ) {
  	return IndexFileNames::fileNameFromGeneration( IndexFileNames::SEGMENTS, "", getCurrentSegmentGeneration( directory ));
  }
  
  const char* SegmentInfos::getCurrentSegmentFileName() {
  	return IndexFileNames::fileNameFromGeneration( IndexFileNames::SEGMENTS, "", lastGeneration );
  }
  
  int64_t SegmentInfos::generationFromSegmentsFileName( const char* fileName ) {
  	if ( strcmp( fileName, IndexFileNames::SEGMENTS ) == 0 ) {
  		return 0;
  	} else if ( strncmp( fileName, IndexFileNames::SEGMENTS, strlen(IndexFileNames::SEGMENTS) ) == 0 ) {
  		return CL_NS(util)::Misc::base36ToLong( fileName + strlen( IndexFileNames::SEGMENTS )+1 );
  	} else {
  		// throw exception
  		return 0;
  	}
  }
  
  const char* SegmentInfos::getNextSegmentFileName() {
  	int64_t nextGeneration;
  	
  	if ( generation == -1 ) {
  		nextGeneration = 1;
  	} else {
  		nextGeneration = generation+1;
  	}
  	
  	return  IndexFileNames::fileNameFromGeneration( IndexFileNames::SEGMENTS, "", nextGeneration );
  }
  
  SegmentInfos::FindSegmentsFile::FindSegmentsFile( CL_NS(store)::Directory* dir ) {
	  this->directory = dir;
  }
  
  SegmentInfos::FindSegmentsFile::~FindSegmentsFile() {	 
  }
  
	void* SegmentInfos::FindSegmentsFile::run() {
		
		char* segmentFileName = NULL;
		int64_t lastGen = -1;
		int64_t gen = 0;
		int32_t genLookaheadCount = 0;
		bool retry = false;
		int32_t method = 0;
		void* value = NULL;
		
		while( true ) {
			
			char** files = NULL;
			
			if ( 0 == method ) {
				
				files = directory->list();
					
				gen = getCurrentSegmentGeneration( files );

				_CLDELETE_CaARRAY_ALL( files );

				if ( gen == -1 ) {
					_CLTHROWA(CL_ERR_IO, "No segments* file found");
				}
			
			}
		
			if ( 1 == method || ( 0 == method && lastGen == gen && retry )) {
				
				method = 1;
				
				for ( int32_t i = 0; i < defaultGenFileRetryCount; i++ ) {
					CL_NS(store)::IndexInput* genInput = directory->openInput( IndexFileNames::SEGMENTS_GEN );
					if ( genInput != NULL ) {
						try {
							int32_t version = genInput->readInt();
							if ( version == FORMAT_LOCKLESS ) {
								int64_t gen0 = genInput->readLong();
								int64_t gen1 = genInput->readLong();
								if ( gen0 == gen1 ) {
									if ( gen0 > gen ) {
										gen = gen0;
									}
									break;
								}
							}
						}_CLFINALLY(
								genInput->close();
						);
					}
					_LUCENE_SLEEP(defaultGenFileRetryPauseMsec);
				}
			}
		
			if ( 2 == method || ( 1 == method && lastGen == gen && retry )) {
				
				method = 2;
				
				if ( genLookaheadCount < defaultGenLookaheadCount ) {
					gen++;
					genLookaheadCount++;    					
				}
			}
			
			if ( lastGen == gen ) {
				if ( retry ) {
					// throw exception    					
				} else {
					retry = true;
				}
			} else {
				retry = false;    				
			}
			
			lastGen = gen;
			
			segmentFileName = IndexFileNames::fileNameFromGeneration( IndexFileNames::SEGMENTS, "", gen );
			
			try {
				value = doBody( segmentFileName );
				_CLDELETE_LARRAY( segmentFileName );
				return value;    				
			} catch (...) {
				
				_CLDELETE_LARRAY( segmentFileName );
				
				if ( !retry && gen > 1 ) {
					
					const char* prevSegmentFileName = IndexFileNames::fileNameFromGeneration( IndexFileNames::SEGMENTS, "", gen-1 );
					
					if ( directory->fileExists( prevSegmentFileName )) {
						try {
							value = doBody( prevSegmentFileName );
							return value;
						} _CLFINALLY(
							_CLDELETE_CaARRAY( prevSegmentFileName );
						);
					}
				}
			}
		}
	}
	
	SegmentInfos::FindSegmentsReader::FindSegmentsReader( CL_NS(store)::Directory* dir ) : SegmentInfos::FindSegmentsFile(dir) {
		
	}
	
	void* SegmentInfos::FindSegmentsReader::doBody( const char* segmentFileName ) {
		
	   //Instantiate SegmentInfos
       SegmentInfos* infos = _CLNEW SegmentInfos;
	   try{
			//Have SegmentInfos read the segments file in directory
			infos->read(directory, segmentFileName);
	   }catch(...){
	        //make sure infos is cleaned up
			_CLDELETE(infos);
			throw;
	   }

       // If there is at least one segment (if infos.size() >= 1), the last
       // SegmentReader object will close the directory when the SegmentReader
       // object itself is closed (see SegmentReader::doClose).
       // If there are no segments, there will be no "last SegmentReader object"
       // to fulfill this responsibility, so we need to explicitly close the
       // directory in the segmentsreader.close
       
	   //Count the number segments in the directory
	   const uint32_t nSegs = infos->size();

       if (nSegs == 1 ) {
			// index is optimized 
            return _CLNEW SegmentReader(infos, infos->info(0));
	    }else{
			//Instantiate an array of pointers to SegmentReaders of size nSegs (The number of segments in the index)
			IndexReader** readers = NULL;

			if (nSegs > 0){
				//uint32_t infosize=infos->size();
				readers = _CL_NEWARRAY(IndexReader*,nSegs+1);
				//for (uint32_t i = nSegs-1; i > 0; i--) {
		        for (uint32_t i = 0; i < nSegs; i++ ) {
					//Instantiate a SegementReader responsible for reading the i-th segment and store it in
					//the readers array
					readers[i] = _CLNEW SegmentReader(infos->info(i));
				}
				readers[nSegs] = NULL;
			}

			//return an instance of SegmentsReader which is a reader that manages all Segments
			return _CLNEW MultiReader(directory, infos, readers);
        }// end if
       
	}
	
	SegmentInfos::FindSegmentsVersion::FindSegmentsVersion( CL_NS(store)::Directory* dir ) : SegmentInfos::FindSegmentsFile(dir) {
		
	}
	
	void* SegmentInfos::FindSegmentsVersion::doBody( const char* segmentFileName ) {
		
		IndexInput* input = directory->openInput( segmentFileName );
		
	    int32_t format = 0;
	    int64_t* version = new int64_t;
	    
	    try {
	      format = input->readInt();
	      if(format < 0){
	         if(format < CURRENT_FORMAT){
	            TCHAR err[30];
	            _sntprintf(err,30,_T("Unknown format version: %d"),format);
	            _CLTHROWT(CL_ERR_Runtime,err);
	         }
	         *version = input->readLong(); // read version
	       }
	     }
	     _CLFINALLY( input->close(); _CLDELETE(input); );
	     
	     if(format < 0)
	      return version;

	    // We cannot be sure about the format of the file.
	    // Therefore we have to read the whole file and cannot simply seek to the version entry.
	    SegmentInfos* sis = _CLNEW SegmentInfos();
	    sis->read(directory);
	    *version = sis->getVersion();
	    _CLDELETE(sis);
	    
	    return version;
		
	}

CL_NS_END
