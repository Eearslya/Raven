#pragma once

#include <fmt/core.h>

namespace Raven {
class Log {
 public:
  enum class Level { Fatal = 0, Error, Warning, Info, Debug, Trace };

  static void Initialize() noexcept;
  static void Shutdown() noexcept;
  static void SetLevel(const Level level) noexcept;

  template <typename... Args>
  static void Fatal(const std::string& format, Args&&... args) noexcept {
    Log_(Level::Fatal, format, std::forward<Args>(args)...);
  }
  template <typename... Args>
  static void Error(const std::string& format, Args&&... args) noexcept {
    Log_(Level::Error, format, std::forward<Args>(args)...);
  }
  template <typename... Args>
  static void Warn(const std::string& format, Args&&... args) noexcept {
    Log_(Level::Warning, format, std::forward<Args>(args)...);
  }
  template <typename... Args>
  static void Info(const std::string& format, Args&&... args) noexcept {
    Log_(Level::Info, format, std::forward<Args>(args)...);
  }
  template <typename... Args>
  static void Debug(const std::string& format, Args&&... args) noexcept {
    Log_(Level::Debug, format, std::forward<Args>(args)...);
  }
  template <typename... Args>
  static void Trace(const std::string& format, Args&&... args) noexcept {
    Log_(Level::Trace, format, std::forward<Args>(args)...);
  }

 private:
  static Level sLogLevel;

  template <typename... Args>
  static void Log_(const Level level, const std::string& format, Args&&... args) noexcept {
    if (level <= sLogLevel) {
      const std::string msg{fmt::format(format, std::forward<Args>(args)...)};
      Output(level, msg);
    }
  }

  static void Output(const Level level, const std::string& msg) noexcept;
};
}  // namespace Raven