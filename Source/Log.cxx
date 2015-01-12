#include "Log.hxx"

#include <stdio.h>

namespace Tara {

Log::Level Log::Level_(Level::Debugging);

Log::Level Log::GetLevel()
{
  Level level;
#if defined(__i386__) || defined(__x86_64__)
  __asm__ __volatile__ ("sfence\n\tmovl %1, %0" : "=r"(level) : "m"(Level_));
#else
#error architecture not supported
#endif
  return level;
}

void Log::SetLevel(Level level)
{
#if defined(__i386__) || defined(__x86_64__)
  __asm__ __volatile__ ("lfence\n\tmovl %1, %0" : "=m"(Level_) : "r"(level));
#else
#error architecture not supported
#endif
}

Log::Log()
  : outputStream_(std::ostringstream::ate)
{}

Log::~Log()
{
  outputStream_.put('\n');
  fputs(outputStream_.str().c_str(), stderr);
}

} // namespace Tara
