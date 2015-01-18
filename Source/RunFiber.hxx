#pragma once

#include <stddef.h>

#define RunFiber TaraRunFiber

namespace Tara {

class Scheduler;

extern "C" {

[[noreturn]] void TaraRunFiber(void (*fiberStart)(Scheduler *),
                               Scheduler *scheduler, unsigned char *stack,
                               size_t stackSize);

} // extern "C"

} // namespace Tara
