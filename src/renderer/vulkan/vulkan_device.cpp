#include "renderer/vulkan/vulkan_device.hpp"

#include "logger.hpp"
#include "renderer/vulkan/vulkan_settings.hpp"

// Helper function forward declaration
bool check_device_extension_support(const vk::PhysicalDevice& device);
bool device_supports_required_features(const vk::PhysicalDeviceFeatures& features);

// Constructor & destructor
VulkanDevice::VulkanDevice(
    vk::Instance* instance,
    vk::AllocationCallbacks* allocator,
    Platform::Surface* surface
) : _vulkan_instance(instance), _vulkan_allocator(allocator), _surface(surface) {
    _vulkan_surface = surface->get_vulkan_surface(*_vulkan_instance, _vulkan_allocator);

    pick_physical_device();
    create_logical_device();
    create_swapchain();
    create_image_views();

    // TODO: TEMP PIPELINE CODE
    create_render_pass();
    create_pipeline();

    // TODO: TEMP FRAMEBUFFER CODE
    create_framebuffers();

    // TODO: TEMP COMMAND CODE
    create_command_pool();

    // TODO: TEMP VERTEX BUFFER CODE
    create_vertex_buffer();

    // TODO: TEMP INDEX BUFFER CODE
    create_index_buffer();

    // TODO: TEMP COMMAND CODE
    create_command_buffers();

    // TODO: TEMP SYNC CODE
    create_sync_objects();
}
VulkanDevice::~VulkanDevice() {
    cleanup_swapchain();

    // TODO: TEMP INDEX BUFFER CODE
    _logical_device.destroyBuffer(_index_buffer, _vulkan_allocator);
    _logical_device.freeMemory(_vertex_buffer_memory, _vulkan_allocator);

    // TODO: TEMP VERTEX BUFFER CODE
    _logical_device.destroyBuffer(_vertex_buffer, _vulkan_allocator);
    _logical_device.freeMemory(_vertex_buffer_memory, _vulkan_allocator);


    // TODO: TEMP PIPELINE CODE
    _logical_device.destroyPipeline(_graphics_pipeline, _vulkan_allocator);
    _logical_device.destroyPipelineLayout(_pipeline_layout, _vulkan_allocator);
    _logical_device.destroyRenderPass(_render_pass, _vulkan_allocator);


    // TODO: TEMP SYNC CODE
    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        _logical_device.destroySemaphore(_semaphores_image_available[i], _vulkan_allocator);
        _logical_device.destroySemaphore(_semaphores_render_finished[i], _vulkan_allocator);
        _logical_device.destroyFence(_fences_in_flight[i], _vulkan_allocator);
    }


    // TODO: TEMP COMMAND CODE
    _logical_device.destroyCommandPool(_command_pool, _vulkan_allocator);


    _logical_device.destroy(_vulkan_allocator);
    _vulkan_instance->destroySurfaceKHR(_vulkan_surface, _vulkan_allocator);
}

// /////////////////////////////// //
// Vulkan device private functions //
// /////////////////////////////// //

void VulkanDevice::pick_physical_device() {
    // Get list of physical devices with vulkan support
    auto devices = _vulkan_instance->enumeratePhysicalDevices();

    if (devices.size() == 0)
        throw std::runtime_error("Failed to find GPUs with Vulkan support.");

    // Find the most suitable device
    PhysicalDeviceInfo best_device_info{};
    for (const auto& device : devices) {
        auto device_info = rate_device_suitability(device);
        if (device_info.suitability > best_device_info.suitability) {
            best_device_info = device_info;
            _physical_device = device;
        }
    }

    if (best_device_info.suitability == 0)
        throw std::runtime_error("Failed to find a suitable GPU.");

    // Best device selected, log results
    Logger::log("Suitable vulkan device found.");
    Logger::log("Device selected\t\t: ", best_device_info.name);
    Logger::log("GPU type\t\t\t: ", best_device_info.type);
    Logger::log("GPU driver version\t: ", best_device_info.driver_version);
    Logger::log("Vulkan api version\t: ", best_device_info.api_version);
    for (uint32 i = 0; i < best_device_info.memory_size_in_gb.size(); i++) {
        if (best_device_info.memory_is_local[i])
            Logger::log("Local GPU memory\t\t: ", best_device_info.memory_size_in_gb[i], " GiB.");
        else
            Logger::log("Shared GPU memory\t: ", best_device_info.memory_size_in_gb[i], " GiB.");
    }
}

