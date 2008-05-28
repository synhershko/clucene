
#include "CLucene/StdHeader.h"

#include "IndexFileNames.h"
#include "SegmentInfos.h"

CL_NS_DEF(index)

	const char* IndexFileNames::SEGMENTS = "segments";
	const char* IndexFileNames::SEGMENTS_GEN = "segments.gen";
	const char* IndexFileNames::DELETABLE = "deletable";
	const char* IndexFileNames::NORMS_EXTENSION = "nrm";
	const char* IndexFileNames::COMPOUND_FILE_EXTENSION = "cfs";
	const char* IndexFileNames::DELETES_EXTENSION = "del";
	const char* IndexFileNames::PLAIN_NORMS_EXTENSION = "f";
	const char* IndexFileNames::SEPARATE_NORMS_EXTENSION = "s";

	const char* IndexFileNames::INDEX_EXTENSIONS[] = {
			"cfs", "fnm", "fdx", "fdt", "tii", "tis", "frq", 
			"prx", "del", "tvx", "tvd", "gen", "nrm"
	};

	const char* IndexFileNames::INDEX_EXTENSIONS_IN_COMPOUND_FILE[] = {
			"fnm", "fdx", "fdt", "tii", "tis", "frq", "prx",
			"tvx", "tvd", "tvf", "nrm"
	};
	
	const char* IndexFileNames::COMPOUND_EXTENSIONS[] = {
			"fnm", "frq", "prx", "fdx", "fdt", "tii", "tis"
	};
	
	const char* IndexFileNames::VECTOR_EXTENSIONS[] = {
			"tvx", "tvd", "tvf"
	};
	
	const char* IndexFileNames::fileNameFromGeneration( const char* base, const char* extension, int64_t gen ) {
		
		char fileName[CL_MAX_PATH];
		
		if ( gen == SegmentInfo::NO ) {
			return NULL;
		} else if ( gen == SegmentInfo::WITHOUT_GEN ) {
			strcpy( fileName, base );
			strcat( fileName, extension );
		} else {
			char *genStr = CL_NS(util)::Misc::longToBase( gen, 36 );
			sprintf( fileName, "%s_%s%s", base, genStr, extension );
			free( genStr );
		}

		return STRDUP_AtoA( fileName );
	}
	
CL_NS_END
