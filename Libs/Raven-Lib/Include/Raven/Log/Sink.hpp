#pragma once

#include <Raven/Lib_Export.h>

#include <Raven/Log/Category.hpp>
#include <Raven/Log/Level.hpp>
#include <string_view>

namespace Raven {
namespace Log {
class Sink {
 public:
  virtual ~Sink() = default;

  virtual void Output(const std::string_view& msg) = 0;
};
}  // namespace Log
}  // namespace Raven