#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vector>

#include "VulkanCore.h"

namespace Raven {
class Window;

struct GlobalConstantBuffer {
  glm::mat4 ViewProjection;
};

struct Vertex {
  glm::vec3 Position;
  glm::vec3 Normal;
};

class Buffer {
 public:
  Buffer(vk::UniqueBuffer buffer, vk::UniqueDeviceMemory memory, vk::DeviceSize size);
  Buffer(const Buffer&) = delete;
  Buffer(Buffer&& o);
  ~Buffer();

  vk::UniqueBuffer Handle;
  vk::UniqueDeviceMemory Memory;
  vk::DeviceSize Size;
};

struct Mesh {
  Buffer VertexBuffer;
  uint64_t VertexCount;

  Mesh(Buffer buffer, uint64_t vertices);
  Mesh(const Mesh&) = delete;
  ~Mesh();
};

class InstanceBuilder final {
 public:
  InstanceBuilder& SetAppName(const std::string& name) noexcept {
    mAppName = name;

    return *this;
  }

  InstanceBuilder& SetAppVersion(uint32_t major, uint32_t minor, uint32_t patch = 0) noexcept {
    mAppVersion = VK_MAKE_VERSION(major, minor, patch);

    return *this;
  }

  InstanceBuilder& RequestValidation(bool validation = true) noexcept {
    mRequestValidation = validation;

    return *this;
  }

  InstanceBuilder& SetApiVersion(uint32_t major, uint32_t minor) noexcept {
    mRequiredVersion = VK_MAKE_VERSION(major, minor, 0);

    return *this;
  }

  operator vk::InstanceCreateInfo() {
    mAppInfo = vk::ApplicationInfo(mAppName.c_str(), mAppVersion, "Raven", VK_MAKE_VERSION(1, 0, 0),
                                   mRequiredVersion);

    const auto availableLayers{vk::enumerateInstanceLayerProperties()};
    const auto availableExtensions{vk::enumerateInstanceExtensionProperties()};

    mLayers.clear();
    mExtensions.clear();
    mExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    mExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

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
        mLayers.insert(mLayers.end(), validationLayers.begin(), validationLayers.end());
        mExtensions.insert(mExtensions.end(), validationExtensions.begin(),
                           validationExtensions.end());
        validation = true;
      }
    }

    return vk::InstanceCreateInfo({}, &mAppInfo, mLayers, mExtensions);
  }

 private:
  vk::ApplicationInfo mAppInfo;
  std::string mAppName{"Raven"};
  uint32_t mAppVersion{VK_MAKE_VERSION(1, 0, 0)};
  bool mRequestValidation{false};
  uint32_t mRequiredVersion{VK_API_VERSION_1_0};

  std::vector<const char*> mLayers;
  std::vector<const char*> mExtensions;
};

struct VulkanSwapchain final {
  vk::UniqueSwapchainKHR Swapchain;
  uint32_t ImageCount{0};
  vk::Extent2D Extent{0, 0};
  std::vector<vk::Image> Images;
  std::vector<vk::UniqueImageView> ImageViews;
  std::vector<vk::UniqueFramebuffer> Framebuffers;
};

struct QueueFamilyInfo final {
  uint32_t Index{};
  vk::QueueFamilyProperties Properties{};
  vk::Bool32 PresentSupport{false};

  bool Graphics() const {
    return static_cast<bool>(Properties.queueFlags & vk::QueueFlagBits::eGraphics);
  }
  bool Compute() const {
    return static_cast<bool>(Properties.queueFlags & vk::QueueFlagBits::eCompute);
  }
  bool Transfer() const {
    return static_cast<bool>(Properties.queueFlags & vk::QueueFlagBits::eTransfer);
  }
  bool SparseBinding() const {
    return static_cast<bool>(Properties.queueFlags & vk::QueueFlagBits::eSparseBinding);
  }
  constexpr bool Present() const { return PresentSupport; }
};

struct PhysicalDeviceInfo final {
  vk::PhysicalDeviceFeatures Features;
  vk::PhysicalDeviceMemoryProperties MemoryProperties;
  vk::PhysicalDeviceProperties Properties;
  std::vector<QueueFamilyInfo> QueueFamilies;
  std::vector<vk::ExtensionProperties> Extensions;
  vk::SurfaceCapabilitiesKHR SurfaceCapabilities;
  vk::SurfaceFormatKHR OptimalSwapchainFormat;
  vk::PresentModeKHR OptimalPresentMode;

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
  void CreateRenderPass();
  void CreateFramebuffers();
  void CreatePipeline();
  void CreateCommandPools();
  void CreateCommandBuffers();
  void CreateSyncObjects();
  void CreateScene();

  Buffer CreateBuffer(const vk::DeviceSize size, vk::BufferUsageFlags usage,
                      vk::MemoryPropertyFlags memoryType);
  Buffer CreateVertexBuffer(const std::vector<Vertex>& vertices);
  uint32_t FindMemoryType(uint32_t filter, vk::MemoryPropertyFlags properties);
  vk::UniqueShaderModule CreateShaderModule(const std::string& path);
  std::shared_ptr<Mesh> LoadMesh(const std::string& path);

  void DestroySwapchain() noexcept;

  void Render();

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
  vk::DynamicLoader mDynamicLoader;
  vk::UniqueInstance mInstance;
  vk::UniqueDebugUtilsMessengerEXT mDebugMessenger;
  vk::UniqueSurfaceKHR mSurface;
  vk::PhysicalDevice mPhysicalDevice;
  PhysicalDeviceInfo mDeviceInfo{};
  vk::UniqueDevice mDevice;
  vk::Queue mGraphicsQueue;
  vk::Queue mPresentQueue;
  vk::Queue mTransferQueue;
  vk::Queue mComputeQueue;
  VulkanSwapchain mSwapchain{};
  vk::UniqueRenderPass mRenderPass;
  vk::UniquePipelineLayout mPipelineLayout;
  vk::UniquePipelineLayout mTriPipelineLayout;
  vk::UniquePipeline mBgPipeline;
  vk::UniquePipeline mTriPipeline;
  vk::UniqueCommandPool mGraphicsPool;
  std::vector<vk::UniqueCommandBuffer> mGraphicsBuffers;
  vk::UniqueSemaphore mPresentSemaphore;
  vk::UniqueSemaphore mRenderSemaphore;
  vk::UniqueFence mRenderFence;

  std::vector<std::shared_ptr<Mesh>> mMeshes;
};
}  // namespace Raven