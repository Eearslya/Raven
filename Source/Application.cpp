#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "Application.h"
#include "Core.h"

#include <chrono>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <tiny_gltf.h>

#include "VulkanCore.h"
#include "Window.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace Raven {
/* ==========================================================================================
 * Vulkan Debug Callbacks and Info
 * ========================================================================================== */

VkBool32 VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                             VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                             void* pUserData) {
  std::string msgType{""};
  if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
    msgType += "GEN";
  }
  if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
    if (msgType.size() > 0) {
      msgType += "|";
    }
    msgType += "PRF";
  }
  if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
    if (msgType.size() > 0) {
      msgType += "|";
    }
    msgType += "VLD";
  }

  const char* msg{pCallbackData->pMessage};
  std::string customMsg{""};
  // Attempt to perform additional parsing to clean up validation messages.
  if (messageTypes == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
    std::string layerMsg{msg};
    const size_t firstPipe{layerMsg.find("|", 0)};
    if (firstPipe != std::string::npos) {
      const size_t secondPipe{layerMsg.find("|", firstPipe + 1)};
      if (secondPipe != std::string::npos) {
        std::string severity{""};
        switch (messageSeverity) {
          case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            severity = "Error";
            break;
          case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            severity = "Warning";
            break;
          case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            severity = "Info";
            break;
          case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            severity = "Verbose";
            break;
        }
        customMsg = fmt::format("Validation {}: {}\n - {}", severity, pCallbackData->pMessageIdName,
                                layerMsg.substr(secondPipe + 2));

        if (pCallbackData->objectCount > 0) {
          customMsg += "\n - Related Objects:";
          for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
            const VkDebugUtilsObjectNameInfoEXT& objData{pCallbackData->pObjects[i]};

            const vk::ObjectType objType{objData.objectType};
            const void* objHandle{reinterpret_cast<void*>(objData.objectHandle)};
            const char* objName{objData.pObjectName};

            customMsg +=
                fmt::format("\n   - Object {}: {} <{}>", i, vk::to_string(objType), objHandle);
            if (objName) {
              customMsg += fmt::format(" \"{}\"", objName);
            }
          }
        }

        msg = customMsg.c_str();
      }
    }
  }

  switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      Log::Error("[Vulkan-ERR-{}] {}", msgType, msg);
      __debugbreak();
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      Log::Warn("[Vulkan-WRN-{}] {}", msgType, msg);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        Log::Debug("[Vulkan-INF-{}] {}", msgType, msg);
      } else {
        Log::Trace("[Vulkan-INF-{}] {}", msgType, msg);
      }
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      Log::Trace("[Vulkan-VRB-{}] {}", msgType, msg);
      break;
  }

  return VK_FALSE;
}

constexpr const vk::DebugUtilsMessengerCreateInfoEXT gDebugMessengerCI(
    {},
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose,
    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
    VulkanDebugCallback, nullptr);

/* ==========================================================================================
 * Local Helper Classes
 * ========================================================================================== */

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

class PipelineBuilder final {
 public:
  PipelineBuilder(vk::Extent2D extent) {
    InputAssembly =
        vk::PipelineInputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleList, false);

    Viewport = vk::Viewport(0, 0, extent.width, extent.height, 0.0f, 1.0f);
    Scissor = vk::Rect2D({0, 0}, extent);

