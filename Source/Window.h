#pragma once

#include <vulkan/vulkan.h>

namespace Raven {
class Window final {
 public:
  Window();
  Window(const Window&) = delete;
  ~Window();

  void Update() noexcept;
  VkSurfaceKHR CreateSurface(VkInstance instance) const;

 private:
  HWND mHandle;
};
}  // namespace Raven