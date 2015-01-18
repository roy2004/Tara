__asm__ __volatile__ (".globl TaraRunFiber");

#if defined __i386__

// param1: 4(%esp): fiberStart
// param2: 8(%esp): scheduler
// param3: 12(%esp): stack
// param4: 16(%esp): stackSize

__asm__ __volatile__ (" \
TaraRunFiber:           \
\n\tmovl 4(%esp), %eax  \
\n\tmovl 8(%esp), %edx  \
\n\tmovl 12(%esp), %ecx \
\n\taddl 16(%esp), %ecx \
\n\tmovl $0, %ebp       \
\n\tmovl %ecx, %esp     \
\n\tpushl %edx          \
\n\tpushl $0            \
\n\tjmpl *%eax          \
");

#elif defined __x86_64__

// param1: %rdi: fiberStart
// param2: %rsi: scheduler
// param3: %rdx: stack
// param4: %rcx: stackSize

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
