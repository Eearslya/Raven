#pragma once

#include "VulkanCore.h"

namespace Raven {
namespace Vulkan {
class Instance;

class PhysicalDevice final {};

class PhysicalDeviceSelector final {
 public:
  PhysicalDeviceSelector(const std::shared_ptr<Instance>& instance);

  std::shared_ptr<PhysicalDevice> Build() const;
};
}  // namespace Vulkan
}  // namespace Raven