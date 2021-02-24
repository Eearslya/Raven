#include <Raven/Log/Level.hpp>

namespace std {
std::string to_string(Raven::Log::Level lvl) {
#define X(level)                 \
  case Raven::Log::Level::level: \
    return #level;
  switch (lvl) {
    X(Fatal)
    X(Error)
    X(Warning)
    X(Info)
    X(Debug)
    X(Verbose)
  }
#undef X

  return "Unknown";
}
}  // namespace std