#include "renderer/vulkan/vulkan_backend.hpp"

#include "logger.hpp"
#include "renderer/vulkan/vulkan_settings.hpp"

// Helper function forward declaration
bool check_device_extension_support(const vk::PhysicalDevice& device);
bool device_supports_required_features(const vk::PhysicalDeviceFeatures& features);


// /////////////////////////////// //
// Vulkan device private functions //
// /////////////////////////////// //

vk::PhysicalDevice VulkanBackend::pick_physical_device() {
    // Get list of physical devices with vulkan support
    auto devices = _vulkan_instance.enumeratePhysicalDevices();

    if (devices.size() == 0)
        throw std::runtime_error("Failed to find GPUs with Vulkan support.");

    // Find the most suitable device
    vk::PhysicalDevice best_device;
    int32 best_device_suitability = 0;
    for (const auto& device : devices) {
        auto device_suitability = rate_device_suitability(device);
        if (device_suitability > best_device_suitability) {
            best_device_suitability = device_suitability;
            best_device = device;
        }
    }

    if (best_device_suitability == 0)
        throw std::runtime_error("Failed to find a suitable GPU.");

    // Remember device queue family indices
    _queue_family_indices = find_queue_families(best_device);

    // Get other physical device info
    _physical_device_info = get_physical_device_info(best_device);

    // Set maximum MSAA samples
    auto count = _physical_device_info.framebuffer_color_sample_counts &
        _physical_device_info.framebuffer_depth_sample_counts;

    _msaa_samples = vk::SampleCountFlagBits::e1;
    if (count & vk::SampleCountFlagBits::e64) _msaa_samples = vk::SampleCountFlagBits::e64;
    else if (count & vk::SampleCountFlagBits::e32) _msaa_samples = vk::SampleCountFlagBits::e32;
    else if (count & vk::SampleCountFlagBits::e16) _msaa_samples = vk::SampleCountFlagBits::e16;
    else if (count & vk::SampleCountFlagBits::e8) _msaa_samples = vk::SampleCountFlagBits::e8;
    else if (count & vk::SampleCountFlagBits::e4) _msaa_samples = vk::SampleCountFlagBits::e4;
    else if (count & vk::SampleCountFlagBits::e2) _msaa_samples = vk::SampleCountFlagBits::e2;

    if (_msaa_samples > VulkanSettings::max_msaa_samples)
        _msaa_samples = VulkanSettings::max_msaa_samples;

    // Best device selected, log results
    Logger::log("Suitable vulkan device found.");
    Logger::log("Device selected\t\t: ", _physical_device_info.name);
    Logger::log("GPU type\t\t\t: ", _physical_device_info.type);
    Logger::log("GPU driver version\t: ", _physical_device_info.driver_version);
    Logger::log("Vulkan api version\t: ", _physical_device_info.api_version);
    for (uint32 i = 0; i < _physical_device_info.memory_size_in_gb.size(); i++) {
        if (_physical_device_info.memory_is_local[i])
            Logger::log("Local GPU memory\t\t: ", _physical_device_info.memory_size_in_gb[i], " GiB.");
        else
            Logger::log("Shared GPU memory\t: ", _physical_device_info.memory_size_in_gb[i], " GiB.");
    }

    return best_device;
}

PhysicalDeviceInfo VulkanBackend::get_physical_device_info(vk::PhysicalDevice physical_device) {
    vk::PhysicalDeviceProperties device_properties = physical_device.getProperties();
    // vk::PhysicalDeviceFeatures device_features = physical_device.getFeatures();
    vk::PhysicalDeviceMemoryProperties device_memory = physical_device.getMemoryProperties();

    // Fill device info
    PhysicalDeviceInfo device_info{};

    // Info from properties
    device_info.name = std::string(device_properties.deviceName);
    device_info.type = vk::to_string(device_properties.deviceType);
    device_info.driver_version =
        std::to_string(VK_VERSION_MAJOR(device_properties.driverVersion)) + "." +
        std::to_string(VK_VERSION_MINOR(device_properties.driverVersion)) + "." +
        std::to_string(VK_VERSION_PATCH(device_properties.driverVersion));
    device_info.api_version =
        std::to_string(VK_VERSION_MAJOR(device_properties.apiVersion)) + "." +
        std::to_string(VK_VERSION_MINOR(device_properties.apiVersion)) + "." +
        std::to_string(VK_VERSION_PATCH(device_properties.apiVersion));
    device_info.max_sampler_anisotropy = device_properties.limits.maxSamplerAnisotropy;
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
    device_info.get_swapchain_support_details = [=]()->SwapchainSupportDetails {
        return query_swapchain_support_details(physical_device);
    };

    // Format properties callback
    device_info.get_format_properties = [=](vk::Format format)->vk::FormatProperties {
        return physical_device.getFormatProperties(format);
    };

    return device_info;
}

