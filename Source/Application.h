#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <unordered_map>
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

struct GlobalPushConstants final {
  glm::mat4 ViewProjection;
  glm::mat4 Model;
};

struct VertexDescription final {
  std::vector<vk::VertexInputAttributeDescription> Attributes;
  std::vector<vk::VertexInputBindingDescription> Bindings;
};

struct Vertex {
  glm::vec3 Position;
  glm::vec3 Normal;

  static VertexDescription GetVertexDescription();
};

struct Mesh {
  Mesh(uint32_t count, Buffer&& buffer);

  uint32_t VertexCount;
  Buffer VertexBuffer;
};

struct Material {
  Material(std::shared_ptr<vk::UniquePipelineLayout> layout,
           std::shared_ptr<vk::UniquePipeline> pipeline);

  std::shared_ptr<vk::UniquePipeline> Pipeline;
  std::shared_ptr<vk::UniquePipelineLayout> Layout;
};

struct RenderObject {
  std::shared_ptr<Mesh> Mesh;
  std::shared_ptr<Material> Material;
  glm::mat4 Transform;
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

struct FrameData final {
  vk::UniqueSemaphore PresentSemaphore;
  vk::UniqueSemaphore RenderSemaphore;
  vk::UniqueFence RenderFence;
  vk::UniqueCommandPool CommandPool;
  vk::UniqueCommandBuffer MainCommandBuffer;
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
  Buffer CreateVertexBuffer(const std::vector<Vertex>& vertices);
  std::shared_ptr<Mesh> LoadMesh(const std::string& path);
  uint32_t FindMemoryType(uint32_t filter, vk::MemoryPropertyFlags properties);
  vk::Format FindFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
                        vk::FormatFeatureFlags features);
  vk::UniqueShaderModule CreateShaderModule(const std::string& path);
  std::shared_ptr<Material> CreateMaterial(const std::shared_ptr<vk::UniquePipelineLayout> layout,
                                           const std::shared_ptr<vk::UniquePipeline> pipeline,
                                           const std::string& name);
  std::shared_ptr<Material> GetMaterial(const std::string& name);
  std::shared_ptr<Mesh> GetMesh(const std::string& name);

  constexpr static const unsigned int FRAME_OVERLAP{2};
  bool mRunning{false};
  bool mValidation{true};
  uint64_t mCurrentFrame{0};
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
  FrameData mFrames[FRAME_OVERLAP];

  std::vector<RenderObject> mRenderables;
  std::unordered_map<std::string, std::shared_ptr<Material>> mMaterials;
  std::unordered_map<std::string, std::shared_ptr<Mesh>> mMeshes;
};
}  // namespace Raven