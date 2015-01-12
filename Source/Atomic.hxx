#pragma once

#if defined(__i386__) || defined(__x86_64__)

#define TARA_LOCK_XADD(LHS1, LHS2) \
  __asm__ __volatile__ ("lock xadd %1, %0" : "+m"(LHS1), "+r"(LHS2))
  // LHS1 <-> LHS2
  // LHS1 <- LHS1 + LHS2

#define TARA_LOCK_XCHG(LHS1, LHS2) \
  __asm__ __volatile__ ("lock xchg %1, %0" : "+m"(LHS1), "+r"(LHS2))
  // LHS1 <-> LHS2

#define TARA_LOCK_CMPXCHG(LHS1, LHS2, RHS3)                            \
  __asm__ __volatile__ ("lock cmpxchg %2, %0" : "+m"(LHS1), "+a"(LHS2) \
                                              : "r"(RHS3))
  // if LHS1 = LHS2
  //   LHS1 <- RHS3
  // else
  //   LHS2 <- LHS1

#endif