void VulkanBackend::create_logical_device(vk::PhysicalDevice physical_device) {
    // Queues used
    auto unique_indices = _queue_family_indices.get_unique_indices();

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

    // Creating the logical device
    vk::DeviceCreateInfo create_info{};
    create_info.setQueueCreateInfoCount(static_cast<uint32>(queue_create_infos.size()));
    create_info.setPQueueCreateInfos(queue_create_infos.data());
    create_info.setPEnabledFeatures(&device_features);
    create_info.setEnabledExtensionCount(static_cast<uint32>(VulkanSettings::device_required_extensions.size()));
    create_info.setPpEnabledExtensionNames(VulkanSettings::device_required_extensions.data());

    vk::Result result = physical_device.createDevice(&create_info, _allocator, &_device);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create logical device.");

    // Retrieving queue handles
    if (VulkanSettings::graphics_family_required)
        _graphics_queue = _device.getQueue(_queue_family_indices.graphics_family.value(), 0);
    if (VulkanSettings::present__family_required)
        _presentation_queue = _device.getQueue(_queue_family_indices.present_family.value(), 0);
    if (VulkanSettings::transfer_family_required)
        _transfer_queue = _device.getQueue(_queue_family_indices.transfer_family.value(), 0);
    if (VulkanSettings::compute__family_required)
        _compute_queue = _device.getQueue(_queue_family_indices.compute_family.value(), 0);
}

QueueFamilyIndices VulkanBackend::find_queue_families(const vk::PhysicalDevice& device) {
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
        if (device.getSurfaceSupportKHR(i, _vulkan_surface) == true) {
            indices.present_family = i;
        }
        i++;
    }

    return indices;
}

int32 VulkanBackend::rate_device_suitability(const vk::PhysicalDevice& device) {
    vk::PhysicalDeviceProperties device_properties = device.getProperties();
    vk::PhysicalDeviceFeatures device_features = device.getFeatures();

    // Is device suitable at all
    auto queue_family_indices = find_queue_families(device);
    if (!queue_family_indices.is_complete() ||              // Device must posses all required queue families
        !check_device_extension_support(device) ||          // Device must support all required extensions
        !device_supports_required_features(device_features) // Device must posses all required features
        ) return {};
    auto swapchain_support = query_swapchain_support_details(device);
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

SwapchainSupportDetails VulkanBackend::query_swapchain_support_details(const vk::PhysicalDevice& device) {
    SwapchainSupportDetails support_details;

    support_details.capabilities = device.getSurfaceCapabilitiesKHR(_vulkan_surface);       // Get surface capabilities
    support_details.formats = device.getSurfaceFormatsKHR(_vulkan_surface);                 // Get surface formats
    support_details.presentation_modes = device.getSurfacePresentModesKHR(_vulkan_surface); // Get present modes

    return support_details;
}

// ////////////////////////////// //
// Vulkan device public functions //
// ////////////////////////////// //


// ////////////////////////////// //
// Vulkan device helper functions //
// ////////////////////////////// //

bool check_device_extension_support(const vk::PhysicalDevice& device) {
    auto available_extensions = device.enumerateDeviceExtensionProperties();

    std::set<std::string> required_extensions{
        VulkanSettings::device_required_extensions.begin(),
        VulkanSettings::device_required_extensions.end()
    };

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