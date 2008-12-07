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
		   }
		   this->normGenLen = src->normGenLen;
		   memcpy(this->normGen, src->normGen, this->normGenLen);
	   }
	   isCompoundFile = src->isCompoundFile;
	   hasSingleNormFile = src->hasSingleNormFile;
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
	   _CLDELETE_CARRAY(tname);

	   docCount = input->readInt();
	   if (format <= SegmentInfos::FORMAT_LOCKLESS) {
		   delGen = input->readLong();
		   if (format <= SegmentInfos::FORMAT_SHARED_DOC_STORE) {
			   docStoreOffset = input->readInt();
			   if (docStoreOffset != -1) {
				   tname = input->readString();
				   char* aname = new char[CL_MAX_PATH]; // don't ref-count aname since it will be handled by CLStringIntern
				   STRCPY_TtoA(aname, tname, CL_MAX_PATH);
				   _CLDELETE_CARRAY(tname);
				   
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

   void SegmentInfo::setNumFields(const int32_t numFields) {
	   if (normGen == NULL) {
		   // normGen is null if we loaded a pre-2.1 segment
		   // file, or, if this segments file hasn't had any
		   // norms set against it yet:
		   normGen = _CL_NEWARRAY(int64_t, numFields);
		   // todo: make sure memory was allocated successfully?
		   normGenLen = numFields;

		   if (preLockless) {
			   // Do nothing: thus leaving normGen[k]==CHECK_DIR (==0), so that later we know  
			   // we have to check filesystem for norm files, because this is prelockless.

		   } else {
			   // This is a FORMAT_LOCKLESS segment, which means
			   // there are no separate norms:
			   for(int32_t i=0;i<numFields;i++) {
				   normGen[i] = NO;
			   }
		   }
	   }
   }

   bool SegmentInfo::hasDeletions() const {
	   // Cases:
	   //
	   //   delGen == NO: this means this segment was written
	   //     by the LOCKLESS code and for certain does not have
	   //     deletions yet
	   //
	   //   delGen == CHECK_DIR: this means this segment was written by
	   //     pre-LOCKLESS code which means we must check
	   //     directory to see if .del file exists
	   //
	   //   delGen >= YES: this means this segment was written by
	   //     the LOCKLESS code and for certain has
	   //     deletions
	   //
	   if (delGen == NO) {
		   return false;
	   } else if (delGen >= YES) {
		   return true;
	   } else {
		   return dir->fileExists(getDelFileName());
	   }
   }

   void SegmentInfo::advanceDelGen() {
	   // delGen 0 is reserved for pre-LOCKLESS format
	   if (delGen == NO) {
		   delGen = YES;
	   } else {
		   delGen++;
	   }
	   clearFiles();
   }

   void SegmentInfo::clearDelGen() {
	   delGen = NO;
	   clearFiles();
   }

   SegmentInfo* SegmentInfo::clone () {
	   SegmentInfo* si = _CLNEW SegmentInfo(name, docCount, dir, false, hasSingleNormFile,
		   docStoreOffset, docStoreSegment, docStoreIsCompoundFile);
	   si->isCompoundFile = isCompoundFile;
	   si->delGen = delGen;
	   si->preLockless = preLockless;
	   if (this->normGen != NULL) {
		   si->normGen = _CL_NEWARRAY(int64_t, this->normGenLen);
		   si->normGenLen = this->normGenLen;
		   memcpy(si->normGen, this->normGen, this->normGenLen);
	   }
	   return si;
   }

   char* SegmentInfo::getDelFileName() const {
	   if (delGen == NO) {
		   // In this case we know there is no deletion filename
		   // against this segment
		   return NULL;
	   } else {
		   // If delGen is CHECK_DIR, it's the pre-lockless-commit file format
		   char fn[8];
		   cl_sprintf(fn, 8, ".%s", IndexFileNames::DELETES_EXTENSION);
		   return IndexFileNames::fileNameFromGeneration(name, fn, delGen); 
	   }
   }

   bool SegmentInfo::hasSeparateNorms(const int32_t fieldNumber) const {
	   if ((normGen == NULL && preLockless) || (normGen != NULL && normGen[fieldNumber] == CHECK_DIR)) {
		   // Must fallback to directory file exists check:
		   char fileName[255]; // todo: try to see if we can put less than 255
		   cl_sprintf(fileName, 255, "%s.s%d", name, fieldNumber);
		   return dir->fileExists(fileName);
	   } else if (normGen == NULL || normGen[fieldNumber] == NO) {
		   return false;
	   } else {
		   return true;
	   }
   }

   bool SegmentInfo::hasSeparateNorms() const {  
	   if (normGen == NULL) {
		   if (!preLockless) {
			   // This means we were created w/ LOCKLESS code and no
			   // norms are written yet:
			   return false;
		   } else {
			   // This means this segment was saved with pre-LOCKLESS
			   // code.  So we must fallback to the original
			   // directory list check:
			   char** result = dir->list();
			   if (result == NULL) {
				   _CLTHROWA(CL_ERR_IO, "cannot read directory: list() returned NULL"); // todo: add dir name
			   }

			   char pattern[255]; // todo: make better size estimation for this
			   cl_sprintf(pattern, 255, "%s.s", name);
			   size_t patternLength = strlen(pattern);
			   for(size_t i = 0; result[i] != NULL; i++){
				   if(strncmp(result[i], pattern, strlen(pattern)) == 0 && isdigit(result[i][patternLength])) {
					   _CLDELETE_CaARRAY_ALL(result);
					   return true;
				   }
			   }
			   _CLDELETE_CaARRAY_ALL(result);
			   return false;
		   }
	   } else {
		   // This means this segment was saved with LOCKLESS
		   // code so we first check whether any normGen's are >= 1
		   // (meaning they definitely have separate norms):
		   for(size_t i=0;i<normGenLen;i++) {
			   if (normGen[i] >= YES) {
				   return true;
			   }
		   }
		   // Next we look for any == 0.  These cases were
		   // pre-LOCKLESS and must be checked in directory:
		   for(size_t j=0;j<normGenLen;j++) {
			   if (normGen[j] == CHECK_DIR) {
				   if (hasSeparateNorms(j)) {
					   return true;
				   }
			   }
		   }
	   }

	   return false;
   }

   void SegmentInfo::advanceNormGen(const int32_t fieldIndex) {
	   if (normGen[fieldIndex] == NO) {
		   normGen[fieldIndex] = YES;
	   } else {
		   normGen[fieldIndex]++;
	   }
	   clearFiles();
   }

   char* SegmentInfo::getNormFileName(const int32_t number) const {
	   char prefix[10];

	   int64_t gen;
	   if (normGen == NULL) {
		   gen = CHECK_DIR;
	   } else {
		   gen = normGen[number];
	   }

	   if (hasSeparateNorms(number)) {
		   // case 1: separate norm
		   cl_sprintf(prefix, 10, ".s%d", number);
		   return IndexFileNames::fileNameFromGeneration(name, prefix, gen);
	   }

	   if (hasSingleNormFile) {
		   // case 2: lockless (or nrm file exists) - single file for all norms 
		   cl_sprintf(prefix, 10, ".%s", IndexFileNames::NORMS_EXTENSION);
		   return IndexFileNames::fileNameFromGeneration(name, prefix, WITHOUT_GEN);
	   }

	   // case 3: norm file for each field
	   cl_sprintf(prefix, 10, ".f%d", number);
	   return IndexFileNames::fileNameFromGeneration(name, prefix, WITHOUT_GEN);
   }

   void SegmentInfo::setUseCompoundFile(const bool isCompoundFile) {
	   if (isCompoundFile) {
		   this->isCompoundFile = YES;
	   } else {
		   this->isCompoundFile = NO;
	   }
	   clearFiles();
   }

   bool SegmentInfo::getUseCompoundFile() const {
	   if (isCompoundFile == NO) {
		   return false;
	   } else if (isCompoundFile == YES) {
		   return true;
	   } else {
		   char fn[255]; // todo: make size more exact
		   cl_sprintf(fn, 255, "%s.%s", name, IndexFileNames::COMPOUND_FILE_EXTENSION);
		   return dir->fileExists(fn);
	   }
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

   void SegmentInfo::clearFiles() {
	   //files = null;
	   sizeInBytes = -1;
   }

   /** We consider another SegmentInfo instance equal if it
   *  has the same dir and same name. */
   bool SegmentInfo::equals(SegmentInfo* obj) {
	   return (obj->dir == this->dir && strcmp(obj->name, this->name) == 0);
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
		  TCHAR err[CL_MAX_PATH + 35];
		  _sntprintf(err,CL_MAX_PATH + 35,_T("fileName \"%s\" is not a segments file"), fileName);
		  _CLTHROWA(CL_ERR_IllegalArgument, err);
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
  SegmentInfo* SegmentInfos::elementAt(int32_t pos) {
	  return infos.at(pos);
  }
  void SegmentInfos::setElementAt(SegmentInfo* si, int32_t pos) {
	  infos.set(pos, si);
  }
  void SegmentInfos::clear() { infos.clear(); }

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
				  char err[30];
				  cl_sprintf(err,30,"Unknown format version: %d", format);
				  _CLTHROWA(CL_ERR_CorruptIndex, err);
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

  void SegmentInfos::read(Directory* directory) {
	  generation = lastGeneration = -1;

	  FindSegmentsRead find(directory);
	  
	  //todo: see if we can do better than allocating a new SegmentInfos...
	  void* tmp = find.run();
	  if (tmp)
		delete tmp;
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

  SegmentInfos* SegmentInfos::clone() {
	  SegmentInfos* sis = _CLNEW SegmentInfos(true, infos.size());
	  for(size_t i=0;i<infos.size();i++) {
		  sis->setElementAt(infos[i]->clone(), i);
	  }
	  return sis;
  }

  int64_t SegmentInfos::getVersion() const { return version; }
  int64_t SegmentInfos::getGeneration() const { return generation; }
  int64_t SegmentInfos::getLastGeneration() const { return lastGeneration; }

  int64_t SegmentInfos::readCurrentVersion(Directory* directory){

	  FindSegmentsVersion find(directory);
	  
	  int64_t* tmp = static_cast<int64_t*>(find.run());
	  int64_t version = *tmp;
	  delete tmp;
	  
	  return version;
  }

  //void SegmentInfos::setDefaultGenFileRetryCount(const int32_t count) { defaultGenFileRetryCount = count; }
  int32_t SegmentInfos::getDefaultGenFileRetryCount() { return defaultGenFileRetryCount; }

  //void SegmentInfos::setDefaultGenFileRetryPauseMsec(const int32_t msec) { defaultGenFileRetryPauseMsec = msec; }
  int32_t SegmentInfos::getDefaultGenFileRetryPauseMsec() { return defaultGenFileRetryPauseMsec; }

  //void SegmentInfos::setDefaultGenLookaheadCount(const int32_t count) { defaultGenLookaheadCount = count;}
  int32_t SegmentInfos::getDefaultGenLookahedCount() { return defaultGenLookaheadCount; }

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
				  CLuceneError e;
				  for(int32_t i=0;i<defaultGenFileRetryCount;i++) {
					  IndexInput* genInput = NULL;
					  if ( ! directory->openInput(IndexFileNames::SEGMENTS_GEN, genInput, e) ){
						  //if (e.number == CL_ERR_FileNotFound) { // FileNotFound not yet exists...
						  //	CL_TRACE("segments.gen open: FileNotFoundException %s", e);
						  //    _CLLDELETE(genInput);
						  //	break;
						  //} else
						  if (e.number() == CL_ERR_IO) {
							  //CL_TRACE("segments.gen open: IOException %s", e);
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
			  void* v = doBody(segmentFileName);
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
						  void* v = doBody(prevSegmentFileName);
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
				  char err[30];
				  cl_sprintf(err,30,"Unknown format version: %d",format);
				  _CLTHROWA(CL_ERR_CorruptIndex,err);
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
