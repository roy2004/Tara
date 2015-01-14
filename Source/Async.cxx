#include "Async.hxx"

#include <sys/eventfd.h>
#include <unistd.h>
#
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#
#include "Error.hxx"
#include "IOEvent.hxx"
#include "Log.hxx"
#include "Scheduler.hxx"
#include "Utility.hxx"

namespace Tara {

namespace {

struct Job final
{
  QUEUE queueItem;
  Fiber *const fiber;
  const Task *const tasks;
  const unsigned int taskCount;

  Job(Fiber *fiber, const Task *tasks, unsigned int taskCount);
};

int xeventfd(unsigned int initval, int flags);
size_t xwrite(int fd, const void *buf, size_t nbytes);
size_t xread(int fd, void *buf, size_t nbytes);
void xclose(int fd);
void xthread_mutex_init(pthread_mutex_t *mutex,
                        const pthread_mutexattr_t *attr);
void xthread_mutex_destroy(pthread_mutex_t *mutex);
void xthread_mutex_lock(pthread_mutex_t *mutex);
void xthread_mutex_unlock(pthread_mutex_t *mutex);
void xthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
void xthread_cond_destroy(pthread_cond_t *cond);
void xthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
void xthread_cond_signal(pthread_cond_t *cond);
void xthread_cond_broadcast(pthread_cond_t *cond);
void xthread_create(pthread_t *thread, const pthread_attr_t *attr,
                    void *(*start_routine)(void *), void *arg);
void xthread_join(pthread_t thread, void **retval);

} // namespace

Async::Async(Scheduler *scheduler)
  : scheduler_(scheduler), fd_(xeventfd(0, 0)), workIsDone_(false), jobCount_(0)
{
  assert(scheduler_ != nullptr);
  QUEUE_INIT(&jobQueues_[0]);
  QUEUE_INIT(&jobQueues_[1]);
  xthread_mutex_init(&mutexes_[0], nullptr);
  xthread_mutex_init(&mutexes_[1], nullptr);
  xthread_cond_init(&condition_, nullptr);
  for (int i = 0; i < TARA_LENGTH_OF(threads_); ++i) {
    xthread_create(&threads_[i], nullptr,
                   reinterpret_cast<void *(*)(void *)>(Worker), this);
  }
}

Async::~Async()
{
  xthread_mutex_lock(&mutexes_[0]);
  workIsDone_ = true;
  xthread_mutex_unlock(&mutexes_[0]);
  xthread_cond_broadcast(&condition_);
  for (int i = 0; i < TARA_LENGTH_OF(threads_); ++i) {
    xthread_join(threads_[i], nullptr);
  }
  xthread_mutex_destroy(&mutexes_[0]);
  xthread_mutex_destroy(&mutexes_[1]);
  xthread_cond_destroy(&condition_);
  xclose(fd_);
}

void Async::doWork()
{
  for (;;) {
    xthread_mutex_lock(&mutexes_[0]);
    while (!workIsDone_ && QUEUE_EMPTY(&jobQueues_[0])) {
      xthread_cond_wait(&condition_, &mutexes_[0]);
    }
    if (workIsDone_) {
      xthread_mutex_unlock(&mutexes_[0]);
      break;
    }
    auto job = QUEUE_DATA(QUEUE_HEAD(&jobQueues_[0]), Job, queueItem);
    QUEUE_REMOVE(&job->queueItem);
    xthread_mutex_unlock(&mutexes_[0]);
    for (int i = 0; i < job->taskCount; ++i) {
      job->tasks[i]();
    }
    xthread_mutex_lock(&mutexes_[1]);
    QUEUE_INSERT_TAIL(&jobQueues_[1], &job->queueItem);
    uint64_t value = 1;
    static_cast<void>(xwrite(fd_, &value, sizeof value));
    xthread_mutex_unlock(&mutexes_[1]);
  }
}

void Async::awaitTasks(const Task *tasks, unsigned int taskCount)
{
  Job job(scheduler_->getCurrentFiber(), tasks, taskCount);
  xthread_mutex_lock(&mutexes_[0]);
  QUEUE_INSERT_TAIL(&jobQueues_[0], &job.queueItem);
  xthread_cond_signal(&condition_);
  xthread_mutex_unlock(&mutexes_[0]);
  ++jobCount_;
  if (jobCount_ == 1) {
    scheduler_->callCoroutine([this] {
      scheduler_->watchIO(fd_);
      do {
        scheduler_->awaitIOEvent(fd_, IOEvent::Readability, -1);
        xthread_mutex_lock(&mutexes_[1]);
        uint64_t value;
        static_cast<void>(xread(fd_, &value, sizeof value));
        QUEUE jobQueue;
        QUEUE *q = QUEUE_HEAD(&jobQueues_[1]);
        QUEUE_SPLIT(&jobQueues_[1], q, &jobQueue);
        xthread_mutex_unlock(&mutexes_[1]);
        QUEUE_FOREACH(q, &jobQueue) {
          auto job = QUEUE_DATA(q, Job, queueItem);
          scheduler_->resumeFiber(job->fiber);
          --jobCount_;
        }
      } while (jobCount_ != 0);
      scheduler_->unwatchIO(fd_);
    });
  }
  scheduler_->suspendCurrentFiber();
}

namespace {

Job::Job(Fiber *fiber, const Task *tasks, unsigned int taskCount)
  : fiber(fiber), tasks(tasks), taskCount(taskCount)
{
  assert(this->fiber != nullptr);
  assert(this->taskCount == 0 || this->tasks != nullptr);
}

int xeventfd(unsigned int initval, int flags)
{
  int fd = eventfd(initval, flags);
  if (fd < 0) {
    TARA_FATALITY_LOG("eventfd failed: ", Error(errno));
  }
  return fd;
}

size_t xwrite(int fd, const void *buf, size_t nbytes)
{
  ssize_t n;
  do {
    n = write(fd, buf, nbytes);
    if (n >= 0) {
      break;
    }
  } while (errno == EINTR);
  if (n < 0) {
    TARA_FATALITY_LOG("write failed: ", Error(errno));
  }
  return n;
}

size_t xread(int fd, void *buf, size_t nbytes)
{
  ssize_t n;
  do {
    n = read(fd, buf, nbytes);
    if (n >= 0) {
      break;
    }
  } while (errno == EINTR);
  if (n < 0) {
    TARA_FATALITY_LOG("read failed: ", Error(errno));
  }
  return n;
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

void xthread_mutex_init(pthread_mutex_t *mutex,
                        const pthread_mutexattr_t *attr)
{
  int errorNumber;
  do {
    errorNumber = pthread_mutex_init(mutex, attr);
    if (errorNumber == 0) {
      break;
    }
  } while (errorNumber == EAGAIN);
  if (errorNumber != 0) {
    TARA_FATALITY_LOG("pthread_mutex_init failed: ", Error(errorNumber));
  }
}

void xthread_mutex_destroy(pthread_mutex_t *mutex)
{
  int errorNumber = pthread_mutex_destroy(mutex);
  if (errorNumber != 0) {
    TARA_FATALITY_LOG("pthread_mutex_destroy failed: ", Error(errorNumber));
  }
}

void xthread_mutex_lock(pthread_mutex_t *mutex)
{
  int errorNumber;
  do {
    errorNumber = pthread_mutex_lock(mutex);
    if (errorNumber == 0) {
      break;
    }
  } while (errorNumber == EAGAIN);
  if (errorNumber != 0) {
    TARA_FATALITY_LOG("pthread_mutex_lock failed: ", Error(errorNumber));
  }
}

void xthread_mutex_unlock(pthread_mutex_t *mutex)
{
  int errorNumber = pthread_mutex_unlock(mutex);
  if (errorNumber != 0) {
    TARA_FATALITY_LOG("pthread_mutex_unlock failed: ", Error(errorNumber));
  }
}

void xthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
  int errorNumber;
  do {
    errorNumber = pthread_cond_init(cond, attr);
    if (errorNumber == 0) {
      break;
    }
  } while (errorNumber == EAGAIN);
  if (errorNumber != 0) {
    TARA_FATALITY_LOG("pthread_cond_init failed: ", Error(errorNumber));
  }
}

void xthread_cond_destroy(pthread_cond_t *cond)
{
  int errorNumber = pthread_cond_destroy(cond);
  if (errorNumber != 0) {
    TARA_FATALITY_LOG("pthread_cond_destroy failed: ", Error(errorNumber));
  }
}

void xthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
  int errorNumber = pthread_cond_wait(cond, mutex);
  if (errorNumber != 0) {
    TARA_FATALITY_LOG("pthread_cond_wait failed: ", Error(errorNumber));
  }
}

void xthread_cond_signal(pthread_cond_t *cond)
{
  int errorNumber = pthread_cond_signal(cond);
  if (errorNumber != 0) {
    TARA_FATALITY_LOG("pthread_cond_signal failed: ", Error(errorNumber));
  }
}

void xthread_cond_broadcast(pthread_cond_t *cond)
{
  int errorNumber = pthread_cond_broadcast(cond);
  if (errorNumber != 0) {
    TARA_FATALITY_LOG("pthread_cond_broadcast failed: ", Error(errorNumber));
  }
}

void xthread_create(pthread_t *thread, const pthread_attr_t *attr,
                    void *(*start_routine)(void *), void *arg)
{
  int errorNumber;
  do {
    errorNumber = pthread_create(thread, attr, start_routine, arg);
    if (errorNumber == 0) {
      break;
    }
  } while (errorNumber == EAGAIN);
  if (errorNumber != 0) {
    TARA_FATALITY_LOG("pthread_create failed: ", Error(errorNumber));
  }
}

void xthread_join(pthread_t thread, void **retval)
{
  int errorNumber = pthread_join(thread, retval);
  if (errorNumber != 0) {
    TARA_FATALITY_LOG("pthread_join failed: ", Error(errorNumber));
  }
}

} // namespace

} // namespace Tara