    Rasterizer = vk::PipelineRasterizationStateCreateInfo(
        {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone,
        vk::FrontFace::eCounterClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);

    Multisampling = vk::PipelineMultisampleStateCreateInfo();

    DepthStencil = vk::PipelineDepthStencilStateCreateInfo();

    ColorAttachment = vk::PipelineColorBlendAttachmentState(
        true, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
        vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    ColorBlending =
        vk::PipelineColorBlendStateCreateInfo({}, false, vk::LogicOp::eCopy, ColorAttachment);
  }

  operator vk::GraphicsPipelineCreateInfo() {
    ViewportState = vk::PipelineViewportStateCreateInfo({}, Viewport, Scissor);

    return vk::GraphicsPipelineCreateInfo({}, ShaderStages, &VertexInput, &InputAssembly,
                                          &Tesselation, &ViewportState, &Rasterizer, &Multisampling,
                                          &DepthStencil, &ColorBlending, {}, Layout, RenderPass, 0);
  }

  PipelineBuilder& AddShader(vk::ShaderStageFlagBits stage, vk::ShaderModule& shader) {
    ShaderStages.push_back(vk::PipelineShaderStageCreateInfo({}, stage, shader, "main"));

    return *this;
  }

  PipelineBuilder& ClearShaders() {
    ShaderStages.clear();

    return *this;
  }

  std::vector<vk::PipelineShaderStageCreateInfo> ShaderStages;
  vk::PipelineVertexInputStateCreateInfo VertexInput;
  vk::PipelineInputAssemblyStateCreateInfo InputAssembly;
  vk::PipelineTessellationStateCreateInfo Tesselation;
  vk::Viewport Viewport;
  vk::Rect2D Scissor;
  vk::PipelineViewportStateCreateInfo ViewportState;
  vk::PipelineRasterizationStateCreateInfo Rasterizer;
  vk::PipelineMultisampleStateCreateInfo Multisampling;
  vk::PipelineDepthStencilStateCreateInfo DepthStencil;
  vk::PipelineColorBlendAttachmentState ColorAttachment;
  vk::PipelineColorBlendStateCreateInfo ColorBlending;
  vk::PipelineLayout Layout;
  vk::RenderPass RenderPass;
};

/* ==========================================================================================
 * Public Application Methods
 * ========================================================================================== */

Application::Application(const std::vector<const char*>& cmdArgs) {
  // TODO: Command line arguments?
  Log::Info("Raven is starting...");
  mValidation = true;
  mWindow = std::make_shared<Window>();
  InitializeVulkan();
}

Application::~Application() {
  Log::Info("Raven is shutting down...");
  ShutdownVulkan();
}

void Application::Run() {
  mRunning = true;

  auto startTime{std::chrono::high_resolution_clock::now()};
  float msAcc{0.0f};
  uint64_t sampleCount{0};
  while (mRunning) {
    auto endTime{std::chrono::high_resolution_clock::now()};
    auto deltaUs{
        std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count()};
    startTime = endTime;
    auto deltaMs{deltaUs / 1000.0f};
    msAcc += deltaMs;
    sampleCount++;

    if (msAcc > 1000.0f) {
      const float msAvg{msAcc / sampleCount};
      const std::string title{
          fmt::format("Raven - {:.2f}ms ({} FPS)", msAvg, static_cast<uint32_t>(1000.0f / msAvg))};
      mWindow->SetTitle(title);
      msAcc = 0.0f;
      sampleCount = 0;
    }

    Render();

    mRunning = mWindow->Update();
  }
}

/* ==========================================================================================
 * Private Application Methods
 * ========================================================================================== */

void Application::Render() {
  mDevice->waitForFences(*mRenderFence, true, std::numeric_limits<uint64_t>::max());
  mDevice->resetFences(*mRenderFence);

  uint32_t imageIndex{mDevice->acquireNextImageKHR(
      *mSwapchain.Swapchain, std::numeric_limits<uint64_t>::max(), *mPresentSemaphore)};

  const vk::UniqueCommandBuffer& cmd{mGraphicsBuffers[imageIndex]};

  const vk::CommandBufferBeginInfo beginInfo;
  cmd->begin(beginInfo);

  const std::array<float, 4> clearColor{0.0f, 0.0f, 1.0f, 1.0f};
  const std::vector<vk::ClearValue> clearValues{vk::ClearColorValue(clearColor),
                                                vk::ClearDepthStencilValue(1.0f, 0)};
  const vk::RenderPassBeginInfo rpInfo(*mRenderPass, *mSwapchain.Framebuffers[imageIndex],
                                       {{0, 0}, mSwapchain.Extent}, clearValues);
  cmd->beginRenderPass(rpInfo, vk::SubpassContents::eInline);

  cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *mBgPipeline);
  cmd->draw(3, 1, 0, 0);

  cmd->endRenderPass();

  cmd->end();

  const std::vector<vk::Semaphore> waitSemaphores{*mPresentSemaphore};
  const std::vector<vk::Semaphore> signalSemaphores{*mRenderSemaphore};
  const std::vector<vk::PipelineStageFlags> waitStages{
      vk::PipelineStageFlagBits::eColorAttachmentOutput};
  const std::vector<vk::CommandBuffer> cmdBuffers{*cmd};
  const vk::SubmitInfo submitInfo(waitSemaphores, waitStages, cmdBuffers, signalSemaphores);
  mGraphicsQueue.submit(submitInfo, *mRenderFence);

  const vk::PresentInfoKHR presentInfo(signalSemaphores, *mSwapchain.Swapchain, imageIndex);
  mGraphicsQueue.presentKHR(presentInfo);
}

