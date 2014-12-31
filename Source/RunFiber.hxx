#pragma once

namespace Tara {

class Scheduler;

extern "C" {

[[noreturn]] void RunFiber(void (*fiberStart)(Scheduler *),
                           Scheduler *scheduler, void *fiberStack);

} // extern "C"

} // namespace Tara

#if defined(__amd64__) || defined(__x86_64__)

__asm__ __volatile__ ("\
RunFiber:              \
  movq $0, %rbp;       \
  movq %rdx, %rsp;     \
  xchgq %rsi, %rdi;    \
  pushq $0;            \
  jmpq *%rsi;          \
");

#endif
