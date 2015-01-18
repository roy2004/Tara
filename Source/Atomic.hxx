#pragma once

namespace Tara {

#if defined __i386__ || defined __x86_64__

template<typename TYPE>
inline void Exchange(TYPE &lvalue1, TYPE &lvalue2)
{
  __asm__ __volatile__ ("lock xchg %1, %0" : "+m"(lvalue1), "+r"(lvalue2));
  // lvalue1 <-> lvalue2
}

template<typename TYPE>
inline void ExchangeAdd(TYPE &lvalue1, TYPE &lvalue2)
{
  __asm__ __volatile__ ("lock xadd %1, %0" : "+m"(lvalue1), "+r"(lvalue2));
  // lvalue1 <-> lvalue2
  // lvalue1 <- lvalue1 + lvalue2
}

template<typename TYPE>
inline bool CompareExchange(TYPE &lvalue1, TYPE &lvalue2, const TYPE &rvalue3)
{
  unsigned char result;
  __asm__ __volatile__ ("lock cmpxchg %3, %0\n\t"
                        "sete %2" : "+m"(lvalue1), "+a"(lvalue2), "=r"(result)
                                  : "r"(rvalue3));
  // if lvalue1 = lvalue2
  //   lvalue1 <- rvalue3
  //   result <- 1
  // else
  //   lvalue2 <- lvalue1
  //   result <- 0
  return result;
}

#else
#error architecture not supported
#endif

} // namespace Tara