void VulkanDevice::create_logical_device() {
    // Queues used
    auto indices = find_queue_families(_physical_device);
    auto unique_indices = indices.get_unique_indices();

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

    vk::Result result = _physical_device.createDevice(&create_info, _vulkan_allocator, &_logical_device);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create logical device.");

    // Retrieving queue handles
    if (VulkanSettings::graphics_family_required)
        _graphics_queue = _logical_device.getQueue(indices.graphics_family.value(), 0);
    if (VulkanSettings::present__family_required)
        _presentation_queue = _logical_device.getQueue(indices.present_family.value(), 0);
    if (VulkanSettings::transfer_family_required)
        _transfer_queue = _logical_device.getQueue(indices.transfer_family.value(), 0);
    if (VulkanSettings::compute__family_required)
        _compute_queue = _logical_device.getQueue(indices.compute_family.value(), 0);
}

void VulkanDevice::create_swapchain() {
    SwapchainSupportDetails swapchain_support = query_swapchain_support_details(_physical_device);

    vk::Extent2D extent = swapchain_support.get_extent(_surface->get_width_in_pixels(), _surface->get_height_in_pixels());
    vk::SurfaceFormatKHR surface_format = swapchain_support.get_surface_format();
    vk::PresentModeKHR presentation_mode = swapchain_support.get_presentation_mode();

    uint32 min_image_count = swapchain_support.capabilities.minImageCount + 1;
    uint32 max_image_count = swapchain_support.capabilities.maxImageCount;
    if (max_image_count != 0 && min_image_count > max_image_count)
        min_image_count = max_image_count;

    vk::SwapchainCreateInfoKHR create_info{};
    create_info.setSurface(_vulkan_surface);                                //
    create_info.setMinImageCount(min_image_count);                          //
    create_info.setImageExtent(extent);                                     //
    create_info.setImageFormat(surface_format.format);                      //
    create_info.setImageColorSpace(surface_format.colorSpace);              //
    create_info.setPresentMode(presentation_mode);                          //
    create_info.setImageArrayLayers(1);                                     //
    create_info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);    // TODO: postprocessing
    create_info.setPreTransform(swapchain_support.capabilities.currentTransform); //
    create_info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);  //
    create_info.setClipped(true);                                           //
    create_info.setOldSwapchain(VK_NULL_HANDLE);                            //

    // indices
    auto indices = find_queue_families(_physical_device);
    const uint32 queue_family_indices[] = { indices.graphics_family.value(), indices.present_family.value() };
    if (indices.graphics_family.value() != indices.present_family.value()) {
        create_info.setImageSharingMode(vk::SharingMode::eConcurrent);
        create_info.setQueueFamilyIndexCount(2);
        create_info.setPQueueFamilyIndices(queue_family_indices);
    } else {
        create_info.setImageSharingMode(vk::SharingMode::eExclusive);
        create_info.setQueueFamilyIndexCount(0);
        create_info.setPQueueFamilyIndices(nullptr);
    }

    auto result = _logical_device.createSwapchainKHR(&create_info, _vulkan_allocator, &_swapchain);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create swapchain.");

    _swapchain_images = _logical_device.getSwapchainImagesKHR(_swapchain);
    _swapchain_format = surface_format.format;
    _swapchain_extent = extent;
}

void VulkanDevice::recreate_swapchain() {
    _logical_device.waitIdle();

    cleanup_swapchain();

    create_swapchain();
    create_image_views();
    create_framebuffers();
}

void VulkanDevice::cleanup_swapchain() {
    // TODO: TEMP FRAMEBUFFER CODE
    for (auto framebuffer : _swapchain_framebuffers)
        _logical_device.destroyFramebuffer(framebuffer, _vulkan_allocator);


    for (auto image_view : _swapchain_image_views)
        _logical_device.destroyImageView(image_view, _vulkan_allocator);

    _logical_device.destroySwapchainKHR(_swapchain, _vulkan_allocator);
}

