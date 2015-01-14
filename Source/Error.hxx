#pragma once

#include <ostream>

namespace Tara {

class Error final
{
  Error(const Error &other) = delete;
  void operator=(const Error &other) = delete;

public:
  explicit Error(int number);

  int getNumber() const { return number_; }

private:
  const int number_;
};

std::ostream &operator<<(std::ostream &outputStream, const Error &error);

} // namespace Tara
