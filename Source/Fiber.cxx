#include "Fiber.hxx"

#include <assert.h>
#
#include <utility>

namespace Tara {

Fiber::Fiber(const Coroutine &coroutine, void *stack)
  : coroutine(coroutine), stack(stack), context(nullptr), status(0), fd(-1)
{
  assert(this->coroutine != nullptr);
  assert(this->stack != nullptr);
}

Fiber::Fiber(Coroutine &&coroutine, void *stack)
  : coroutine(std::move(coroutine)), stack(stack), context(nullptr),
    status(0), fd(-1)
{
  assert(this->coroutine != nullptr);
  assert(this->stack != nullptr);
}

} // namespace Tara
