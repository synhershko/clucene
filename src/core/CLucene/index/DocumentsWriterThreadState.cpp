/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#include "CLucene/store/Directory.h"
#include "CLucene/store/IndexOutput.h"
#include "CLucene/store/_RAMDirectory.h"
#include "CLucene/util/Array.h"
#include "CLucene/util/Misc.h"
#include "CLucene/util/CLStreams.h"
#include "CLucene/document/Field.h"
#include "CLucene/search/Similarity.h"
#include "CLucene/document/Document.h"
#include "_FieldInfos.h"
#include "_CompoundFile.h"
#include "IndexWriter.h"
#include "_IndexFileNames.h"
#include "_FieldsWriter.h"
#include "Term.h"
#include "_Term.h"
#include "_TermInfo.h"
#include "_TermVector.h"
#include "_TermInfosWriter.h"
#include "CLucene/analysis/AnalysisHeader.h"
#include "CLucene/search/Similarity.h"
#include "_TermInfosWriter.h"
#include "_FieldsWriter.h"
#include "_DocumentsWriter.h"
#include <assert.h>

CL_NS_USE(util)
CL_NS_USE(store)
CL_NS_USE(analysis)
CL_NS_USE(document)
CL_NS_USE(search)
CL_NS_DEF(index)


DocumentsWriter::ThreadState::ThreadState(DocumentsWriter* __parent):
  _parent(__parent),
  fieldDataArray(ObjectArray<FieldData>(8)),
  fieldDataHash(ObjectArray<FieldData>(16)),
  vectorFieldPointers(ValueArray<int64_t>(10)),
  vectorFieldNumbers(ValueArray<int32_t>(10)),
  postingsFreeList(ValueArray<Posting>(256)),
  allFieldDataArray(ObjectArray<FieldData>(10)),
  postingsVectors(ObjectArray<PostingVector*>(1)),
  charPool( CharBlockPool(__parent, CHAR_BLOCK_SIZE) ),
  postingsPool( ByteBlockPool(__parent, BYTE_BLOCK_SIZE) ),
  vectorsPool( ByteBlockPool(__parent, BYTE_BLOCK_SIZE) )
{
  fieldDataHashMask = 15;
  postingsFreeCount = 0;
  stringReader = _CLNEW ReusableStringReader(_T(""),0,true);

  isIdle = true;
  numThreads = 1;

  tvfLocal = _CLNEW RAMOutputStream();    // Term vectors for one doc
  fdtLocal = _CLNEW RAMOutputStream();    // Stored fields for one doc
}

void DocumentsWriter::ThreadState::resetPostings() {
  fieldGen = 0;
  maxPostingsVectors = 0;
  doFlushAfter = false;
  if (localFieldsWriter != NULL) {
    localFieldsWriter->close();
    localFieldsWriter = NULL;
  }
  postingsPool.reset();
  charPool.reset();
  _parent->recyclePostings(postingsFreeList, postingsFreeCount);
  postingsFreeCount = 0;
  for(int32_t i=0;i<numAllFieldData;i++) {
    FieldData* fp = allFieldDataArray[i];
    fp->lastGen = -1;
    if (fp->numPostings > 0)
      fp->resetPostingArrays();
  }
}

void DocumentsWriter::ThreadState::writeDocument() {

  // If we hit an exception while appending to the
  // stored fields or term vectors files, we have to
  // abort all documents since we last flushed because
  // it means those files are possibly inconsistent.
  try {
    _parent->numDocsInStore++;

    // Append stored fields to the real FieldsWriter:
    _parent->fieldsWriter->flushDocument(numStoredFields, fdtLocal);
    fdtLocal->reset();

    // Append term vectors to the real outputs:
    if (_parent->tvx != NULL) {
      _parent->tvx->writeLong(_parent->tvd->getFilePointer());
      _parent->tvd->writeVInt(numVectorFields);
      if (numVectorFields > 0) {
        for(int32_t i=0;i<numVectorFields;i++)
          _parent->tvd->writeVInt(vectorFieldNumbers[i]);
        assert(0 == vectorFieldPointers[0]);
        _parent->tvd->writeVLong(_parent->tvf->getFilePointer());
        int64_t lastPos = vectorFieldPointers[0];
        for(int32_t i=1;i<numVectorFields;i++) {
          int64_t pos = vectorFieldPointers[i];
          _parent->tvd->writeVLong(pos-lastPos);
          lastPos = pos;
        }
        tvfLocal->writeTo(_parent->tvf);
        tvfLocal->reset();
      }
    }

    // Append norms for the fields we saw:
    for(int32_t i=0;i<numFieldData;i++) {
      FieldData* fp = fieldDataArray[i];
      if (fp->doNorms) {
        BufferedNorms* bn = _parent->norms[fp->fieldInfo->number];
        assert ( bn != NULL );
        assert ( bn->upto <= docID );
        bn->fill(docID);
        float_t norm = fp->boost * _parent->writer->getSimilarity()->lengthNorm(fp->fieldInfo->name, fp->length);
        bn->add(norm);
      }
    }
  } catch (CLuceneError& t) {
    // Forcefully idle this threadstate -- its state will
    // be reset by abort()
    isIdle = true;
    throw AbortException(t, _parent);
  }

  if (_parent->bufferIsFull && !_parent->flushPending) {
    _parent->flushPending = true;
    _parent->doFlushAfter = true;
  }
}

