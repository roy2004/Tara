#pragma once

#include <stddef.h>
#
#include "Utility.hxx"

namespace Tara {

union MemoryChunk;
union MemoryBlock;

class MemoryPool final
{
  TARA_DISALLOW_COPY(MemoryPool);

public:
  MemoryPool(size_t chunkSize, size_t blockSize);
  ~MemoryPool();

  void *allocateBlock();
  void freeBlock(void *opaqueBlock);

private:
  const size_t chunkSize_;
  const size_t blockSize_;
  MemoryChunk **chunkVector_;
  unsigned int chunkVectorLength_;
  unsigned int chunkCount_;
  MemoryBlock *lastBlock_;

  void increaseBlocks();
  void increaseChunks();
  void expandChunkVector();
};

} // namespace Tara
