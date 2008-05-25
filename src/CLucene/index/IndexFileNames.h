
#ifndef _lucene_index_IndexFileNames_
#define _lucene_index_IndexFileNames_

#if defined(_LUCENE_PRAGMA_ONCE)
# pragma once
#endif

CL_NS_DEF(index)

class IndexFileNames {
	
public:
	
	static const char* SEGMENTS;
	static const char* SEGMENTS_GEN;
	static const char* DELETABLE;
	static const char* NORMS_EXTENSION;
	static const char* COMPOUND_FILE_EXTENSION;
	static const char* DELETES_EXTENSION;
	static const char* PLAIN_NORMS_EXTENSION;
	static const char* SEPARATE_NORMS_EXTENSION;
	
	LUCENE_STATIC_CONSTANT(int32_t,COMPOUND_EXTENSIONS_LENGTH=7);
	LUCENE_STATIC_CONSTANT(int32_t,VECTOR_EXTENSIONS_LENGTH=3);
	
	static const char* INDEX_EXTENSIONS[];
	static const char* INDEX_EXTENSIONS_IN_COMPOUND_FILE[];
	static const char* COMPOUND_EXTENSIONS[];
	static const char* VECTOR_EXTENSIONS[];
	
	static const char* fileNameFromGeneration( const char* base, const char* extension, int64_t gen );
		
};

CL_NS_END
#endif
