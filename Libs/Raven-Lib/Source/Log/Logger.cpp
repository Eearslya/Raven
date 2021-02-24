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
  std::cout << cat.Name() << " : " << std::to_string(lvl) << " : " << msg << '\n';
}
}  // namespace Log
}  // namespace Raven