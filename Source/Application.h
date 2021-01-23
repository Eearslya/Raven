#pragma once

#include <SDL.h>
#include <vector>
#include <vulkan/vulkan.h>

namespace Raven {
struct QueueFamilyInfo final {
  uint32_t Index{};
  VkQueueFamilyProperties Properties{};
};

struct PhysicalDeviceInfo final {
  VkPhysicalDeviceFeatures Features{};
  VkPhysicalDeviceMemoryProperties MemoryProperties{};
  VkPhysicalDeviceProperties Properties{};
  std::vector<QueueFamilyInfo> QueueFamilies;
};

class Application final {
 public:
  Application(const std::vector<const char*>& cmdArgs);
  ~Application();

 private:
  void InitializeSDL();
  void ShutdownSDL();
  void InitializeVulkan();
  void ShutdownVulkan();
  VkResult CreateInstance(const bool validation) noexcept;
  VkResult CreateDebugger() noexcept;
  VkResult CreateSurface() noexcept;
  VkResult SelectPhysicalDevice() noexcept;
  void EnumeratePhysicalDevice(VkPhysicalDevice device, PhysicalDeviceInfo& info) noexcept;

  void SetObjectName(const VkObjectType type, const uint64_t handle, const char* name) noexcept;

  bool mValidation{true};
  SDL_Window* mWindow;
  VkInstance mInstance{VK_NULL_HANDLE};
  VkDebugUtilsMessengerEXT mDebugger{VK_NULL_HANDLE};
  VkSurfaceKHR mSurface{VK_NULL_HANDLE};
  VkPhysicalDevice mPhysicalDevice{VK_NULL_HANDLE};
  VkDevice mDevice{VK_NULL_HANDLE};
};
}  // namespace Raven