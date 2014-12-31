#pragma once

#include <stdlib.h>
#
#include <ostream>
#
#include "Utility.hxx"

#define TARA_LOG(LEVEL, ...)                        \
  do {                                              \
    const Tara::Log *taraLog = Tara::Log::Get();    \
    if (static_cast<int>(Tara::Log::Level::LEVEL) < \
        static_cast<int>(taraLog->getLevel())) {    \
      break;                                        \
    }                                               \
    *taraLog, #LEVEL       " : "                    \
            , __FILE__     " : "                    \
            , __LINE__   , " : "                    \
            , __VA_ARGS__, std::endl;               \
  } while (false)

#ifndef NDEBUG
#define TARA_DEBUGGING_LOG(...) \
  TARA_LOG(Debugging, __VA_ARGS__)
#else
#define TARA_DEBUGGING_LOG(...)
#endif

#define TARA_INFORMATION_LOG(...) \
  TARA_LOG(Information, __VA_ARGS__)

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
  TARA_DISALLOW_COPY(Log);

public:
  enum class Level
  {
    Debugging,
    Information,
    Warning,
    Error,
    Fatality
  };

  static Log *Get();

  template <typename TYPE>
  const Log &operator,(const TYPE &value) const
  {
    *outputStream_ << value;
    return *this;
  }

  const Log &operator,(std::ostream &(*outputStreamManipulator)
                                     (std::ostream &)) const
  {
    *outputStream_ << outputStreamManipulator;
    return *this;
  }

  Level getLevel() const { return level_; }
  void setLevel(Level level) { level_ = level; }

private:
  Level level_;
  std::ostream *const outputStream_;

  Log(Level level, std::ostream *outputStream);
};

} // namespace Tara
