/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#include "CLucene/util/_StringIntern.h"
#include "_SegmentInfos.h"
#include "_IndexFileNames.h"
#include "_SegmentHeader.h"
#include "MultiReader.h"

#include "CLucene/store/Directory.h"
//#include "CLucene/util/VoidMap.h"
#include "CLucene/util/Misc.h"

CL_NS_USE(store)
CL_NS_USE(util)

CL_NS_DEF(index)

//Func - Constructor. Initialises SegmentInfo.
//Pre  - Name holds the unique name in the directory Dir
//       DocCount holds the number of documents in the segment
//       Dir holds the Directory where the segment resides
//Post - The instance has been created. name contains the duplicated string Name.
//       docCount = DocCount and dir references Dir
SegmentInfo::SegmentInfo(const char* _name, const int32_t _docCount, CL_NS(store)::Directory* _dir) :
			docCount(_docCount),
			preLockless(true),
			delGen(SegmentInfo::NO),
			normGen(NULL),
			normGenLen(0),
			isCompoundFile(SegmentInfo::CHECK_DIR),
			hasSingleNormFile(false),
			sizeInBytes(-1),
			docStoreOffset(-1),
			docStoreIsCompoundFile(false)
{
	this->name = const_cast<char*>(CLStringIntern::internA( _name , 2));
	this->docStoreSegment = this->name; // save on intern lookup
	this->dir = _dir;
}

