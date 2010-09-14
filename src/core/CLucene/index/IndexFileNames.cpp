/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#include "_IndexFileNames.h"
#include "_SegmentInfos.h"
#include "CLucene/util/Misc.h"


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

	const char* IndexFileNames_INDEX_EXTENSIONS_s[] =
		{
			IndexFileNames::COMPOUND_FILE_EXTENSION,
			IndexFileNames::FIELD_INFOS_EXTENSION,
			IndexFileNames::FIELDS_INDEX_EXTENSION,
			IndexFileNames::FIELDS_EXTENSION,
			IndexFileNames::TERMS_INDEX_EXTENSION,
			IndexFileNames::TERMS_EXTENSION,
			IndexFileNames::FREQ_EXTENSION,
			IndexFileNames::PROX_EXTENSION,
			IndexFileNames::DELETES_EXTENSION,
			IndexFileNames::VECTORS_INDEX_EXTENSION,
			IndexFileNames::VECTORS_DOCUMENTS_EXTENSION,
			IndexFileNames::VECTORS_FIELDS_EXTENSION,
			IndexFileNames::GEN_EXTENSION,
			IndexFileNames::NORMS_EXTENSION,
			IndexFileNames::COMPOUND_FILE_STORE_EXTENSION
		};
	CL_NS(util)::ConstValueArray<const char*> IndexFileNames::INDEX_EXTENSIONS(IndexFileNames_INDEX_EXTENSIONS_s, 15 );

	const char* IndexFileNames_INDEX_EXTENSIONS_IN_COMPOUND_FILE_s[] = {
		IndexFileNames::FIELD_INFOS_EXTENSION,
		IndexFileNames::FIELDS_INDEX_EXTENSION,
		IndexFileNames::FIELDS_EXTENSION,
		IndexFileNames::TERMS_INDEX_EXTENSION,
		IndexFileNames::TERMS_EXTENSION,
		IndexFileNames::FREQ_EXTENSION,
		IndexFileNames::PROX_EXTENSION,
		IndexFileNames::VECTORS_INDEX_EXTENSION,
		IndexFileNames::VECTORS_DOCUMENTS_EXTENSION,
		IndexFileNames::VECTORS_FIELDS_EXTENSION,
		IndexFileNames::NORMS_EXTENSION
	};
	CL_NS(util)::ConstValueArray<const char*> IndexFileNames::INDEX_EXTENSIONS_IN_COMPOUND_FILE(IndexFileNames_INDEX_EXTENSIONS_IN_COMPOUND_FILE_s, 11 );

	const char* IndexFileNames_STORE_INDEX_EXTENSIONS_s[] = {
		IndexFileNames::VECTORS_INDEX_EXTENSION,
		IndexFileNames::VECTORS_FIELDS_EXTENSION,
		IndexFileNames::VECTORS_DOCUMENTS_EXTENSION,
		IndexFileNames::FIELDS_INDEX_EXTENSION,
		IndexFileNames::FIELDS_EXTENSION
	};
	CL_NS(util)::ConstValueArray<const char*> IndexFileNames::STORE_INDEX_EXTENSIONS(IndexFileNames_STORE_INDEX_EXTENSIONS_s, 5 );
	
	const char* IndexFileNames_NON_STORE_INDEX_EXTENSIONS_s[] = {
		IndexFileNames::FIELD_INFOS_EXTENSION,
		IndexFileNames::FREQ_EXTENSION,
		IndexFileNames::PROX_EXTENSION,
		IndexFileNames::TERMS_EXTENSION,
		IndexFileNames::TERMS_INDEX_EXTENSION,
		IndexFileNames::NORMS_EXTENSION
	};
	CL_NS(util)::ConstValueArray<const char*> IndexFileNames::NON_STORE_INDEX_EXTENSIONS(IndexFileNames_NON_STORE_INDEX_EXTENSIONS_s, 6 );

	const char* IndexFileNames_COMPOUND_EXTENSIONS_s[] = {
		IndexFileNames::FIELD_INFOS_EXTENSION,
		IndexFileNames::FREQ_EXTENSION,
		IndexFileNames::PROX_EXTENSION,
		IndexFileNames::FIELDS_INDEX_EXTENSION,
		IndexFileNames::FIELDS_EXTENSION,
		IndexFileNames::TERMS_INDEX_EXTENSION,
		IndexFileNames::TERMS_EXTENSION
	};
	CL_NS(util)::ConstValueArray<const char*> IndexFileNames::COMPOUND_EXTENSIONS(IndexFileNames_COMPOUND_EXTENSIONS_s, 7 );

	const char* IndexFileNames_VECTOR_EXTENSIONS_s[] = {
		IndexFileNames::VECTORS_INDEX_EXTENSION,
		IndexFileNames::VECTORS_DOCUMENTS_EXTENSION,
		IndexFileNames::VECTORS_FIELDS_EXTENSION
	};
	CL_NS(util)::ConstValueArray<const char*> IndexFileNames::VECTOR_EXTENSIONS(IndexFileNames_VECTOR_EXTENSIONS_s, 3 );

	string IndexFileNames::fileNameFromGeneration( const char* base, const char* extension, int64_t gen ) {
		if ( gen == SegmentInfo::NO ) {
			return "";
		} else if ( gen == SegmentInfo::WITHOUT_GEN ) {
			return string(base) + extension;
		} else {
      char buf[(sizeof(unsigned long) << 3) + 1];
      CL_NS(util)::Misc::longToBase( gen, 36, buf );
      return string(base) + "_" + buf + extension;
		}
	}
	
	bool IndexFileNames::isDocStoreFile( const char* fileName ) {
		
		const char* p = strchr( fileName, (int)'.' );
		
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
