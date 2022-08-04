#include "renderer/vulkan/vulkan_device.hpp"

struct QueueFamilyIndices {
    std::optional<uint32> graphics_family;
    std::optional<uint32> present_family;
    std::optional<uint32> compute_family;
    std::optional<uint32> transfer_family;

    bool is_complete() {
        return
            graphics_family.has_value() &&
            present_family.has_value() &&
            compute_family.has_value() &&
            transfer_family.has_value()
            ;
    }
};

QueueFamilyIndices find_queue_families(vk::PhysicalDevice device);
int32 rate_device_suitability(vk::PhysicalDevice device);


VulkanDevice::VulkanDevice(vk::Instance* instance) : _vulkan_instance(instance) {
    pick_physical_device();
    create_logical_device();
}
VulkanDevice::~VulkanDevice() {
    _logical_device.destroy();
}

// /////////////////////////////// //
// Vulkan device private functions //
// /////////////////////////////// //

void VulkanDevice::pick_physical_device() {
    auto devices = _vulkan_instance->enumeratePhysicalDevices();

    if (devices.size() == 0)
        throw std::runtime_error("Failed to find GPUs with Vulkan support.");

    int best_suitability = 0;
    for (const auto& device : devices) {
        int32 device_suitability = rate_device_suitability(device);
        if (device_suitability > best_suitability) {
            best_suitability = device_suitability;
            _physical_device = device;
        }
    }

    if (best_suitability == 0)
        throw std::runtime_error("Failed to find a suitable GPU.");
}

void VulkanDevice::create_logical_device() {
    // Queues used
    QueueFamilyIndices indices = find_queue_families(_physical_device);
    vk::DeviceQueueCreateInfo queue_create_info{};
    queue_create_info.setQueueFamilyIndex(indices.graphics_family.value());
    queue_create_info.setQueueCount(1);

    // Queue priority
    float32 queue_priority = 1.0f;
    queue_create_info.setPQueuePriorities(&queue_priority);

    // Used features
    vk::PhysicalDeviceFeatures device_features{};

    // Creating the device
    vk::DeviceCreateInfo create_info{};
    create_info.setPQueueCreateInfos(&queue_create_info);
    create_info.setQueueCreateInfoCount(1);
    create_info.setPEnabledFeatures(&device_features);

    vk::Result result = _physical_device.createDevice(&create_info, nullptr, &_logical_device);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create logical device.");

    // Retrieving queue handles
    _graphics_queue = _logical_device.getQueue(indices.graphics_family.value(), 0);
}

// ////////////////////////////// //
// Vulkan device public functions //
// ////////////////////////////// //

// ////////////////////////////// //
// Vulkan device helper functions //
// ////////////////////////////// //

QueueFamilyIndices find_queue_families(vk::PhysicalDevice device) {
    QueueFamilyIndices indices;

    // Get available device queue families
    auto queue_families = device.getQueueFamilyProperties();

    uint32 i = 0;
    uint8 min_weight = -1;
    for (const auto& queue_family : queue_families) {
        uint32 queue_family_weight = 0;
        if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphics_family = i;
            queue_family_weight++;
        }
        if (queue_family.queueFlags & vk::QueueFlagBits::eCompute) {
            indices.compute_family = i;
            queue_family_weight++;
        }
        if (queue_family.queueFlags & vk::QueueFlagBits::eTransfer) {
            // We are searching for a queue family dedicated for transfers
            // Most likely to be the one with the minimum number of other functionalities
            if (queue_family_weight < min_weight) {
                min_weight = queue_family_weight;
                indices.transfer_family = i;
            }
        }
        if (device.getSurfaceSupportKHR(i, nullptr) == true) {
            indices.present_family = i;
        }
        i++;
    }

    return indices;
}

int32 rate_device_suitability(vk::PhysicalDevice device) {
    vk::PhysicalDeviceProperties device_properties = device.getProperties();
    vk::PhysicalDeviceFeatures device_features = device.getFeatures();
    vk::PhysicalDeviceMemoryProperties device_memory = device.getMemoryProperties();

    auto queue_family_indices = find_queue_families(device);

    // Is device suitable at all
    if (!queue_family_indices.is_complete())
        return 0;

    int32 suitability_score = 1;

    // Prefer discrete GPU-s
    if (device_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        suitability_score += 1000;

    // Maximum possible size of textures affects graphics quality
    suitability_score += device_properties.limits.maxImageDimension2D;

    return suitability_score;
}