void DocumentsWriter::ThreadState::init(Document* doc, int32_t docID) {

  assert (!isIdle);
  assert (_parent->writer->testPoint("DocumentsWriter.ThreadState.init start"));

  this->docID = docID;
  docBoost = doc.getBoost();
  numStoredFields = 0;
  numFieldData = 0;
  numVectorFields = 0;
  maxTermPrefix = NULL;

  assert (0 == fdtLocal->length());
  assert (0 == fdtLocal->getFilePointer());
  assert (0 == tvfLocal->length());
  assert (0 == tvfLocal->getFilePointer());
  final int32_t thisFieldGen = fieldGen++;

  List docFields = doc.getFields();
  final int32_t numDocFields = docFields.size();
  bool docHasVectors = false;

  // Absorb any new fields first seen in this document.
  // Also absorb any changes to fields we had already
  // seen before (eg suddenly turning on norms or
  // vectors, etc.):

  for(int32_t i=0;i<numDocFields;i++) {
    Fieldable* field = docFields.get(i);

    FieldInfo* fi = _parent->fieldInfos->add(field->name(), field->isIndexed(), field->isTermVectorStored(),
                                  field->isStorePositionWithTermVector(), field->isStoreOffsetWithTermVector(),
                                  field->getOmitNorms(), false);
    if (fi.isIndexed && !fi.omitNorms) {
      // Maybe grow our buffered norms
      if (_parent->norms.length <= fi.number) {
        int32_t newSize = (int32_t) ((1+fi.number)*1.25);
        BufferedNorms[] newNorms = _CLNEW BufferedNorms[newSize];
        System.arraycopy(_parent->norms, 0, newNorms, 0, _parent->norms.length);
        norms = newNorms;
      }

      if (_parent->norms[fi.number] == NULL)
        _parent->norms[fi.number] = _CLNEW BufferedNorms();

      _parent->hasNorms = true;
    }

    // Make sure we have a FieldData allocated
    int32_t hashPos = fi.name.hashCode() & fieldDataHashMask;
    FieldData* fp = fieldDataHash[hashPos];
    while(fp != NULL && _tcscmp(fp->fieldInfo->name, fi->name) != 0 )
      fp = fp->next;

    if (fp == NULL) {

      fp = _CLNEW FieldData(fi);
      fp->next = fieldDataHash[hashPos];
      fieldDataHash[hashPos] = fp;

      if (numAllFieldData == allFieldDataArray.length) {
        int32_t newSize = (int32_t) (allFieldDataArray.length*1.5);
        int32_t newHashSize = fieldDataHash.length*2;

        FieldData newArray[] = _CLNEW FieldData[newSize];
        FieldData newHashArray[] = _CLNEW FieldData[newHashSize];
        System.arraycopy(allFieldDataArray, 0, newArray, 0, numAllFieldData);

        // Rehash
        fieldDataHashMask = newSize-1;
        for(int32_t j=0;j<fieldDataHash.length;j++) {
          FieldData* fp0 = fieldDataHash[j];
          while(fp0 != NULL) {
            hashPos = fp0->fieldInfo->name.hashCode() & fieldDataHashMask;
            FieldData nextFP0 = fp0.next;
            fp0.next = newHashArray[hashPos];
            newHashArray[hashPos] = fp0;
            fp0 = nextFP0;
          }
        }

        allFieldDataArray = newArray;
        fieldDataHash = newHashArray;
      }
      allFieldDataArray[numAllFieldData++] = fp;
    } else {
      assert (fp->fieldInfo == fi);
    }

    if (thisFieldGen != fp->lastGen) {

      // First time we're seeing this field for this doc
      fp->lastGen = thisFieldGen;
      fp->fieldCount = 0;
      fp->doVectors = fp->doVectorPositions = fp->doVectorOffsets = false;
      fp->doNorms = fi.isIndexed && !fi.omitNorms;

      if (numFieldData == fieldDataArray.length) {
        int32_t newSize = fieldDataArray.length*2;
        FieldData newArray[] = new FieldData[newSize];
        System.arraycopy(fieldDataArray, 0, newArray, 0, numFieldData);
        fieldDataArray = newArray;

      }
      fieldDataArray[numFieldData++] = fp;
    }

    if (field->isTermVectorStored()) {
      if (!fp->doVectors && numVectorFields++ == vectorFieldPointers.length) {
        const int32_t newSize = (numVectorFields*1.5);

		vectorFieldPointers.deleteArray();
        vectorFieldPointers.values = _CL_NEWARRAY(int64_t, newSize);
		vectorFieldPointers.length = newSize;

		vectorFieldNumbers.deleteArray();
        vectorFieldNumbers.values = _CL_NEWARRAY(int32_t, newSize);
		vectorFieldNumbers.length = newSize;
      }
      fp->doVectors = true;
      docHasVectors = true;

      fp->doVectorPositions |= field->isStorePositionWithTermVector();
      fp->doVectorOffsets |= field->isStoreOffsetWithTermVector();
    }

    if (fp->fieldCount == fp->docFields.length) {
      Fieldable[] newArray = new Fieldable[fp->docFields.length*2];
      System.arraycopy(fp->docFields, 0, newArray, 0, fp->docFields.length);
      fp->docFields = newArray;
    }

    // Lazily allocate arrays for postings:
    if (field->isIndexed() && fp->postingsHash == NULL)
      fp->initPostingArrays();

    fp->docFields[fp->fieldCount++] = field;
  }

  // Maybe init the local & global fieldsWriter
  if (localFieldsWriter == NULL) {
    if (fieldsWriter == NULL) {
      assert (docStoreSegment == NULL);
      assert (segment != NULL);
      docStoreSegment = segment;
      // If we hit an exception while init'ing the
      // fieldsWriter, we must abort this segment
      // because those files will be in an unknown
      // state:
      try {
        fieldsWriter = _CLNEW FieldsWriter(directory, docStoreSegment, fieldInfos);
      } catch (CLuceneError& t) {
        throw new AbortException(t, DocumentsWriter.this);
      }
      files = NULL;
    }
    localFieldsWriter = _CLNEW FieldsWriter(NULL, fdtLocal, fieldInfos);
  }

  // First time we see a doc that has field(s) with
  // stored vectors, we init our tvx writer
  if (docHasVectors) {
    if (tvx == NULL) {
      assert (docStoreSegment != NULL);
      // If we hit an exception while init'ing the term
      // vector output files, we must abort this segment
      // because those files will be in an unknown
      // state:
      try {
        tvx = directory.createOutput(docStoreSegment + "." + IndexFileNames.VECTORS_INDEX_EXTENSION);
        tvx->writeInt(TermVectorsReader.FORMAT_VERSION);
        tvd = directory.createOutput(docStoreSegment +  "." + IndexFileNames.VECTORS_DOCUMENTS_EXTENSION);
        tvd->writeInt(TermVectorsReader.FORMAT_VERSION);
        tvf = directory.createOutput(docStoreSegment +  "." + IndexFileNames.VECTORS_FIELDS_EXTENSION);
        tvf.writeInt(TermVectorsReader.FORMAT_VERSION);

        // We must "catch up" for all docs before us
        // that had no vectors:
        for(int32_t i=0;i<numDocsInStore;i++) {
          tvx->writeLong(tvd->getFilePointer());
          tvd->writeVInt(0);
        }

      } catch (CLuceneError& t) {
        throw new AbortException(t, DocumentsWriter.this);
      }
      files = NULL;
    }

    numVectorFields = 0;
  }
}

