#pragma once

#include "libuv/heap-inl.h"
#
#include "Utility.hxx"

namespace Tara {

struct TimerItem;

class Timer final
{
  TARA_DISALLOW_COPY(Timer);

public:
  Timer();

  void addItem(TimerItem *item, int duration);
  void removeItem(TimerItem *item);
  unsigned int removeDueItems(TimerItem **buffer, unsigned int bufferLength);
  int calculateTimeout();

private:
  heap itemHeap_;
};

} // namespace Tara