/* ==========================================================================================
 * Application/Vulkan Setup
 * ========================================================================================== */

void Application::InitializeVulkan() {
  PFN_vkGetInstanceProcAddr loader{
      mDynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr")};
  VULKAN_HPP_DEFAULT_DISPATCHER.init(loader);

  InstanceBuilder builder;
  builder.SetAppName("Raven").SetAppVersion(1, 0).SetApiVersion(1, 2).RequestValidation(
      mValidation);
  const vk::InstanceCreateInfo instanceCI{builder};
  const vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
      instanceChainCI{instanceCI, gDebugMessengerCI};
  mInstance = vk::createInstanceUnique(instanceChainCI.get());
  Log::Debug("[InitializeVulkan] Vulkan Instance created.");
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*mInstance);

  mDebugMessenger = mInstance->createDebugUtilsMessengerEXTUnique(gDebugMessengerCI);
  Log::Debug("[InitializeVulkan] Vulkan Debugger created. <{}>",
             static_cast<void*>(*mDebugMessenger));

  mSurface = mWindow->CreateSurface(*mInstance);
  Log::Debug("[InitializeVulkan] Vulkan Surface created. <{}>", static_cast<void*>(*mSurface));

  SelectPhysicalDevice();
  Log::Debug("[InitializeVulkan] Selected physical device: {}", mDeviceInfo.Properties.deviceName);

  CreateDevice();
  Log::Debug("[InitializeVulkan] Vulkan Device created. <{}>", static_cast<void*>(*mDevice));
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*mDevice);

  GetQueues();
  Log::Debug("[InitializeVulkan] Device queues retrieved.");

  CreateSwapchain();
  Log::Debug("[InitializeVulkan] Vulkan Swapchain created with {} images. <{}>",
             mSwapchain.ImageCount, static_cast<void*>(*mSwapchain.Swapchain));

  CreateRenderPass();
  Log::Debug("[InitializeVulkan] Vulkan Render Pass created. <{}>",
             static_cast<void*>(*mRenderPass));

  CreateFramebuffers();
  Log::Debug("[InitializeVulkan] Vulkan Framebuffers created.");

  CreatePipeline();
  Log::Debug("[InitializeVulkan] Vulkan Pipeline created. <{}>", static_cast<void*>(*mBgPipeline));

  CreateCommandPools();
  Log::Debug("[InitializeVulkan] Vulkan Command Pools created.");

  CreateCommandBuffers();
  Log::Debug("[InitializeVulkan] Vulkan Command Buffers created.");

  CreateSyncObjects();
  Log::Debug("[InitializeVulkan] Vulkan Sync Objects created.");

  CreateScene();
}

void Application::ShutdownVulkan() { mDevice->waitIdle(); }

