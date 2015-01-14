#include "MemoryPool.hxx"

#include <unistd.h>
#
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#
#include "Error.hxx"
#include "Log.hxx"
#include "Utility.hxx"

namespace Tara {

namespace {

size_t NextPageAlignedSize(size_t size);

long xsysconf(int name);

} // namespace

union MemoryBlock
{
  char base[];
  struct {
    MemoryBlock *prev;
  };
};

union MemoryChunk
{
  char base[];
};

MemoryPool::MemoryPool(size_t blockSize, unsigned int chunkLength)
  : blockSize_(blockSize),
    chunkSize_(NextPageAlignedSize(chunkLength * blockSize)),
    lastBlock_(nullptr), chunkVector_(nullptr), chunkVectorLength_(0),
    chunkCount_(0)
{
  assert(blockSize_ != 0);
  assert(chunkSize_ != 0);
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
    TARA_FATALITY_LOG("malloc failed");
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
    TARA_FATALITY_LOG("realloc failed");
  }
}

namespace {

size_t NextPageAlignedSize(size_t size)
{
  --size;
  size |= xsysconf(_SC_PAGE_SIZE) - 1;
  ++size;
  return size;
}

long xsysconf(int name)
{
  long result = sysconf(name);
  if (result < 0) {
    TARA_FATALITY_LOG("sysconf failed: ", Error(errno));
  }
  return result;
}

} // namespace

} // namespace Tara
