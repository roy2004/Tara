#pragma once

#include <stdlib.h>
#
#include <sstream>

#define TARA_LOG(LEVEL, ...)                                              \
  do {                                                                    \
    if (static_cast<int>(Tara::Log::Level::LEVEL) <                       \
        static_cast<int>(Tara::Log::GetLevel())) {                        \
      break;                                                              \
    }                                                                     \
    Tara::Log(), #LEVEL ": ", __FILE__ ": ", __LINE__, ": ", __VA_ARGS__; \
  } while (false)

#ifndef NDEBUG
#define TARA_DEBUGGING_LOG(...) \
  TARA_LOG(Debugging, __VA_ARGS__)
#else
#define TARA_DEBUGGING_LOG(...)
#endif

#define TARA_INFORMING_LOG(...) \
  TARA_LOG(Informing, __VA_ARGS__)

#define TARA_WARNING_LOG(...) \
  TARA_LOG(Warning, __VA_ARGS__)

#define TARA_ERROR_LOG(...) \
  TARA_LOG(Error, __VA_ARGS__)

#define TARA_FATALITY_LOG(...)     \
  TARA_LOG(Fatality, __VA_ARGS__); \
  abort()

namespace Tara {

class Log final
{
  Log(const Log &other) = delete;
  void operator=(const Log &other) = delete;

public:
  enum class Level
  {
    Debugging,
    Informing,
    Warning,
    Error,
    Fatality
  };

  static Level GetLevel();
  static void SetLevel(Level level);

  Log();
  ~Log();

  template <typename TYPE>
  Log &operator,(const TYPE &value)
  {
    outputStream_ << value;
    return *this;
  }

private:
  static Level Level_;

  std::ostringstream outputStream_;
};

} // namespace Tara