void Application::SelectPhysicalDevice() {
  const std::vector<vk::PhysicalDevice> physicalDevices{mInstance->enumeratePhysicalDevices()};
  const size_t physicalDeviceCount{physicalDevices.size()};

  Log::Debug("[SelectPhysicalDevice] Found {} Vulkan devices.", physicalDeviceCount);
  std::vector<PhysicalDeviceInfo> deviceInfos(physicalDeviceCount);

  std::vector<uint64_t> deviceScores(physicalDeviceCount, 0);

  for (uint32_t i = 0; i < physicalDeviceCount; i++) {
    const vk::PhysicalDevice& device{physicalDevices[i]};
    PhysicalDeviceInfo& info{deviceInfos[i]};
    uint64_t& score{deviceScores[i]};

    // Fill device info
    {
      // Standard device info
      info.Features = device.getFeatures();
      info.MemoryProperties = device.getMemoryProperties();
      info.Properties = device.getProperties();
      info.SurfaceCapabilities = device.getSurfaceCapabilitiesKHR(*mSurface);
      info.Extensions = device.enumerateDeviceExtensionProperties();

      // Queue families
      {
        const auto queueFamilies{device.getQueueFamilyProperties()};
        const size_t queueCount{queueFamilies.size()};
        info.QueueFamilies.resize(queueCount);

        for (uint32_t i = 0; i < queueCount; i++) {
          QueueFamilyInfo& q{info.QueueFamilies[i]};
          q.Index = i;
          q.Properties = queueFamilies[i];
          q.PresentSupport = device.getSurfaceSupportKHR(i, *mSurface);
        }
      }

      // Determine queue assignments
      {
        // Graphics
        for (const auto& family : info.QueueFamilies) {
          if (family.Graphics()) {
            info.GraphicsIndex = family.Index;
            if (family.Present()) {
              info.PresentIndex = family.Index;
            }
            break;
          }
        }

        // Transfer
        for (const auto& family : info.QueueFamilies) {
          // Attempt to find a separate queue from Graphics
          if (family.Transfer() && family.Index != info.GraphicsIndex) {
            info.TransferIndex = family.Index;
            break;
          }
        }
        // If unable to find a separate queue, use the same one.
        // By definition, all graphics queues are required to support Transfer,
        // so we don't need to verify.
        if (!info.TransferIndex.has_value()) {
          info.TransferIndex = info.GraphicsIndex;
        }

        // Compute
        // First attempt to find a separate queue from Graphics.
        for (const auto& family : info.QueueFamilies) {
          if (family.Compute() && family.Index != info.GraphicsIndex) {
            info.ComputeIndex = family.Index;
            break;
          }
        }
        // If that fails, find any available compute queue.
        if (!info.ComputeIndex.has_value()) {
          for (const auto& family : info.QueueFamilies) {
            if (family.Compute()) {
              info.ComputeIndex = family.Index;
              break;
            }
          }
        }
      }

      // Determine Swapchain info
      {
        const auto formats{device.getSurfaceFormatsKHR(*mSurface)};
        const size_t formatCount{formats.size()};

        if (formatCount == 1 && formats[0].format == vk::Format::eUndefined) {
          info.OptimalSwapchainFormat.format = vk::Format::eB8G8R8A8Unorm;
          info.OptimalSwapchainFormat.colorSpace = formats[0].colorSpace;
        } else {
          for (const auto& fmt : formats) {
            if (fmt.format == vk::Format::eB8G8R8A8Unorm) {
              info.OptimalSwapchainFormat = fmt;
              break;
            }
          }
        }

        if (info.OptimalSwapchainFormat.format == vk::Format::eUndefined) {
          info.OptimalSwapchainFormat = formats[0];
        }

        const auto presentModes{device.getSurfacePresentModesKHR(*mSurface)};
        const size_t presentModeCount{presentModes.size()};

        for (const auto& pm : presentModes) {
          if (pm == vk::PresentModeKHR::eMailbox) {
            info.OptimalPresentMode = pm;
            break;
          } else if (pm == vk::PresentModeKHR::eImmediate) {
            info.OptimalPresentMode = pm;
          }
        }
      }
    }

    // Dump device info to log
    {
      Log::Trace("[SelectPhysicalDevice] - Physical Device {}: \"{}\"", i,
                 info.Properties.deviceName);

      // Device Type
      {
        switch (info.Properties.deviceType) {
          case vk::PhysicalDeviceType::eOther:
            Log::Trace("[SelectPhysicalDevice]   - Type: Other");
            break;
          case vk::PhysicalDeviceType::eIntegratedGpu:
            Log::Trace("[SelectPhysicalDevice]   - Type: Integrated GPU");
            break;
          case vk::PhysicalDeviceType::eDiscreteGpu:
            Log::Trace("[SelectPhysicalDevice]   - Type: Discrete GPU");
            break;
          case vk::PhysicalDeviceType::eVirtualGpu:
            Log::Trace("[SelectPhysicalDevice]   - Type: Virtual GPU");
            break;
          case vk::PhysicalDeviceType::eCpu:
            Log::Trace("[SelectPhysicalDevice]   - Type: CPU");
            break;
        }
      }

      // Swapchain Properties
      {
        Log::Trace("[SelectPhysicalDevice]   - Optimal Image Format: {}",
                   vk::to_string(info.OptimalSwapchainFormat.format));
      }

      // Memory Properties
      {
        Log::Trace("[SelectPhysicalDevice]   - Memory Properties:");
        Log::Trace("[SelectPhysicalDevice]     - Heaps:");
        for (uint32_t i = 0; i < info.MemoryProperties.memoryHeapCount; i++) {
          const vk::MemoryHeap& heap{info.MemoryProperties.memoryHeaps[i]};
          const float heapMB{heap.size / 1024.0f / 1024.0f};
          Log::Trace("[SelectPhysicalDevice]       - Heap {}: {:.0f}MB{}", i, heapMB,
                     heap.flags & vk::MemoryHeapFlagBits::eDeviceLocal ? ", Device Local" : "");
        }
        Log::Trace("[SelectPhysicalDevice]     - Types:");
        for (uint32_t i = 0; i < info.MemoryProperties.memoryTypeCount; i++) {
          const vk::MemoryType& type{info.MemoryProperties.memoryTypes[i]};

          std::vector<std::string> flags;
          if (type.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) {
            flags.emplace_back("Device Local");
          }
          if (type.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible) {
            flags.emplace_back("Host Visible");
          }
          if (type.propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent) {
            flags.emplace_back("Host Coherent");
          }
          if (type.propertyFlags & vk::MemoryPropertyFlagBits::eHostCached) {
            flags.emplace_back("Host Cached");
          }
          if (flags.size() == 0) {
            flags.emplace_back("No flags");
          }

          Log::Trace("[SelectPhysicalDevice]       - Type {}: {} on Heap {}", i,
                     fmt::join(flags, ", "), type.heapIndex);
        }
      }

      // Device Queues
      {
        Log::Trace("[SelectPhysicalDevice]   - Device Queue Families:");
        for (const auto& family : info.QueueFamilies) {
          std::vector<std::string> caps;
          if (family.Graphics()) {
            caps.push_back(std::string("Graphics") +
                           (family.Index == info.GraphicsIndex ? "*" : ""));
          }
          if (family.Compute()) {
            caps.push_back(std::string("Compute") + (family.Index == info.ComputeIndex ? "*" : ""));
          }
          if (family.Transfer()) {
            caps.push_back(std::string("Transfer") +
                           (family.Index == info.TransferIndex ? "*" : ""));
          }
          if (family.SparseBinding()) {
            caps.push_back("Sparse Binding");
          }
          if (family.Present()) {
            caps.push_back(std::string("Present") + (family.Index == info.PresentIndex ? "*" : ""));
          }

          Log::Trace("[SelectPhysicalDevice]     - Family {}: {} Queues <{}>", family.Index,
                     family.Properties.queueCount, fmt::join(caps, ", "));
        }
      }

      // Extensions
      {
        Log::Trace("[SelectPhysicalDevice]   - Device Extensions:");
        for (const auto& ext : info.Extensions) {
          Log::Trace("[SelectPhysicalDevice]     - {} v{}", ext.extensionName, ext.specVersion);
        }
      }
    }

    // Score device
    {
      if (info.Properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
        score += 10000;
      }
      score += info.Properties.limits.maxImageDimension2D;
    }
  }

  uint64_t bestScore{0};
  size_t bestDevice{std::numeric_limits<size_t>::max()};
  for (uint32_t i = 0; i < physicalDeviceCount; i++) {
    if (deviceScores[i] > bestScore) {
      bestScore = deviceScores[i];
      bestDevice = i;
    }
  }

  if (bestScore == 0) {
    Log::Fatal("[SelectPhysicalDevice] No physical device was found that meets requirements!");
    throw vk::IncompatibleDriverError("No physical device was found that meets requirements!");
  }

  mPhysicalDevice = physicalDevices[bestDevice];
  mDeviceInfo = deviceInfos[bestDevice];
}

