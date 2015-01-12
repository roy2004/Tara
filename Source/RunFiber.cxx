__asm__ __volatile__ (".globl TaraRunFiber");

#ifdef __x86_64__
__asm__ __volatile__ ("\
TaraRunFiber:          \
\n\tmovq $0, %rbp      \
\n\tmovq %rdx, %rsp    \
\n\txchgq %rsi, %rdi   \
\n\tpushq $0           \
\n\tjmpq *%rsi         \
");
#else
#error architecture not supported
#endif
