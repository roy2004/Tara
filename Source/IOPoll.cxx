#include "IOPoll.hxx"

#include <sys/epoll.h>
#include <unistd.h>
#
#include <assert.h>
#include <errno.h>
#
#include "Error.hxx"
#include "IOEvent.hxx"
#include "IOWatcher.hxx"
#include "Log.hxx"

namespace Tara {

namespace {

const uint32_t IOEventFlags[] = {
  [static_cast<int>(IOEvent::Readability)] = EPOLLIN,
  [static_cast<int>(IOEvent::Writability)] = EPOLLOUT
};

} // namespace

IOPoll::IOPoll()
  : fd_(epoll_create1(0)), watcherMemoryPool_(65536, sizeof(IOWatcher))
{
  if (fd_ < 0) {
    TARA_FATALITY_LOG("epoll_create1 failed: ", Error(errno));
  }
  QUEUE_INIT(&dirtyWatcherQueue_);
}

IOPoll::~IOPoll()
{
  int result;
  do {
    result = close(fd_);
    if (result >= 0) {
      break;
    }
  } while (errno == EINTR);
  if (result < 0) {
    TARA_ERROR_LOG("close failed: ", Error(errno));
  }
}

void IOPoll::createWatcher(int fd)
{
  assert(fd >= 0);
  if (fd < watchers_.size()) {
    if (watchers_[fd] != nullptr) {
      IOWatcher *watcher = watchers_[fd];
      ++watcher->refCount;
      return;
    }
  } else {
    watchers_.resize(fd + 1, nullptr);
  }
  auto watcher = new (watcherMemoryPool_.allocateBlock()) IOWatcher(fd);
  QUEUE_INIT(&watcher->queueItem);
  watchers_[fd] = watcher;
}

void IOPoll::destroyWatcher(int fd)
{
  assert(watcherExists(fd));
  IOWatcher *watcher = watchers_[fd];
  if (watcher->refCount != 0) {
    --watcher->refCount;
    return;
  }
  watchers_[fd] = nullptr;
  if (!QUEUE_EMPTY(&watcher->queueItem)) {
    QUEUE_REMOVE(&watcher->queueItem);
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
      if (epoll_ctl(fd_, op, watcher->fd, &event) < 0) {
        TARA_FATALITY_LOG("epoll_ctl failed: ", Error(errno));
      }
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

} // namespace Tara
