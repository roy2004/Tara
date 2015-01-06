#if defined(__amd64__) || defined(__x86_64__)
__asm__ __volatile__ ("\
.globl TaraRunFiber;   \
TaraRunFiber:          \
  movq $0, %rbp;       \
  movq %rdx, %rsp;     \
  xchgq %rsi, %rdi;    \
  pushq $0;            \
  jmpq *%rsi;          \
");
#endif
