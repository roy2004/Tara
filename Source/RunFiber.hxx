#pragma once

#define RunFiber TaraRunFiber

namespace Tara {

class Scheduler;

extern "C" {

[[noreturn]] void TaraRunFiber(void (*fiberStart)(Scheduler *),
                               Scheduler *scheduler, void *fiberStack);

} // extern "C"

} // namespace Tara