void DocumentsWriter::ThreadState::doPostingSort(ValueArray<Posting>& postings, int32_t numPosting) {
  quickSort(postings, 0, numPosting-1);
}

void DocumentsWriter::ThreadState::quickSort(ValueArray<Posting>& postings, int32_t lo, int32_t hi) {
  if (lo >= hi)
    return;

  int32_t mid = (lo + hi) >>> 1;

  if (comparePostings(postings[lo], postings[mid]) > 0) {
    Posting tmp = postings[lo];
    postings[lo] = postings[mid];
    postings[mid] = tmp;
  }

  if (comparePostings(postings[mid], postings[hi]) > 0) {
    Posting tmp = postings[mid];
    postings[mid] = postings[hi];
    postings[hi] = tmp;

    if (comparePostings(postings[lo], postings[mid]) > 0) {
      Posting tmp2 = postings[lo];
      postings[lo] = postings[mid];
      postings[mid] = tmp2;
    }
  }

  int32_t left = lo + 1;
  int32_t right = hi - 1;

  if (left >= right)
    return;

  Posting partition = postings[mid];

  for (; ;) {
    while (comparePostings(postings[right], partition) > 0)
      --right;

    while (left < right && comparePostings(postings[left], partition) <= 0)
      ++left;

    if (left < right) {
      Posting tmp = postings[left];
      postings[left] = postings[right];
      postings[right] = tmp;
      --right;
    } else {
      break;
    }
  }

  quickSort(postings, lo, left);
  quickSort(postings, left + 1, hi);
}

void DocumentsWriter::ThreadState::doVectorSort(ValueArray<PostingVector>& postings, int32_t numPosting) {
  quickSort(postings, 0, numPosting-1);
}

void DocumentsWriter::ThreadState::quickSort(ValueArray<PostingVector>& postings, int32_t lo, int32_t hi) {
  if (lo >= hi)
    return;

  int32_t mid = (lo + hi) >>> 1;

  if (comparePostings(postings[lo].p, postings[mid].p) > 0) {
    PostingVector tmp = postings[lo];
    postings[lo] = postings[mid];
    postings[mid] = tmp;
  }

  if (comparePostings(postings[mid].p, postings[hi].p) > 0) {
    PostingVector tmp = postings[mid];
    postings[mid] = postings[hi];
    postings[hi] = tmp;

    if (comparePostings(postings[lo].p, postings[mid].p) > 0) {
      PostingVector tmp2 = postings[lo];
      postings[lo] = postings[mid];
      postings[mid] = tmp2;
    }
  }

  int32_t left = lo + 1;
  int32_t right = hi - 1;

  if (left >= right)
    return;

  PostingVector partition = postings[mid];

  for (; ;) {
    while (comparePostings(postings[right].p, partition.p) > 0)
      --right;

    while (left < right && comparePostings(postings[left].p, partition.p) <= 0)
      ++left;

    if (left < right) {
      PostingVector tmp = postings[left];
      postings[left] = postings[right];
      postings[right] = tmp;
      --right;
    } else {
      break;
    }
  }

  quickSort(postings, lo, left);
  quickSort(postings, left + 1, hi);
}