void Application::CreateDevice() {
  std::set<uint32_t> queueIndices{mDeviceInfo.GraphicsIndex.value(),
                                  mDeviceInfo.PresentIndex.value(),
                                  mDeviceInfo.TransferIndex.value()};
  if (mDeviceInfo.ComputeIndex.has_value()) {
    queueIndices.insert(mDeviceInfo.ComputeIndex.value());
  }

  std::vector<vk::DeviceQueueCreateInfo> queueCIs(queueIndices.size());
  uint32_t i{0};
  for (const uint32_t idx : queueIndices) {
    constexpr const float priority{0.0f};
    queueCIs[i++] = vk::DeviceQueueCreateInfo({}, idx, 1, &priority);
  }

  std::vector<const char*> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  vk::PhysicalDeviceFeatures requiredFeatures{};

  const vk::DeviceCreateInfo deviceCI({}, queueCIs, {}, deviceExtensions, &requiredFeatures);

  // Dump Instance Information
  {
    Log::Trace("[CreateDevice] --- Raven VkDevice Info --- ");
    Log::Trace("[CreateDevice] - Queues to Create: {}", queueIndices.size());
    Log::Trace("[CreateDevice] - Requested Device Extensions:");
    for (const auto& ext : deviceExtensions) {
      Log::Trace("[CreateDevice]   - {}", ext);
    }
  }

  mDevice = mPhysicalDevice.createDeviceUnique(deviceCI);
}

