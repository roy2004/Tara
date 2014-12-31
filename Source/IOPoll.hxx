#pragma once

#include <vector>
#
#include "libuv/queue.h"
#
#include "MemoryPool.hxx"
#include "Utility.hxx"

namespace Tara {

enum class IOEvent;
struct IOWatcher;

class IOPoll final
{
  TARA_DISALLOW_COPY(IOPoll);

public:
  IOPoll();
  ~IOPoll();

  bool watcherExists(int fd) const
  { return fd >= 0 && fd < watchers_.size() && watchers_[fd] != nullptr; }

  void createWatcher(int fd);
  void destroyWatcher(int fd);
  void addEventAwaiter(QUEUE *eventAwaiterQueueItem, int fd, IOEvent event);
  void removeEventAwaiter(const QUEUE &eventAwaiterQueueItem, int fd);
  void removeEventAwaiters(int fd, QUEUE *eventAwaiterQueue);
  bool waitForEvents(int timeout, QUEUE *eventAwaiterQueue);

private:
  const int fd_;
  MemoryPool watcherMemoryPool_;
  std::vector<IOWatcher *> watchers_;
  QUEUE dirtyWatcherQueue_;
};

} // namespace Tara
