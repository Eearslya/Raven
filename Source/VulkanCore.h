#pragma once

#include <exception>
#include <fmt/core.h>
#include <unordered_map>
#include <vulkan/vulkan.h>

#define VkCall(x)                                                                                \
  do {                                                                                           \
    const VkResult tmpResult{x};                                                                 \
    if (tmpResult != VK_SUCCESS) {                                                               \
      const std::string msg{                                                                     \
          fmt::format("A fatal Vulkan exception has occurred.\nVulkan Call: {}\nResult: {}", #x, \
                      static_cast<int64_t>(tmpResult))};                                         \
      throw std::runtime_error(msg);                                                             \
    }                                                                                            \
  } while (0)

#define X(obj) \
  { obj, #obj }
const std::unordered_map<VkObjectType, const char*> gVkObjectTypes{
    X(VK_OBJECT_TYPE_UNKNOWN),
    X(VK_OBJECT_TYPE_INSTANCE),
    X(VK_OBJECT_TYPE_PHYSICAL_DEVICE),
    X(VK_OBJECT_TYPE_DEVICE),
    X(VK_OBJECT_TYPE_QUEUE),
    X(VK_OBJECT_TYPE_SEMAPHORE),
    X(VK_OBJECT_TYPE_COMMAND_BUFFER),
    X(VK_OBJECT_TYPE_FENCE),
    X(VK_OBJECT_TYPE_DEVICE_MEMORY),
    X(VK_OBJECT_TYPE_BUFFER),
    X(VK_OBJECT_TYPE_IMAGE),
    X(VK_OBJECT_TYPE_EVENT),
    X(VK_OBJECT_TYPE_QUERY_POOL),
    X(VK_OBJECT_TYPE_BUFFER_VIEW),
    X(VK_OBJECT_TYPE_IMAGE_VIEW),
    X(VK_OBJECT_TYPE_SHADER_MODULE),
    X(VK_OBJECT_TYPE_PIPELINE_CACHE),
    X(VK_OBJECT_TYPE_PIPELINE_LAYOUT),
    X(VK_OBJECT_TYPE_RENDER_PASS),
    X(VK_OBJECT_TYPE_PIPELINE),
    X(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT),
    X(VK_OBJECT_TYPE_SAMPLER),
    X(VK_OBJECT_TYPE_DESCRIPTOR_POOL),
    X(VK_OBJECT_TYPE_DESCRIPTOR_SET),
    X(VK_OBJECT_TYPE_FRAMEBUFFER),
    X(VK_OBJECT_TYPE_COMMAND_POOL),
    X(VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION),
    X(VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE),
    X(VK_OBJECT_TYPE_SURFACE_KHR),
    X(VK_OBJECT_TYPE_SWAPCHAIN_KHR),
    X(VK_OBJECT_TYPE_DISPLAY_KHR),
    X(VK_OBJECT_TYPE_DISPLAY_MODE_KHR),
    X(VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT),
    X(VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT),
    X(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR),
    X(VK_OBJECT_TYPE_VALIDATION_CACHE_EXT),
    X(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV),
    X(VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL),
    X(VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR),
    X(VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV),
    X(VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT)};
#undef X