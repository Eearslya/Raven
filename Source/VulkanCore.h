#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VK_USE_PLATFORM_WIN32_KHR

#include <exception>
#include <fmt/core.h>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

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