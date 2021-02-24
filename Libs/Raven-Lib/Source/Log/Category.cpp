#include <Raven/Log/Category.hpp>

namespace Raven {
namespace Log {
std::string_view Category::Name() const noexcept { return mName; }

Level Category::MaxLevel() const noexcept { return mLevel; }

void Category::SetLevel(Level lvl) noexcept { mLevel = lvl; }

Category::Category(std::string_view name, Level lvl) : mName(name), mLevel(lvl) {}
}  // namespace Log
}  // namespace Raven