#include "Application.h"
#include "Core.h"

#include <vulkan/vulkan_win32.h>

#include "VulkanCore.h"
#include "Window.h"

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

            const auto objTypeIt{gVkObjectTypes.find(objData.objectType)};
            const char* objType{objTypeIt == gVkObjectTypes.end() ? "Unknown" : objTypeIt->second};
            const void* objHandle{reinterpret_cast<void*>(objData.objectHandle)};
            const char* objName{objData.pObjectName};

            customMsg += fmt::format("\n   - Object {}: {} <{}>", i, objType, objHandle);
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

constexpr const VkDebugUtilsMessengerCreateInfoEXT gDebugMessengerCI{
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,  // sType
    nullptr,                                                  // pNext
    0,                                                        // flags
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,  // messageSeverity
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,  // messageType
    VulkanDebugCallback,                                 // pfnUserCallback
    nullptr                                              // pUserData
};

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

  while (mRunning) {
    mWindow->Update();
  }
}

void Application::InitializeVulkan() {
  VkCall(CreateInstance(mValidation));
  Log::Debug("[InitializeVulkan] Vulkan Instance created.");

  if (mValidation) {
    VkCall(CreateDebugger());
    Log::Debug("[InitializeVulkan] Vulkan Debugger created. <{}>",
               reinterpret_cast<void*>(mDebugger));
  }

  VkCall(CreateSurface());
  Log::Debug("[InitializeVulkan] Vulkan Surface created. <{}>", reinterpret_cast<void*>(mSurface));

  VkCall(SelectPhysicalDevice());
  Log::Debug("[InitializeVulkan] Selected physical device: {}", mDeviceInfo.Properties.deviceName);

  VkCall(CreateDevice());
  Log::Debug("[InitializeVulkan] Vulkan Device created. <{}>", reinterpret_cast<void*>(mDevice));
  SetObjectName(VK_OBJECT_TYPE_DEVICE, mDevice, "Main Device");

  GetQueues();
  Log::Debug("[InitializeVulkan] Device queues retrieved.");

  VkCall(CreateSwapchain());
  Log::Debug("[InitializeVulkan] Vulkan Swapchain created with {} images. <{}>",
             mSwapchain.ImageCount, reinterpret_cast<void*>(mSwapchain.Swapchain));
}

