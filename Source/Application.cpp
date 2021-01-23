#include "Application.h"
#include "Core.h"

#include <SDL_vulkan.h>
#include <vulkan/vulkan_win32.h>

#include "VulkanCore.h"

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
  InitializeSDL();
  InitializeVulkan();
}

Application::~Application() {
  Log::Info("Raven is shutting down...");
  ShutdownVulkan();
  ShutdownSDL();
}

void Application::InitializeSDL() {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  mWindow = SDL_CreateWindow("Raven", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900,
                             SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
}

void Application::ShutdownSDL() {
  SDL_DestroyWindow(mWindow);
  SDL_Quit();
}

void Application::InitializeVulkan() {
  VkCall(CreateInstance(mValidation));
  // SetObjectName(VK_OBJECT_TYPE_INSTANCE, reinterpret_cast<uint64_t>(mInstance), "Main Instance");
  Log::Debug("[InitializeVulkan] Vulkan Instance created.");

  if (mValidation) {
    VkCall(CreateDebugger());
    Log::Debug("[InitializeVulkan] Vulkan Debugger created. <{}>",
               reinterpret_cast<void*>(mDebugger));
  }

  VkCall(CreateSurface());
  Log::Debug("[InitializeVulkan] Vulkan Surface created. <{}>", reinterpret_cast<void*>(mSurface));

  VkCall(SelectPhysicalDevice());
}

void Application::ShutdownVulkan() {
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
    Log::Debug("[CreateInstance] --- Vulkan Instance Information --- ");
    Log::Debug("[CreateInstance] - Instance Version: {}.{}.{}", VK_VERSION_MAJOR(instanceVersion),
               VK_VERSION_MINOR(instanceVersion), VK_VERSION_PATCH(instanceVersion));
    Log::Debug("[CreateInstance] - Available Layers:");
    for (const auto& layer : availableLayers) {
      Log::Debug("[CreateInstance]   - {} v{}.{}.{}", layer.layerName,
                 VK_VERSION_MAJOR(layer.specVersion), VK_VERSION_MINOR(layer.specVersion),
                 VK_VERSION_PATCH(layer.specVersion));
    }
    Log::Debug("[CreateInstance] - Available Extensions:");
    for (const auto& ext : availableExtensions) {
      Log::Debug("[CreateInstance]   - {} v{}", ext.extensionName, ext.specVersion);
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
    Log::Debug("[CreateInstance] --- Raven VkInstance Info --- ");
    Log::Debug("[CreateInstance] - API Version: {}.{}", VK_VERSION_MAJOR(appInfo.apiVersion),
               VK_VERSION_MINOR(appInfo.apiVersion));
    Log::Debug("[CreateInstance] - Requested Instance Layers ({}):", requiredLayers.size());
    for (const auto& layer : requiredLayers) {
      Log::Debug("[CreateInstance]   - {}", layer);
    }
    Log::Debug("[CreateInstance] - Requested Instance Extensions:");
    for (const auto& ext : requiredExtensions) {
      Log::Debug("[CreateInstance]   - {}", ext);
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
  const SDL_bool created{SDL_Vulkan_CreateSurface(mWindow, mInstance, &mSurface)};
  if (!created) {
    Log::Fatal("[CreateSurface] Failed to create a surface with SDL!");
    return VK_ERROR_SURFACE_LOST_KHR;
  }

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

  for (uint32_t i = 0; i < physicalDeviceCount; i++) {
    VkPhysicalDevice device{physicalDevices[i]};
    PhysicalDeviceInfo& info{deviceInfos[i]};

    // Fill device info
    {
      vkGetPhysicalDeviceFeatures(device, &info.Features);
      vkGetPhysicalDeviceMemoryProperties(device, &info.MemoryProperties);
      vkGetPhysicalDeviceProperties(device, &info.Properties);
    }

    // Dump device info to log
    {
      Log::Trace("[SelectPhysicalDevice] - Physical Device {}: \"{}\"", i,
                 info.Properties.deviceName);

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

          std::string flags{""};
          if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            flags += "Device Local";
          }
          if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            if (flags.size() > 0) {
              flags += ", ";
            }
            flags += "Host Visible";
          }
          if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
            if (flags.size() > 0) {
              flags += ", ";
            }
            flags += "Host Coherent";
          }
          if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
            if (flags.size() > 0) {
              flags += ", ";
            }
            flags += "Host Cached";
          }
          if (flags.size() == 0) {
            flags = "No flags";
          }

          Log::Trace("[SelectPhysicalDevice]       - Type {}: {} on Heap {}", i, flags,
                     type.heapIndex);
        }
      }

      // Device Queues
      {
        uint32_t queueCount{0};
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueCount);
        info.QueueFamilies.resize(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, queueFamilies.data());

        Log::Trace("[SelectPhysicalDevice]   - Device Queue Families:");
        for (size_t i = 0; i < queueCount; i++) {
          const VkQueueFamilyProperties& queue{queueFamilies[i]};
          std::string caps{""};
          if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            caps += "Graphics";
          }
          if (queue.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            if (caps.size() > 0) {
              caps += ", ";
            }
            caps += "Compute";
          }
          if (queue.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            if (caps.size() > 0) {
              caps += ", ";
            }
            caps += "Transfer";
          }
          if (queue.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
            if (caps.size() > 0) {
              caps += ", ";
            }
            caps += "Sparse Binding";
          }
          VkBool32 present{VK_FALSE};
          vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface, &present);
          if (present) {
            if (caps.size() > 0) {
              caps += ", ";
            }
            caps += "Present";
          }

          Log::Trace("[SelectPhysicalDevice]     - Family {}: {} Queues <{}>", i, queue.queueCount, caps);
        }
      }
    }
  }

  return VkResult();
}

void Application::EnumeratePhysicalDevice(VkPhysicalDevice device,
                                          PhysicalDeviceInfo& info) noexcept {}

void Application::SetObjectName(const VkObjectType type, const uint64_t handle,
                                const char* name) noexcept {
  static PFN_vkSetDebugUtilsObjectNameEXT func{reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
      vkGetInstanceProcAddr(mInstance, "vkSetDebugUtilsObjectNameEXT"))};
  if (func && mValidation) {
    const VkDebugUtilsObjectNameInfoEXT nameInfo{
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,  // sType
        nullptr,                                             // pNext
        type,                                                // objectType
        handle,                                              // objectHandle
        name                                                 // pObjectName
    };
    func(mDevice, &nameInfo);
  }
}
}  // namespace Raven