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

  Call([] {
    puts("2");
  });
  Task task([] {
    puts("3");
  });
  puts("1");
  async.awaitTasks(&task, 1);
  puts("4");

  return 0;
}