void Application::ShutdownVulkan() {
  vkDeviceWaitIdle(mDevice);

  DestroySwapchain();

  vkDestroyDevice(mDevice, nullptr);
  vkDestroySurfaceKHR(mInstance, mSurface, nullptr);

  if (mValidation) {
    PFN_vkDestroyDebugUtilsMessengerEXT func{reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT"))};
    if (func != nullptr) {
      func(mInstance, mDebugger, nullptr);
    }
  }
  vkDestroyInstance(mInstance, nullptr);
}

VkResult Application::CreateInstance(const bool validation) noexcept {
  // ##########
  // # Enumerate Vulkan Instance Capabilities
  // ##########
  uint32_t instanceVersion{0};
  vkEnumerateInstanceVersion(&instanceVersion);

  uint32_t availableLayerCount{0};
  vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
  std::vector<VkLayerProperties> availableLayers(availableLayerCount);
  vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());

  uint32_t availableExtensionCount{0};
  vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
  std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount,
                                         availableExtensions.data());

  // Dump enumerated information
  {
    Log::Trace("[CreateInstance] --- Vulkan Instance Information --- ");
    Log::Trace("[CreateInstance] - Instance Version: {}.{}.{}", VK_VERSION_MAJOR(instanceVersion),
               VK_VERSION_MINOR(instanceVersion), VK_VERSION_PATCH(instanceVersion));
    Log::Trace("[CreateInstance] - Available Layers:");
    for (const auto& layer : availableLayers) {
      Log::Trace("[CreateInstance]   - {} v{}.{}.{}", layer.layerName,
                 VK_VERSION_MAJOR(layer.specVersion), VK_VERSION_MINOR(layer.specVersion),
                 VK_VERSION_PATCH(layer.specVersion));
    }
    Log::Trace("[CreateInstance] - Available Extensions:");
    for (const auto& ext : availableExtensions) {
      Log::Trace("[CreateInstance]   - {} v{}", ext.extensionName, ext.specVersion);
    }
  }

  // ##########
  // # Determine Application Requirements
  // ##########
  constexpr const VkApplicationInfo appInfo{
      VK_STRUCTURE_TYPE_APPLICATION_INFO,  // sType
      nullptr,                             // pNext
      "Raven",                             // pApplicationName
      VK_MAKE_VERSION(1, 0, 0),            // applicationVersion
      "Raven",                             // pEngineName
      VK_MAKE_VERSION(1, 0, 0),            // engineVersion
      VK_API_VERSION_1_0                   // apiVersion
  };

  std::vector<const char*> requiredExtensions{VK_KHR_SURFACE_EXTENSION_NAME,
                                              VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
  std::vector<const char*> requiredLayers{};
  if (validation) {
    requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
  }

  // Dump Instance Information
  {
    Log::Trace("[CreateInstance] --- Raven VkInstance Info --- ");
    Log::Trace("[CreateInstance] - API Version: {}.{}", VK_VERSION_MAJOR(appInfo.apiVersion),
               VK_VERSION_MINOR(appInfo.apiVersion));
    Log::Trace("[CreateInstance] - Requested Instance Layers ({}):", requiredLayers.size());
    for (const auto& layer : requiredLayers) {
      Log::Trace("[CreateInstance]   - {}", layer);
    }
    Log::Trace("[CreateInstance] - Requested Instance Extensions:");
    for (const auto& ext : requiredExtensions) {
      Log::Trace("[CreateInstance]   - {}", ext);
    }
  }

  // ##########
  // # Validate Application Requirements
  // ##########
  {
    VkResult res{VK_SUCCESS};
    std::vector<const char*> missingLayers{};
    std::vector<const char*> missingExtensions{};

    if (VK_VERSION_MAJOR(appInfo.apiVersion) > VK_VERSION_MAJOR(instanceVersion) ||
        VK_VERSION_MINOR(appInfo.apiVersion) > VK_VERSION_MINOR(instanceVersion)) {
      Log::Fatal("[CreateInstance] Required Vulkan version {}.{} is not available!",
                 VK_VERSION_MAJOR(appInfo.apiVersion), VK_VERSION_MINOR(appInfo.apiVersion));
      Log::Fatal("[CreateInstance] Available Vulkan version is {}.{}.",
                 VK_VERSION_MAJOR(instanceVersion), VK_VERSION_MINOR(instanceVersion));
      res = VK_ERROR_INCOMPATIBLE_DRIVER;
    }

    for (const auto& extName : requiredExtensions) {
      bool found{false};
      for (const auto& ext : availableExtensions) {
        if (strcmp(extName, ext.extensionName) == 0) {
          found = true;
          break;
        }
      }

      if (!found) {
        missingExtensions.push_back(extName);
      }
    }

    for (const auto& layerName : requiredLayers) {
      bool found{false};
      for (const auto& layer : availableLayers) {
        if (strcmp(layerName, layer.layerName) == 0) {
          found = true;
          break;
        }
      }

      if (!found) {
        missingLayers.push_back(layerName);
      }
    }

    if (!missingLayers.empty()) {
      Log::Fatal("[CreateInstance] Required Vulkan Layers are missing:");
      for (const auto& layer : missingLayers) {
        Log::Fatal(" - {}", layer);
      }
      res = VK_ERROR_LAYER_NOT_PRESENT;
    }

    if (!missingExtensions.empty()) {
      Log::Fatal("[CreateInstance] Required Vulkan Extensions are missing:");
      for (const auto& ext : missingExtensions) {
        Log::Fatal(" - {}", ext);
      }
      res = VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    if (res != VK_SUCCESS) {
      return res;
    }
  }

  const VkInstanceCreateInfo instanceCI{
      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,            // sType
      &gDebugMessengerCI,                                // pNext
      0,                                                 // flags
      &appInfo,                                          // pApplicationInfo
      static_cast<uint32_t>(requiredLayers.size()),      // enabledLayerCount
      requiredLayers.data(),                             // ppEnabledLayerNames
      static_cast<uint32_t>(requiredExtensions.size()),  // enabledExtensionCount
      requiredExtensions.data()                          // ppEnabledExtensions
  };

  return vkCreateInstance(&instanceCI, nullptr, &mInstance);
}

VkResult Application::CreateDebugger() noexcept {
  PFN_vkCreateDebugUtilsMessengerEXT func{reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT"))};
  if (func == nullptr) {
    Log::Fatal("[CreateDebugger] Failed to load function vkCreateDebugUtilsMessengerEXT!");
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  return func(mInstance, &gDebugMessengerCI, nullptr, &mDebugger);
}

VkResult Application::CreateSurface() noexcept {
  mSurface = mWindow->CreateSurface(mInstance);

  return VK_SUCCESS;
}

VkResult Application::SelectPhysicalDevice() noexcept {
  uint32_t physicalDeviceCount{0};
  vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, nullptr);
  if (physicalDeviceCount == 0) {
    Log::Fatal("[SelectPhysicalDevice] No physical devices supporting Vulkan are present!");
    return VK_ERROR_DEVICE_LOST;
  }
  std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
  vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, physicalDevices.data());

  Log::Debug("[SelectPhysicalDevice] Found {} Vulkan devices.", physicalDeviceCount);
  std::vector<PhysicalDeviceInfo> deviceInfos(physicalDeviceCount);

  std::vector<uint64_t> deviceScores(physicalDeviceCount, 0);

  for (uint32_t i = 0; i < physicalDeviceCount; i++) {
    VkPhysicalDevice device{physicalDevices[i]};
    PhysicalDeviceInfo& info{deviceInfos[i]};
    uint64_t& score{deviceScores[i]};

    // Fill device info
    {
      // Standard device info
      vkGetPhysicalDeviceFeatures(device, &info.Features);
      vkGetPhysicalDeviceMemoryProperties(device, &info.MemoryProperties);
      vkGetPhysicalDeviceProperties(device, &info.Properties);
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, mSurface, &info.SurfaceCapabilities);

      // Extensions
      uint32_t extensionCount{0};
      vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
      info.Extensions.resize(extensionCount);
      vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                           info.Extensions.data());

      // Queue families
      {
        uint32_t queueCount{0};
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueCount);
        info.QueueFamilies.resize(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueCount; i++) {
          QueueFamilyInfo& q{info.QueueFamilies[i]};
          q.Index = i;
          q.Properties = queueFamilies[i];
          vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface, &q.PresentSupport);
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
        uint32_t formatCount{0};
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, formats.data());

        if (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
          info.OptimalSwapchainFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
          info.OptimalSwapchainFormat.colorSpace = formats[0].colorSpace;
        } else {
          for (const auto& fmt : formats) {
            if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM) {
              info.OptimalSwapchainFormat = fmt;
              break;
            }
          }
        }

        if (info.OptimalSwapchainFormat.format == VK_FORMAT_UNDEFINED) {
          info.OptimalSwapchainFormat = formats[0];
        }

        uint32_t presentModeCount{0};
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModeCount,
                                                  presentModes.data());

        for (const auto& pm : presentModes) {
          if (pm == VK_PRESENT_MODE_MAILBOX_KHR) {
            info.OptimalPresentMode = pm;
            break;
          } else if (pm == VK_PRESENT_MODE_IMMEDIATE_KHR) {
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
          case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            Log::Trace("[SelectPhysicalDevice]   - Type: Other");
            break;
          case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            Log::Trace("[SelectPhysicalDevice]   - Type: Integrated GPU");
            break;
          case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            Log::Trace("[SelectPhysicalDevice]   - Type: Discrete GPU");
            break;
          case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            Log::Trace("[SelectPhysicalDevice]   - Type: Virtual GPU");
            break;
          case VK_PHYSICAL_DEVICE_TYPE_CPU:
            Log::Trace("[SelectPhysicalDevice]   - Type: CPU");
            break;
        }
      }

      // Swapchain Properties
      {
        Log::Trace("[SelectPhysicalDevice]   - Optimal Image Format: {}",
                   gVkFormats.find(info.OptimalSwapchainFormat.format)->second);
      }

      // Memory Properties
      {
        Log::Trace("[SelectPhysicalDevice]   - Memory Properties:");
        Log::Trace("[SelectPhysicalDevice]     - Heaps:");
        for (uint32_t i = 0; i < info.MemoryProperties.memoryHeapCount; i++) {
          const VkMemoryHeap& heap{info.MemoryProperties.memoryHeaps[i]};
          const float heapMB{heap.size / 1024.0f / 1024.0f};
          Log::Trace("[SelectPhysicalDevice]       - Heap {}: {:.0f}MB{}", i, heapMB,
                     heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ? ", Device Local" : "");
        }
        Log::Trace("[SelectPhysicalDevice]     - Types:");
        for (uint32_t i = 0; i < info.MemoryProperties.memoryTypeCount; i++) {
          const VkMemoryType& type{info.MemoryProperties.memoryTypes[i]};

          std::vector<std::string> flags;
          if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            flags.emplace_back("Device Local");
          }
          if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            flags.emplace_back("Host Visible");
          }
          if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
            flags.emplace_back("Host Coherent");
          }
          if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
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
      if (info.Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
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

  std::vector<VkDeviceQueueCreateInfo> queueCIs(queueIndices.size());
  uint32_t i{0};
  for (const uint32_t idx : queueIndices) {
    constexpr const float priority{0.0f};
    queueCIs[i++] = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,  // sType
        nullptr,                                     // pNext
        0,                                           // flags
        idx,                                         // queueFamilyIndex
        1,                                           // queueCount
        &priority                                    // pQueuePriorities
    };
  }

  std::vector<const char*> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  VkPhysicalDeviceFeatures requiredFeatures{};

  const VkDeviceCreateInfo deviceCI{
      VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,            // sType
      nullptr,                                         // pNext
      0,                                               // flags
      static_cast<uint32_t>(queueCIs.size()),          // queueCreateInfoCount
      queueCIs.data(),                                 // pQueueCreateInfos
      0,                                               // enabledLayerCount
      nullptr,                                         // ppEnabledLayerNames
      static_cast<uint32_t>(deviceExtensions.size()),  // enabledExtensionCount
      deviceExtensions.data(),                         // ppEnabledExtensionNames
      &requiredFeatures                                // pEnabledFeatures
  };

  // Dump Instance Information
  {
    Log::Trace("[CreateDevice] --- Raven VkDevice Info --- ");
    Log::Trace("[CreateDevice] - Queues to Create: {}", queueIndices.size());
    Log::Trace("[CreateDevice] - Requested Device Extensions:");
    for (const auto& ext : deviceExtensions) {
      Log::Trace("[CreateDevice]   - {}", ext);
    }
  }

  return vkCreateDevice(mPhysicalDevice, &deviceCI, nullptr, &mDevice);
}

