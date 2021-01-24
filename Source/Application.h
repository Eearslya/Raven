#pragma once

#include <SDL.h>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

namespace Raven {
struct QueueFamilyInfo final {
  uint32_t Index{};
  VkQueueFamilyProperties Properties{};
  VkBool32 PresentSupport{VK_FALSE};

  constexpr bool Graphics() const { return Properties.queueFlags & VK_QUEUE_GRAPHICS_BIT; }
  constexpr bool Compute() const { return Properties.queueFlags & VK_QUEUE_COMPUTE_BIT; }
  constexpr bool Transfer() const { return Properties.queueFlags & VK_QUEUE_TRANSFER_BIT; }
  constexpr bool SparseBinding() const {
    return Properties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
  }
  constexpr bool Present() const { return PresentSupport; }
};

struct PhysicalDeviceInfo final {
  VkPhysicalDeviceFeatures Features{};
  VkPhysicalDeviceMemoryProperties MemoryProperties{};
  VkPhysicalDeviceProperties Properties{};
  std::vector<QueueFamilyInfo> QueueFamilies;
  std::vector<VkExtensionProperties> Extensions;

  std::optional<uint32_t> GraphicsIndex;
  std::optional<uint32_t> ComputeIndex;
  std::optional<uint32_t> TransferIndex;
  std::optional<uint32_t> PresentIndex;
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
  PhysicalDeviceInfo mDeviceInfo{};
  VkDevice mDevice{VK_NULL_HANDLE};
};
}  // namespace Raven