void Application::GetQueues() noexcept {
  std::set<uint32_t> queueIndices{mDeviceInfo.GraphicsIndex.value(),
                                  mDeviceInfo.PresentIndex.value(),
                                  mDeviceInfo.TransferIndex.value()};

  mGraphicsQueue = mDevice->getQueue(mDeviceInfo.GraphicsIndex.value(), 0);
  mTransferQueue = mDevice->getQueue(mDeviceInfo.TransferIndex.value(), 0);
  mPresentQueue = mDevice->getQueue(mDeviceInfo.PresentIndex.value(), 0);
  if (mDeviceInfo.ComputeIndex.has_value()) {
    mComputeQueue = mDevice->getQueue(mDeviceInfo.ComputeIndex.value(), 0);
    queueIndices.insert(mDeviceInfo.ComputeIndex.value());
  }

  // if (mValidation) {
  //  for (const uint32_t idx : queueIndices) {
  //    std::vector<std::string> roles;
  //    idx == mDeviceInfo.GraphicsIndex ? roles.push_back("Graphics") : 0;
  //    idx == mDeviceInfo.PresentIndex ? roles.push_back("Present") : 0;
  //    idx == mDeviceInfo.TransferIndex ? roles.push_back("Transfer") : 0;
  //    idx == mDeviceInfo.ComputeIndex ? roles.push_back("Compute") : 0;

  //    if (idx == mDeviceInfo.GraphicsIndex) {
  //      SetObjectName(VK_OBJECT_TYPE_QUEUE, mGraphicsQueue,
  //                    fmt::format("{} Queue", fmt::join(roles, "/")).c_str());
  //      continue;
  //    }
  //    if (idx == mDeviceInfo.PresentIndex) {
  //      SetObjectName(VK_OBJECT_TYPE_QUEUE, mPresentQueue,
  //                    fmt::format("{} Queue", fmt::join(roles, "/")).c_str());
  //      continue;
  //    }
  //    if (idx == mDeviceInfo.TransferIndex) {
  //      SetObjectName(VK_OBJECT_TYPE_QUEUE, mTransferQueue,
  //                    fmt::format("{} Queue", fmt::join(roles, "/")).c_str());
  //      continue;
  //    }
  //    if (idx == mDeviceInfo.ComputeIndex) {
  //      SetObjectName(VK_OBJECT_TYPE_QUEUE, mComputeQueue,
  //                    fmt::format("{} Queue", fmt::join(roles, "/")).c_str());
  //      continue;
  //    }
  //  }
  //}
}

void Application::CreateSwapchain() {
  mSwapchain.ImageCount = std::min(mDeviceInfo.SurfaceCapabilities.minImageCount + 1,
                                   mDeviceInfo.SurfaceCapabilities.maxImageCount);
  mSwapchain.Extent = vk::Extent2D(mWindow->Width(), mWindow->Height());

  vk::SharingMode sharing{vk::SharingMode::eExclusive};
  std::vector<uint32_t> queues{mDeviceInfo.GraphicsIndex.value()};
  if (mDeviceInfo.GraphicsIndex != mDeviceInfo.PresentIndex) {
    sharing = vk::SharingMode::eConcurrent;
    queues.push_back(mDeviceInfo.PresentIndex.value());
  }

  vk::SurfaceTransformFlagBitsKHR preTransform;
  if (mDeviceInfo.SurfaceCapabilities.supportedTransforms &
      vk::SurfaceTransformFlagBitsKHR::eIdentity) {
    preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
  } else {
    preTransform = mDeviceInfo.SurfaceCapabilities.currentTransform;
  }

  vk::CompositeAlphaFlagBitsKHR compositeAlpha{vk::CompositeAlphaFlagBitsKHR::eOpaque};
  constexpr const vk::CompositeAlphaFlagBitsKHR compositeAlphas[]{
      vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
      vk::CompositeAlphaFlagBitsKHR::ePostMultiplied, vk::CompositeAlphaFlagBitsKHR::eInherit};
  for (const auto& ca : compositeAlphas) {
    if (mDeviceInfo.SurfaceCapabilities.supportedCompositeAlpha & ca) {
      compositeAlpha = ca;
      break;
    }
  }

  const vk::SwapchainCreateInfoKHR swapchainCI(
      {}, *mSurface, mSwapchain.ImageCount, mDeviceInfo.OptimalSwapchainFormat.format,
      mDeviceInfo.OptimalSwapchainFormat.colorSpace, mSwapchain.Extent, 1,
      vk::ImageUsageFlagBits::eColorAttachment, sharing, queues, preTransform, compositeAlpha,
      mDeviceInfo.OptimalPresentMode, true);

  mSwapchain.Swapchain = mDevice->createSwapchainKHRUnique(swapchainCI);

  mSwapchain.Images = mDevice->getSwapchainImagesKHR(*mSwapchain.Swapchain);
  mSwapchain.ImageViews.resize(mSwapchain.Images.size());

  for (size_t i = 0; i < mSwapchain.Images.size(); i++) {
    const vk::ImageViewCreateInfo imageViewCI(
        {}, mSwapchain.Images[i], vk::ImageViewType::e2D, swapchainCI.imageFormat, {},
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    mSwapchain.ImageViews[i] = mDevice->createImageViewUnique(imageViewCI);
  }

  const std::vector<vk::Format> depthFormats{vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
                                             vk::Format::eD24UnormS8Uint};
  mSwapchain.DepthFormat = FindFormat(depthFormats, vk::ImageTiling::eOptimal,
                                      vk::FormatFeatureFlagBits::eDepthStencilAttachment);

  const vk::ImageCreateInfo depthCI(
      {}, vk::ImageType::e2D, mSwapchain.DepthFormat, vk::Extent3D(mSwapchain.Extent, 1), 1, 1,
      vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eDepthStencilAttachment, sharing, queues);
  mSwapchain.DepthImage = mDevice->createImageUnique(depthCI);

  const vk::MemoryRequirements depthReq{
      mDevice->getImageMemoryRequirements(*mSwapchain.DepthImage)};
  const uint32_t memType{
      FindMemoryType(depthReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)};

  const vk::MemoryAllocateInfo memoryAI(depthReq.size, memType);
  mSwapchain.DepthMemory = mDevice->allocateMemoryUnique(memoryAI);

  mDevice->bindImageMemory(*mSwapchain.DepthImage, *mSwapchain.DepthMemory, 0);

  const vk::ImageViewCreateInfo depthViewCI(
      {}, *mSwapchain.DepthImage, vk::ImageViewType::e2D, depthCI.format, {},
      vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1));
  mSwapchain.DepthImageView = mDevice->createImageViewUnique(depthViewCI);
}

