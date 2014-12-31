#include "Timer.hxx"

#include <assert.h>
#include <errno.h>
#include <time.h>
#
#include "Error.hxx"
#include "Log.hxx"
#include "TimerItem.hxx"

namespace Tara {

namespace {

uint64_t GetTime(void);

int heap_compare(const heap_node* a, const heap_node* b);

} // namespace

Timer::Timer()
{
  heap_init(&itemHeap_);
}

void Timer::addItem(TimerItem *item, int duration)
{
  assert(item != nullptr);
  item->dueTime = duration >= 0 ? GetTime() + duration : UINT64_MAX;
  heap_insert(&itemHeap_, &item->heapNode, heap_compare);
}

void Timer::removeItem(TimerItem *item)
{
  assert(item != nullptr);
  heap_remove(&itemHeap_, &item->heapNode, heap_compare);
}

unsigned int Timer::removeDueItems(TimerItem **buffer,
                                   unsigned int bufferLength)
{
  if (bufferLength == 0) {
    return 0;
  }
  assert(buffer != nullptr);
  heap_node *itemHeapNode = heap_min(&itemHeap_);
  if (itemHeapNode == nullptr) {
    return 0;
  }
  uint64_t now = GetTime();
  int i = 0;
  for (;;) {
    auto item = TARA_CONTAINER_OF(itemHeapNode, TimerItem, heapNode);
    if (item->dueTime > now) {
      break;
    }
    heap_remove(&itemHeap_, &item->heapNode, heap_compare);
    buffer[i++] = item;
    if (i == bufferLength) {
      break;
    }
    itemHeapNode = heap_min(&itemHeap_);
    if (itemHeapNode == nullptr) {
      break;
    }
  }
  return i;
}

int Timer::calculateTimeout()
{
  heap_node *itemHeapNode = heap_min(&itemHeap_);
  if (itemHeapNode == nullptr) {
    return -1;
  }
  auto item = TARA_CONTAINER_OF(itemHeapNode, TimerItem, heapNode);
  if (item->dueTime == UINT64_MAX) {
    return -1;
  }
  uint64_t now = GetTime();
  return item->dueTime > now ? item->dueTime - now : 0;
}

namespace {

uint64_t GetTime(void)
{
  timespec time;
  if (clock_gettime(CLOCK_MONOTONIC_COARSE, &time) < 0) {
    TARA_FATALITY_LOG("clock_gettime failed: ", Error(errno));
  }
  return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

int heap_compare(const heap_node* a, const heap_node* b)
{
  assert(a != nullptr);
  assert(b != nullptr);
  return TARA_CONTAINER_OF(a, const TimerItem, heapNode)->dueTime <
         TARA_CONTAINER_OF(b, const TimerItem, heapNode)->dueTime;
}

} // namespace

} // namespace Tara