void Application::GetQueues() noexcept {
  std::set<uint32_t> queueIndices{mDeviceInfo.GraphicsIndex.value(),
                                  mDeviceInfo.PresentIndex.value(),
                                  mDeviceInfo.TransferIndex.value()};

  vkGetDeviceQueue(mDevice, mDeviceInfo.GraphicsIndex.value(), 0, &mGraphicsQueue);
  vkGetDeviceQueue(mDevice, mDeviceInfo.PresentIndex.value(), 0, &mPresentQueue);
  vkGetDeviceQueue(mDevice, mDeviceInfo.TransferIndex.value(), 0, &mTransferQueue);
  if (mDeviceInfo.ComputeIndex.has_value()) {
    vkGetDeviceQueue(mDevice, mDeviceInfo.ComputeIndex.value(), 0, &mComputeQueue);
    queueIndices.insert(mDeviceInfo.ComputeIndex.value());
  }

  if (mValidation) {
    for (const uint32_t idx : queueIndices) {
      std::vector<std::string> roles;
      idx == mDeviceInfo.GraphicsIndex ? roles.push_back("Graphics") : 0;
      idx == mDeviceInfo.PresentIndex ? roles.push_back("Present") : 0;
      idx == mDeviceInfo.TransferIndex ? roles.push_back("Transfer") : 0;
      idx == mDeviceInfo.ComputeIndex ? roles.push_back("Compute") : 0;

      if (idx == mDeviceInfo.GraphicsIndex) {
        SetObjectName(VK_OBJECT_TYPE_QUEUE, mGraphicsQueue,
                      fmt::format("{} Queue", fmt::join(roles, "/")).c_str());
        continue;
      }
      if (idx == mDeviceInfo.PresentIndex) {
        SetObjectName(VK_OBJECT_TYPE_QUEUE, mPresentQueue,
                      fmt::format("{} Queue", fmt::join(roles, "/")).c_str());
        continue;
      }
      if (idx == mDeviceInfo.TransferIndex) {
        SetObjectName(VK_OBJECT_TYPE_QUEUE, mTransferQueue,
                      fmt::format("{} Queue", fmt::join(roles, "/")).c_str());
        continue;
      }
      if (idx == mDeviceInfo.ComputeIndex) {
        SetObjectName(VK_OBJECT_TYPE_QUEUE, mComputeQueue,
                      fmt::format("{} Queue", fmt::join(roles, "/")).c_str());
        continue;
      }
    }
  }
}

