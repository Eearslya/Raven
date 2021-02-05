#include "Instance.h"

#include <vulkan/vulkan_win32.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace Raven {
namespace Vulkan {
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

InstanceBuilder& InstanceBuilder::SetAppName(const std::string& name) noexcept {
  mAppName = name;

  return *this;
}

InstanceBuilder& InstanceBuilder::SetAppVersion(uint32_t major, uint32_t minor,
                                                uint32_t patch) noexcept {
  mAppVersion = VK_MAKE_VERSION(major, minor, patch);

  return *this;
}

InstanceBuilder& InstanceBuilder::RequestValidation() noexcept {
  mRequestValidation = true;

  return *this;
}

InstanceBuilder& InstanceBuilder::SetApiVersion(uint32_t major, uint32_t minor) noexcept {
  mRequiredVersion = VK_MAKE_VERSION(major, minor, 0);

  return *this;
}

std::shared_ptr<Instance> InstanceBuilder::Build() const {
  const uint32_t instanceVersion{vk::enumerateInstanceVersion()};
  const auto availableLayers{vk::enumerateInstanceLayerProperties()};
  const auto availableExtensions{vk::enumerateInstanceExtensionProperties()};

  if (VK_VERSION_MAJOR(instanceVersion) < VK_VERSION_MAJOR(mRequiredVersion) ||
      VK_VERSION_MINOR(instanceVersion) < VK_VERSION_MINOR(mRequiredVersion)) {
    Log::Fatal("Required Vulkan version is not available! (Available: {}.{} | Required: {}.{})",
               VK_VERSION_MAJOR(instanceVersion), VK_VERSION_MINOR(instanceVersion),
               VK_VERSION_MAJOR(mRequiredVersion), VK_VERSION_MINOR(mRequiredVersion));
    throw std::runtime_error("Required Vulkan version is not available on this system!");
  }

  const vk::ApplicationInfo appInfo(mAppName.c_str(), mAppVersion, "Raven",
                                    VK_MAKE_VERSION(1, 0, 0), mRequiredVersion);
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

  vk::InstanceCreateInfo instanceCI({}, &appInfo, enabledLayers, enabledExtensions);
  if (validation) {
    instanceCI.setPNext(&gDebugMessengerCI);
  }

  return Instance::Create(instanceCI, validation);
}

Instance::Instance(vk::Instance instance, bool validation)
    : mInstance(std::move(instance)), mValidation(validation) {
  if (mValidation) {
    mDebugMessenger = mInstance.createDebugUtilsMessengerEXT(gDebugMessengerCI);
  }
}

Instance::~Instance() {
  if (mValidation) {
    mInstance.destroyDebugUtilsMessengerEXT(mDebugMessenger);
  }
  mInstance.destroy();
}

bool Instance::HasValidation() const noexcept { return mValidation; }

vk::SurfaceKHR Instance::CreateSurface(HINSTANCE instance, HWND window) const {
  const vk::Win32SurfaceCreateInfoKHR surfaceCI({}, instance, window);

  return mInstance.createWin32SurfaceKHR(surfaceCI);
}

void Instance::DestroySurface(const vk::SurfaceKHR& surface) const noexcept {
  mInstance.destroySurfaceKHR(surface);
}

Instance::operator vk::Instance() const { return mInstance; }

Instance::operator VkInstance() const { return mInstance; }

std::shared_ptr<Instance> Instance::Create(const vk::InstanceCreateInfo& createInfo,
                                           bool validation) {
  vk::Instance instance{vk::createInstance(createInfo)};
  VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

  return std::make_shared<Instance>(instance, validation);
}
}  // namespace Vulkan
}  // namespace Raven