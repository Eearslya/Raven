#include <Raven/Log/Level.hpp>
#include <Raven/Log/Logger.hpp>
#include <iostream>

namespace Raven {
namespace Log {
Logger& Logger::Instance() {
  static Logger gLogger;
  return gLogger;
}

void Logger::Log(Category& cat, Level lvl, std::string_view msg) {
  if (lvl > cat.MaxLevel()) {
    return;
  }

  const std::scoped_lock lock{mSinkLock};
  const std::string logMsg{fmt::format("[{}-{}] {}", std::to_string(lvl), cat.Name(), msg)};

  for (const auto& sink : mSinks) {
    sink->Output(logMsg);
  }
}

bool Logger::RegisterSink(const std::shared_ptr<Sink>& sink) {
  const std::scoped_lock lock{mSinkLock};

  for (const auto& s : mSinks) {
    if (s == sink) {
      return false;
    }
  }

  mSinks.push_back(sink);

  return true;
}

bool Logger::DeregisterSink(const std::shared_ptr<Sink>& sink) {
  const std::scoped_lock lock{mSinkLock};

  for (auto it = mSinks.begin(); it != mSinks.end(); ++it) {
    if (sink == *it) {
      mSinks.erase(it);
      return true;
    }
  }

  return false;
}
}  // namespace Log
}  // namespace Raven