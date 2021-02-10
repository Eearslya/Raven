#include "Application.h"
#include "Core.h"

#include <chrono>
#include <fstream>

#include "VulkanCore.h"
#include "Window.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace Raven {
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
      const std::string title{fmt::format("Raven - {:.2f}ms ({} FPS)", msAvg, static_cast<uint32_t>(1000.0f / msAvg))};
      mWindow->SetTitle(title);
      msAcc = 0.0f;
      sampleCount = 0;
    }

    Render();

    mRunning = mWindow->Update();
  }
}

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

  VkCall(SelectPhysicalDevice());
  Log::Debug("[InitializeVulkan] Selected physical device: {}", mDeviceInfo.Properties.deviceName);

  VkCall(CreateDevice());
  Log::Debug("[InitializeVulkan] Vulkan Device created. <{}>", static_cast<void*>(*mDevice));
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*mDevice);
  // SetObjectName(VK_OBJECT_TYPE_DEVICE, mDevice, "Main Device");

  GetQueues();
  Log::Debug("[InitializeVulkan] Device queues retrieved.");

  VkCall(CreateSwapchain());
  Log::Debug("[InitializeVulkan] Vulkan Swapchain created with {} images. <{}>",
             mSwapchain.ImageCount, static_cast<void*>(*mSwapchain.Swapchain));

  CreateRenderPass();
  Log::Debug("[InitializeVulkan] Vulkan Render Pass created. <{}>",
             static_cast<void*>(*mRenderPass));

  CreateFramebuffers();
  Log::Debug("[InitializeVulkan] Vulkan Framebuffers created.");

  CreatePipeline();
  Log::Debug("[InitializeVulkan] Vulkan Pipeline created. <{}>", static_cast<void*>(*mPipeline));

  CreateCommandPools();
  Log::Debug("[InitializeVulkan] Vulkan Command Pools created.");

  CreateCommandBuffers();
  Log::Debug("[InitializeVulkan] Vulkan Command Buffers created.");

  CreateSyncObjects();
  Log::Debug("[InitializeVulkan] Vulkan Sync Objects created.");
}

void Application::ShutdownVulkan() { mDevice->waitIdle(); }

VkResult Application::CreateInstance(const bool validation) noexcept { return VK_SUCCESS; }

VkResult Application::SelectPhysicalDevice() noexcept {
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
    return VK_ERROR_INCOMPATIBLE_DRIVER;
  }

  mPhysicalDevice = physicalDevices[bestDevice];
  mDeviceInfo = deviceInfos[bestDevice];

  return VK_SUCCESS;
}

VkResult Application::CreateDevice() noexcept {
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

  return VK_SUCCESS;
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

VkResult Application::CreateSwapchain() noexcept {
  mSwapchain.ImageCount = std::min(mDeviceInfo.SurfaceCapabilities.minImageCount + 1,
                                   mDeviceInfo.SurfaceCapabilities.maxImageCount);
  mSwapchain.Extent = vk::Extent2D(1600, 900);
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

  // SetObjectName(VK_OBJECT_TYPE_SWAPCHAIN_KHR, mSwapchain.Swapchain, "Main Swapchain");

  mSwapchain.Images = mDevice->getSwapchainImagesKHR(*mSwapchain.Swapchain);
  mSwapchain.ImageViews.resize(mSwapchain.Images.size());

  for (size_t i = 0; i < mSwapchain.Images.size(); i++) {
    // SetObjectName(VK_OBJECT_TYPE_IMAGE, mSwapchain.Images[i],
    //              fmt::format("Swapchain Image {}", i).c_str());

    const vk::ImageViewCreateInfo imageViewCI(
        {}, mSwapchain.Images[i], vk::ImageViewType::e2D, swapchainCI.imageFormat, {},
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    mSwapchain.ImageViews[i] = mDevice->createImageViewUnique(imageViewCI);

    // SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, mSwapchain.ImageViews[i],
    //              fmt::format("Swapchain Image View {}", i).c_str());
  }

  return VK_SUCCESS;
}

void Application::DestroySwapchain() noexcept {
  for (auto& iv : mSwapchain.ImageViews) {
    iv.reset();
  }
  mSwapchain.Swapchain.reset();
}

void Application::Render() {
  const std::vector<vk::Fence> renderFences{*mRenderFence};
  mDevice->waitForFences(renderFences, true, std::numeric_limits<uint64_t>::max());
  mDevice->resetFences(renderFences);

  uint32_t imageIndex{mDevice->acquireNextImageKHR(
      *mSwapchain.Swapchain, std::numeric_limits<uint64_t>::max(), *mPresentSemaphore)};

  const vk::UniqueCommandBuffer& cmd{mGraphicsBuffers[imageIndex]};

  const vk::CommandBufferBeginInfo beginInfo;
  cmd->begin(beginInfo);

  const std::array<float, 4> clearColor{0.0f, 0.0f, 1.0f, 1.0f};
  const std::vector<vk::ClearValue> clearValues{vk::ClearColorValue(clearColor)};
  const vk::RenderPassBeginInfo rpInfo(*mRenderPass, *mSwapchain.Framebuffers[imageIndex],
                                       {{0, 0}, {1600, 900}}, clearValues);
  cmd->beginRenderPass(rpInfo, vk::SubpassContents::eInline);

  cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *mPipeline);
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

void Application::CreateRenderPass() {
  const vk::AttachmentDescription colorAttachment(
      {}, mDeviceInfo.OptimalSwapchainFormat.format, vk::SampleCountFlagBits::e1,
      vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
      vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
      vk::ImageLayout::ePresentSrcKHR);
  const std::vector<vk::AttachmentDescription> attachments{colorAttachment};

  const vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);
  const std::vector<vk::AttachmentReference> attachmentRefs{colorAttachmentRef};

  const vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, attachmentRefs);
  const std::vector<vk::SubpassDescription> subpasses{subpass};

  const vk::RenderPassCreateInfo renderPassCI({}, attachments, subpasses);
  mRenderPass = mDevice->createRenderPassUnique(renderPassCI);
}

