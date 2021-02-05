#pragma once

#include <vulkan/vulkan.hpp>

namespace Raven {
namespace Vulkan {
class Surface final {
 public:
  Surface(const std::shared_ptr<Instance>& instance, vk::SurfaceKHR surface);
  Surface(const Surface&) = delete;
  ~Surface();

  operator vk::SurfaceKHR();
  operator VkSurfaceKHR();

 private:
  std::shared_ptr<Instance>& mInstance;
  vk::SurfaceKHR mSurface;
};
}  // namespace Vulkan
}  // namespace Raven