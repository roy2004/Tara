__asm__ __volatile__ (".globl TaraRunFiber");

#ifdef __x86_64__

// param1: rdi: fiberStart
// param2: rsi: scheduler
// param3: rdx: stack
// param4: rcx: stackSize

__asm__ __volatile__ ("     \
TaraRunFiber:               \
\n\tmovq $0, %rbp           \
\n\tleaq (%rdx, %rcx), %rsp \
\n\txchgq %rsi, %rdi        \
\n\tpushq $0                \
\n\tjmpq *%rsi              \
");

#else
#error architecture not supported
#endif