void Application::CreateFramebuffers() {
  mSwapchain.Framebuffers.resize(mSwapchain.ImageCount);
  for (uint32_t i = 0; i < mSwapchain.ImageCount; i++) {
    const std::vector<vk::ImageView> attachments{*mSwapchain.ImageViews[i]};
    const vk::FramebufferCreateInfo framebufferCI(
        {}, *mRenderPass, attachments, mSwapchain.Extent.width, mSwapchain.Extent.height, 1);
    mSwapchain.Framebuffers[i] = mDevice->createFramebufferUnique(framebufferCI);
  }
}

void Application::CreatePipeline() {
  auto vertShader{CreateShaderModule("../Shaders/Basic.vert.glsl.spv")};
  auto fragShader{CreateShaderModule("../Shaders/Basic.frag.glsl.spv")};

  const vk::PipelineShaderStageCreateInfo vertStage({}, vk::ShaderStageFlagBits::eVertex,
                                                    *vertShader, "main");
  const vk::PipelineShaderStageCreateInfo fragStage({}, vk::ShaderStageFlagBits::eFragment,
                                                    *fragShader, "main");
  const std::vector<vk::PipelineShaderStageCreateInfo> stages{vertStage, fragStage};

  const vk::PipelineVertexInputStateCreateInfo vertexInput;

  const vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
      {}, vk::PrimitiveTopology::eTriangleList, false);

  const vk::Viewport viewport(0, 0, 1600, 900, 0.0f, 1.0f);
  const std::vector<vk::Viewport> viewports{viewport};
  const vk::Rect2D scissor({0, 0}, {1600, 900});
  const std::vector<vk::Rect2D> scissors{scissor};
  const vk::PipelineViewportStateCreateInfo viewportState({}, viewports, scissors);

  const vk::PipelineRasterizationStateCreateInfo rasterizer(
      {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone,
      vk::FrontFace::eCounterClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);

  const vk::PipelineMultisampleStateCreateInfo multisampling;

  const vk::PipelineColorBlendAttachmentState colorBlendAttachment(
      true, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
      vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
  const std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments{
      colorBlendAttachment};
  const vk::PipelineColorBlendStateCreateInfo colorBlending({}, false, vk::LogicOp::eCopy,
                                                            colorBlendAttachments);

  const vk::PipelineLayoutCreateInfo pipelineLayoutCI;
  mPipelineLayout = mDevice->createPipelineLayoutUnique(pipelineLayoutCI);

  const vk::GraphicsPipelineCreateInfo pipelineCI(
      {}, stages, &vertexInput, &inputAssembly, {}, &viewportState, &rasterizer, &multisampling, {},
      &colorBlending, {}, *mPipelineLayout, *mRenderPass, 0);
  mPipeline = mDevice->createGraphicsPipelineUnique({}, pipelineCI);
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
}  // namespace Raven