void DocumentsWriter::ThreadState::trimFields() {

  int32_t upto = 0;
  for(int32_t i=0;i<numAllFieldData;i++) {
    FieldData fp = allFieldDataArray[i];
    if (fp->lastGen == -1) {
      // This field was not seen since the previous
      // flush, so, free up its resources now

      // Unhash
      final int32_t hashPos = fp->fieldInfo.name.hashCode() & fieldDataHashMask;
      FieldData last = NULL;
      FieldData fp0 = fieldDataHash[hashPos];
      while(fp0 != fp) {
        last = fp0;
        fp0 = fp0.next;
      }
      assert fp0 != NULL;

      if (last == NULL)
        fieldDataHash[hashPos] = fp->next;
      else
        last.next = fp->next;

      if (infoStream != NULL)
        infoStream.println("  remove field=" + fp->fieldInfo.name);

    } else {
      // Reset
      fp->lastGen = -1;
      allFieldDataArray[upto++] = fp;

      if (fp->numPostings > 0 && ((float_t) fp->numPostings) / fp->postingsHashSize < 0.2) {
        int32_t hashSize = fp->postingsHashSize;

        // Reduce hash so it's between 25-50% full
        while (fp->numPostings < (hashSize>>1) && hashSize >= 2)
          hashSize >>= 1;
        hashSize <<= 1;

        if (hashSize != fp->postingsHash.length)
          fp->rehashPostings(hashSize);
      }
    }
  }

  // If we didn't see any norms for this field since
  // last flush, free it
  for(int32_t i=0;i<norms.length;i++) {
    BufferedNorms n = norms[i];
    if (n != NULL && n.upto == 0)
      norms[i] = NULL;
  }

  numAllFieldData = upto;

  // Also pare back PostingsVectors if it's excessively
  // large
  if (maxPostingsVectors * 1.5 < postingsVectors.length) {
    final int32_t newSize;
    if (0 == maxPostingsVectors)
      newSize = 1;
    else
      newSize = (int32_t) (1.5*maxPostingsVectors);
    PostingVector[] newArray = _CLNEW PostingVector[newSize];
    System.arraycopy(postingsVectors, 0, newArray, 0, newSize);
    postingsVectors = newArray;
  }
}

void DocumentsWriter::ThreadState::processDocument(Analyzer analyzer)
  {

  final int32_t numFields = numFieldData;

  assert 0 == fdtLocal->length();

  if (tvx != NULL)
    // If we are writing vectors then we must visit
    // fields in sorted order so they are written in
    // sorted order.  TODO: we actually only need to
    // sort the subset of fields that have vectors
    // enabled; we could save [small amount of] CPU
    // here.
    Arrays.sort(fieldDataArray, 0, numFields);

  // We process the document one field at a time
  for(int32_t i=0;i<numFields;i++)
    fieldDataArray[i].processField(analyzer);

  if (maxTermPrefix != NULL && infoStream != NULL)
    infoStream.println("WARNING: document contains at least one immense term (longer than the max length " + MAX_TERM_LENGTH + "), all of which were skipped.  Please correct the analyzer to not produce such terms.  The prefix of the first immense term is: '" + maxTermPrefix + "...'");

  if (ramBufferSize != IndexWriter.DISABLE_AUTO_FLUSH
      && numBytesUsed > 0.95 * ramBufferSize)
    balanceRAM();
}

