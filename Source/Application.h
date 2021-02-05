#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "VulkanCore.h"

namespace Raven {
class Window;

class InstanceBuilder final {
 public:
  InstanceBuilder& SetAppName(const std::string& name) noexcept { mAppName = name; }

  InstanceBuilder& SetAppVersion(uint32_t major, uint32_t minor, uint32_t patch = 0) noexcept {
    mAppVersion = VK_MAKE_VERSION(major, minor, patch);
  }

  InstanceBuilder& RequestValidation(bool validation = true) noexcept {
    mRequestValidation = validation;
  }

  InstanceBuilder& SetApiVersion(uint32_t major, uint32_t minor) noexcept {
    mRequiredVersion = VK_MAKE_VERSION(major, minor, 0);
  }

  operator vk::InstanceCreateInfo() {
    mAppInfo = vk::ApplicationInfo(mAppName.c_str(), mAppVersion, "Raven", VK_MAKE_VERSION(1, 0, 0),
                                   mRequiredVersion);

    const auto availableLayers{vk::enumerateInstanceLayerProperties()};
    const auto availableExtensions{vk::enumerateInstanceExtensionProperties()};

    std::vector<const char*> enabledLayers;
    std::vector<const char*> enabledExtensions{VK_KHR_SURFACE_EXTENSION_NAME,
                                               VK_KHR_WIN32_SURFACE_EXTENSION_NAME};

    bool validation{false};
    if (mRequestValidation) {
      const std::vector<const char*> validationLayers{"VK_LAYER_KHRONOS_validation"};
      const std::vector<const char*> validationExtensions{VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
      bool fail{false};

      for (const auto& layerName : validationLayers) {
        bool found{false};
        for (const auto& layer : availableLayers) {
          if (strcmp(layerName, layer.layerName) == 0) {
            found = true;
            break;
          }
        }
        if (!found) {
          fail = true;
          Log::Warn("Missing required layer for validation: {}", layerName);
        }
      }

      for (const auto& extName : validationExtensions) {
        bool found{false};
        for (const auto& ext : availableExtensions) {
          if (strcmp(extName, ext.extensionName) == 0) {
            found = true;
            break;
          }
        }
        if (!found) {
          fail = true;
          Log::Warn("Missing required extension for validation: {}", extName);
        }
      }

      if (fail) {
        Log::Warn("Validation layers could not be enabled.");
      } else {
        enabledLayers.insert(enabledLayers.end(), validationLayers.begin(), validationLayers.end());
        enabledExtensions.insert(enabledExtensions.end(), validationExtensions.begin(),
                                 validationExtensions.end());
        validation = true;
      }
    }

    return vk::InstanceCreateInfo({}, &mAppInfo, enabledLayers, enabledExtensions);
  }

 private:
  vk::ApplicationInfo mAppInfo;
  std::string mAppName{"Raven"};
  uint32_t mAppVersion{VK_MAKE_VERSION(1, 0, 0)};
  bool mRequestValidation{false};
  uint32_t mRequiredVersion{VK_API_VERSION_1_0};
};

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
  void InitializeVulkan();
  void ShutdownVulkan();
  VkResult CreateInstance(const bool validation) noexcept;
  VkResult SelectPhysicalDevice() noexcept;
  VkResult CreateDevice() noexcept;
  void GetQueues() noexcept;
  VkResult CreateSwapchain() noexcept;

  void DestroySwapchain() noexcept;

  template <typename Type>
  void SetObjectName(const VkObjectType type, const Type handle, const char* name) noexcept {
    static PFN_vkSetDebugUtilsObjectNameEXT func{reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
        vkGetInstanceProcAddr(*mInstance, "vkSetDebugUtilsObjectNameEXT"))};
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
  vk::UniqueInstance mInstance;
  vk::UniqueSurfaceKHR mSurface;
  std::shared_ptr<Vulkan::PhysicalDevice> mPhysicalDevice;
  VkPhysicalDevice mPhysDev{VK_NULL_HANDLE};
  PhysicalDeviceInfo mDeviceInfo{};
  VkDevice mDevice{VK_NULL_HANDLE};
  VkQueue mGraphicsQueue{VK_NULL_HANDLE};
  VkQueue mPresentQueue{VK_NULL_HANDLE};
  VkQueue mTransferQueue{VK_NULL_HANDLE};
  VkQueue mComputeQueue{VK_NULL_HANDLE};
  VulkanSwapchain mSwapchain{};
};
}  // namespace Raven