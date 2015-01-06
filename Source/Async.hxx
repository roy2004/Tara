#pragma once

#include "libuv/queue.h"
#
#include "TaskFunc.hxx"
#include "Utility.hxx"

namespace Tara {

using TaskFunc = std::function<void ()>;

class Async final
{
  TARA_DISALLOW_COPY(Async);

public:
  Async();

  void awaitTask(const TaskFunc *taskFunc);

private:
  QUEUE pendingTaskQueue_;
  QUEUE completedTaskQueue_;
};

} // namespace Tara
