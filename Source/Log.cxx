#include "Log.hxx"

#include <assert.h>
#
#include <iostream>

namespace Tara {

Log *Log::Get()
{
  static Log instance(Level::Debugging, &std::clog);
  return &instance;
}

Log::Log(Level level, std::ostream *outputStream)
  : level_(level), outputStream_(outputStream)
{
  assert(outputStream_ != nullptr);
}

} // namespace Tara
