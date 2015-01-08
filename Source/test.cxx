#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#
#include "Runtime.hxx"
#include "Async.hxx"

namespace Tara {

extern thread_local Scheduler *const TheScheduler;

}

using namespace Tara;

int Main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  Async async(TheScheduler);
  Call([&async] {
    int i = 0;
    Task task1([&i] {
      printf("[1] = %d\n", ++i);
    });
    for (;;)
      async.awaitTask(&task1);
  });
  Call([&async] {
    int i = 0;
    Task task2([&i] {
      printf("[2] = %d\n", ++i);
    });
    for (;;)
      async.awaitTask(&task2);
  });
  Sleep(-1);
  return 0;
}
