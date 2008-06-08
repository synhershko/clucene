
#include "CLucene/StdHeader.h"

#include "IndexFileNames.h"
#include "SegmentInfos.h"

CL_NS_DEF(index)

	const char* IndexFileNames::SEGMENTS = "segments";
	const char* IndexFileNames::SEGMENTS_GEN = "segments.gen";
	const char* IndexFileNames::DELETABLE = "deletable";
	const char* IndexFileNames::NORMS_EXTENSION = "nrm";
	const char* IndexFileNames::FREQ_EXTENSION = "frq";
	const char* IndexFileNames::PROX_EXTENSION = "prx";
	const char* IndexFileNames::TERMS_EXTENSION = "tis";
	const char* IndexFileNames::TERMS_INDEX_EXTENSION = "tii";
	const char* IndexFileNames::FIELDS_INDEX_EXTENSION = "fdx";
	const char* IndexFileNames::FIELDS_EXTENSION = "fdt";
	const char* IndexFileNames::VECTORS_FIELDS_EXTENSION = "tvf";
	const char* IndexFileNames::VECTORS_DOCUMENTS_EXTENSION = "tvd";
	const char* IndexFileNames::VECTORS_INDEX_EXTENSION = "tvx";
	const char* IndexFileNames::COMPOUND_FILE_EXTENSION = "cfs";
	const char* IndexFileNames::COMPOUND_FILE_STORE_EXTENSION = "cfx";
	const char* IndexFileNames::DELETES_EXTENSION = "del";
	const char* IndexFileNames::FIELD_INFOS_EXTENSION = "fnm";
	const char* IndexFileNames::PLAIN_NORMS_EXTENSION = "f";
	const char* IndexFileNames::SEPARATE_NORMS_EXTENSION = "s";
	const char* IndexFileNames::GEN_EXTENSION = "gen";

	const char* IndexFileNames::INDEX_EXTENSIONS[] = {
		COMPOUND_FILE_EXTENSION,
		FIELD_INFOS_EXTENSION,
		FIELDS_INDEX_EXTENSION,
		FIELDS_EXTENSION,
		TERMS_INDEX_EXTENSION,
		TERMS_EXTENSION,
		FREQ_EXTENSION,
		PROX_EXTENSION,
		DELETES_EXTENSION,
		VECTORS_INDEX_EXTENSION,
		VECTORS_DOCUMENTS_EXTENSION,
		VECTORS_FIELDS_EXTENSION,
		GEN_EXTENSION,
		NORMS_EXTENSION,
		COMPOUND_FILE_STORE_EXTENSION
	};

	const char* IndexFileNames::INDEX_EXTENSIONS_IN_COMPOUND_FILE[] = {
		FIELD_INFOS_EXTENSION,
		FIELDS_INDEX_EXTENSION,
		FIELDS_EXTENSION,
		TERMS_INDEX_EXTENSION,
		TERMS_EXTENSION,
		FREQ_EXTENSION,
		PROX_EXTENSION,
		VECTORS_INDEX_EXTENSION,
		VECTORS_DOCUMENTS_EXTENSION,
		VECTORS_FIELDS_EXTENSION,
		NORMS_EXTENSION
	};
	
	const char* IndexFileNames::STORE_INDEX_EXTENSIONS[] = {
		VECTORS_INDEX_EXTENSION,
		VECTORS_FIELDS_EXTENSION,
		VECTORS_DOCUMENTS_EXTENSION,
		FIELDS_INDEX_EXTENSION,
		FIELDS_EXTENSION
	};
	
	const char* IndexFileNames::NON_STORE_INDEX_EXTENSIONS[] = {
		FIELD_INFOS_EXTENSION,
		FREQ_EXTENSION,
		PROX_EXTENSION,
		TERMS_EXTENSION,
		TERMS_INDEX_EXTENSION,
		NORMS_EXTENSION
	};
	
	const char* IndexFileNames::COMPOUND_EXTENSIONS[] = {
		FIELD_INFOS_EXTENSION,
		FREQ_EXTENSION,
		PROX_EXTENSION,
		FIELDS_INDEX_EXTENSION,
		FIELDS_EXTENSION,
		TERMS_INDEX_EXTENSION,
		TERMS_EXTENSION
	};
	
	const char* IndexFileNames::VECTOR_EXTENSIONS[] = {
		VECTORS_INDEX_EXTENSION,
		VECTORS_DOCUMENTS_EXTENSION,
		VECTORS_FIELDS_EXTENSION
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
	
	bool IndexFileNames::isDocStoreFile( char* fileName ) {
		
		char* p = strchr( fileName, (int)'.' );
		
		if ( p != NULL && strcmp( p+1, COMPOUND_FILE_STORE_EXTENSION ) == 0 ) {
			return true;
		}
		for ( int32_t i = 0; i < STORE_INDEX_EXTENSIONS_LENGTH; i++ ) {
			if ( p != NULL && strcmp( p+1, STORE_INDEX_EXTENSIONS[i] ) == 0 ) {
				return true;
			}
		}
		return false;
	}
		
CL_NS_END