void Application::DestroySwapchain() noexcept {
  for (auto& iv : mSwapchain.ImageViews) {
    iv.reset();
  }
  mSwapchain.DepthImageView.reset();
  mSwapchain.DepthMemory.reset();
  mSwapchain.DepthImage.reset();
  mSwapchain.Swapchain.reset();
}

void Application::CreateRenderPass() {
  const vk::AttachmentDescription colorAttachment(
      {}, mDeviceInfo.OptimalSwapchainFormat.format, vk::SampleCountFlagBits::e1,
      vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
      vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
      vk::ImageLayout::ePresentSrcKHR);
  const vk::AttachmentDescription depthAttachment(
      {}, mSwapchain.DepthFormat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
      vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare,
      vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
      vk::ImageLayout::eDepthStencilAttachmentOptimal);
  const std::vector<vk::AttachmentDescription> attachments{colorAttachment, depthAttachment};

  const vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);
  const vk::AttachmentReference depthAttachmentRef(1,
                                                   vk::ImageLayout::eDepthStencilAttachmentOptimal);
  const std::vector<vk::AttachmentReference> colorAttachmentRefs{colorAttachmentRef};

  const vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {},
                                       colorAttachmentRefs, {}, &depthAttachmentRef);
  const std::vector<vk::SubpassDescription> subpasses{subpass};

  const vk::RenderPassCreateInfo renderPassCI({}, attachments, subpasses);
  mRenderPass = mDevice->createRenderPassUnique(renderPassCI);
}

void Application::CreateFramebuffers() {
  mSwapchain.Framebuffers.resize(mSwapchain.ImageCount);
  for (uint32_t i = 0; i < mSwapchain.ImageCount; i++) {
    const std::vector<vk::ImageView> attachments{*mSwapchain.ImageViews[i],
                                                 *mSwapchain.DepthImageView};
    const vk::FramebufferCreateInfo framebufferCI(
        {}, *mRenderPass, attachments, mSwapchain.Extent.width, mSwapchain.Extent.height, 1);
    mSwapchain.Framebuffers[i] = mDevice->createFramebufferUnique(framebufferCI);
  }
}

