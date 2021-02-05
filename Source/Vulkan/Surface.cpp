#include "Surface.h"

#include <memory>

namespace Raven {
namespace Vulkan {
class Instance;

Surface::Surface(const std::shared_ptr<Instance>& instance, vk::SurfaceKHR surface)
    : mInstance(instance), mSurface(surface) {}

Surface::~Surface() { mInstance.DestroySurface(mSurface); }

Surface::operator vk::SurfaceKHR() { return mSurface; }

Surface::operator VkSurfaceKHR() { return mSurface; }
}  // namespace Vulkan
}  // namespace Raven