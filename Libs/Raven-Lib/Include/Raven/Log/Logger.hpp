#pragma once

#include <fmt/core.h>

#include <Raven/Log/Category.hpp>
#include <Raven/Log/Level.hpp>
#include <string_view>

namespace Raven {
namespace Log {
class Logger {
 public:
  RAVEN_LIB_EXPORT static Logger& Instance();
  RAVEN_LIB_EXPORT void Log(Category& category, Level level, std::string_view msg);

  template <typename... Args>
  inline void Log(Category& category, Level level, std::string_view msg, Args... args) {
    Log(category, level, fmt::format(msg, args...));
  }
};
}  // namespace Log
}  // namespace Raven