#include "Scheduler.hxx"

namespace Tara {

thread_local Scheduler *TheScheduler;

} // namespace Tara

using namespace Tara;

int Main(int argc, char **argv);

int main(int argc, char **argv)
{
  int status = 0;
  Scheduler scheduler;
  TheScheduler = &scheduler;
  scheduler.callCoroutine([argc, argv, &status] () {
    status = Main(argc, argv);
  });
  scheduler.run();
  return status;
}
