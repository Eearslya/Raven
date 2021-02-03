#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

#include "VulkanCore.h"

namespace Raven {
class Window;

struct VulkanSwapchain final {
  VkSwapchainKHR Swapchain{VK_NULL_HANDLE};
  uint32_t ImageCount{0};
  VkExtent2D Extent{0, 0};
  std::vector<VkImage> Images;
  std::vector<VkImageView> ImageViews;
};

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
  VkSurfaceCapabilitiesKHR SurfaceCapabilities{};
  VkSurfaceFormatKHR OptimalSwapchainFormat{};
  VkPresentModeKHR OptimalPresentMode{VK_PRESENT_MODE_FIFO_KHR};

  std::optional<uint32_t> GraphicsIndex;
  std::optional<uint32_t> ComputeIndex;
  std::optional<uint32_t> TransferIndex;
  std::optional<uint32_t> PresentIndex;
};

class Application final {
 public:
  Application(const std::vector<const char*>& cmdArgs);
  ~Application();

  void Run();

 private:
  void InitializeSDL();
  void ShutdownSDL();
  void InitializeVulkan();
  void ShutdownVulkan();
  VkResult CreateInstance(const bool validation) noexcept;
  VkResult CreateDebugger() noexcept;
  VkResult CreateSurface() noexcept;
  VkResult SelectPhysicalDevice() noexcept;
  VkResult CreateDevice() noexcept;
  void GetQueues() noexcept;
  VkResult CreateSwapchain() noexcept;

  void DestroySwapchain() noexcept;

  template <typename Type>
  void SetObjectName(const VkObjectType type, const Type handle, const char* name) noexcept {
    static PFN_vkSetDebugUtilsObjectNameEXT func{reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
        vkGetInstanceProcAddr(mInstance, "vkSetDebugUtilsObjectNameEXT"))};
    if (func && mValidation) {
      const VkDebugUtilsObjectNameInfoEXT nameInfo{
          VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,  // sType
          nullptr,                                             // pNext
          type,                                                // objectType
          reinterpret_cast<uint64_t>(handle),                  // objectHandle
          name                                                 // pObjectName
      };
      Log::Trace("[SetObjectName] {} <{}> set to \"{}\"", gVkObjectTypes.find(type)->second,
                 reinterpret_cast<void*>(handle), name);
      func(mDevice, &nameInfo);
    }
  }

  bool mRunning{false};
  bool mValidation{true};
  std::shared_ptr<Window> mWindow;
  VkInstance mInstance{VK_NULL_HANDLE};
  VkDebugUtilsMessengerEXT mDebugger{VK_NULL_HANDLE};
  VkSurfaceKHR mSurface{VK_NULL_HANDLE};
  VkPhysicalDevice mPhysicalDevice{VK_NULL_HANDLE};
  PhysicalDeviceInfo mDeviceInfo{};
  VkDevice mDevice{VK_NULL_HANDLE};
  VkQueue mGraphicsQueue{VK_NULL_HANDLE};
  VkQueue mPresentQueue{VK_NULL_HANDLE};
  VkQueue mTransferQueue{VK_NULL_HANDLE};
  VkQueue mComputeQueue{VK_NULL_HANDLE};
  VulkanSwapchain mSwapchain{};
};
}  // namespace Raven