SegmentInfo::SegmentInfo(const char* _name, const int32_t _docCount, CL_NS(store)::Directory* _dir,
			bool _isCompoundFile, bool _hasSingleNormFile,
			int32_t _docStoreOffset, const char* _docStoreSegment, bool _docStoreIsCompoundFile)
			:
			docCount(_docCount),
			preLockless(false),
			delGen(SegmentInfo::NO),
			normGen(NULL),
			normGenLen(0),
			isCompoundFile(_isCompoundFile ? SegmentInfo::YES : SegmentInfo::NO),
			hasSingleNormFile(_hasSingleNormFile),
			sizeInBytes(-1),
			docStoreOffset(_docStoreOffset),
			docStoreSegment( const_cast<char*>(CLStringIntern::internA( _docStoreSegment )) ),
			docStoreIsCompoundFile(_docStoreIsCompoundFile)
{
	CND_PRECONDITION(docStoreOffset == -1 || docStoreSegment != NULL, "failed testing for (docStoreOffset == -1 || docStoreSegment != NULL)");

	this->name = const_cast<char*>(CLStringIntern::internA( _name ));
	this->dir = _dir;
}

   SegmentInfo::SegmentInfo(CL_NS(store)::Directory* _dir, int32_t format, CL_NS(store)::IndexInput* input)
	   : name(NULL), normGen(NULL), normGenLen(0), sizeInBytes(-1), docStoreSegment(NULL)
   {
	   this->dir = _dir;

	   TCHAR* tname = input->readString();
	   {	
		   //todo: further reduce string duplications somehow, perhaps using a new input->readStringA ?
		   char* aname = new char[CL_MAX_PATH]; // don't ref-count aname since it will be handled by CLStringIntern
		   STRCPY_TtoA(aname, tname, CL_MAX_PATH);
		   name = const_cast<char*>(CLStringIntern::internA( const_cast<const char*>(aname), 1, true)); // Intern string, optimized to reuse the same buffer
	   }
	   _CLDELETE_CaARRAY(tname);

	   docCount = input->readInt();
	   if (format <= SegmentInfos::FORMAT_LOCKLESS) {
		   delGen = input->readLong();
		   if (format <= SegmentInfos::FORMAT_SHARED_DOC_STORE) {
			   docStoreOffset = input->readInt();
			   if (docStoreOffset != -1) {
				   tname = input->readString();
				   char* aname = new char[CL_MAX_PATH]; // don't ref-count aname since it will be handled by CLStringIntern
				   STRCPY_TtoA(aname, tname, CL_MAX_PATH);
				   _CLDELETE_CaARRAY(tname);
				   
				   //todo: possible optimization would be to strcmp(aname, docStoreSegment) and avoid inernA if == 0
				   
				   docStoreSegment = const_cast<char*>(CLStringIntern::internA( const_cast<const char*>(aname), 1, true ));  // Intern string, optimized to reuse the same buffer
				   docStoreIsCompoundFile = (1 == input->readByte());
			   } else {
				   docStoreSegment = const_cast<char*>(CLStringIntern::internA( name ));
				   docStoreIsCompoundFile = false;
			   }
		   } else {
			   docStoreOffset = -1;
			   docStoreSegment = const_cast<char*>(CLStringIntern::internA( name ));
			   docStoreIsCompoundFile = false;
		   }
		   if (format <= SegmentInfos::FORMAT_SINGLE_NORM_FILE) {
			   hasSingleNormFile = (1 == input->readByte());
		   } else {
			   hasSingleNormFile = false;
		   }
		   int32_t numNormGen = input->readInt();
		   if (normGen) {
			   // free memory and set normGen = NULL
			   _CLDELETE_ARRAY(normGen);
		   }
		   if (numNormGen == NO) {
			   // normGen is already NULL, we'll just set normGenLen to 0
			   normGenLen=0;
		   } else {
			   normGen = _CL_NEWARRAY(int64_t, numNormGen);
			   normGenLen=numNormGen;
			   for(int32_t j=0;j<numNormGen;j++) {
				   normGen[j] = input->readLong();
			   }
		   }
		   isCompoundFile = input->readByte();
		   preLockless = (isCompoundFile == CHECK_DIR);
	   } else {
		   delGen = CHECK_DIR;
		   //normGen=NULL; normGenLen=0;
		   isCompoundFile = CHECK_DIR;
		   preLockless = true;
		   hasSingleNormFile = false;
		   docStoreOffset = -1;
		   docStoreIsCompoundFile = false;
		   //docStoreSegment = NULL;
	   }
   }
     
   SegmentInfo::~SegmentInfo(){
	   	_CLDELETE_ARRAY(normGen);
		if (name == docStoreSegment){
			CLStringIntern::uninternA( name, 2 );
		} else {
			CLStringIntern::uninternA( name );
			CLStringIntern::uninternA( docStoreSegment );
		}
   }

   SegmentInfo* SegmentInfo::clone () {
	   SegmentInfo* si = _CLNEW SegmentInfo(name, docCount, dir, false, hasSingleNormFile,
		   docStoreOffset, docStoreSegment, docStoreIsCompoundFile);
	   si->isCompoundFile = isCompoundFile;
	   si->delGen = delGen;
	   si->preLockless = preLockless;
	   if (normGen != NULL) {
		   memcpy(si->normGen, this->normGen, normGenLen);
	   }
	   return si;
   }

   void SegmentInfo::clearFiles() {
	   //files = null;
	   sizeInBytes = -1;
   }

   void SegmentInfo::reset(const SegmentInfo* src) {
	   clearFiles();
	   if (this->name != src->name) {
		   CLStringIntern::uninternA( this->name );
		   this->name = const_cast<char*>(CLStringIntern::internA( src->name ));
	   }
	   docCount = src->docCount;
	   dir = src->dir;
	   preLockless = src->preLockless;
	   delGen = src->delGen;
	   docStoreOffset = src->docStoreOffset;
	   docStoreIsCompoundFile = src->docStoreIsCompoundFile;
	   if (src->normGen == NULL) {
		   _CLDELETE_ARRAY(normGen);
		   normGenLen = 0;
	   } else {
		   // optimized case to allocate new array only if current memory buffer is too small
		   if (this->normGenLen < src->normGenLen) {
			   _CLDELETE_LARRAY(normGen);
			   normGen = _CL_NEWARRAY(int64_t, src->normGenLen);
			   this->normGenLen = src->normGenLen;
		   }
		   memcpy(this->normGen, src->normGen, this->normGenLen);
	   }
	   isCompoundFile = src->isCompoundFile;
	   hasSingleNormFile = src->hasSingleNormFile;
   }

   void SegmentInfo::write(CL_NS(store)::IndexOutput* output) {
	   // todo: due to the following conversions, either have a pre-allocated buffer in the class,
	   //		 use a TCHAR* for storing name and docStoreSegment, or code a new WriteStringA.
	   //		 There are a few conversion in the IndexInput constructor as well
	   TCHAR* tname = STRDUP_AtoT(name);
	   output->writeString(tname, _tcslen(tname) );
	   _CLDELETE_ARRAY(tname);
	   output->writeInt(docCount);
	   output->writeLong(delGen);
	   output->writeInt(docStoreOffset);
	   if (docStoreOffset != -1) {
		   tname = STRDUP_AtoT(docStoreSegment);
		   output->writeString(tname, _tcslen(tname));
		   _CLDELETE_ARRAY(tname);
		   output->writeByte(static_cast<uint8_t>(docStoreIsCompoundFile ? 1:0));
	   }

	   output->writeByte(static_cast<uint8_t>(hasSingleNormFile ? 1:0));
	   if (normGen == NULL) {
		   output->writeInt(NO);
	   } else {
		   output->writeInt(normGenLen);
		   for(size_t j = 0; j < normGenLen; j++) {
			   output->writeLong(normGen[j]);
		   }
	   }
	   output->writeByte(isCompoundFile);
   }

  char* SegmentInfo::getDelFileName() {
	  if (delGen == NO) {
		  // In this case we know there is no deletion filename
		  // against this segment
		  return NULL;
	  } else {
		  // If delGen is CHECK_DIR, it's the pre-lockless-commit file format
		  char fn[8];
		  _snprintf(fn, 8,".%s", IndexFileNames::DELETES_EXTENSION);
		  return IndexFileNames::fileNameFromGeneration(name, fn, delGen); 
	  }
  }

  void SegmentInfo::advanceNormGen(const int32_t fieldIndex) {
	  if (normGen[fieldIndex] == NO) {
		  normGen[fieldIndex] = YES;
	  } else {
		  normGen[fieldIndex]++;
	  }
	  clearFiles();
  }

  void SegmentInfo::setUseCompoundFile(const bool isCompoundFile) {
	  if (isCompoundFile) {
		  this->isCompoundFile = YES;
	  } else {
		  this->isCompoundFile = NO;
	  }
	  clearFiles();
  }

  int32_t SegmentInfo::getDocStoreOffset() const { return docStoreOffset; }

  bool SegmentInfo::getDocStoreIsCompoundFile() const { return docStoreIsCompoundFile; }

  void SegmentInfo::setDocStoreIsCompoundFile(const bool v) {
	  docStoreIsCompoundFile = v;
	  clearFiles();
  }

  char* SegmentInfo::getDocStoreSegment() const { return docStoreSegment; }

  void SegmentInfo::setDocStoreOffset(const int32_t offset) {
	  docStoreOffset = offset;
	  clearFiles();
  }


  SegmentInfos::SegmentInfos(bool deleteMembers, int32_t reserveCount) :
      generation(0),lastGeneration(0), infos(deleteMembers) {
  //Func - Constructor
  //Pre  - deleteMembers indicates if the instance to be created must delete
  //       all SegmentInfo instances it manages when the instance is destroyed or not
  //       true -> must delete, false may not delete
  //Post - An instance of SegmentInfos has been created.
  
      //initialize counter to 0
      counter = 0;
      version = Misc::currentTimeMillis();
	  if (reserveCount > 1)
		  infos.reserve(reserveCount);
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

	  FindSegmentsRead find(directory);
	  
	  //todo: see if we can do better than allocating a new SegmentInfos...
	  void* tmp = find.run();
	  if (tmp)
		delete tmp;
  }


  void SegmentInfos::read(Directory* directory, const char* segmentFileName){
	  bool success = false;

	  // Clear any previous segments:
	  clear();

	  IndexInput* input = directory->openInput(segmentFileName);
	  CND_CONDITION(input != NULL,"input == NULL"); // todo: make sure if at all necessary

	  generation = generationFromSegmentsFileName( segmentFileName );
	  lastGeneration = generation;
	  
	  try {
		  int32_t format = input->readInt();
		  if(format < 0){     // file contains explicit format info
			  // check that it is a format we can understand
			  if (format < CURRENT_FORMAT){
				  TCHAR err[30];
				  _sntprintf(err,30,_T("Unknown format version: %d"), format);
				  _CLTHROWT(CL_ERR_CorruptIndex, err);
			  }
			  version = input->readLong(); // read version
			  counter = input->readInt(); // read counter
		  }
		  else{     // file is in old format without explicit format info
			  counter = format;
		  }

		  for (int32_t i = input->readInt(); i > 0; i--) { // read segmentInfos
			  infos.push_back( _CLNEW SegmentInfo(directory, format, input) );
		  }

		  if(format >= 0){    // in old format the version number may be at the end of the file
			  if (input->getFilePointer() >= input->length())
				  version = CL_NS(util)::Misc::currentTimeMillis(); // old file format without version number
			  else
				  version = input->readLong(); // read version
		  }
		  success = true;
	  } _CLFINALLY({
		  input->close();
		  _CLDELETE(input);
		  if (!success) {
			  // Clear any segment infos we had loaded so we
			  // have a clean slate on retry:
			  clear();
		  }
	  });
  }
  
  //void SegmentInfos::read(Directory* directory, const char* segmentFileName){
  ////Func - Reads segments file that resides in directory. 
  ////Pre  - directory contains a valid reference
  ////Post - The segments file has been read and for each segment found
  ////       a SegmentsInfo intance has been created and stored.

	 // bool success = false;

	 // // Clear any previous segments:
	 // clear();
	 // 
	 // IndexInput* input = directory->openInput( segmentFileName );
	 // CND_CONDITION(input != NULL,"input == NULL"); // todo: make sure if at all necessary

	 // generation = generationFromSegmentsFileName( segmentFileName );
	 // lastGeneration = generation;
	 // 
	 // try {
		//  int32_t format = input->readInt();

		//  if(format < 0){     // file contains explicit format info
		//	  // check that it is a format we can understand
		//	  if (format < CURRENT_FORMAT){
		//		  TCHAR err[30];
		//		  _sntprintf(err,30,_T("Unknown format version: %d"), format);
		//		  _CLTHROWT(CL_ERR_CorruptIndex, err);
		//	  }
		//	  version = input->readLong(); // read version
		//	  counter = input->readInt(); // read counter
		//  }
		//  else{     // file is in old format without explicit format info
		//	  counter = format;
		//  }

		//  //for (int32_t i = input->readInt(); i > 0; i--) { // read segmentInfos
		////	  addElement(_CLNEW SegmentInfo(directory, format, input));
		//  //}

		//  //Temporary variable for storing the name of the segment
		//  TCHAR tname[CL_MAX_PATH];
		//  char aname[CL_MAX_PATH];
		//  SegmentInfo* si  = NULL;

		//  int32_t size = 0;
		//  int64_t delGen = 0;
		//  int64_t *normGen = NULL;
		//  bool hasSingleNorm = false;
		//  uint8_t isCompoundFile = 0;
		//  bool preLockless = true;

		//  //read segmentInfos
		//  for (int32_t i = input->readInt(); i > 0; --i){

		//	  // read the name of the segment
		//	  input->readString(tname, CL_MAX_PATH); 
		//	  STRCPY_TtoA(aname,tname,CL_MAX_PATH);

		//	  size = input->readInt();

		//	  if ( format <= FORMAT_LOCKLESS ) {						
		//		  delGen = input->readLong();
		//		  if ( format <= FORMAT_SINGLE_NORM_FILE ) {
		//			  hasSingleNorm = (1 == input->readByte());
		//		  } else {
		//			  hasSingleNorm = false;
		//		  }
		//		  int32_t numNormGen = input->readInt();
		//		  if ( numNormGen == -1 ) {
		//			  normGen = NULL;
		//		  } else {
		//			  normGen = _CL_NEWARRAY(int64_t, numNormGen);
		//			  for ( int32_t j = 0; j < numNormGen; j++ ) {
		//				  normGen[j] = input->readLong();
		//			  }
		//		  }
		//		  isCompoundFile = input->readByte();
		//		  preLockless = ( isCompoundFile == 0 );

		//		  //todo: temporary patch. this whole function needs to be rewritted
		//		  si = _CLNEW SegmentInfo(aname, size, directory); //, iscompoundfile, hassinglenormfile, docstoreoffset, docstoresegment, docstoreiscompoundfile);
		//		  ////si = _CLNEW SegmentInfo(aname, size, directory, delGen, normGen, isCompoundFile, hasSingleNorm, preLockless );

		//	  } else {	

		//		  si = _CLNEW SegmentInfo(aname, size, directory); 

		//	  }

		//	  //Condition check to see if si points to an instance
		//	  CND_CONDITION(si != NULL, "Memory allocation for si failed")	;

		//	  //store SegmentInfo si
		//	  infos.push_back(si);

		//  } 

		//  if(format >= 0){ // in old format the version number may be at the end of the file
		//	  if (input->getFilePointer() >= input->length())
		//		  version = Misc::currentTimeMillis(); // old file format without version number
		//	  else
		//		  version = input->readLong(); // read version
		//  }

		//  success = true;
	 // } _CLFINALLY(
		//  //destroy the inputStream input. The destructor of IndexInput will 
		//  //also close the Inputstream input
		//  _CLDELETE( input );
	 // if ( !success ) {
		//  infos.clear();
	 // }
	 // );

  //}


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
	  
	  int64_t* tmp = static_cast<int64_t*>(find.run());
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
		TCHAR* strdir = directory->toString();
		TCHAR err[CL_MAX_PATH + 65];
		_sntprintf(err,CL_MAX_PATH + 65,_T("cannot read directory %s: list() returned NULL"), strdir);
		_CLDELETE_ARRAY(strdir);
		_CLTHROWT(CL_ERR_IO, err);
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
  		//todo: throw new IllegalArgumentException("fileName \"" + fileName + "\" is not a segments file");
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
  	
  	return IndexFileNames::fileNameFromGeneration( IndexFileNames::SEGMENTS, "", nextGeneration );
  }

  SegmentInfo* SegmentInfos::elementAt(int32_t pos) {
	  return infos.at(pos);
  }

  void SegmentInfos::setElementAt(SegmentInfo* si, int32_t pos) {
	  infos.set(pos, si);
  }

  void SegmentInfos::clear() { infos.clear(); }

  SegmentInfos* SegmentInfos::clone() {
	  SegmentInfos* sis = _CLNEW SegmentInfos(true, infos.size());
	  for(size_t i=0;i<infos.size();i++) {
		  sis->setElementAt(infos[i]->clone(), i);
	  }
	  return sis;
  }

  SegmentInfos::FindSegmentsFile::FindSegmentsFile( CL_NS(store)::Directory* dir ) {
	  this->directory = dir;
  }
  
  SegmentInfos::FindSegmentsFile::~FindSegmentsFile() {	 
  }
  

  void* SegmentInfos::FindSegmentsFile::run() {

	  // todo: this function is obsolete by 2.3.2, and needs to be re-written
	  // ish: already started work on this

	  char* segmentFileName = NULL;
	  int64_t lastGen = -1;
	  int64_t gen = 0;
	  int32_t genLookaheadCount = 0;
	  bool retry = false;

	  int32_t method = 0;
	  void* value = NULL;

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
			  char** files = NULL;

			  int64_t genA = -1;

			  if (directory != NULL)
				  files = directory->list();
			  //else
			  //todo: files = fileDirectory.list();

			  if (files != NULL) {
				  genA = getCurrentSegmentGeneration( files );
				  _CLDELETE_CaARRAY_ALL( files );
			  }

			  //CL_TRACE("directory listing genA=%d\n", genA);


			  // Method 2: open segments.gen and read its
			  // contents.  Then we take the larger of the two
			  // gen's.  This way, if either approach is hitting
			  // a stale cache (NFS) we have a better chance of
			  // getting the right generation.
			  int64_t genB = -1;
			  if (directory != NULL) {
				  for(int32_t i=0;i<defaultGenFileRetryCount;i++) {
					  IndexInput* genInput = NULL;
					  try {
						  genInput = directory->openInput(IndexFileNames::SEGMENTS_GEN);
					  } catch (CLuceneError &e) {
						  //if (e.number == CL_ERR_FileNotFound) { // FileNotFound not yet exists...
						  //	CL_TRACE("segments.gen open: FileNotFoundException %s", e);
						  //	break;
						  //} else
					      if (e.number() == CL_ERR_IO) {
							//CL_TRACE("segments.gen open: IOException %s", e);
						  } else
							  throw e;
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
									  break;
								  }
							  }
						  } catch (CLuceneError &err2) {
							  if (err2.number() != CL_ERR_IO) throw err2; // retry only for IOException
						  } _CLFINALLY({
							  genInput->close();
							  _CLDELETE(genInput);
						  });
					  }
					  /*
					  try {
						  //todo: Thread.sleep(defaultGenFileRetryPauseMsec);
					  } catch (CLuceneError &e) {
						  //if (err2.number != CL_ERR_Interrupted) // retry only for InterruptedException
						  // todo: see if CL_ERR_Interrupted needs to be added...
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
				  _CLTHROWA(CL_ERR_IO, "No segments* file found"); //todo: add folder name (directory->toString())
			  } 
		  }

		  // Third method (fallback if first & second methods
		  // are not reliable): since both directory cache and
		  // file contents cache seem to be stale, just
		  // advance the generation.
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

  SegmentInfos::FindSegmentsRead::FindSegmentsRead( CL_NS(store)::Directory* dir ) : SegmentInfos::FindSegmentsFile(dir) {
  }
  void* SegmentInfos::FindSegmentsRead::doBody( const char* segmentFileName ) {
	  //read(directory, segmentFileName);
	  
	  //Instantiate SegmentInfos
	  SegmentInfos* infos = _CLNEW SegmentInfos;
	  try{
		  //Have SegmentInfos read the segments file in directory
		  infos->read(directory, segmentFileName);
	  } _CLFINALLY(
		  //make sure infos is cleaned up
		  _CLDELETE(infos);
	  );
	  return NULL;
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
				  _CLTHROWT(CL_ERR_CorruptIndex,err);
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
	  sis->read(directory, segmentFileName);
	  *version = sis->getVersion();
	  _CLDELETE(sis);

	  return version;

  }

CL_NS_END