// USE ONLY FOR DEBUGGING!
/*
  String getPostingText() {
  char[] text = charPool.buffers[p.textStart >> CHAR_BLOCK_SHIFT];
  int32_t upto = p.textStart & CHAR_BLOCK_MASK;
  while(text[upto] != 0xffff)
  upto++;
  return new String(text, p.textStart, upto-(p.textStart & BYTE_BLOCK_MASK));
  }

bool DocumentsWriter::ThreadState::postingEquals(final char[] tokenText, final int32_t tokenTextLen) {

  final char[] text = charPool.buffers[p.textStart >> CHAR_BLOCK_SHIFT];
  assert text != NULL;
  int32_t pos = p.textStart & CHAR_BLOCK_MASK;

  int32_t tokenPos = 0;
  for(;tokenPos<tokenTextLen;pos++,tokenPos++)
    if (tokenText[tokenPos] != text[pos])
      return false;
  return 0xffff == text[pos];
}

int32_t DocumentsWriter::ThreadState::comparePostings(Posting p1, Posting p2) {
  final char[] text1 = charPool.buffers[p1.textStart >> CHAR_BLOCK_SHIFT];
  int32_t pos1 = p1.textStart & CHAR_BLOCK_MASK;
  final char[] text2 = charPool.buffers[p2.textStart >> CHAR_BLOCK_SHIFT];
  int32_t pos2 = p2.textStart & CHAR_BLOCK_MASK;
  while(true) {
    final char c1 = text1[pos1++];
    final char c2 = text2[pos2++];
    if (c1 < c2)
      if (0xffff == c2)
        return 1;
      else
        return -1;
    else if (c2 < c1)
      if (0xffff == c1)
        return -1;
      else
        return 1;
    else if (0xffff == c1)
      return 0;
  }
}

void DocumentsWriter::ThreadState::writeFreqVInt(int32_t i) {
  while ((i & ~0x7F) != 0) {
    writeFreqByte((uint8_t)((i & 0x7f) | 0x80));
    i >>>= 7;
  }
  writeFreqByte((uint8_t) i);
}

void DocumentsWriter::ThreadState::writeProxVInt(int32_t i) {
  while ((i & ~0x7F) != 0) {
    writeProxByte((uint8_t)((i & 0x7f) | 0x80));
    i >>>= 7;
  }
  writeProxByte((uint8_t) i);
}

void DocumentsWriter::ThreadState::writeFreqByte(uint8_t b) {
  assert freq != NULL;
  if (freq[freqUpto] != 0) {
    freqUpto = postingsPool.allocSlice(freq, freqUpto);
    freq = postingsPool.buffer;
    p.freqUpto = postingsPool.uint8_tOffset;
  }
  freq[freqUpto++] = b;
}

void DocumentsWriter::ThreadState::writeProxByte(uint8_t b) {
  assert prox != NULL;
  if (prox[proxUpto] != 0) {
    proxUpto = postingsPool.allocSlice(prox, proxUpto);
    prox = postingsPool.buffer;
    p.proxUpto = postingsPool.uint8_tOffset;
    assert prox != NULL;
  }
  prox[proxUpto++] = b;
  assert proxUpto != prox.length;
}

void DocumentsWriter::ThreadState::writeProxBytes(uint8_t[] b, int32_t offset, int32_t len) {
  final int32_t offsetEnd = offset + len;
  while(offset < offsetEnd) {
    if (prox[proxUpto] != 0) {
      // End marker
      proxUpto = postingsPool.allocSlice(prox, proxUpto);
      prox = postingsPool.buffer;
      p.proxUpto = postingsPool.uint8_tOffset;
    }

    prox[proxUpto++] = b[offset++];
    assert proxUpto != prox.length;
  }
}

void DocumentsWriter::ThreadState::writeOffsetVInt(int32_t i) {
  while ((i & ~0x7F) != 0) {
    writeOffsetByte((uint8_t)((i & 0x7f) | 0x80));
    i >>>= 7;
  }
  writeOffsetByte((uint8_t) i);
}

void DocumentsWriter::ThreadState::writeOffsetByte(uint8_t b) {
  assert offsets != NULL;
  if (offsets[offsetUpto] != 0) {
    offsetUpto = vectorsPool->allocSlice(offsets, offsetUpto);
    offsets = vectorsPool->buffer;
    vector.offsetUpto = vectorsPool->uint8_tOffset;
  }
  offsets[offsetUpto++] = b;
}

void DocumentsWriter::ThreadState::writePosVInt(int32_t i) {
  while ((i & ~0x7F) != 0) {
    writePosByte((uint8_t)((i & 0x7f) | 0x80));
    i >>>= 7;
  }
  writePosByte((uint8_t) i);
}

void DocumentsWriter::ThreadState::writePosByte(uint8_t b) {
  assert pos != NULL;
  if (pos[posUpto] != 0) {
    posUpto = vectorsPool->allocSlice(pos, posUpto);
    pos = vectorsPool->buffer;
    vector.posUpto = vectorsPool->uint8_tOffset;
  }
  pos[posUpto++] = b;
}




DocumentsWriter::ThreadState::FieldData::FieldData(DocumentsWriter* __parent, FieldInfo fieldInfo):
  _parent(__parent),
  docFields(ObjectArray<Fieldable*>(1)),
  localToken (_CLNEW Token),
  vectorSliceReader(_CLNEW ByteSliceReader())
{
  this->lastGen = -1;
  this->fieldInfo = fieldInfo;
  threadState = ThreadState.this;
}

void DocumentsWriter::ThreadState::FieldData::resetPostingArrays() {
  if (!postingsCompacted)
    compactPostings();
  recyclePostings(this->postingsHash, numPostings);
  Arrays.fill(postingsHash, 0, postingsHash.length, NULL);
  postingsCompacted = false;
  numPostings = 0;
}

void DocumentsWriter::ThreadState::FieldData::initPostingArrays() {
  // Target hash fill factor of <= 50%
  // NOTE: must be a power of two for hash collision
  // strategy to work correctly
  postingsHashSize = 4;
  postingsHashHalfSize = 2;
  postingsHashMask = postingsHashSize-1;
  postingsHash = _CLNEW Posting[postingsHashSize];
}

int32_t DocumentsWriter::ThreadState::FieldData::compareTo(Object o) {
  return fieldInfo.name.compareTo(((FieldData) o).fieldInfo.name);
}

void DocumentsWriter::ThreadState::FieldData::compactPostings() {
  int32_t upto = 0;
  for(int32_t i=0;i<postingsHashSize;i++)
    if (postingsHash[i] != NULL)
      postingsHash[upto++] = postingsHash[i];

  assert upto == numPostings;
  postingsCompacted = true;
}

Posting[] DocumentsWriter::ThreadState::FieldData::sortPostings() {
  compactPostings();
  doPostingSort(postingsHash, numPostings);
  return postingsHash;
}

void DocumentsWriter::ThreadState::FieldData::processField(Analyzer analyzer) {
  length = 0;
  position = 0;
  offset = 0;
  boost = docBoost;

  final int32_t maxFieldLength = writer.getMaxFieldLength();

  final int32_t limit = fieldCount;
  final Fieldable[] docFieldsFinal = docFields;

  bool doWriteVectors = true;

  // Walk through all occurrences in this doc for this
  // field:
  try {
    for(int32_t j=0;j<limit;j++) {
      Fieldable field = docFieldsFinal[j];

      if (field->isIndexed())
        invertField(field, analyzer, maxFieldLength);

      if (field->isStored()) {
        numStoredFields++;
        bool success = false;
        try {
          localFieldsWriter->writeField(fieldInfo, field);
          success = true;
        } finally {
          // If we hit an exception inside
          // localFieldsWriter->writeField, the
          // contents of fdtLocal can be corrupt, so
          // we must discard all stored fields for
          // this document:
          if (!success)
            fdtLocal->reset();
        }
      }

      docFieldsFinal[j] = NULL;
    }
  } catch (AbortException& ae) {
    doWriteVectors = false;
    throw ae;
  } finally {
    if (postingsVectorsUpto > 0) {
      try {
        if (doWriteVectors) {
          // Add term vectors for this field
          bool success = false;
          try {
            writeVectors(fieldInfo);
            success = true;
          } finally {
            if (!success) {
              // If we hit an exception inside
              // writeVectors, the contents of tvfLocal
              // can be corrupt, so we must discard all
              // term vectors for this document:
              numVectorFields = 0;
              tvfLocal->reset();
            }
          }
        }
      } finally {
        if (postingsVectorsUpto > maxPostingsVectors)
          maxPostingsVectors = postingsVectorsUpto;
        postingsVectorsUpto = 0;
        vectorsPool->reset();
      }
    }
  }
}

void DocumentsWriter::ThreadState::FieldData::invertField(Fieldable field, Analyzer analyzer, final int32_t maxFieldLength) {

  if (length>0)
    position += analyzer.getPositionIncrementGap(fieldInfo.name);

  if (!field->isTokenized()) {     // un-tokenized field
    String stringValue = field->stringValue();
    final int32_t valueLength = stringValue.length();
    Token token = localToken;
    token.clear();
    char[] termBuffer = token.termBuffer();
    if (termBuffer.length < valueLength)
      termBuffer = token.resizeTermBuffer(valueLength);
    stringValue.getChars(0, valueLength, termBuffer, 0);
    token.setTermLength(valueLength);
    token.setStartOffset(offset);
    token.setEndOffset(offset + stringValue.length());
    addPosition(token);
    offset += stringValue.length();
    length++;
  } else {                                  // tokenized field
    final TokenStream stream;
    final TokenStream streamValue = field->tokenStreamValue();

    if (streamValue != NULL)
      stream = streamValue;
    else {
      // the field does not have a TokenStream,
      // so we have to obtain one from the analyzer
      final Reader reader;        // find or make Reader
      final Reader readerValue = field->readerValue();

      if (readerValue != NULL)
        reader = readerValue;
      else {
        String stringValue = field->stringValue();
        if (stringValue == NULL)
          _CLTHROWA(CL_ERR_IllegalArgument, "field must have either TokenStream, String or Reader value");
        stringReader.init(stringValue);
        reader = stringReader;
      }

      // Tokenize field and add to postingTable
      stream = analyzer.reusableTokenStream(fieldInfo.name, reader);
    }

    // reset the TokenStream to the first token
    stream.reset();

    try {
      offsetEnd = offset-1;
      for(;;) {
        Token token = stream.next(localToken);
        if (token == NULL) break;
        position += (token.getPositionIncrement() - 1);
        addPosition(token);
        if (++length >= maxFieldLength) {
          if (infoStream != NULL)
            infoStream.println("maxFieldLength " +maxFieldLength+ " reached for field " + fieldInfo.name + ", ignoring following tokens");
          break;
        }
      }
      offset = offsetEnd+1;
    } finally {
      stream.close();
    }
  }

  boost *= field->getBoost();
}

PostingVector DocumentsWriter::ThreadState::FieldData::iaddNewVector() {

  if (postingsVectorsUpto == postingsVectors.length) {
    final int32_t newSize;
    if (postingsVectors.length < 2)
      newSize = 2;
    else
      newSize = (int32_t) (1.5*postingsVectors.length);
    PostingVector[] newArray = new PostingVector[newSize];
    System.arraycopy(postingsVectors, 0, newArray, 0, postingsVectors.length);
    postingsVectors = newArray;
  }

  p.vector = postingsVectors[postingsVectorsUpto];
  if (p.vector == NULL)
    p.vector = postingsVectors[postingsVectorsUpto] = new PostingVector();

  postingsVectorsUpto++;

  final PostingVector v = p.vector;
  v.p = p;

  final int32_t firstSize = levelSizeArray[0];

  if (doVectorPositions) {
    final int32_t upto = vectorsPool->newSlice(firstSize);
    v.posStart = v.posUpto = vectorsPool->uint8_tOffset + upto;
  }

  if (doVectorOffsets) {
    final int32_t upto = vectorsPool->newSlice(firstSize);
    v.offsetStart = v.offsetUpto = vectorsPool->uint8_tOffset + upto;
  }

  return v;
}

void DocumentsWriter::ThreadState::FieldData::iaddPosition(Token token) {

  final Payload payload = token.getPayload();

  // Get the text of this term.  Term can either
  // provide a String token or offset into a char[]
  // array
  final char[] tokenText = token.termBuffer();
  final int32_t tokenTextLen = token.termLength();

  int32_t code = 0;

  // Compute hashcode
  int32_t downto = tokenTextLen;
  while (downto > 0)
    code = (code*31) + tokenText[--downto];

  // System.out.println("  addPosition: buffer=" + new String(tokenText, 0, tokenTextLen) + " pos=" + position + " offsetStart=" + (offset+token.startOffset()) + " offsetEnd=" + (offset + token.endOffset()) + " docID=" + docID + " doPos=" + doVectorPositions + " doOffset=" + doVectorOffsets);

  int32_t hashPos = code & postingsHashMask;

  assert !postingsCompacted;

  // Locate Posting in hash
  p = postingsHash[hashPos];

  if (p != NULL && !postingEquals(tokenText, tokenTextLen)) {
    // Conflict: keep searching different locations in
    // the hash table.
    final int32_t inc = ((code>>8)+code)|1;
    do {
      code += inc;
      hashPos = code & postingsHashMask;
      p = postingsHash[hashPos];
    } while (p != NULL && !postingEquals(tokenText, tokenTextLen));
  }

  final int32_t proxCode;

  // If we hit an exception below, it's possible the
  // posting list or term vectors data will be
  // partially written and thus inconsistent if
  // flushed, so we have to abort all documents
  // since the last flush:

  try {

    if (p != NULL) {       // term seen since last flush

      if (docID != p.lastDocID) { // term not yet seen in this doc

        // System.out.println("    seen before (new docID=" + docID + ") freqUpto=" + p.freqUpto +" proxUpto=" + p.proxUpto);

        assert p.docFreq > 0;

        // Now that we know doc freq for previous doc,
        // write it & lastDocCode
        freqUpto = p.freqUpto & BYTE_BLOCK_MASK;
        freq = postingsPool.buffers[p.freqUpto >> BYTE_BLOCK_SHIFT];
        if (1 == p.docFreq)
          writeFreqVInt(p.lastDocCode|1);
        else {
          writeFreqVInt(p.lastDocCode);
          writeFreqVInt(p.docFreq);
        }
        p.freqUpto = freqUpto + (p.freqUpto & BYTE_BLOCK_NOT_MASK);

        if (doVectors) {
          vector = addNewVector();
          if (doVectorOffsets) {
            offsetStartCode = offsetStart = offset + token.startOffset();
            offsetEnd = offset + token.endOffset();
          }
        }

        proxCode = position;

        p.docFreq = 1;

        // Store code so we can write this after we're
        // done with this new doc
        p.lastDocCode = (docID-p.lastDocID) << 1;
        p.lastDocID = docID;

      } else {                                // term already seen in this doc
        // System.out.println("    seen before (same docID=" + docID + ") proxUpto=" + p.proxUpto);
        p.docFreq++;

        proxCode = position-p.lastPosition;

        if (doVectors) {
          vector = p.vector;
          if (vector == NULL)
            vector = addNewVector();
          if (doVectorOffsets) {
            offsetStart = offset + token.startOffset();
            offsetEnd = offset + token.endOffset();
            offsetStartCode = offsetStart-vector.lastOffset;
          }
        }
      }
    } else {            // term not seen before
      // System.out.println("    never seen docID=" + docID);

      // Refill?
      if (0 == postingsFreeCount) {
        getPostings(postingsFreeList);
        postingsFreeCount = postingsFreeList.length;
      }

      final int32_t textLen1 = 1+tokenTextLen;
      if (textLen1 + charPool.uint8_tUpto > CHAR_BLOCK_SIZE) {
        if (textLen1 > CHAR_BLOCK_SIZE) {
          // Just skip this term, to remain as robust as
          // possible during indexing.  A TokenFilter
          // can be inserted into the analyzer chain if
          // other behavior is wanted (pruning the term
          // to a prefix, throwing an exception, etc).
          if (maxTermPrefix == NULL)
            maxTermPrefix = new String(tokenText, 0, 30);

          // Still increment position:
          position++;
          return;
        }
        charPool.nextBuffer();
      }
      final char[] text = charPool.buffer;
      final int32_t textUpto = charPool.uint8_tUpto;

      // Pull next free Posting from free list
      p = postingsFreeList[--postingsFreeCount];

      p.textStart = textUpto + charPool.uint8_tOffset;
      charPool.uint8_tUpto += textLen1;

      System.arraycopy(tokenText, 0, text, textUpto, tokenTextLen);

      text[textUpto+tokenTextLen] = 0xffff;

      assert postingsHash[hashPos] == NULL;

      postingsHash[hashPos] = p;
      numPostings++;

      if (numPostings == postingsHashHalfSize)
        rehashPostings(2*postingsHashSize);

      // Init first slice for freq & prox streams
      final int32_t firstSize = levelSizeArray[0];

      final int32_t upto1 = postingsPool.newSlice(firstSize);
      p.freqStart = p.freqUpto = postingsPool.uint8_tOffset + upto1;

      final int32_t upto2 = postingsPool.newSlice(firstSize);
      p.proxStart = p.proxUpto = postingsPool.uint8_tOffset + upto2;

      p.lastDocCode = docID << 1;
      p.lastDocID = docID;
      p.docFreq = 1;

      if (doVectors) {
        vector = addNewVector();
        if (doVectorOffsets) {
          offsetStart = offsetStartCode = offset + token.startOffset();
          offsetEnd = offset + token.endOffset();
        }
      }

      proxCode = position;
    }

    proxUpto = p.proxUpto & BYTE_BLOCK_MASK;
    prox = postingsPool.buffers[p.proxUpto >> BYTE_BLOCK_SHIFT];
    assert prox != NULL;

    if (payload != NULL && payload.length > 0) {
      writeProxVInt((proxCode<<1)|1);
      writeProxVInt(payload.length);
      writeProxBytes(payload.data, payload.offset, payload.length);
      fieldInfo.storePayloads = true;
    } else
      writeProxVInt(proxCode<<1);

    p.proxUpto = proxUpto + (p.proxUpto & BYTE_BLOCK_NOT_MASK);

    p.lastPosition = position++;

    if (doVectorPositions) {
      posUpto = vector.posUpto & BYTE_BLOCK_MASK;
      pos = vectorsPool->buffers[vector.posUpto >> BYTE_BLOCK_SHIFT];
      writePosVInt(proxCode);
      vector.posUpto = posUpto + (vector.posUpto & BYTE_BLOCK_NOT_MASK);
    }

    if (doVectorOffsets) {
      offsetUpto = vector.offsetUpto & BYTE_BLOCK_MASK;
      offsets = vectorsPool->buffers[vector.offsetUpto >> BYTE_BLOCK_SHIFT];
      writeOffsetVInt(offsetStartCode);
      writeOffsetVInt(offsetEnd-offsetStart);
      vector.lastOffset = offsetEnd;
      vector.offsetUpto = offsetUpto + (vector.offsetUpto & BYTE_BLOCK_NOT_MASK);
    }
  } catch (CLuceneError& t) {
    throw new AbortException(t, DocumentsWriter.this);
  }
}

void DocumentsWriter::ThreadState::FieldData::rehashPostings(final int32_t newSize) {

  final int32_t newMask = newSize-1;

  Posting[] newHash = new Posting[newSize];
  for(int32_t i=0;i<postingsHashSize;i++) {
    Posting p0 = postingsHash[i];
    if (p0 != NULL) {
      final int32_t start = p0.textStart & CHAR_BLOCK_MASK;
      final char[] text = charPool.buffers[p0.textStart >> CHAR_BLOCK_SHIFT];
      int32_t pos = start;
      while(text[pos] != 0xffff)
        pos++;
      int32_t code = 0;
      while (pos > start)
        code = (code*31) + text[--pos];

      int32_t hashPos = code & newMask;
      assert hashPos >= 0;
      if (newHash[hashPos] != NULL) {
        final int32_t inc = ((code>>8)+code)|1;
        do {
          code += inc;
          hashPos = code & newMask;
        } while (newHash[hashPos] != NULL);
      }
      newHash[hashPos] = p0;
    }
  }

  postingsHashMask =  newMask;
  postingsHash = newHash;
  postingsHashSize = newSize;
  postingsHashHalfSize = newSize >> 1;
}

void DocumentsWriter::ThreadState::FieldData::writeVectors(FieldInfo fieldInfo) {

  assert fieldInfo.storeTermVector;

  vectorFieldNumbers[numVectorFields] = fieldInfo.number;
  vectorFieldPointers[numVectorFields] = tvfLocal->getFilePointer();
  numVectorFields++;

  final int32_t numPostingsVectors = postingsVectorsUpto;

  tvfLocal->writeVInt(numPostingsVectors);
  uint8_t bits = 0x0;
  if (doVectorPositions)
    bits |= TermVectorsReader.STORE_POSITIONS_WITH_TERMVECTOR;
  if (doVectorOffsets)
    bits |= TermVectorsReader.STORE_OFFSET_WITH_TERMVECTOR;
  tvfLocal->writeByte(bits);

  doVectorSort(postingsVectors, numPostingsVectors);

  Posting lastPosting = NULL;

  final ByteSliceReader reader = vectorSliceReader;

  for(int32_t j=0;j<numPostingsVectors;j++) {
    PostingVector vector = postingsVectors[j];
    Posting posting = vector.p;
    final int32_t freq = posting.docFreq;

    final int32_t prefix;
    final char[] text2 = charPool.buffers[posting.textStart >> CHAR_BLOCK_SHIFT];
    final int32_t start2 = posting.textStart & CHAR_BLOCK_MASK;
    int32_t pos2 = start2;

    // Compute common prefix between last term and
    // this term
    if (lastPosting == NULL)
      prefix = 0;
    else {
      final char[] text1 = charPool.buffers[lastPosting.textStart >> CHAR_BLOCK_SHIFT];
      final int32_t start1 = lastPosting.textStart & CHAR_BLOCK_MASK;
      int32_t pos1 = start1;
      while(true) {
        final char c1 = text1[pos1];
        final char c2 = text2[pos2];
        if (c1 != c2 || c1 == 0xffff) {
          prefix = pos1-start1;
          break;
        }
        pos1++;
        pos2++;
      }
    }
    lastPosting = posting;

    // Compute length
    while(text2[pos2] != 0xffff)
      pos2++;

    final int32_t suffix = pos2 - start2 - prefix;
    tvfLocal->writeVInt(prefix);
    tvfLocal->writeVInt(suffix);
    tvfLocal->writeChars(text2, start2 + prefix, suffix);
    tvfLocal->writeVInt(freq);

    if (doVectorPositions) {
      reader.init(vectorsPool, vector.posStart, vector.posUpto);
      reader.writeTo(tvfLocal);
    }

    if (doVectorOffsets) {
      reader.init(vectorsPool, vector.offsetStart, vector.offsetUpto);
      reader.writeTo(tvfLocal);
    }
  }
}
*/
CL_NS_END