VkResult Application::CreateSwapchain() noexcept {
  mSwapchain.ImageCount = std::min(mDeviceInfo.SurfaceCapabilities.minImageCount + 1,
                                   mDeviceInfo.SurfaceCapabilities.maxImageCount);
  mSwapchain.Extent = {1600, 900};
  VkSharingMode sharing{VK_SHARING_MODE_EXCLUSIVE};
  std::vector<uint32_t> queues{mDeviceInfo.GraphicsIndex.value()};
  if (mDeviceInfo.GraphicsIndex != mDeviceInfo.PresentIndex) {
    sharing = VK_SHARING_MODE_CONCURRENT;
    queues.push_back(mDeviceInfo.PresentIndex.value());
  }
  VkSurfaceTransformFlagBitsKHR preTransform;
  if (mDeviceInfo.SurfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    preTransform = mDeviceInfo.SurfaceCapabilities.currentTransform;
  }
  VkCompositeAlphaFlagBitsKHR compositeAlpha{VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR};
  constexpr const VkCompositeAlphaFlagBitsKHR compositeAlphas[]{
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR};
  for (const auto& ca : compositeAlphas) {
    if (mDeviceInfo.SurfaceCapabilities.supportedCompositeAlpha & ca) {
      compositeAlpha = ca;
      break;
    }
  }

  const VkSwapchainCreateInfoKHR swapchainCI{
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,    // sType
      nullptr,                                        // pNext
      0,                                              // flags
      mSurface,                                       // surface
      mSwapchain.ImageCount,                          // minImageCount
      mDeviceInfo.OptimalSwapchainFormat.format,      // imageFormat
      mDeviceInfo.OptimalSwapchainFormat.colorSpace,  // imageColorSpace
      mSwapchain.Extent,                              // imageExtent
      1,                                              // imageArrayLayers
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,            // imageUsage
      sharing,                                        // imageSharingMode
      static_cast<uint32_t>(queues.size()),           // queueFamilyIndexCount
      queues.data(),                                  // pQueueFamilyIndices
      preTransform,                                   // preTransform
      compositeAlpha,                                 // compositeAlpha
      mDeviceInfo.OptimalPresentMode,                 // presentMode
      VK_TRUE,                                        // clipped
      mSwapchain.Swapchain                            // oldSwapchain
  };

  const VkResult swapchainRes{
      vkCreateSwapchainKHR(mDevice, &swapchainCI, nullptr, &mSwapchain.Swapchain)};
  if (swapchainRes != VK_SUCCESS) {
    return swapchainRes;
  }

  SetObjectName(VK_OBJECT_TYPE_SWAPCHAIN_KHR, mSwapchain.Swapchain, "Main Swapchain");

  vkGetSwapchainImagesKHR(mDevice, mSwapchain.Swapchain, &mSwapchain.ImageCount, nullptr);
  mSwapchain.Images.resize(mSwapchain.ImageCount);
  mSwapchain.ImageViews.resize(mSwapchain.ImageCount);
  vkGetSwapchainImagesKHR(mDevice, mSwapchain.Swapchain, &mSwapchain.ImageCount,
                          mSwapchain.Images.data());

  for (uint32_t i = 0; i < mSwapchain.ImageCount; i++) {
    SetObjectName(VK_OBJECT_TYPE_IMAGE, mSwapchain.Images[i],
                  fmt::format("Swapchain Image {}", i).c_str());

    const VkImageViewCreateInfo imageViewCI{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // sType
        nullptr,                                   // pNext
        0,                                         // flags
        mSwapchain.Images[i],                      // image
        VK_IMAGE_VIEW_TYPE_2D,                     // viewType
        swapchainCI.imageFormat,                   // format
        {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
         VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},  // components
        {
            VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask
            0,                          // baseMipLevel
            1,                          // levelCount
            0,                          // baseArrayLayer
            1                           // layerCount
        }                               // subresourceRange
    };
    const VkResult imageViewRes{
        vkCreateImageView(mDevice, &imageViewCI, nullptr, &mSwapchain.ImageViews[i])};
    if (imageViewRes != VK_SUCCESS) {
      return imageViewRes;
    }
    SetObjectName(VK_OBJECT_TYPE_IMAGE_VIEW, mSwapchain.ImageViews[i],
                  fmt::format("Swapchain Image View {}", i).c_str());
  }

  return VK_SUCCESS;
}

void Application::DestroySwapchain() noexcept {
  for (auto& iv : mSwapchain.ImageViews) {
    vkDestroyImageView(mDevice, iv, nullptr);
  }
  vkDestroySwapchainKHR(mDevice, mSwapchain.Swapchain, nullptr);
}
}  // namespace Raven