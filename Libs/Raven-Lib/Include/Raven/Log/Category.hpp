#pragma once

#include <Raven/Lib_Export.h>

#include <Raven/Log/Level.hpp>
#include <string_view>

namespace Raven {
namespace Log {
class Category {
 public:
  RAVEN_LIB_EXPORT std::string_view Name() const noexcept;
  RAVEN_LIB_EXPORT Level MaxLevel() const noexcept;
  RAVEN_LIB_EXPORT void SetLevel(Level lvl) noexcept;

 protected:
  RAVEN_LIB_EXPORT Category(std::string_view name, Level lvl);

 private:
  const std::string_view mName;
  Level mLevel;
};
}  // namespace Log
}  // namespace Raven