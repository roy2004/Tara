#include "Scheduler.hxx"

namespace Tara {

thread_local Scheduler *TheScheduler;

} // namespace Tara

int TaraMain(int argc, char **argv);

int main(int argc, char **argv)
{
  int status = 0;
  Tara::Scheduler scheduler;
  Tara::TheScheduler = &scheduler;
  scheduler.callCoroutine([argc, argv, &status] () {
    status = TaraMain(argc, argv);
  });
  scheduler.run();
  return status;
}
