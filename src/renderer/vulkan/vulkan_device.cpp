#include "renderer/vulkan/vulkan_device.hpp"

#include "renderer/vulkan/vulkan_settings.hpp"

struct QueueFamilyIndices {
    std::optional<uint32> graphics_family;
    std::optional<uint32> compute_family;
    std::optional<uint32> transfer_family;
    std::optional<uint32> present_family;

    bool is_complete() {
        return
            (!VulkanSettings::graphics_family_required || graphics_family.has_value()) &&
            (!VulkanSettings::compute__family_required || compute_family.has_value()) &&
            (!VulkanSettings::transfer_family_required || transfer_family.has_value()) &&
            (!VulkanSettings::present__family_required || present_family.has_value());
    }
};

QueueFamilyIndices find_queue_families(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface);
int32 rate_device_suitability(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface);


VulkanDevice::VulkanDevice(vk::Instance* instance, const vk::SurfaceKHR& surface) : _vulkan_instance(instance) {
    pick_physical_device(surface);
    create_logical_device(surface);
}
VulkanDevice::~VulkanDevice() {
    _logical_device.destroy();
}

// /////////////////////////////// //
// Vulkan device private functions //
// /////////////////////////////// //

void VulkanDevice::pick_physical_device(const vk::SurfaceKHR& surface) {
    auto devices = _vulkan_instance->enumeratePhysicalDevices();

    if (devices.size() == 0)
        throw std::runtime_error("Failed to find GPUs with Vulkan support.");

    int best_suitability = 0;
    for (const auto& device : devices) {
        int32 device_suitability = rate_device_suitability(device, surface);
        if (device_suitability > best_suitability) {
            best_suitability = device_suitability;
            _physical_device = device;
        }
    }

    if (best_suitability == 0)
        throw std::runtime_error("Failed to find a suitable GPU.");
}

void VulkanDevice::create_logical_device(const vk::SurfaceKHR& surface) {
    // Queues used
    QueueFamilyIndices indices = find_queue_families(_physical_device, surface);

    std::set<uint32> unique_indices = {
        indices.graphics_family.value_or(-1),
        indices.compute_family.value_or(-1),
        indices.transfer_family.value_or(-1),
        indices.present_family.value_or(-1)
    };

    float32 queue_priority = 1.0f;

    // Creation info for each queue used
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos{};
    for (auto queue_index : unique_indices) {
        if (queue_index == -1) continue;
        vk::DeviceQueueCreateInfo queue_create_info;
        queue_create_info.setQueueFamilyIndex(queue_index);
        queue_create_info.setQueueCount(1);
        queue_create_info.setPQueuePriorities(&queue_priority);
        queue_create_infos.push_back(queue_create_info);
    }

    // Used features
    vk::PhysicalDeviceFeatures device_features{};

    // Creating the device
    vk::DeviceCreateInfo create_info{};
    create_info.setQueueCreateInfoCount(static_cast<uint32>(queue_create_infos.size()));
    create_info.setPQueueCreateInfos(queue_create_infos.data());
    create_info.setPEnabledFeatures(&device_features);

    vk::Result result = _physical_device.createDevice(&create_info, nullptr, &_logical_device);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create logical device.");

    // Retrieving queue handles
    if (VulkanSettings::graphics_family_required)
        _graphics_queue = _logical_device.getQueue(indices.graphics_family.value(), 0);
    if (VulkanSettings::present__family_required)
        _presentation_queue = _logical_device.getQueue(indices.present_family.value(), 0);
}

// ////////////////////////////// //
// Vulkan device public functions //
// ////////////////////////////// //

// ////////////////////////////// //
// Vulkan device helper functions //
// ////////////////////////////// //

QueueFamilyIndices find_queue_families(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface) {
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
        if (device.getSurfaceSupportKHR(i, surface) == true) {
            indices.present_family = i;
        }
        i++;
    }

    return indices;
}

int32 rate_device_suitability(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface) {
    vk::PhysicalDeviceProperties device_properties = device.getProperties();
    vk::PhysicalDeviceFeatures device_features = device.getFeatures();
    vk::PhysicalDeviceMemoryProperties device_memory = device.getMemoryProperties();

    auto queue_family_indices = find_queue_families(device, surface);

    // Is device suitable at all
    if (!queue_family_indices.is_complete())
        return 0;

    int32 suitability_score = VulkanSettings::base_score;

    // Prefer discrete GPU-s
    if (device_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        suitability_score += VulkanSettings::discrete_gpu_score;

    // Maximum possible size of textures affects graphics quality
    suitability_score += VulkanSettings::max_texture_size_weight * device_properties.limits.maxImageDimension2D;

    return suitability_score;
}