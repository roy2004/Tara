#pragma once

#include <sys/socket.h>
#include <sys/types.h>
#
#include "Coroutine.hxx"

namespace Tara {

void Call(const Coroutine &coroutine);
void Call(Coroutine &&coroutine);
void Yield();
void Sleep(int duration);
[[noreturn]] void Exit();

int Open(const char *path, int flags, mode_t mode = 0);
int Pipe2(int *fds, int flags);
int Socket(int domain, int type, int protocol);
int Close(int fd);
ssize_t Read(int fd, void *buf, size_t buflen, int timeout);
ssize_t Write(int fd, const void *buf, size_t buflen, int timeout);
int Accept4(int fd, sockaddr *addr, socklen_t *addrlen, int flags, int timeout);
int Connect(int fd, const sockaddr *addr, socklen_t addrlen, int timeout);
ssize_t Recv(int fd, void *buf, size_t buflen, int flags, int timeout);
ssize_t Send(int fd, const void *buf, size_t buflen, int flags, int timeout);
ssize_t RecvFrom(int fd, void *buf, size_t buflen, int flags, sockaddr *addr,
                 socklen_t *addrlen, int timeout);
ssize_t SendTo(int fd, const void *buf, size_t buflen, int flags,
               const sockaddr *addr, socklen_t addrlen, int timeout);

} // namespace Tara
