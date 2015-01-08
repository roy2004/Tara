#pragma once

#include <pthread.h>
#
#include <functional>
#
#include "libuv/queue.h"
#
#include "Utility.hxx"

namespace Tara {

class Scheduler;

using Task = std::function<void ()>;

class Async final
{
  TARA_DISALLOW_COPY(Async);

public:
  explicit Async(Scheduler *scheduler);
  ~Async();

  void awaitTask(const Task *task) { awaitTasks(task, 1); }

  void awaitTasks(const Task *tasks, unsigned int taskCount);

private:
  static void Worker(Async *async) { async->doWork(); }

  Scheduler *const scheduler_;
  const int fd_;
  bool workIsDone_;
  unsigned int jobCount_;
  QUEUE jobQueues_[2];
  pthread_mutex_t mutexes_[2];
  pthread_cond_t condition_;
  pthread_t threads_[4];

  void doWork();
};

} // namespace Tara
