#pragma once

#include <stdint.h>
#
#include "libuv/queue.h"

namespace Tara {

struct IOWatcher final
{
  QUEUE queueItem;
  const int fd;
  uint32_t eventFlags;
  uint32_t pendingEventFlags;
  QUEUE eventAwaiterQueues[2];
  unsigned int refCount;

  explicit IOWatcher(int fd);
};

} // namespace Tara
