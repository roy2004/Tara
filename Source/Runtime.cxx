#include "Runtime.hxx"

#include <fcntl.h>
#include <sys/eventfd.h>
#include <unistd.h>
#
#include <errno.h>
#
#include <utility>
#
#include "IOEvent.hxx"
#include "Log.hxx"
#include "Scheduler.hxx"

#define CHECK_THE_SCHEDULER              \
  do {                                   \
    if (TheScheduler == nullptr) {       \
      TARA_FATALITY_LOG("No scheduler"); \
    }                                    \
  } while (false)

namespace Tara {

extern thread_local Scheduler *const TheScheduler;

void Call(const Coroutine &coroutine)
{
  CHECK_THE_SCHEDULER;
  if (coroutine != nullptr) {
    TheScheduler->callCoroutine(coroutine);
  }
}

void Call(Coroutine &&coroutine)
{
  CHECK_THE_SCHEDULER;
  if (coroutine != nullptr) {
    TheScheduler->callCoroutine(std::move(coroutine));
  }
}

void Yield()
{
  CHECK_THE_SCHEDULER;
  TheScheduler->yieldCurrentFiber();
}

void Sleep(int duration)
{
  CHECK_THE_SCHEDULER;
  TheScheduler->sleepCurrentFiber(duration);
}

void Exit()
{
  CHECK_THE_SCHEDULER;
  TheScheduler->exitCurrentFiber();
}

int Open(const char *path, int flags, mode_t mode)
{
  CHECK_THE_SCHEDULER;
  int fd;
  do {
    fd = open(path, flags | O_NONBLOCK, mode);
    if (fd >= 0) {
      break;
    }
  } while (errno == EINTR);
  if (fd < 0) {
    return -1;
  }
  TheScheduler->watchIO(fd);
  return fd;
}

int Pipe2(int *fds, int flags)
{
  CHECK_THE_SCHEDULER;
  if (pipe2(fds, flags | O_NONBLOCK) < 0) {
    return -1;
  }
  TheScheduler->watchIO(fds[0]);
  TheScheduler->watchIO(fds[1]);
  return 0;
}

int Socket(int domain, int type, int protocol)
{
  CHECK_THE_SCHEDULER;
  int fd = socket(domain, type | SOCK_NONBLOCK, protocol);
  if (fd < 0) {
    return -1;
  }
  TheScheduler->watchIO(fd);
  return fd;
}

int Close(int fd)
{
  CHECK_THE_SCHEDULER;
  if (!TheScheduler->ioIsWatched(fd)) {
    errno = EBADF;
    return -1;
  }
  int result;
  do {
    result = close(fd);
    if (result >= 0) {
      break;
    }
  } while (errno == EINTR);
  if (result < 0) {
    TheScheduler->unwatchIO(fd);
    return -1;
  }
  TheScheduler->unwatchIO(fd);
  return 0;
}

ssize_t Read(int fd, void *buf, size_t buflen, int timeout)
{
  CHECK_THE_SCHEDULER;
  if (!TheScheduler->ioIsWatched(fd)) {
    errno = EBADF;
    return -1;
  }
  ssize_t n;
  for (;;) {
    n = read(fd, buf, buflen);
    if (n >= 0) {
      break;
    }
    if (errno == EWOULDBLOCK) {
      if (TheScheduler->awaitIOEvent(fd, IOEvent::Readability, timeout) < 0) {
        break;
      }
      continue;
    }
    if (errno == EINTR) {
      continue;
    }
    break;
  }
  if (n < 0) {
    return -1;
  }
  return n;
}

ssize_t Write(int fd, const void *buf, size_t buflen, int timeout)
{
  CHECK_THE_SCHEDULER;
  if (!TheScheduler->ioIsWatched(fd)) {
    errno = EBADF;
    return -1;
  }
  ssize_t n;
  for (;;) {
    n = write(fd, buf, buflen);
    if (n >= 0) {
      break;
    }
    if (errno == EWOULDBLOCK) {
      if (TheScheduler->awaitIOEvent(fd, IOEvent::Writability, timeout) < 0) {
        break;
      }
      continue;
    }
    if (errno == EINTR) {
      continue;
    }
    break;
  }
  if (n < 0) {
    return -1;
  }
  return n;
}

int Accept4(int fd, sockaddr *addr, socklen_t *addrlen, int flags, int timeout)
{
  CHECK_THE_SCHEDULER;
  if (!TheScheduler->ioIsWatched(fd)) {
    errno = EBADF;
    return -1;
  }
  int subfd;
  for (;;) {
    subfd = accept4(fd, addr, addrlen, flags);
    if (subfd >= 0) {
      break;
    }
    if (errno == EWOULDBLOCK) {
      if (TheScheduler->awaitIOEvent(fd, IOEvent::Readability, timeout) < 0) {
        break;
      }
      continue;
    }
    if (errno == EINTR) {
      continue;
    }
    break;
  }
  if (subfd < 0) {
    return -1;
  }
  TheScheduler->watchIO(subfd);
  return subfd;
}

int Connect(int fd, const sockaddr *addr, socklen_t addrlen, int timeout)
{
  CHECK_THE_SCHEDULER;
  if (!TheScheduler->ioIsWatched(fd)) {
    errno = EBADF;
    return -1;
  }
  if (connect(fd, addr, addrlen) < 0) {
    if (errno != EINTR && errno != EINPROGRESS) {
      return -1;
    }
    if (TheScheduler->awaitIOEvent(fd, IOEvent::Writability, timeout) < 0) {
      return -1;
    }
    int optval;
    socklen_t optlen = sizeof optval;
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
      return -1;
    }
    if (optval != 0) {
      errno = optval;
      return -1;
    }
  }
  return 0;
}

