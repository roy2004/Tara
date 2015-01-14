#pragma once

#include <stddef.h>

namespace Tara {

union MemoryBlock;
union MemoryChunk;

class MemoryPool final
{
  MemoryPool(const MemoryPool &other) = delete;
  void operator=(const MemoryPool &other) = delete;

public:
  MemoryPool(size_t blockSize, unsigned int chunkLength);
  ~MemoryPool();

  void *allocateBlock();
  void freeBlock(void *opaqueBlock);

private:
  const size_t blockSize_;
  const size_t chunkSize_;
  MemoryBlock *lastBlock_;
  MemoryChunk **chunkVector_;
  unsigned int chunkVectorLength_;
  unsigned int chunkCount_;

  void increaseBlocks();
  void increaseChunks();
  void expandChunkVector();
};

} // namespace Tara
