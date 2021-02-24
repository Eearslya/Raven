#pragma once

#include <Raven/Lib_Export.h>

#include <string>

namespace Raven {
namespace Log {
enum class Level { Fatal = 0, Error, Warning, Info, Debug, Verbose, All = Verbose };
}  // namespace Log
}  // namespace Raven

namespace std {
RAVEN_LIB_EXPORT ::std::string to_string(Raven::Log::Level lvl);
}