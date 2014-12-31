#include "IOWatcher.hxx"

namespace Tara {

IOWatcher::IOWatcher(int fd)
  : fd(fd), eventFlags(0), pendingEventFlags(0)
{
  QUEUE_INIT(&this->eventAwaiterQueues[0]);
  QUEUE_INIT(&this->eventAwaiterQueues[1]);
}

} // namespace Tara
