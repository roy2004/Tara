#include "Log.hxx"

#include <stdio.h>

namespace Tara {

Log::Level Log::Level_(Level::Debugging);

Log::Log()
  : outputStream_(std::ostringstream::ate)
{}

Log::~Log()
{
  outputStream_.put('\n');
  fputs(outputStream_.str().c_str(), stderr);
}

} // namespace Tara