ssize_t Recv(int fd, void *buf, size_t buflen, int flags, int timeout)
{
  CHECK_THE_SCHEDULER;
  if (!TheScheduler->ioIsWatched(fd)) {
    errno = EBADF;
    return -1;
  }
  ssize_t n;
  for (;;) {
    n = recv(fd, buf, buflen, flags);
    if (n >= 0) {
      break;
    }
    if (errno == EWOULDBLOCK) {
      if (TheScheduler->awaitIOEvent(fd, IOEvent::Readability, timeout) < 0) {
        break;
      }
      continue;
    }
    if (errno == EINTR) {
      continue;
    }
    break;
  }
  if (n < 0) {
    return -1;
  }
  return n;
}

ssize_t Send(int fd, const void *buf, size_t buflen, int flags, int timeout)
{
  CHECK_THE_SCHEDULER;
  if (!TheScheduler->ioIsWatched(fd)) {
    errno = EBADF;
    return -1;
  }
  ssize_t n;
  for (;;) {
    n = send(fd, buf, buflen, flags);
    if (n >= 0) {
      break;
    }
    if (errno == EWOULDBLOCK) {
      if (TheScheduler->awaitIOEvent(fd, IOEvent::Writability, timeout) < 0) {
        break;
      }
      continue;
    }
    if (errno == EINTR) {
      continue;
    }
    break;
  }
  if (n < 0) {
    return -1;
  }
  return n;
}

ssize_t RecvFrom(int fd, void *buf, size_t buflen, int flags, sockaddr *addr,
                 socklen_t *addrlen, int timeout)
{
  CHECK_THE_SCHEDULER;
  if (!TheScheduler->ioIsWatched(fd)) {
    errno = EBADF;
    return -1;
  }
  ssize_t n;
  for (;;) {
    n = recvfrom(fd, buf, buflen, flags, addr, addrlen);
    if (n >= 0) {
      break;
    }
    if (errno == EWOULDBLOCK) {
      if (TheScheduler->awaitIOEvent(fd, IOEvent::Readability, timeout) < 0) {
        break;
      }
      continue;
    }
    if (errno == EINTR) {
      continue;
    }
    break;
  }
  if (n < 0) {
    return -1;
  }
  return n;
}

ssize_t SendTo(int fd, const void *buf, size_t buflen, int flags,
               const sockaddr *addr, socklen_t addrlen, int timeout)
{
  CHECK_THE_SCHEDULER;
  if (!TheScheduler->ioIsWatched(fd)) {
    errno = EBADF;
    return -1;
  }
  ssize_t n;
  for (;;) {
    n = sendto(fd, buf, buflen, flags, addr, addrlen);
    if (n >= 0) {
      break;
    }
    if (errno == EWOULDBLOCK) {
      if (TheScheduler->awaitIOEvent(fd, IOEvent::Writability, timeout) < 0) {
        break;
      }
      continue;
    }
    if (errno == EINTR) {
      continue;
    }
    break;
  }
  if (n < 0) {
    return -1;
  }
  return n;
}

int OpenAsync(const char *path, int flags, mode_t mode)
{
  CHECK_THE_SCHEDULER;
  int fd;
  int errorNumber = 0;
  Task task([&path, &flags, &mode, &fd, &errorNumber] {
    do {
      fd = open(path, flags & ~O_NONBLOCK, mode);
      if (fd >= 0) {
        break;
      }
    } while (errno == EINTR);
    if (fd < 0) {
      errorNumber = errno;
    }
  });
  TheScheduler->awaitTask(&task);
  if (errorNumber != 0) {
    errno = errorNumber;
    return -1;
  }
  return fd;
}

int CloseAsync(int fd)
{
  CHECK_THE_SCHEDULER;
  int result;
  int errorNumber = 0;
  Task task([&fd, &result, &errorNumber] {
    do {
      result = close(fd);
      if (result >= 0) {
        break;
      }
    } while (errno == EINTR);
    if (result < 0) {
      errorNumber = errno;
    }
  });
  TheScheduler->awaitTask(&task);
  if (errorNumber != 0) {
    errno = errorNumber;
    return -1;
  }
  return result;
}

ssize_t ReadAsync(int fd, void *buf, size_t buflen)
{
  CHECK_THE_SCHEDULER;
  ssize_t n;
  int errorNumber = 0;
  Task task([&fd, &buf, &buflen, &n, &errorNumber] {
    do {
      n = read(fd, buf, buflen);
      if (n >= 0) {
        break;
      }
    } while (errno == EINTR);
    if (n < 0) {
      errorNumber = errno;
    }
  });
  TheScheduler->awaitTask(&task);
  if (errorNumber != 0) {
    errno = errorNumber;
    return -1;
  }
  return n;
}

ssize_t WriteAsync(int fd, const void *buf, size_t buflen)
{
  CHECK_THE_SCHEDULER;
  ssize_t n;
  int errorNumber = 0;
  Task task([&fd, &buf, &buflen, &n, &errorNumber] {
    do {
      n = write(fd, buf, buflen);
      if (n >= 0) {
        break;
      }
    } while (errno == EINTR);
    if (n < 0) {
      errorNumber = errno;
    }
  });
  TheScheduler->awaitTask(&task);
  if (errorNumber != 0) {
    errno = errorNumber;
    return -1;
  }
  return n;
}

} // namespace Tara
