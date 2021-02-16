#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vector>

#include "VulkanCore.h"

namespace Raven {
class Window;

class Buffer {
 public:
  Buffer() {}
  Buffer(vk::UniqueBuffer buffer, vk::UniqueDeviceMemory memory, vk::DeviceSize size);
  Buffer(const Buffer&) = delete;
  Buffer(Buffer&& o);
  Buffer& operator=(Buffer&& o);
  ~Buffer();

  vk::UniqueBuffer Handle;
  vk::UniqueDeviceMemory Memory;
  vk::DeviceSize Size;
};

struct VulkanSwapchain final {
  vk::UniqueSwapchainKHR Swapchain;
  uint32_t ImageCount{0};
  vk::Extent2D Extent{0, 0};
  std::vector<vk::Image> Images;
  std::vector<vk::UniqueImageView> ImageViews;
  std::vector<vk::UniqueFramebuffer> Framebuffers;
  vk::Format DepthFormat;
  vk::UniqueImage DepthImage;
  vk::UniqueDeviceMemory DepthMemory;
  vk::UniqueImageView DepthImageView;
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
  void Render();

  void InitializeVulkan();
  void ShutdownVulkan();
  void SelectPhysicalDevice();
  void CreateDevice();
  void GetQueues() noexcept;
  void CreateSwapchain();
  void DestroySwapchain() noexcept;
  void CreateRenderPass();
  void CreateFramebuffers();
  void CreatePipeline();
  void CreateCommandPools();
  void CreateCommandBuffers();
  void CreateSyncObjects();
  void CreateScene();

  Buffer CreateBuffer(const vk::DeviceSize size, vk::BufferUsageFlags usage,
                      vk::MemoryPropertyFlags memoryType);
  uint32_t FindMemoryType(uint32_t filter, vk::MemoryPropertyFlags properties);
  vk::Format FindFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
                        vk::FormatFeatureFlags features);
  vk::UniqueShaderModule CreateShaderModule(const std::string& path);

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
  vk::UniquePipeline mBgPipeline;
  vk::UniqueCommandPool mGraphicsPool;
  std::vector<vk::UniqueCommandBuffer> mGraphicsBuffers;
  vk::UniqueSemaphore mPresentSemaphore;
  vk::UniqueSemaphore mRenderSemaphore;
  vk::UniqueFence mRenderFence;
};
}  // namespace Raven