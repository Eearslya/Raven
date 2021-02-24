#include <Raven/Graphics/Context.hpp>
#include <Raven/WSI.hpp>
#include <mutex>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

namespace Raven {
namespace Graphics {
class ContextImpl {
  constexpr const static unsigned int MaxQueueFamilies{8};
  constexpr const static unsigned int MaxQueuesPerFamily{8};

  struct QueueFamily final {
    vk::QueueFlags Flags;
    uint32_t FamilyIndex;
    uint32_t QueueCount;
    std::mutex Sync[MaxQueuesPerFamily];
    vk::Queue Queues[MaxQueuesPerFamily];
  };

 public:
  ContextImpl(const std::string& appName, uint32_t appVersion) {
    const vk::ApplicationInfo appInfo(appName.c_str(), appVersion, "Raven",
                                      VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_1);

    const std::vector<const char*> extensions{WindowGetVulkanExtensions()};

    const vk::InstanceCreateInfo instanceCI({}, &appInfo, nullptr, extensions);
    Instance = vk::createInstance(instanceCI);

    const std::vector<vk::PhysicalDevice> physicalDevices{Instance.enumeratePhysicalDevices()};
    if (physicalDevices.size() == 0) {
      throw std::runtime_error("No compatible graphics devices are available!");
    }
    PhysicalDevice = physicalDevices[0];

    const std::vector<vk::QueueFamilyProperties> families{
        PhysicalDevice.getQueueFamilyProperties()};
    std::vector<vk::DeviceQueueCreateInfo> queueCIs(families.size());

    uint32_t highestQueueCount{0};
    for (size_t i = 0; i < families.size(); i++) {
      QueueFamily& fam{QueueFamilies[i]};
      fam.Flags = families[i].queueFlags;
      fam.FamilyIndex = i;
      fam.QueueCount = families[i].queueCount;
      queueCIs[i] = vk::DeviceQueueCreateInfo({}, i);
      highestQueueCount = std::max(highestQueueCount, families[i].queueCount);
    }

    std::vector<float> queuePriorities(highestQueueCount, 0.0f);
    for (size_t i = 0; i < families.size(); i++) {
      queueCIs[i].queueCount = highestQueueCount;
      queueCIs[i].pQueuePriorities = queuePriorities.data();
    }

    const vk::DeviceCreateInfo deviceCI({}, queueCIs);
    Device = PhysicalDevice.createDevice(deviceCI);

    for (size_t i = 0; i < families.size(); i++) {
      const size_t queues{std::min(MaxQueuesPerFamily, queueCIs[i].queueCount)};
      for (size_t q = 0; q < queues; q++) {
        Device.getQueue(i, q, &QueueFamilies[i].Queues[q]);
      }
    }
  }

  ~ContextImpl() {
    if (Device) {
      Device.waitIdle();
      Device.destroy();
    }
    if (Instance) {
      Instance.destroy();
    }
  }

  vk::Instance Instance;
  vk::PhysicalDevice PhysicalDevice;
  vk::Device Device;

  uint32_t QueueFamilyCount{0};
  QueueFamily QueueFamilies[MaxQueueFamilies];
};

GraphicsContext::GraphicsContext(const std::string& appName, uint32_t appVersion) :
    mImpl(rvn::MakeUniqueImpl<ContextImpl>(appName, appVersion)) {}
}  // namespace Graphics
}  // namespace Raven