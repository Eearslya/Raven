#pragma once

#include <memory>
#include <string>

#include "VulkanCore.h"
#include "Win32.h"

namespace Raven {
namespace Vulkan {
class Instance final {
 public:
  Instance(vk::Instance instance, bool validation = false);
  Instance(const Instance&) = delete;
  ~Instance();

  bool HasValidation() const noexcept;
  vk::SurfaceKHR CreateSurface(HINSTANCE instance, HWND window) const;
  void DestroySurface(const vk::SurfaceKHR& surface) const noexcept;

  operator vk::Instance() const;
  operator VkInstance() const;

  static std::shared_ptr<Instance> Create(const vk::InstanceCreateInfo& createInfo,
                                          bool validation);

 private:
  bool mValidation{false};
  vk::Instance mInstance;
  vk::DebugUtilsMessengerEXT mDebugMessenger;
};

class InstanceBuilder final {
 public:
  InstanceBuilder& SetAppName(const std::string& name) noexcept;
  InstanceBuilder& SetAppVersion(uint32_t major, uint32_t minor, uint32_t patch = 0) noexcept;
  InstanceBuilder& RequestValidation() noexcept;
  InstanceBuilder& SetApiVersion(uint32_t major, uint32_t minor) noexcept;

  std::shared_ptr<Instance> Build() const;

 private:
  std::string mAppName{"Raven"};
  uint32_t mAppVersion{VK_MAKE_VERSION(1, 0, 0)};
  bool mRequestValidation{false};
  uint32_t mRequiredVersion{VK_API_VERSION_1_0};
};
}  // namespace Vulkan
}  // namespace Raven