void VulkanDevice::create_image_views() {
    _swapchain_image_views.resize(_swapchain_images.size());

    for (uint32 i = 0; i < _swapchain_images.size(); i++) {
        vk::ImageViewCreateInfo create_info{};
        create_info.setImage(_swapchain_images[i]);      //
        create_info.setViewType(vk::ImageViewType::e2D); //
        create_info.setFormat(_swapchain_format);        //

        // 
        create_info.components.setR(vk::ComponentSwizzle::eIdentity);
        create_info.components.setG(vk::ComponentSwizzle::eIdentity);
        create_info.components.setB(vk::ComponentSwizzle::eIdentity);
        create_info.components.setA(vk::ComponentSwizzle::eIdentity);

        create_info.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor); //
        create_info.subresourceRange.setBaseMipLevel(0);                             //
        create_info.subresourceRange.setLevelCount(1);                               //
        create_info.subresourceRange.setBaseArrayLayer(0);                           //
        create_info.subresourceRange.setLayerCount(1);                               //

        auto result = _logical_device.createImageView(&create_info, _vulkan_allocator, &_swapchain_image_views[i]);
        if (result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to create image views.");
    }
}

QueueFamilyIndices VulkanDevice::find_queue_families(const vk::PhysicalDevice& device) {
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

PhysicalDeviceInfo VulkanDevice::rate_device_suitability(const vk::PhysicalDevice& device) {
    vk::PhysicalDeviceProperties device_properties = device.getProperties();
    vk::PhysicalDeviceFeatures device_features = device.getFeatures();
    vk::PhysicalDeviceMemoryProperties device_memory = device.getMemoryProperties();

    auto queue_family_indices = find_queue_families(device);

    // Is device suitable at all
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

    // Fill device info
    PhysicalDeviceInfo device_info{};

    device_info.suitability = suitability_score;
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
    device_info.memory_size_in_gb.resize(device_memory.memoryHeapCount);
    device_info.memory_is_local.resize(device_memory.memoryHeapCount);

    for (uint32 i = 0; i < device_memory.memoryHeapCount; i++) {
        device_info.memory_size_in_gb[i] = 1.0f * device_memory.memoryHeaps[i].size / 1024.f / 1024.f / 1024.f;
        device_info.memory_is_local[i] = (bool) (device_memory.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal);
    }

    return device_info;
}

SwapchainSupportDetails VulkanDevice::query_swapchain_support_details(const vk::PhysicalDevice& device) {
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

// ////////////////////////////// //
// Structure function definitions //
// ////////////////////////////// //

bool QueueFamilyIndices::is_complete() {
    return
        (!VulkanSettings::graphics_family_required || graphics_family.has_value()) &&
        (!VulkanSettings::compute__family_required || compute_family.has_value()) &&
        (!VulkanSettings::transfer_family_required || transfer_family.has_value()) &&
        (!VulkanSettings::present__family_required || present_family.has_value());
}
std::set<uint32> QueueFamilyIndices::get_unique_indices() {
    std::set<uint32> unique_indices = {
        graphics_family.value_or(-1),
        compute_family.value_or(-1),
        transfer_family.value_or(-1),
        present_family.value_or(-1)
    };
    unique_indices.erase(-1);
    return unique_indices;
}

vk::Extent2D SwapchainSupportDetails::get_extent(uint32 width, uint32 height) {
    // Return required width and height if supported
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    return {
        std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}
vk::SurfaceFormatKHR SwapchainSupportDetails::get_surface_format() {
    // Return preferred format if supported, otherwise return first supported format
    for (auto format : formats) {
        if (format.format == VulkanSettings::preferred_swapchain_format &&
            format.colorSpace == VulkanSettings::preferred_swapchain_color_space)
            return format;
    }
    return formats[0];
}
vk::PresentModeKHR SwapchainSupportDetails::get_presentation_mode() {
    // Return preferred presentation mode if supported, otherwise return FIFO
    for (const auto& presentation_mode : presentation_modes) {
        if (presentation_mode == VulkanSettings::preferred_swapchain_presentation_mode)
            return presentation_mode;
    }
    return vk::PresentModeKHR::eFifo;
}

/// TODO: TEMP
// PIPELINE CREATION
#include <fstream>

std::vector<byte> read_file(const std::string& filepath);

void VulkanDevice::create_render_pass() {
    vk::AttachmentDescription color_attachment{};
    color_attachment.setFormat(_swapchain_format);
    color_attachment.setSamples(vk::SampleCountFlagBits::e1);
    color_attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
    color_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    color_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    color_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    color_attachment.setInitialLayout(vk::ImageLayout::eUndefined);
    color_attachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference color_attachment_ref{};
    color_attachment_ref.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    color_attachment_ref.setAttachment(0);

    vk::SubpassDescription subpass{};
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpass.setColorAttachmentCount(1);
    subpass.setPColorAttachments(&color_attachment_ref);

    // Subpass dependencies
    vk::SubpassDependency dependency{};
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
    dependency.setDstSubpass(0);
    dependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    dependency.setSrcAccessMask(vk::AccessFlagBits::eNone);
    dependency.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    dependency.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

    vk::RenderPassCreateInfo create_info{};
    create_info.setAttachmentCount(1);
    create_info.setPAttachments(&color_attachment);
    create_info.setSubpassCount(1);
    create_info.setPSubpasses(&subpass);
    create_info.setDependencyCount(1);
    create_info.setPDependencies(&dependency);

    auto result = _logical_device.createRenderPass(&create_info, _vulkan_allocator, &_render_pass);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create a render pass.");
}

void VulkanDevice::create_pipeline() {
    auto vertex_code = read_file("shaders/simple_vertex_shader.vert.spv");
    auto fragment_code = read_file("shaders/simple_fragment_shader.frag.spv");

    // Vertex and fragment shaders
    auto vertex_shader_module = create_shader_module(vertex_code);
    auto fragment_shader_module = create_shader_module(fragment_code);

    vk::PipelineShaderStageCreateInfo vertex_shader_stage_info;
    vertex_shader_stage_info.setStage(vk::ShaderStageFlagBits::eVertex);
    vertex_shader_stage_info.setModule(vertex_shader_module);
    vertex_shader_stage_info.setPName("main");
    vertex_shader_stage_info.setPSpecializationInfo(nullptr);           // Set initial shader constants

    vk::PipelineShaderStageCreateInfo fragment_shader_stage_info;
    fragment_shader_stage_info.setStage(vk::ShaderStageFlagBits::eFragment);
    fragment_shader_stage_info.setModule(fragment_shader_module);
    fragment_shader_stage_info.setPName("main");
    fragment_shader_stage_info.setPSpecializationInfo(nullptr);


    vk::PipelineShaderStageCreateInfo shader_stages[] = { vertex_shader_stage_info, fragment_shader_stage_info };

    // Vertex input
    auto binding_description = Vertex::get_binding_description();
    auto attribute_description = Vertex::get_attribute_descriptions();

    vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.setVertexBindingDescriptionCount(1);
    vertex_input_info.setPVertexBindingDescriptions(&binding_description);
    vertex_input_info.setVertexAttributeDescriptionCount(static_cast<uint32>(attribute_description.size()));
    vertex_input_info.setVertexAttributeDescriptions(attribute_description);

    // Input assembly
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{};
    input_assembly_info.setTopology(vk::PrimitiveTopology::eTriangleList);
    input_assembly_info.setPrimitiveRestartEnable(false);

    // Viewport and scissors
    vk::PipelineViewportStateCreateInfo viewport_state_info{};
    viewport_state_info.setViewportCount(1);
    viewport_state_info.setScissorCount(1);

    // Rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterization_info{};
    rasterization_info.setDepthClampEnable(false);               // Clamp values beyond far/near planes instead of discarding them (feature required for enabling)
    rasterization_info.setRasterizerDiscardEnable(false);        // Disable output to framebuffer (feature required for enabling)
    rasterization_info.setPolygonMode(vk::PolygonMode::eFill);   // Determines how fragments are generated for geometry (feature required for changing)
    rasterization_info.setLineWidth(1.0f);                       // Line thickness (feature required for values above 1)
    rasterization_info.setCullMode(vk::CullModeFlagBits::eBack); // Triangle face to cull
    rasterization_info.setFrontFace(vk::FrontFace::eClockwise);  // Set vertex order of front-facing triangles
    // Change depth information in some manner
    rasterization_info.setDepthBiasEnable(false);
    rasterization_info.setDepthBiasConstantFactor(0.0f);
    rasterization_info.setDepthBiasClamp(0.0f);
    rasterization_info.setDepthBiasSlopeFactor(0.0f);

    // Multisampling; TODO: Enable
    vk::PipelineMultisampleStateCreateInfo multisampling_info{};
    multisampling_info.setSampleShadingEnable(false);
    multisampling_info.setRasterizationSamples(vk::SampleCountFlagBits::e1);
    multisampling_info.setMinSampleShading(1.0f);
    multisampling_info.setPSampleMask(nullptr);
    multisampling_info.setAlphaToCoverageEnable(false);
    multisampling_info.setAlphaToOneEnable(false);

    // Depth and stencil testing; TODO: implement

    // Color blending
    vk::PipelineColorBlendAttachmentState color_blend_attachment{};
    // Since blend is disabled, no blend will be preformed
    color_blend_attachment.setBlendEnable(false);
    color_blend_attachment.setColorWriteMask(
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA
    );
    // Color blend
    color_blend_attachment.setSrcColorBlendFactor(vk::BlendFactor::eOne);
    color_blend_attachment.setDstColorBlendFactor(vk::BlendFactor::eZero);
    color_blend_attachment.setColorBlendOp(vk::BlendOp::eAdd);
    // Alpha blend
    color_blend_attachment.setSrcAlphaBlendFactor(vk::BlendFactor::eOne);
    color_blend_attachment.setDstAlphaBlendFactor(vk::BlendFactor::eZero);
    color_blend_attachment.setAlphaBlendOp(vk::BlendOp::eAdd);

    vk::PipelineColorBlendStateCreateInfo color_blend_state_info{};
    color_blend_state_info.setLogicOpEnable(false);
    color_blend_state_info.setLogicOp(vk::LogicOp::eOr);
    color_blend_state_info.setAttachmentCount(1);
    color_blend_state_info.setPAttachments(&color_blend_attachment);

    // Pipeline layout fo UNIFORM values
    vk::PipelineLayoutCreateInfo layout_info{};
    layout_info.setSetLayoutCount(0);
    layout_info.setPSetLayouts(nullptr);
    layout_info.setPushConstantRangeCount(0);
    layout_info.setPPushConstantRanges(nullptr);

    _pipeline_layout = _logical_device.createPipelineLayout(layout_info, _vulkan_allocator);

    // Dynamic state
    std::vector<vk::DynamicState> dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state_info{};
    dynamic_state_info.setDynamicStateCount(static_cast<uint32>(dynamic_states.size()));
    dynamic_state_info.setPDynamicStates(dynamic_states.data());

    // Create pipeline object
    vk::GraphicsPipelineCreateInfo create_info{};
    // Programable pipeline stages
    create_info.setStageCount(2);
    create_info.setPStages(shader_stages);
    // Fixed-function stages
    create_info.setPVertexInputState(&vertex_input_info);
    create_info.setPInputAssemblyState(&input_assembly_info);
    create_info.setPViewportState(&viewport_state_info);
    create_info.setPRasterizationState(&rasterization_info);
    create_info.setPMultisampleState(&multisampling_info);
    create_info.setPDepthStencilState(nullptr); // TODO: implement
    create_info.setPColorBlendState(&color_blend_state_info);
    create_info.setPDynamicState(&dynamic_state_info);
    // Pipeline layout handle
    create_info.setLayout(_pipeline_layout);
    // Render passes
    create_info.setRenderPass(_render_pass);
    create_info.setSubpass(0);
    // Other
    create_info.setBasePipelineHandle(VK_NULL_HANDLE);
    create_info.setBasePipelineIndex(-1);

    auto result = _logical_device.createGraphicsPipeline(VK_NULL_HANDLE, create_info, _vulkan_allocator);
    if (result.result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create graphics pipeline.");
    _graphics_pipeline = result.value;

    // Free unused objects
    _logical_device.destroyShaderModule(vertex_shader_module, _vulkan_allocator);
    _logical_device.destroyShaderModule(fragment_shader_module, _vulkan_allocator);
}

std::vector<byte> read_file(const std::string& filepath) {
    std::ifstream file{ filepath, std::ios::ate | std::ios::binary };

    if (!file.is_open()) throw std::runtime_error("Failed to open file: " + filepath);

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<byte> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();

    return buffer;
}

vk::ShaderModule VulkanDevice::create_shader_module(const std::vector<byte>& code) {
    vk::ShaderModuleCreateInfo create_info{};
    create_info.setCodeSize(code.size());
    create_info.setPCode(reinterpret_cast<const uint32*> (code.data()));
    return _logical_device.createShaderModule(create_info, _vulkan_allocator);
}

// FRAMEBUFFER
void VulkanDevice::create_framebuffers() {
    _swapchain_framebuffers.resize(_swapchain_image_views.size());

    for (uint32 i = 0; i < _swapchain_framebuffers.size(); i++) {
        vk::ImageView attachments[] = {
            _swapchain_image_views[i]
        };

        // Create framebuffer
        vk::FramebufferCreateInfo framebuffer_info{};
        framebuffer_info.setRenderPass(_render_pass);
        framebuffer_info.setAttachmentCount(1);
        framebuffer_info.setPAttachments(attachments);
        framebuffer_info.setWidth(_swapchain_extent.width);
        framebuffer_info.setHeight(_swapchain_extent.height);
        framebuffer_info.setLayers(1);

        auto result = _logical_device.createFramebuffer(&framebuffer_info, _vulkan_allocator, &_swapchain_framebuffers[i]);
        if (result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to create framebuffer.");
    }
}

// COMMAND BUFFER CODE
void VulkanDevice::create_command_pool() {
    auto queue_family_indices = find_queue_families(_physical_device);

    vk::CommandPoolCreateInfo command_pool_info{};
    command_pool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    command_pool_info.setQueueFamilyIndex(queue_family_indices.graphics_family.value());

    auto result = _logical_device.createCommandPool(&command_pool_info, _vulkan_allocator, &_command_pool);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create command pool.");
}

void VulkanDevice::create_command_buffers() {
    _command_buffers.resize(VulkanSettings::max_frames_in_flight);

    vk::CommandBufferAllocateInfo alloc_info{};
    alloc_info.setCommandPool(_command_pool);
    alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);
    alloc_info.setCommandBufferCount((uint32) _command_buffers.size());

    auto result = _logical_device.allocateCommandBuffers(&alloc_info, _command_buffers.data());
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to allocate command buffers.");
}


void VulkanDevice::record_command_buffer(vk::CommandBuffer command_buffer, uint32 image_index) {
    // Begin recoding
    vk::CommandBufferBeginInfo begin_info{};
    /* begin_info.setFlags(0); */
    begin_info.setPInheritanceInfo(nullptr);

    command_buffer.begin(begin_info);

    // Begin render pass
    const std::array<float32, 4> clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    vk::ClearValue clear_value = { clear_color };

    vk::RenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.setRenderPass(_render_pass);
    render_pass_begin_info.setFramebuffer(_swapchain_framebuffers[image_index]);
    render_pass_begin_info.renderArea.setOffset({ 0, 0 });
    render_pass_begin_info.renderArea.setExtent(_swapchain_extent);
    render_pass_begin_info.setClearValueCount(1);
    render_pass_begin_info.setPClearValues(&clear_value);

    command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

    // Bind graphics pipeline
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _graphics_pipeline);

    // Bind vertex buffer
    std::vector<vk::Buffer>vertex_buffers = { _vertex_buffer };
    std::vector<vk::DeviceSize> offsets = { 0 };
    command_buffer.bindVertexBuffers(0, vertex_buffers, offsets);

    // Bind index buffer
    command_buffer.bindIndexBuffer(_index_buffer, 0, vk::IndexType::eUint16);

    // Dynamic states
    vk::Viewport viewport{};
    viewport.setX(0.0f);
    viewport.setY(0.0f);
    viewport.setWidth(static_cast<float32>(_swapchain_extent.width));
    viewport.setHeight(static_cast<float32>(_swapchain_extent.height));
    viewport.setMinDepth(0.0f);
    viewport.setMaxDepth(1.0f);

    command_buffer.setViewport(0, 1, &viewport);

    vk::Rect2D scissor{};
    scissor.setOffset({ 0, 0 });
    scissor.setExtent(_swapchain_extent);

    command_buffer.setScissor(0, 1, &scissor);

    // Triangle draw command
    command_buffer.drawIndexed(static_cast<uint32>(indices.size()), 1, 0, 0, 0);

    // End render pass
    command_buffer.endRenderPass();

    // End recording
    command_buffer.end();
}


// SYNC CODE
void VulkanDevice::create_sync_objects() {
    _semaphores_image_available.resize(VulkanSettings::max_frames_in_flight);
    _semaphores_render_finished.resize(VulkanSettings::max_frames_in_flight);
    _fences_in_flight.resize(VulkanSettings::max_frames_in_flight);

    vk::SemaphoreCreateInfo semaphore_info{};
    vk::FenceCreateInfo fence_info{};
    fence_info.setFlags(vk::FenceCreateFlagBits::eSignaled); // Fence becomes signaled on initialization

    vk::Result result;
    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        result = _logical_device.createSemaphore(&semaphore_info, _vulkan_allocator, &_semaphores_image_available[i]);
        if (result != vk::Result::eSuccess) throw std::runtime_error("Failed to create semaphores.");
        result = _logical_device.createSemaphore(&semaphore_info, _vulkan_allocator, &_semaphores_render_finished[i]);
        if (result != vk::Result::eSuccess) throw std::runtime_error("Failed to create semaphores.");
        result = _logical_device.createFence(&fence_info, _vulkan_allocator, &_fences_in_flight[i]);
        if (result != vk::Result::eSuccess) throw std::runtime_error("Failed to create fences.");
    }
}

// DRAW CODE
void VulkanDevice::draw_frame() {
    // Wait for previous frame to finish drawing
    auto result = _logical_device.waitForFences(1, &_fences_in_flight[current_frame], true, UINT64_MAX);
    if (result != vk::Result::eSuccess) throw std::runtime_error("Failed to draw frame.");

    // Obtain a swapchain image
    auto obtained = _logical_device.acquireNextImageKHR(_swapchain, UINT64_MAX, _semaphores_image_available[current_frame]);
    if (obtained.result == vk::Result::eErrorOutOfDateKHR) {
        recreate_swapchain();
        return;
    } else if (obtained.result != vk::Result::eSuccess && obtained.result != vk::Result::eSuboptimalKHR)
        throw std::runtime_error("Failed to obtain a swapchain image.");
    auto image_index = obtained.value;

    // Reset fence
    result = _logical_device.resetFences(1, &_fences_in_flight[current_frame]);
    if (result != vk::Result::eSuccess) throw std::runtime_error("Failed to draw frame.");

    // Record commands
    _command_buffers[current_frame].reset();
    record_command_buffer(_command_buffers[current_frame], image_index);

    // Submit command buffer
    vk::Semaphore wait_semaphores[] = { _semaphores_image_available[current_frame] };
    vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    vk::Semaphore signal_semaphores[] = { _semaphores_render_finished[current_frame] };

    vk::SubmitInfo submit_info{};
    submit_info.setWaitSemaphoreCount(1);
    submit_info.setPWaitSemaphores(wait_semaphores);
    submit_info.setPWaitDstStageMask(wait_stages);
    submit_info.setCommandBufferCount(1);
    submit_info.setPCommandBuffers(&_command_buffers[current_frame]);
    submit_info.setSignalSemaphoreCount(1);
    submit_info.setPSignalSemaphores(signal_semaphores);

    result = _graphics_queue.submit(1, &submit_info, _fences_in_flight[current_frame]);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to submit draw command buffer.");

    // Result presentation
    vk::SwapchainKHR swapchains[] = { _swapchain };

    vk::PresentInfoKHR present_info{};
    present_info.setWaitSemaphoreCount(1);
    present_info.setPWaitSemaphores(signal_semaphores);
    present_info.setSwapchainCount(1);
    present_info.setPSwapchains(swapchains);
    present_info.setPImageIndices(&image_index);

    result = _presentation_queue.presentKHR(present_info);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || _surface->resized) {
        recreate_swapchain();
        _surface->resized = false;
    } else if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to present rendered image.");

    // Advance current frame
    current_frame = (current_frame + 1) % VulkanSettings::max_frames_in_flight;
}


// VERTEX BUFFER
void VulkanDevice::create_buffer(
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    vk::Buffer& buffer,
    vk::DeviceMemory& buffer_memory
) {
    // Create buffer
    vk::BufferCreateInfo buffer_info{};
    buffer_info.setSize(size);
    buffer_info.setUsage(usage);
    buffer_info.setSharingMode(vk::SharingMode::eExclusive);

    auto result = _logical_device.createBuffer(&buffer_info, _vulkan_allocator, &buffer);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create vertex buffer.");

    // Allocate memory to the buffer
    auto memory_requirements = _logical_device.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocation_info{};
    allocation_info.setAllocationSize(memory_requirements.size);
    allocation_info.setMemoryTypeIndex(find_memory_type(memory_requirements.memoryTypeBits, properties));

    result = _logical_device.allocateMemory(&allocation_info, _vulkan_allocator, &buffer_memory);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to allocate vertex buffer memory.");

    // Bind allocated memory to buffer
    _logical_device.bindBufferMemory(buffer, buffer_memory, 0);
}

void VulkanDevice::create_vertex_buffer() {
    vk::DeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

    // Create staging buffer
    vk::Buffer staging_buffer;
    vk::DeviceMemory staging_buffer_memory;

    create_buffer(
        buffer_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_buffer, staging_buffer_memory
    );

    // Fill created memory with data
    auto data = _logical_device.mapMemory(staging_buffer_memory, 0, buffer_size);
    memcpy(data, vertices.data(), (size_t) buffer_size);
    _logical_device.unmapMemory(staging_buffer_memory);

    // Create vertex buffer
    create_buffer(
        buffer_size,
        vk::BufferUsageFlagBits::eTransferDst |
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        _vertex_buffer, _vertex_buffer_memory
    );

    copy_buffer(staging_buffer, _vertex_buffer, buffer_size);

    // Cleanup
    _logical_device.destroyBuffer(staging_buffer, _vulkan_allocator);
    _logical_device.freeMemory(staging_buffer_memory, _vulkan_allocator);
}


uint32 VulkanDevice::find_memory_type(uint32 type_filter, vk::MemoryPropertyFlags properties) {
    auto memory_properties = _physical_device.getMemoryProperties();

    for (uint32 i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) &&
            (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("Failed to find suitable memory type.");
}

void VulkanDevice::copy_buffer(vk::Buffer source_buffer, vk::Buffer destination_buffer, vk::DeviceSize size) {
    vk::CommandBufferAllocateInfo allocation_info{};
    allocation_info.setLevel(vk::CommandBufferLevel::ePrimary);
    allocation_info.setCommandBufferCount(1);
    allocation_info.setCommandPool(_command_pool);

    vk::CommandBuffer command_buffer;
    command_buffer = _logical_device.allocateCommandBuffers(allocation_info)[0];

    // Begin recording commands
    vk::CommandBufferBeginInfo begin_info{};
    begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    command_buffer.begin(begin_info);

    // Issue transfer
    vk::BufferCopy copy_region{};
    copy_region.setSrcOffset(0);
    copy_region.setDstOffset(0);
    copy_region.setSize(size);
    command_buffer.copyBuffer(source_buffer, destination_buffer, 1, &copy_region);

    // Finish recording
    command_buffer.end();

    // Execute command buffer
    vk::SubmitInfo submit_info{};
    submit_info.setCommandBufferCount(1);
    submit_info.setPCommandBuffers(&command_buffer);

    _graphics_queue.submit(submit_info);
    _graphics_queue.waitIdle();

    // Free temp command buffer
    _logical_device.freeCommandBuffers(_command_pool, 1, &command_buffer);
}

// INDEX BUFFER

void VulkanDevice::create_index_buffer() {
    vk::DeviceSize buffer_size = sizeof(indices[0]) * indices.size();

    // Create staging buffer
    vk::Buffer staging_buffer;
    vk::DeviceMemory staging_buffer_memory;

    create_buffer(
        buffer_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_buffer, staging_buffer_memory
    );

    // Fill created memory with data
    auto data = _logical_device.mapMemory(staging_buffer_memory, 0, buffer_size);
    memcpy(data, indices.data(), (size_t) buffer_size);
    _logical_device.unmapMemory(staging_buffer_memory);

    // Create vertex buffer
    create_buffer(
        buffer_size,
        vk::BufferUsageFlagBits::eTransferDst |
        vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        _index_buffer, _index_buffer_memory
    );

    copy_buffer(staging_buffer, _index_buffer, buffer_size);

    // Cleanup
    _logical_device.destroyBuffer(staging_buffer, _vulkan_allocator);
    _logical_device.freeMemory(staging_buffer_memory, _vulkan_allocator);
}