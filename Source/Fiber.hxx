#pragma once

#include <setjmp.h>
#
#include "libuv/queue.h"
#
#include "Coroutine.hxx"
#include "TimerItem.hxx"

namespace Tara {

struct Fiber final
{
  QUEUE queueItem;
  TimerItem timerItem;
  const Coroutine coroutine;
  void *const stack;
  jmp_buf *context;
  int status;
  int fd;

  Fiber(const Coroutine &coroutine, void *stack);
  Fiber(Coroutine &&coroutine, void *stack);
};

} // namespace Tara
