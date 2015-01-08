#include "IOPoll.hxx"

#include <sys/epoll.h>
#include <unistd.h>
#
#include <assert.h>
#include <errno.h>
#
#include "Error.hxx"
#include "IOEvent.hxx"
#include "Log.hxx"

namespace Tara {

struct IOWatcher final
{
  QUEUE queueItem;
  const int fd;
  uint32_t eventFlags;
  uint32_t pendingEventFlags;
  QUEUE eventAwaiterQueues[2];

  explicit IOWatcher(int fd);
};

namespace {

const uint32_t IOEventFlags[] = {
  [static_cast<int>(IOEvent::Readability)] = EPOLLIN,
  [static_cast<int>(IOEvent::Writability)] = EPOLLOUT
};

uint32_t NextPowerOfTwo(uint32_t value);

int xepoll_create1(int flags);
void xepoll_ctl(int epfd, int op, int fd, epoll_event *event);
void xclose(int fd);

} // namespace

IOPoll::IOPoll()
  : fd_(xepoll_create1(0)), watcherMemoryPool_(65536, sizeof(IOWatcher))
{
  QUEUE_INIT(&dirtyWatcherQueue_);
}

IOPoll::~IOPoll()
{
  xclose(fd_);
}

void IOPoll::createWatcher(int fd)
{
  assert(fd >= 0);
  if (fd >= watchers_.size()) {
    watchers_.resize(NextPowerOfTwo(fd + 1), nullptr);
  }
  assert(watchers_[fd] == nullptr);
  auto watcher = new (watcherMemoryPool_.allocateBlock()) IOWatcher(fd);
  QUEUE_INIT(&watcher->queueItem);
  watchers_[fd] = watcher;
}

void IOPoll::destroyWatcher(int fd)
{
  assert(watcherExists(fd));
  IOWatcher *watcher = watchers_[fd];
  watchers_[fd] = nullptr;
  if (!QUEUE_EMPTY(&watcher->queueItem)) {
    QUEUE_REMOVE(&watcher->queueItem);
  }
  if (watcher->eventFlags != 0) {
    xepoll_ctl(fd_, EPOLL_CTL_DEL, watcher->fd, nullptr);
  }
  watcher->~IOWatcher();
  watcherMemoryPool_.freeBlock(watcher);
}

void IOPoll::addEventAwaiter(QUEUE *eventAwaiterQueueItem, int fd,
                             IOEvent event)
{
  assert(eventAwaiterQueueItem != nullptr);
  assert(watcherExists(fd));
  IOWatcher *watcher = watchers_[fd];
  QUEUE *eventAwaiterQueue = &watcher->eventAwaiterQueues
                                       [static_cast<int>(event)];
  QUEUE_INSERT_TAIL(eventAwaiterQueue, eventAwaiterQueueItem);
  uint32_t eventFlag = IOEventFlags[static_cast<int>(event)];
  if ((watcher->pendingEventFlags & eventFlag) == 0) {
    watcher->pendingEventFlags |= eventFlag;
    if (QUEUE_EMPTY(&watcher->queueItem)) {
      QUEUE_INSERT_TAIL(&dirtyWatcherQueue_, &watcher->queueItem);
    }
  }
}

void IOPoll::removeEventAwaiter(const QUEUE &eventAwaiterQueueItem, int fd)
{
  assert(watcherExists(fd));
  IOWatcher *watcher = watchers_[fd];
  QUEUE_REMOVE(&eventAwaiterQueueItem);
  if (QUEUE_NEXT(&eventAwaiterQueueItem) ==
      QUEUE_PREV(&eventAwaiterQueueItem)) {
    uint32_t eventFlag = IOEventFlags[QUEUE_NEXT(&eventAwaiterQueueItem) -
                                      watcher->eventAwaiterQueues];
    watcher->pendingEventFlags &= ~eventFlag;
    if (QUEUE_EMPTY(&watcher->queueItem)) {
      QUEUE_INSERT_TAIL(&dirtyWatcherQueue_, &watcher->queueItem);
    }
  }
}

void IOPoll::removeEventAwaiters(int fd, QUEUE *eventAwaiterQueue)
{
  assert(watcherExists(fd));
  assert(eventAwaiterQueue != nullptr);
  IOWatcher *watcher = watchers_[fd];
  if (watcher->pendingEventFlags == 0) {
    return;
  }
  if (!QUEUE_EMPTY(&watcher->eventAwaiterQueues[0])) {
    QUEUE_ADD(eventAwaiterQueue, &watcher->eventAwaiterQueues[0]);
    QUEUE_INIT(&watcher->eventAwaiterQueues[0]);
  }
  if (!QUEUE_EMPTY(&watcher->eventAwaiterQueues[1])) {
    QUEUE_ADD(eventAwaiterQueue, &watcher->eventAwaiterQueues[1]);
    QUEUE_INIT(&watcher->eventAwaiterQueues[1]);
  }
  watcher->pendingEventFlags = 0;
  if (QUEUE_EMPTY(&watcher->queueItem)) {
    QUEUE_INSERT_TAIL(&dirtyWatcherQueue_, &watcher->queueItem);
  }
}

bool IOPoll::waitForEvents(int timeout, QUEUE *eventAwaiterQueue)
{
  assert(eventAwaiterQueue != nullptr);
  if (!QUEUE_EMPTY(&dirtyWatcherQueue_)) {
    QUEUE *q = QUEUE_HEAD(&dirtyWatcherQueue_);
    do {
      auto watcher = QUEUE_DATA(q, IOWatcher, queueItem);
      q = QUEUE_NEXT(q);
      QUEUE_INIT(&watcher->queueItem);
      if (watcher->eventFlags == watcher->pendingEventFlags) {
        continue;
      }
      int op;
      if (watcher->eventFlags == 0) {
        op = EPOLL_CTL_ADD;
      } else {
        if (watcher->pendingEventFlags == 0) {
          op = EPOLL_CTL_DEL;
        } else {
          op = EPOLL_CTL_MOD;
        }
      }
      epoll_event event;
      event.events = watcher->pendingEventFlags;
      event.data.ptr = watcher;
      xepoll_ctl(fd_, op, watcher->fd, &event);
      watcher->eventFlags = watcher->pendingEventFlags;
    } while (q != &dirtyWatcherQueue_);
    QUEUE_INIT(&dirtyWatcherQueue_);
  }
  epoll_event events[1024];
  int n = epoll_wait(fd_, events, TARA_LENGTH_OF(events), timeout);
  if (n < 0) {
    if (errno == EINTR) {
      return false;
    }
    TARA_FATALITY_LOG("epoll_wait failed: ", Error(errno));
  }
  for (int i = 0; i < n; ++i) {
    const epoll_event &event = events[i];
    auto watcher = static_cast<IOWatcher *>(event.data.ptr);
    if ((event.events & (EPOLLERR | EPOLLHUP)) != 0) {
      removeEventAwaiters(watcher->fd, eventAwaiterQueue);
      continue;
    }
    if ((event.events & IOEventFlags[0]) != 0) {
      QUEUE *q = QUEUE_HEAD(&watcher->eventAwaiterQueues[0]);
      removeEventAwaiter(*q, watcher->fd);
      QUEUE_INSERT_TAIL(eventAwaiterQueue, q);
    }
    if ((event.events & IOEventFlags[1]) != 0) {
      QUEUE *q = QUEUE_HEAD(&watcher->eventAwaiterQueues[1]);
      removeEventAwaiter(*q, watcher->fd);
      QUEUE_INSERT_TAIL(eventAwaiterQueue, q);
    }
  }
  return true;
}

IOWatcher::IOWatcher(int fd)
  : fd(fd), eventFlags(0), pendingEventFlags(0)
{
  QUEUE_INIT(&this->eventAwaiterQueues[0]);
  QUEUE_INIT(&this->eventAwaiterQueues[1]);
}

namespace {

uint32_t NextPowerOfTwo(uint32_t value)
{
  --value;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  ++value;
  return value;
}

int xepoll_create1(int flags)
{
  int fd = epoll_create1(flags);
  if (fd < 0) {
    TARA_FATALITY_LOG("epoll_create1 failed: ", Error(errno));
  }
  return fd;
}

void xepoll_ctl(int epfd, int op, int fd, epoll_event *event)
{
  if (epoll_ctl(epfd, op, fd, event) < 0) {
    TARA_FATALITY_LOG("epoll_ctl failed: ", Error(errno));
  }
}

void xclose(int fd)
{
  int result;
  do {
    result = close(fd);
    if (result >= 0) {
      break;
    }
  } while (errno == EINTR);
  if (result < 0) {
    TARA_FATALITY_LOG("close failed: ", Error(errno));
  }
}

} // namespace

} // namespace Tara
