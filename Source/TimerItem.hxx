#pragma once

#include <stdint.h>
#
#include "libuv/heap-inl.h"

namespace Tara {

struct TimerItem final
{
  heap_node heapNode;
  uint64_t dueTime;
};

} // namespace Tara