void Application::CreatePipeline() {
  auto bgVertShader{CreateShaderModule("../Shaders/Basic.vert.glsl.spv")};
  auto bgFragShader{CreateShaderModule("../Shaders/Basic.frag.glsl.spv")};

  const vk::PipelineLayoutCreateInfo pipelineLayoutCI;
  mPipelineLayout = mDevice->createPipelineLayoutUnique(pipelineLayoutCI);

  PipelineBuilder builder(mSwapchain.Extent);
  builder.Layout = *mPipelineLayout;
  builder.RenderPass = *mRenderPass;
  builder.AddShader(vk::ShaderStageFlagBits::eVertex, *bgVertShader)
      .AddShader(vk::ShaderStageFlagBits::eFragment, *bgFragShader);
  mBgPipeline = mDevice->createGraphicsPipelineUnique({}, builder);
}

void Application::CreateCommandPools() {
  const vk::CommandPoolCreateInfo graphicsPoolCI(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                 mDeviceInfo.GraphicsIndex.value());
  mGraphicsPool = mDevice->createCommandPoolUnique(graphicsPoolCI);
}

void Application::CreateCommandBuffers() {
  const vk::CommandBufferAllocateInfo cmdAI(*mGraphicsPool, vk::CommandBufferLevel::ePrimary,
                                            mSwapchain.ImageCount);
  mGraphicsBuffers = mDevice->allocateCommandBuffersUnique(cmdAI);
}

void Application::CreateSyncObjects() {
  const vk::SemaphoreCreateInfo semaphoreCI;
  const vk::FenceCreateInfo fenceCI(vk::FenceCreateFlagBits::eSignaled);

  mRenderSemaphore = mDevice->createSemaphoreUnique(semaphoreCI);
  mPresentSemaphore = mDevice->createSemaphoreUnique(semaphoreCI);
  mRenderFence = mDevice->createFenceUnique(fenceCI);
}

void Application::CreateScene() {}

/* ==========================================================================================
 * Application Helper Methods
 * ========================================================================================== */

Buffer Application::CreateBuffer(const vk::DeviceSize size, vk::BufferUsageFlags usage,
                                 vk::MemoryPropertyFlags memoryType) {
  const vk::BufferCreateInfo bufferCI({}, size, usage);
  vk::UniqueBuffer buffer{mDevice->createBufferUnique(bufferCI)};

  const vk::MemoryRequirements require{mDevice->getBufferMemoryRequirements(*buffer)};
  const uint32_t memType{FindMemoryType(
      require.memoryTypeBits,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)};

  const vk::MemoryAllocateInfo memoryAI(require.size, memType);
  vk::UniqueDeviceMemory memory{mDevice->allocateMemoryUnique(memoryAI)};

  mDevice->bindBufferMemory(*buffer, *memory, 0);

  return Buffer(std::move(buffer), std::move(memory), require.size);
}

uint32_t Application::FindMemoryType(uint32_t filter, vk::MemoryPropertyFlags properties) {
  for (uint32_t i = 0; i < mDeviceInfo.MemoryProperties.memoryTypeCount; i++) {
    if ((filter & (1 << i)) &&
        (mDeviceInfo.MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("No memory type is available that meets requirements!");
}

vk::Format Application::FindFormat(const std::vector<vk::Format>& candidates,
                                   vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
  for (const vk::Format& fmt : candidates) {
    const vk::FormatProperties props{mPhysicalDevice.getFormatProperties(fmt)};
    if ((tiling == vk::ImageTiling::eLinear &&
         (props.linearTilingFeatures & features) == features) ||
        (tiling == vk::ImageTiling::eOptimal &&
         (props.optimalTilingFeatures & features) == features)) {
      return fmt;
    }
  }

  throw vk::FormatNotSupportedError(
      "No format could be found that supports the required features!");
}

vk::UniqueShaderModule Application::CreateShaderModule(const std::string& path) {
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  const size_t fileSize{static_cast<size_t>(file.tellg())};
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  const vk::ShaderModuleCreateInfo shaderModuleCI({}, fileSize,
                                                  reinterpret_cast<uint32_t*>(buffer.data()));
  return mDevice->createShaderModuleUnique(shaderModuleCI);
}

/* ==========================================================================================
 * Helper Class Methods
 * ========================================================================================== */

Buffer::Buffer(vk::UniqueBuffer buffer, vk::UniqueDeviceMemory memory, vk::DeviceSize size)
    : Handle(std::move(buffer)), Memory(std::move(memory)), Size(size) {}

Buffer::Buffer(Buffer&& o) {
  Handle = std::move(o.Handle);
  Memory = std::move(o.Memory);
  Size = o.Size;
}

Buffer& Buffer::operator=(Buffer&& o) {
  Handle = std::move(o.Handle);
  Memory = std::move(o.Memory);
  Size = o.Size;

  return *this;
}

Buffer::~Buffer() {}
}  // namespace Raven