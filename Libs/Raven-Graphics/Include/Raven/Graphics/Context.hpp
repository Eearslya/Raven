#pragma once

#include <Raven/Graphics_Export.h>

#include <Raven/Pimpl.hpp>
#include <string>

namespace Raven {
namespace Graphics {
class ContextImpl;

class GraphicsContext final {
 public:
  GraphicsContext(const std::string& appName, uint32_t appVersion);

 private:
  rvn::UniqueImpl<ContextImpl> mImpl;
};
}  // namespace Graphics
}  // namespace Raven