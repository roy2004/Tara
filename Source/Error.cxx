#include "Error.hxx"

#include <string.h>

namespace Tara {

Error::Error(int number)
  : number_(number)
{}

std::ostream &operator<<(std::ostream &outputStream, const Error &error)
{
  outputStream << strerror(error.getNumber());
  return outputStream;
}

} // namespace Tara
