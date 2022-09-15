#include "renderer/vulkan/vulkan_device.hpp"
#include "renderer/vulkan/vulkan_settings.hpp"

// Helper function forward declaration
bool check_device_extension_support(const vk::PhysicalDevice& device);
bool device_supports_required_features(const vk::PhysicalDeviceFeatures& features);

// Constructor & Destructor
VulkanDevice::VulkanDevice(
    const vk::Instance& vulkan_instance,
    const vk::SurfaceKHR& vulkan_surface,
    const vk::AllocationCallbacks* const allocator
) : _allocator(allocator) {
    // Pick physical device
    auto physical_device = pick_physical_device(vulkan_instance, vulkan_surface);

    // Remember device queue family indices
    queue_family_indices = find_queue_families(physical_device, vulkan_surface);

    // Get other physical device info
    info = get_physical_device_info(physical_device);

    // Log basic device info
    Logger::log("Suitable vulkan device found.");
    Logger::log("Device selected\t\t: ", info.name);
    Logger::log("GPU type\t\t\t: ", info.type);
    Logger::log("GPU driver version\t: ", info.driver_version);
    Logger::log("Vulkan api version\t: ", info.api_version);
    for (uint32 i = 0; i < info.memory_size_in_gb.size(); i++) {
        if (info.memory_is_local[i])
            Logger::log("Local GPU memory\t\t: ", info.memory_size_in_gb[i], " GiB.");
        else
            Logger::log("Shared GPU memory\t: ", info.memory_size_in_gb[i], " GiB.");
    }

    // Create logical device
    handle = create_logical_device(physical_device);

    // Retrieving queue handles
    if (VulkanSettings::graphics_family_required)
        graphics_queue = handle.getQueue(queue_family_indices.graphics_family.value(), 0);
    if (VulkanSettings::present__family_required)
        presentation_queue = handle.getQueue(queue_family_indices.present_family.value(), 0);
    if (VulkanSettings::transfer_family_required)
        transfer_queue = handle.getQueue(queue_family_indices.transfer_family.value(), 0);
    if (VulkanSettings::compute__family_required)
        compute_queue = handle.getQueue(queue_family_indices.compute_family.value(), 0);
}

VulkanDevice::~VulkanDevice() {
    handle.destroy(_allocator);
}

// /////////////////////////////// //
// Vulkan device private functions //
// /////////////////////////////// //

vk::PhysicalDevice VulkanDevice::pick_physical_device(
    const vk::Instance& vulkan_instance,
    const vk::SurfaceKHR& vulkan_surface
) const {
    // Get list of physical devices with vulkan support
    auto devices = vulkan_instance.enumeratePhysicalDevices();

    if (devices.size() == 0)
        Logger::fatal("Failed to find GPUs with Vulkan support.");

    // Find the most suitable physical device
    vk::PhysicalDevice best_device;
    int32 best_device_suitability = 0;
    for (const auto& device : devices) {
        auto device_suitability = rate_device_suitability(device, vulkan_surface);
        if (device_suitability > best_device_suitability) {
            best_device_suitability = device_suitability;
            best_device = device;
        }
    }

    if (best_device_suitability == 0)
        Logger::fatal("Failed to find a suitable GPU.");

    return best_device;
}

vk::Device VulkanDevice::create_logical_device(const vk::PhysicalDevice physical_device) const {
    // Queue indices used
    auto unique_indices = queue_family_indices.get_unique_indices();

    float32 queue_priority = 1.0f;

    // Creation info for each queue used
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos{};
    for (auto queue_index : unique_indices) {
        vk::DeviceQueueCreateInfo queue_create_info;
        // TODO: more flexible queue count setting
        // TODO: non-flat queue priority
        queue_create_info.setQueueFamilyIndex(queue_index);     // Queue family index
        queue_create_info.setQueueCount(1);                     // Number of queues for given family. 
        queue_create_info.setPQueuePriorities(&queue_priority); // Scheduling priority
        queue_create_infos.push_back(queue_create_info);
    }

    // Used features (automatically use required)
    auto device_features = vk::PhysicalDeviceFeatures(VulkanSettings::required_device_features);

    // Creating the logical device with required features and extensions enabled
    vk::DeviceCreateInfo create_info{};
    create_info.setQueueCreateInfos(queue_create_infos);
    create_info.setPEnabledFeatures(&device_features);
    create_info.setPEnabledExtensionNames(VulkanSettings::device_required_extensions);

    try {
        return physical_device.createDevice(create_info, _allocator);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }
    return vk::Device();
}

