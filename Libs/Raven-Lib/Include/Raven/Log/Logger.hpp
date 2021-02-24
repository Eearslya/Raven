#pragma once

#include <fmt/core.h>

#include <Raven/Log/Category.hpp>
#include <Raven/Log/Level.hpp>
#include <Raven/Log/Sink.hpp>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

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

  RAVEN_LIB_EXPORT bool RegisterSink(const std::shared_ptr<Sink>& sink);
  RAVEN_LIB_EXPORT bool DeregisterSink(const std::shared_ptr<Sink>& sink);

 private:
  Logger() = default;

  std::mutex mSinkLock;
  std::vector<std::shared_ptr<Sink>> mSinks;
};
}  // namespace Log
}  // namespace Raven