#pragma once

#include <iostream>

#include "Sink.hpp"

namespace Raven {
namespace Log {
namespace Sinks {
class StdoutSink : public Sink {
 public:
  void Output(const std::string_view& msg) override { std::cout << msg << '\n'; }
};
}  // namespace Sinks
}  // namespace Log
}  // namespace Raven