PhysicalDeviceInfo VulkanDevice::get_physical_device_info(
    const vk::PhysicalDevice physical_device
) const {
    vk::PhysicalDeviceProperties device_properties = physical_device.getProperties();
    vk::PhysicalDeviceMemoryProperties device_memory = physical_device.getMemoryProperties();

    // Fill device info
    PhysicalDeviceInfo device_info{};

    // Info from properties
    device_info.name = std::string(device_properties.deviceName);   // Device name
    device_info.type = vk::to_string(device_properties.deviceType); // Integrated or Dedicated GPU
    device_info.driver_version =                                    // Driver version number
        std::to_string(VK_VERSION_MAJOR(device_properties.driverVersion)) + "." +
        std::to_string(VK_VERSION_MINOR(device_properties.driverVersion)) + "." +
        std::to_string(VK_VERSION_PATCH(device_properties.driverVersion));
    device_info.api_version =                                       // Vulkan API version number
        std::to_string(VK_VERSION_MAJOR(device_properties.apiVersion)) + "." +
        std::to_string(VK_VERSION_MINOR(device_properties.apiVersion)) + "." +
        std::to_string(VK_VERSION_PATCH(device_properties.apiVersion));
    // Maximum number of samples used for anisotropic filtering (for textures)
    device_info.max_sampler_anisotropy = device_properties.limits.maxSamplerAnisotropy;
    // Maximum number of samples used for Multisample Anti-aliasing (max of the two)
    device_info.framebuffer_color_sample_counts = device_properties.limits.framebufferColorSampleCounts;
    device_info.framebuffer_depth_sample_counts = device_properties.limits.framebufferDepthSampleCounts;

    // Info from memory properties
    device_info.memory_size_in_gb.resize(device_memory.memoryHeapCount);
    device_info.memory_types.resize(device_memory.memoryTypeCount);
    device_info.memory_is_local.resize(device_memory.memoryHeapCount);

    for (uint32 i = 0; i < device_memory.memoryHeapCount; i++) {
        device_info.memory_size_in_gb[i] = 1.0f * device_memory.memoryHeaps[i].size / 1024.f / 1024.f / 1024.f;
        device_info.memory_is_local[i] = (bool) (device_memory.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal);
    }
    for (uint32 i = 0; i < device_memory.memoryTypeCount; i++)
        device_info.memory_types[i] = device_memory.memoryTypes[i];

    // Queue swapchain support details
    device_info.get_swapchain_support_details = [=](const vk::SurfaceKHR& surface)->SwapchainSupportDetails {
        return query_swapchain_support_details(physical_device, surface);
    };

    // Format properties callback
    device_info.get_format_properties = [=](const vk::Format format)->vk::FormatProperties {
        return physical_device.getFormatProperties(format);
    };

    return device_info;
}

QueueFamilyIndices VulkanDevice::find_queue_families(
    const vk::PhysicalDevice& device,
    const vk::SurfaceKHR& vulkan_surface
) const {
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
        if (device.getSurfaceSupportKHR(i, vulkan_surface) == true) {
            indices.present_family = i;
        }
        i++;
    }

    return indices;
}

