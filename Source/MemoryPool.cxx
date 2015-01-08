#include "MemoryPool.hxx"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#
#include "Error.hxx"
#include "Log.hxx"

namespace Tara {

union MemoryChunk
{
  char base[];
};

union MemoryBlock
{
  char base[];
  struct {
    MemoryBlock *prev;
  };
};

MemoryPool::MemoryPool(size_t chunkSize, size_t blockSize)
  : chunkSize_(chunkSize), blockSize_(blockSize), chunkVector_(nullptr),
    chunkVectorLength_(0), chunkCount_(0), lastBlock_(nullptr)
{
  assert(chunkSize_ != 0);
  assert(blockSize_ != 0);
  assert(chunkSize_ >= blockSize_);
}

MemoryPool::~MemoryPool()
{
  for (int i = chunkCount_ - 1; i >= 0; --i) {
    free(chunkVector_[i]);
  }
  free(chunkVector_);
}

void *MemoryPool::allocateBlock()
{
  if (lastBlock_ == nullptr) {
    increaseBlocks();
  }
  MemoryBlock *block = lastBlock_;
  lastBlock_ = block->prev;
  return block;
}

void MemoryPool::freeBlock(void *opaqueBlock)
{
  assert(opaqueBlock != nullptr);
  auto block = static_cast<MemoryBlock *>(opaqueBlock);
  block->prev = lastBlock_;
  lastBlock_ = block;
}

void MemoryPool::increaseBlocks()
{
  increaseChunks();
  MemoryChunk *chunk = chunkVector_[chunkCount_ - 1];
  auto block = TARA_CONTAINER_OF(chunk->base + chunkSize_ - blockSize_,
                                 MemoryBlock, base);
  lastBlock_ = block;
  for (;;) {
    auto blockPrev = TARA_CONTAINER_OF(block->base - blockSize_, MemoryBlock,
                                       base);
    if (blockPrev->base < chunk->base) {
      break;
    }
    block->prev = blockPrev;
    block = blockPrev;
  }
  block->prev = nullptr;
}

void MemoryPool::increaseChunks()
{
  if (chunkCount_ == chunkVectorLength_) {
    expandChunkVector();
  }
  auto chunk = static_cast<MemoryChunk *>(malloc(chunkSize_));
  if (chunk == nullptr) {
    TARA_FATALITY_LOG("malloc failed: ", Error(errno));
  }
  chunkVector_[chunkCount_++] = chunk;
}

void MemoryPool::expandChunkVector()
{
  chunkVectorLength_ = chunkVectorLength_ == 0 ? 1 : 2 * chunkVectorLength_;
  chunkVector_ = static_cast<MemoryChunk **>
                 (realloc(chunkVector_,
                          chunkVectorLength_ * sizeof *chunkVector_));
  if (chunkVector_ == nullptr) {
    TARA_FATALITY_LOG("realloc failed: ", Error(errno));
  }
}

} // namespace Tara
