#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#
#include "Runtime.hxx"
#include "Scheduler.hxx"

namespace Tara {

extern thread_local Scheduler *const TheScheduler;

}

using namespace Tara;

int Main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  for (int i = 0; ; ++i) {
    Task task([i] {
      printf("%d\n", i);
    });
    TheScheduler->awaitTask(&task);
  }
/*  for (int i = 0; ; ++i) {
    printf("%d\n", i);
  };*/
  Sleep(-1);
  return 0;
}