int32 VulkanDevice::rate_device_suitability(
    const vk::PhysicalDevice& device,
    const vk::SurfaceKHR& vulkan_surface
) const {
    vk::PhysicalDeviceProperties device_properties = device.getProperties();
    vk::PhysicalDeviceFeatures device_features = device.getFeatures();

    // Is device suitable at all
    auto queue_family_indices = find_queue_families(device, vulkan_surface);
    if (!queue_family_indices.is_complete() ||              // Device must posses all required queue families
        !check_device_extension_support(device) ||          // Device must support all required extensions
        !device_supports_required_features(device_features) // Device must posses all required features
        ) return {};
    auto swapchain_support = query_swapchain_support_details(device, vulkan_surface);
    if (swapchain_support.formats.empty() ||                // Device support at least one format
        swapchain_support.presentation_modes.empty()        // Device support at least one presentation mode
        ) return {};

    // Compute suitability score of suitable device
    int32 suitability_score = VulkanSettings::base_score;

    // Prefer discrete GPU-s
    if (device_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        suitability_score += VulkanSettings::discrete_gpu_score;

    // Maximum possible size of textures affects graphics quality
    suitability_score += VulkanSettings::max_texture_size_weight * device_properties.limits.maxImageDimension2D;

    return suitability_score;
}

SwapchainSupportDetails VulkanDevice::query_swapchain_support_details(
    const vk::PhysicalDevice& device,
    const vk::SurfaceKHR& surface
) const {
    // Get basic swapchain info
    SwapchainSupportDetails support_details;

    support_details.capabilities = device.getSurfaceCapabilitiesKHR(surface);       // Get surface capabilities
    support_details.formats = device.getSurfaceFormatsKHR(surface);                 // Get surface formats
    support_details.presentation_modes = device.getSurfacePresentModesKHR(surface); // Get present modes

    return support_details;
}

// ////////////////////////////// //
// Vulkan device public functions //
// ////////////////////////////// //

uint32 VulkanDevice::find_memory_type(
    const uint32 type_filter,
    const vk::MemoryPropertyFlags properties
) const {
    for (uint32 i = 0; i < info.memory_types.size(); i++) {
        if ((type_filter & (1 << i)) &&
            (info.memory_types[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("Failed to find suitable memory type.");
}

// ////////////////////////////// //
// Vulkan device helper functions //
// ////////////////////////////// //

bool check_device_extension_support(const vk::PhysicalDevice& device) {
    auto available_extensions = device.enumerateDeviceExtensionProperties();

    std::set<std::string> required_extensions{
        VulkanSettings::device_required_extensions.begin(),
        VulkanSettings::device_required_extensions.end()
    };

    // Check if all required extensions are available
    for (const auto& extension : available_extensions) {
        required_extensions.erase(extension.extensionName);
        if (required_extensions.empty()) return true;
    }

    return false;
}

bool device_supports_required_features(const vk::PhysicalDeviceFeatures& features) {
    auto required_features = VulkanSettings::required_device_features;
    return
        (!required_features.robustBufferAccess || features.robustBufferAccess) &&
        (!required_features.fullDrawIndexUint32 || features.fullDrawIndexUint32) &&
        (!required_features.imageCubeArray || features.imageCubeArray) &&
        (!required_features.independentBlend || features.independentBlend) &&
        (!required_features.geometryShader || features.geometryShader) &&
        (!required_features.tessellationShader || features.tessellationShader) &&
        (!required_features.sampleRateShading || features.sampleRateShading) &&
        (!required_features.dualSrcBlend || features.dualSrcBlend) &&
        (!required_features.logicOp || features.logicOp) &&
        (!required_features.multiDrawIndirect || features.multiDrawIndirect) &&
        (!required_features.drawIndirectFirstInstance || features.drawIndirectFirstInstance) &&
        (!required_features.depthClamp || features.depthClamp) &&
        (!required_features.depthBiasClamp || features.depthBiasClamp) &&
        (!required_features.fillModeNonSolid || features.fillModeNonSolid) &&
        (!required_features.depthBounds || features.depthBounds) &&
        (!required_features.wideLines || features.wideLines) &&
        (!required_features.largePoints || features.largePoints) &&
        (!required_features.alphaToOne || features.alphaToOne) &&
        (!required_features.multiViewport || features.multiViewport) &&
        (!required_features.samplerAnisotropy || features.samplerAnisotropy) &&
        (!required_features.textureCompressionETC2 || features.textureCompressionETC2) &&
        (!required_features.textureCompressionASTC_LDR || features.textureCompressionASTC_LDR) &&
        (!required_features.textureCompressionBC || features.textureCompressionBC) &&
        (!required_features.occlusionQueryPrecise || features.occlusionQueryPrecise) &&
        (!required_features.pipelineStatisticsQuery || features.pipelineStatisticsQuery) &&
        (!required_features.vertexPipelineStoresAndAtomics || features.vertexPipelineStoresAndAtomics) &&
        (!required_features.fragmentStoresAndAtomics || features.fragmentStoresAndAtomics) &&
        (!required_features.shaderTessellationAndGeometryPointSize || features.shaderTessellationAndGeometryPointSize) &&
        (!required_features.shaderImageGatherExtended || features.shaderImageGatherExtended) &&
        (!required_features.shaderStorageImageExtendedFormats || features.shaderStorageImageExtendedFormats) &&
        (!required_features.shaderStorageImageMultisample || features.shaderStorageImageMultisample) &&
        (!required_features.shaderStorageImageReadWithoutFormat || features.shaderStorageImageReadWithoutFormat) &&
        (!required_features.shaderStorageImageWriteWithoutFormat || features.shaderStorageImageWriteWithoutFormat) &&
        (!required_features.shaderUniformBufferArrayDynamicIndexing || features.shaderUniformBufferArrayDynamicIndexing) &&
        (!required_features.shaderSampledImageArrayDynamicIndexing || features.shaderSampledImageArrayDynamicIndexing) &&
        (!required_features.shaderStorageBufferArrayDynamicIndexing || features.shaderStorageBufferArrayDynamicIndexing) &&
        (!required_features.shaderStorageImageArrayDynamicIndexing || features.shaderStorageImageArrayDynamicIndexing) &&
        (!required_features.shaderClipDistance || features.shaderClipDistance) &&
        (!required_features.shaderCullDistance || features.shaderCullDistance) &&
        (!required_features.shaderFloat64 || features.shaderFloat64) &&
        (!required_features.shaderInt64 || features.shaderInt64) &&
        (!required_features.shaderInt16 || features.shaderInt16) &&
        (!required_features.shaderResourceResidency || features.shaderResourceResidency) &&
        (!required_features.shaderResourceMinLod || features.shaderResourceMinLod) &&
        (!required_features.sparseBinding || features.sparseBinding) &&
        (!required_features.sparseResidencyBuffer || features.sparseResidencyBuffer) &&
        (!required_features.sparseResidencyImage2D || features.sparseResidencyImage2D) &&
        (!required_features.sparseResidencyImage3D || features.sparseResidencyImage3D) &&
        (!required_features.sparseResidency2Samples || features.sparseResidency2Samples) &&
        (!required_features.sparseResidency4Samples || features.sparseResidency4Samples) &&
        (!required_features.sparseResidency8Samples || features.sparseResidency8Samples) &&
        (!required_features.sparseResidency16Samples || features.sparseResidency16Samples) &&
        (!required_features.sparseResidencyAliased || features.sparseResidencyAliased) &&
        (!required_features.variableMultisampleRate || features.variableMultisampleRate) &&
        (!required_features.inheritedQueries || features.inheritedQueries);
}