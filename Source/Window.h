#pragma once

#include <memory>
#include <string>

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

  bool Update() noexcept;
  uint32_t Width() const noexcept;
  uint32_t Height() const noexcept;
  void SetTitle(const std::string& title) noexcept;
  vk::UniqueSurfaceKHR CreateSurface(const vk::Instance& instance) const;

 private:
  HWND mHandle;
  uint32_t mWidth;
  uint32_t mHeight;
};
}  // namespace Raven