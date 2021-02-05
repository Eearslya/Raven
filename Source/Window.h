#pragma once

#include <memory>

namespace Raven {
namespace Vulkan {
class Instance;
class Surface;
}  // namespace Vulkan

class Window final {
 public:
  Window();
  Window(const Window&) = delete;
  ~Window();

  void Update() noexcept;
  vk::UniqueSurfaceKHR CreateSurface(const vk::Instance& instance) const;

 private:
  HWND mHandle;
};
}  // namespace Raven