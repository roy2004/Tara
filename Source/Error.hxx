#pragma once

#include <ostream>
#
#include "Utility.hxx"

namespace Tara {

class Error final
{
  TARA_DISALLOW_COPY(Error);

public:
  explicit Error(int number);

  int getNumber() const { return number_; }

private:
  const int number_;
};

std::ostream &operator<<(std::ostream &outputStream, const Error &error);

} // namespace Tara
