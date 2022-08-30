#include "renderer/vulkan/vulkan_backend.hpp"

#include "renderer/vulkan/vulkan_settings.hpp"
#include "logger.hpp"

// TODO: TEMP IMAGE LOADING LIBS
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// TODO: TEMP MODEL LOADING LIBS
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>


VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback_function(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
);


VulkanBackend::VulkanBackend(Platform::Surface* surface) : RendererBackend(surface) {
    create_vulkan_instance();
    setup_debug_messenger();

    // Device code
    _surface = surface;
    _vulkan_surface = surface->get_vulkan_surface(_vulkan_instance, _allocator);

    auto physical_device = pick_physical_device();
    create_logical_device(physical_device);

    create_swapchain();
    create_swapchain_image_views();

    // TODO: TEMP UNIFORM CODE
    create_descriptor_set_layout();

    // TODO: TEMP PIPELINE CODE
    create_render_pass();
    create_pipeline();

    // TODO: TEMP COMMAND CODE
    create_command_pool();

    // TODO: TEMP MSAA CODE
    create_color_resource();

    // TODO: TEMP DEPTH BUFFER CODE
    create_depth_resources();

    // TODO: TEMP FRAMEBUFFER CODE
    create_framebuffers();

    // TODO: TEMP IMAGE TEXTURE CODE
    create_texture_image();
    create_texture_image_view();
    create_texture_sampler();

    // TODO: TEMP MODEL LOADING CODE
    load_model();

    // TODO: TEMP VERTEX BUFFER CODE
    create_vertex_buffer();

    // TODO: TEMP INDEX BUFFER CODE
    create_index_buffer();

    // TODO: TEMP UNIFORM CODE
    create_uniform_buffers();
    create_descriptor_pool();
    create_descriptor_sets();

    // TODO: TEMP COMMAND CODE
    create_command_buffers();

    // TODO: TEMP SYNC CODE
    create_sync_objects();
}

VulkanBackend::~VulkanBackend() {
    cleanup_swapchain();


    // TODO: TEMP IMAGE TEXTURE CODE
    _device.destroySampler(_texture_sampler, _allocator);
    _device.destroyImageView(_texture_image_view, _allocator);
    _device.destroyImage(_texture_image, _allocator);
    _device.freeMemory(_texture_image_memory, _allocator);


    // TODO: TEMP UNIFORM CODE
    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        _device.destroyBuffer(_uniform_buffers[i]);
        _device.freeMemory(_uniform_buffers_memory[i]);
    }
    _device.destroyDescriptorPool(_descriptor_pool, _allocator);
    _device.destroyDescriptorSetLayout(_descriptor_set_layout, _allocator);


    // TODO: TEMP INDEX BUFFER CODE
    _device.destroyBuffer(_index_buffer, _allocator);
    _device.freeMemory(_vertex_buffer_memory, _allocator);


    // TODO: TEMP VERTEX BUFFER CODE
    _device.destroyBuffer(_vertex_buffer, _allocator);
    _device.freeMemory(_vertex_buffer_memory, _allocator);


    // TODO: TEMP PIPELINE CODE
    _device.destroyPipeline(_graphics_pipeline, _allocator);
    _device.destroyPipelineLayout(_pipeline_layout, _allocator);
    _device.destroyRenderPass(_render_pass, _allocator);


    // TODO: TEMP SYNC CODE
    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        _device.destroySemaphore(_semaphores_image_available[i], _allocator);
        _device.destroySemaphore(_semaphores_render_finished[i], _allocator);
        _device.destroyFence(_fences_in_flight[i], _allocator);
    }


    // TODO: TEMP COMMAND CODE
    _device.destroyCommandPool(_command_pool, _allocator);


    _device.destroy(_allocator);
    _vulkan_instance.destroySurfaceKHR(_vulkan_surface, _allocator);


    if (VulkanSettings::enable_validation_layers)
        _vulkan_instance.destroyDebugUtilsMessengerEXT(_debug_messenger, _allocator,
            vk::DispatchLoaderDynamic{ _vulkan_instance, vkGetInstanceProcAddr });
    _vulkan_instance.destroy(_allocator);
}

// ///////////////////////////////// //
// VULKAN RENDERER PRIVATE FUNCTIONS //
// ///////////////////////////////// //

void VulkanBackend::create_vulkan_instance() {
    if (VulkanSettings::enable_validation_layers && !all_validation_layers_are_available())
        throw std::runtime_error("Validation layer was requested, but not available.");

    // Optional application info
    vk::ApplicationInfo app_info{};
    app_info.pApplicationName = APP_NAME;                   // Application name
    app_info.applicationVersion = 1;                        // Application version
    app_info.pEngineName = ENGINE_NAME;                     // Engine name
    app_info.engineVersion = 1;                             // Engine version
    app_info.apiVersion = VulkanSettings::vulkan_version;   // Set vulkan version

    // Mandatory vulkan info info
    vk::InstanceCreateInfo create_info{};
    create_info.pApplicationInfo = &app_info;

    // Specifying required extensions:
    // - Platform extensions
    // - Debug message extension
    auto extensions = Platform::get_required_vulkan_extensions();
    if (VulkanSettings::enable_validation_layers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    create_info.enabledExtensionCount = static_cast<uint32>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    // Validation layers debugging
    vk::DebugUtilsMessengerCreateInfoEXT debug_create_info;
    if (VulkanSettings::enable_validation_layers) {
        debug_create_info = debug_messenger_create_info();
        create_info.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*) &debug_create_info;
        create_info.enabledLayerCount = static_cast<uint32>(_validation_layers.size());
        create_info.ppEnabledLayerNames = _validation_layers.data();
    } else create_info.enabledLayerCount = 0;

    // Create instance
    vk::Result result = vk::createInstance(&create_info, _allocator, &_vulkan_instance);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create VULKAN instance.");

#define PRINT_EXTENSIONS 0
#if PRINT_EXTENSIONS
    // Get extension data
    auto extensions = vk::enumerateInstanceExtensionProperties();
    for (const auto& extension : extensions) std::cout << '\t' << extension.extensionName << '\n';
#endif
}

void VulkanBackend::setup_debug_messenger() {
    if (!VulkanSettings::enable_validation_layers) return;

    auto create_info = debug_messenger_create_info();

    // Dispatcher needed for vulkan extension functions
    auto dispatcher = vk::DispatchLoaderDynamic{ _vulkan_instance, vkGetInstanceProcAddr };

    // Create debugger
    vk::Result result = _vulkan_instance.createDebugUtilsMessengerEXT(&create_info, _allocator, &_debug_messenger, dispatcher);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to set up debug messenger!");
}

// Debug messenger
bool VulkanBackend::all_validation_layers_are_available() {
    auto available_layers = vk::enumerateInstanceLayerProperties();

    for (auto validation_layer : _validation_layers) {
        bool validation_layer_available = false;
        for (auto layer : available_layers) {
            if (std::strcmp(layer.layerName, validation_layer) == 0) {
                validation_layer_available = true;
                break;
            }
        }
        if (validation_layer_available == false)
            return false;
    }

    return true;
}

vk::DebugUtilsMessengerCreateInfoEXT VulkanBackend::debug_messenger_create_info() {
    vk::DebugUtilsMessengerCreateInfoEXT create_info{};
    create_info.setMessageSeverity(VulkanSettings::enabled_message_security_levels); // Severity levels enabled
    create_info.setMessageType(VulkanSettings::enabled_message_types);               // Message types enabled
    create_info.setPfnUserCallback(debug_callback_function); // User defined debug callback
    create_info.setPUserData(nullptr);                       // (Optional) Additional user data (can be anything)

    return create_info;
}

uint32 VulkanBackend::find_memory_type(uint32 type_filter, vk::MemoryPropertyFlags properties) {
    for (uint32 i = 0; i < _physical_device_info.memory_types.size(); i++) {
        if ((type_filter & (1 << i)) &&
            (_physical_device_info.memory_types[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("Failed to find suitable memory type.");
}

// Image functions
void VulkanBackend::create_image(
    uint32 width,
    uint32 height,
    uint32 mip_levels,
    vk::SampleCountFlagBits number_of_samples,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    vk::Image& image, vk::DeviceMemory& image_memory
) {
    // Create device side image
    vk::ImageCreateInfo image_info{};
    image_info.setImageType(vk::ImageType::e2D);
    image_info.extent.setWidth(width);
    image_info.extent.setHeight(height);
    image_info.extent.setDepth(1);
    image_info.setMipLevels(mip_levels);
    image_info.setArrayLayers(1);
    image_info.setFormat(format);
    image_info.setTiling(tiling);
    image_info.setInitialLayout(vk::ImageLayout::eUndefined);
    image_info.setUsage(usage);
    image_info.setSharingMode(vk::SharingMode::eExclusive);
    image_info.setSamples(number_of_samples);

    auto result = _device.createImage(&image_info, _allocator, &image);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create image.");

    // Allocate memory for texture
    auto memory_requirements = _device.getImageMemoryRequirements(image);

    vk::MemoryAllocateInfo allocation_info{};
    allocation_info.setAllocationSize(memory_requirements.size);
    allocation_info.setMemoryTypeIndex(find_memory_type(memory_requirements.memoryTypeBits, properties));

    result = _device.allocateMemory(&allocation_info, _allocator, &image_memory);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to allocate image memory.");

    _device.bindImageMemory(image, image_memory, 0);
}

vk::ImageView VulkanBackend::create_image_view(
    vk::Image image,
    vk::Format format,
    vk::ImageAspectFlags aspect_flags,
    uint32 mip_levels
) {
    vk::ImageViewCreateInfo create_info{};
    create_info.setImage(image);
    create_info.setViewType(vk::ImageViewType::e2D);
    create_info.setFormat(format);
    create_info.subresourceRange.setAspectMask(aspect_flags);
    create_info.subresourceRange.setBaseMipLevel(0);
    create_info.subresourceRange.setLevelCount(mip_levels);
    create_info.subresourceRange.setBaseArrayLayer(0);
    create_info.subresourceRange.setLayerCount(1);

    vk::ImageView image_view;
    auto result = _device.createImageView(&create_info, _allocator, &image_view);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create image view.");

    return image_view;
}

void VulkanBackend::transition_image_layout(
    vk::Image image,
    vk::Format format,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    uint32 mip_levels
) {
    auto command_buffer = begin_single_time_commands();

    // Implement transition barrier
    vk::ImageMemoryBarrier barrier{};
    barrier.setOldLayout(old_layout);
    barrier.setNewLayout(new_layout);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setImage(image);
    barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
    barrier.subresourceRange.setBaseMipLevel(0);
    barrier.subresourceRange.setLevelCount(mip_levels);
    barrier.subresourceRange.setBaseArrayLayer(0);
    barrier.subresourceRange.setLayerCount(1);

    vk::PipelineStageFlags source_stage, destination_stage;

    if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eNone);
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

        source_stage = vk::PipelineStageFlagBits::eTopOfPipe;
        destination_stage = vk::PipelineStageFlagBits::eTransfer;
    } else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        source_stage = vk::PipelineStageFlagBits::eTransfer;
        destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
    } else
        throw std::invalid_argument("Unsupported layout transition.");

    command_buffer.pipelineBarrier(
        source_stage, destination_stage,
        vk::DependencyFlags(),
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    end_single_time_commands(command_buffer);
}

// Buffer functions
void VulkanBackend::create_buffer(
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

    auto result = _device.createBuffer(&buffer_info, _allocator, &buffer);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create vertex buffer.");

    // Allocate memory to the buffer
    auto memory_requirements = _device.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocation_info{};
    allocation_info.setAllocationSize(memory_requirements.size);
    allocation_info.setMemoryTypeIndex(find_memory_type(memory_requirements.memoryTypeBits, properties));

    result = _device.allocateMemory(&allocation_info, _allocator, &buffer_memory);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to allocate vertex buffer memory.");

    // Bind allocated memory to buffer
    _device.bindBufferMemory(buffer, buffer_memory, 0);
}

void VulkanBackend::copy_buffer(vk::Buffer source_buffer, vk::Buffer destination_buffer, vk::DeviceSize size) {
    auto command_buffer = begin_single_time_commands();

    // Issue transfer
    vk::BufferCopy copy_region{};
    copy_region.setSrcOffset(0);
    copy_region.setDstOffset(0);
    copy_region.setSize(size);
    command_buffer.copyBuffer(source_buffer, destination_buffer, 1, &copy_region);

    end_single_time_commands(command_buffer);
}

void VulkanBackend::copy_buffer_to_image(vk::Buffer buffer, vk::Image image, uint32 width, uint32 height) {
    auto command_buffer = begin_single_time_commands();

    // Preform copy ops
    vk::BufferImageCopy region{};
    // Buffer info
    region.setBufferOffset(0);
    region.setBufferRowLength(0);
    region.setBufferImageHeight(0);
    // Image info
    region.imageSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
    region.imageSubresource.setMipLevel(0);
    region.imageSubresource.setBaseArrayLayer(0);
    region.imageSubresource.setLayerCount(1);
    region.setImageOffset({ 0,0,0 });
    region.setImageExtent({ width, height, 1 });

    command_buffer.copyBufferToImage(
        buffer, image,
        vk::ImageLayout::eTransferDstOptimal,
        1, &region
    );

    end_single_time_commands(command_buffer);
}

// //////////////////////////////// //
// VULKAN RENDERER PUBLIC FUNCTIONS //
// //////////////////////////////// //

void VulkanBackend::resized(uint32 width, uint32 height) {}
bool VulkanBackend::begin_frame(float32 delta_time) {
    draw_frame();
    return true;
}
bool VulkanBackend::end_frame(float32 delta_time) {
    return true;
}

// //////////////// //
// Helper functions //
// //////////////// //

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback_function(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
) {
    switch (message_severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        Logger::log("VULKAN :: ", callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        Logger::verbose("VULKAN :: ", callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        Logger::warning("VULKAN :: ", callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        if (message_type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
            Logger::fatal("VULKAN :: ", callback_data->pMessage);
        else
            Logger::error("VULKAN :: ", callback_data->pMessage);
        break;
    default:
        break;
    }

    return VK_FALSE;
}

/// TODO: TEMP


// SYNC CODE
void VulkanBackend::create_sync_objects() {
    _semaphores_image_available.resize(VulkanSettings::max_frames_in_flight);
    _semaphores_render_finished.resize(VulkanSettings::max_frames_in_flight);
    _fences_in_flight.resize(VulkanSettings::max_frames_in_flight);

    vk::SemaphoreCreateInfo semaphore_info{};
    vk::FenceCreateInfo fence_info{};
    fence_info.setFlags(vk::FenceCreateFlagBits::eSignaled); // Fence becomes signaled on initialization

    vk::Result result;
    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        result = _device.createSemaphore(&semaphore_info, _allocator, &_semaphores_image_available[i]);
        if (result != vk::Result::eSuccess) throw std::runtime_error("Failed to create semaphores.");
        result = _device.createSemaphore(&semaphore_info, _allocator, &_semaphores_render_finished[i]);
        if (result != vk::Result::eSuccess) throw std::runtime_error("Failed to create semaphores.");
        result = _device.createFence(&fence_info, _allocator, &_fences_in_flight[i]);
        if (result != vk::Result::eSuccess) throw std::runtime_error("Failed to create fences.");
    }
}

// DRAW CODE
void VulkanBackend::draw_frame() {
    // Wait for previous frame to finish drawing
    auto result = _device.waitForFences(1, &_fences_in_flight[current_frame], true, UINT64_MAX);
    if (result != vk::Result::eSuccess) throw std::runtime_error("Failed to draw frame.");

    // Obtain a swapchain image
    auto obtained = _device.acquireNextImageKHR(_swapchain, UINT64_MAX, _semaphores_image_available[current_frame]);
    if (obtained.result == vk::Result::eErrorOutOfDateKHR) {
        recreate_swapchain();
        return;
    } else if (obtained.result != vk::Result::eSuccess && obtained.result != vk::Result::eSuboptimalKHR)
        throw std::runtime_error("Failed to obtain a swapchain image.");
    auto image_index = obtained.value;

    // Reset fence
    result = _device.resetFences(1, &_fences_in_flight[current_frame]);
    if (result != vk::Result::eSuccess) throw std::runtime_error("Failed to draw frame.");

    // Record commands
    _command_buffers[current_frame].reset();
    record_command_buffer(_command_buffers[current_frame], image_index);

    // Update uniform buffer data
    update_uniform_buffer(current_frame);

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
void VulkanBackend::create_vertex_buffer() {
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
    auto data = _device.mapMemory(staging_buffer_memory, 0, buffer_size);
    memcpy(data, vertices.data(), (size_t) buffer_size);
    _device.unmapMemory(staging_buffer_memory);

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
    _device.destroyBuffer(staging_buffer, _allocator);
    _device.freeMemory(staging_buffer_memory, _allocator);
}

// INDEX BUFFER

void VulkanBackend::create_index_buffer() {
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
    auto data = _device.mapMemory(staging_buffer_memory, 0, buffer_size);
    memcpy(data, indices.data(), (size_t) buffer_size);
    _device.unmapMemory(staging_buffer_memory);

    // Create index buffer
    create_buffer(
        buffer_size,
        vk::BufferUsageFlagBits::eTransferDst |
        vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        _index_buffer, _index_buffer_memory
    );

    copy_buffer(staging_buffer, _index_buffer, buffer_size);

    // Cleanup
    _device.destroyBuffer(staging_buffer, _allocator);
    _device.freeMemory(staging_buffer_memory, _allocator);
}

// UNIFORM CODE
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

void VulkanBackend::create_descriptor_set_layout() {
    vk::DescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.setBinding(0);
    ubo_layout_binding.setDescriptorType(vk::DescriptorType::eUniformBuffer);
    ubo_layout_binding.setDescriptorCount(1);
    ubo_layout_binding.setStageFlags(vk::ShaderStageFlagBits::eVertex);
    ubo_layout_binding.setPImmutableSamplers(nullptr);

    vk::DescriptorSetLayoutBinding sampler_layout_binding{};
    sampler_layout_binding.setBinding(1);
    sampler_layout_binding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
    sampler_layout_binding.setDescriptorCount(1);
    sampler_layout_binding.setStageFlags(vk::ShaderStageFlagBits::eFragment);
    sampler_layout_binding.setPImmutableSamplers(nullptr);

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
        ubo_layout_binding,
        sampler_layout_binding
    };

    vk::DescriptorSetLayoutCreateInfo layout_info{};
    layout_info.setBindings(bindings);

    auto result = _device.createDescriptorSetLayout(&layout_info, _allocator, &_descriptor_set_layout);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create descriptor set layout.");
}

void VulkanBackend::create_uniform_buffers() {
    vk::DeviceSize buffer_size = sizeof(UniformBufferObject);

    _uniform_buffers.resize(VulkanSettings::max_frames_in_flight);
    _uniform_buffers_memory.resize(VulkanSettings::max_frames_in_flight);

    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        create_buffer(
            buffer_size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
            _uniform_buffers[i], _uniform_buffers_memory[i]
        );
    }
}

void VulkanBackend::update_uniform_buffer(uint32 current_image) {
    static auto start_time = std::chrono::high_resolution_clock::now();

    // Calculate delta time
    auto current_time = std::chrono::high_resolution_clock::now();
    float32 delta_time = std::chrono::duration<float32, std::chrono::seconds::period>(current_time - start_time).count();

    // Define ubo transformations
    float32 screen_ratio = _swapchain_extent.width / (float32) _swapchain_extent.height;
    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), delta_time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.project = glm::perspective(glm::radians(45.0f), screen_ratio, 0.1f, 100.0f);
    ubo.project[1][1] *= -1; // Flip Y axis

    // Copy ubo data to the buffer
    auto data = _device.mapMemory(_uniform_buffers_memory[current_image], 0, sizeof(ubo));
    memcpy(data, &ubo, sizeof(ubo));
    _device.unmapMemory(_uniform_buffers_memory[current_image]);
}

void VulkanBackend::create_descriptor_pool() {
    std::array<vk::DescriptorPoolSize, 2> pool_sizes{};
    pool_sizes[0].setType(vk::DescriptorType::eUniformBuffer);
    pool_sizes[0].setDescriptorCount(VulkanSettings::max_frames_in_flight);
    pool_sizes[1].setType(vk::DescriptorType::eCombinedImageSampler);
    pool_sizes[1].setDescriptorCount(VulkanSettings::max_frames_in_flight);

    vk::DescriptorPoolCreateInfo create_info{};
    create_info.setPoolSizes(pool_sizes);
    create_info.setMaxSets(VulkanSettings::max_frames_in_flight);

    auto result = _device.createDescriptorPool(&create_info, _allocator, &_descriptor_pool);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create descriptor pool.");
}

void VulkanBackend::create_descriptor_sets() {
    std::vector<vk::DescriptorSetLayout> layouts(VulkanSettings::max_frames_in_flight, _descriptor_set_layout);
    vk::DescriptorSetAllocateInfo allocation_info{};
    allocation_info.setDescriptorPool(_descriptor_pool);
    allocation_info.setDescriptorSetCount(VulkanSettings::max_frames_in_flight);
    allocation_info.setPSetLayouts(layouts.data());

    _descriptor_sets.resize(VulkanSettings::max_frames_in_flight);

    auto result = _device.allocateDescriptorSets(&allocation_info, _descriptor_sets.data());
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to allocate descriptor set.");

    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        // UBO info
        vk::DescriptorBufferInfo buffer_info{};
        buffer_info.setBuffer(_uniform_buffers[i]);
        buffer_info.setOffset(0);
        buffer_info.setRange(sizeof(UniformBufferObject));

        // Texture info
        vk::DescriptorImageInfo image_info{};
        image_info.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        image_info.setImageView(_texture_image_view);
        image_info.setSampler(_texture_sampler);

        // Combined descriptor
        std::array<vk::WriteDescriptorSet, 2> descriptor_writes{};

        descriptor_writes[0].setDstSet(_descriptor_sets[i]);
        descriptor_writes[0].setDstBinding(0);
        descriptor_writes[0].setDstArrayElement(0);
        descriptor_writes[0].setDescriptorType(vk::DescriptorType::eUniformBuffer);
        descriptor_writes[0].setDescriptorCount(1);
        descriptor_writes[0].setPBufferInfo(&buffer_info);

        descriptor_writes[1].setDstSet(_descriptor_sets[i]);
        descriptor_writes[1].setDstBinding(1);
        descriptor_writes[1].setDstArrayElement(0);
        descriptor_writes[1].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
        descriptor_writes[1].setDescriptorCount(1);
        descriptor_writes[1].setPImageInfo(&image_info);

        // descriptor_writes[1].setPTexelBufferView(nullptr);

        _device.updateDescriptorSets(
            static_cast<uint32>(descriptor_writes.size()), descriptor_writes.data(),
            0, nullptr
        );
    }

}

// TEXTURE IMAGE
void VulkanBackend::create_texture_image() {
    // Load image
    int32 width, height, channels;
    stbi_uc* pixels = stbi_load(texture_path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    vk::DeviceSize image_size = width * height * 4;

    if (!pixels)
        throw std::runtime_error("Failed to load texture image.");

    // Calculate mip levels
    _mip_levels = std::floor(std::log2(std::max(width, height))) + 1;

    // Create staging buffer
    vk::Buffer staging_buffer;
    vk::DeviceMemory staging_buffer_memory;

    create_buffer(
        image_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent,
        staging_buffer, staging_buffer_memory
    );

    // Fill created memory with data
    auto data = _device.mapMemory(staging_buffer_memory, 0, image_size);
    memcpy(data, pixels, (size_t) image_size);
    _device.unmapMemory(staging_buffer_memory);
    stbi_image_free(pixels);

    // Create device side image
    create_image(
        width, height, _mip_levels,
        vk::SampleCountFlagBits::e1,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc |
        vk::ImageUsageFlagBits::eTransferDst |
        vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        _texture_image, _texture_image_memory
    );

    // Transition image to a layout optimal for data transfer
    transition_image_layout(
        _texture_image,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        _mip_levels
    );

    // Copy buffer data to image
    copy_buffer_to_image(staging_buffer, _texture_image, width, height);

    // Generate mipmaps, this also transitions image to a layout optimal for sampling
    generate_mipmaps(_texture_image, vk::Format::eR8G8B8A8Srgb, width, height, _mip_levels);

    // Cleanup
    _device.destroyBuffer(staging_buffer, _allocator);
    _device.freeMemory(staging_buffer_memory, _allocator);
}

void VulkanBackend::create_texture_image_view() {
    _texture_image_view = create_image_view(
        _texture_image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, _mip_levels
    );
}

void VulkanBackend::create_texture_sampler() {
    vk::SamplerCreateInfo sampler_info{};
    sampler_info.setAddressModeU(vk::SamplerAddressMode::eRepeat);
    sampler_info.setAddressModeV(vk::SamplerAddressMode::eRepeat);
    sampler_info.setAddressModeW(vk::SamplerAddressMode::eRepeat);
    sampler_info.setAnisotropyEnable(true);
    sampler_info.setMaxAnisotropy(_physical_device_info.max_sampler_anisotropy);
    sampler_info.setBorderColor(vk::BorderColor::eIntOpaqueBlack);
    sampler_info.setUnnormalizedCoordinates(false);
    sampler_info.setCompareEnable(false);
    sampler_info.setCompareOp(vk::CompareOp::eAlways);
    // Mipmap settings
    sampler_info.setMagFilter(vk::Filter::eLinear);
    sampler_info.setMinFilter(vk::Filter::eLinear);
    sampler_info.setMipmapMode(vk::SamplerMipmapMode::eLinear);
    sampler_info.setMipLodBias(0.0f);
    sampler_info.setMinLod(0.0f);
    sampler_info.setMaxLod(static_cast<float>(_mip_levels));

    auto result = _device.createSampler(&sampler_info, _allocator, &_texture_sampler);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create texture sampler.");
}

// DEPTH BUFFER CODE
void VulkanBackend::create_depth_resources() {
    auto depth_format = find_depth_format();

    create_image(
        _swapchain_extent.width, _swapchain_extent.height, 1,
        _msaa_samples,
        depth_format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        _depth_image, _depth_image_memory
    );
    _depth_image_view = create_image_view(_depth_image, depth_format, vk::ImageAspectFlagBits::eDepth, 1);
}

vk::Format VulkanBackend::find_supported_formats(
    const std::vector<vk::Format>& candidates,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags features
) {
    for (auto format : candidates) {
        auto properties = _physical_device_info.get_format_properties(format);
        vk::FormatFeatureFlags supported_features;
        if (tiling == vk::ImageTiling::eLinear && (features & properties.linearTilingFeatures) == features)
            return format;
        else if (tiling == vk::ImageTiling::eOptimal && (features & properties.optimalTilingFeatures) == features)
            return format;
    }
    throw std::runtime_error("Failed to find supported format.");
}

vk::Format VulkanBackend::find_depth_format() {
    return find_supported_formats(
        { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

// MODEL LOADING
void VulkanBackend::load_model() {
    // Load model
    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(model_path))
        throw std::runtime_error("");

    if (!reader.Warning().empty())
        Logger::warning("TinyObjReader :: ", reader.Warning());

    auto& attributes = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    // Loop over shapes
    std::unordered_map<Vertex, uint32> unique_vertices = {};
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.position = {
                attributes.vertices[3 * index.vertex_index + 0],
                attributes.vertices[3 * index.vertex_index + 1],
                attributes.vertices[3 * index.vertex_index + 2]
            };

            vertex.texture_coord = {
                attributes.texcoords[2 * index.texcoord_index + 0],
                1.0f - attributes.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f,1.0f,1.0f };

            if (unique_vertices.count(vertex) == 0) {
                unique_vertices[vertex] = static_cast   <uint32>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(unique_vertices[vertex]);
        }
    }
}

// MIPMAP CODE

void VulkanBackend::generate_mipmaps(vk::Image image, vk::Format format, uint32 width, uint32 height, uint32 mip_levels) {
    // Check if image format supports linear blitting
    auto properties = _physical_device_info.get_format_properties(format);
    if (!(properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        throw std::runtime_error("Texture image format does not support linear blitting.");

    // Generate mipmaps
    auto command_buffer = begin_single_time_commands();

    vk::ImageMemoryBarrier barrier{};
    barrier.setImage(image);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
    barrier.subresourceRange.setBaseArrayLayer(0);
    barrier.subresourceRange.setLayerCount(1);
    barrier.subresourceRange.setLevelCount(1);

    uint32 mip_width = width;
    uint32 mip_height = height;
    for (uint32 i = 1; i < mip_levels; i++) {
        // Transition bitmap layer i-1 to transfer optimal layout
        barrier.subresourceRange.setBaseMipLevel(i - 1);
        barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
        barrier.setNewLayout(vk::ImageLayout::eTransferSrcOptimal);
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);

        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlags(),
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        // Create bitmap level
        vk::ImageBlit blit{};
        blit.srcOffsets[0] = vk::Offset3D(0, 0, 0);
        blit.srcOffsets[1] = vk::Offset3D(mip_width, mip_height, 1);
        blit.srcSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
        blit.srcSubresource.setMipLevel(i - 1);
        blit.srcSubresource.setBaseArrayLayer(0);
        blit.srcSubresource.setLayerCount(1);
        blit.dstOffsets[0] = vk::Offset3D(0, 0, 0);
        blit.dstOffsets[1] = vk::Offset3D(mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1);
        blit.dstSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
        blit.dstSubresource.setMipLevel(i);
        blit.dstSubresource.setBaseArrayLayer(0);
        blit.dstSubresource.setLayerCount(1);

        command_buffer.blitImage(
            image, vk::ImageLayout::eTransferSrcOptimal,
            image, vk::ImageLayout::eTransferDstOptimal,
            1, &blit,
            vk::Filter::eLinear
        );

        // Transition bitmap layer i-1 to read only optimal layout
        barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal);
        barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::DependencyFlags(),
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        // Half mipmap resolution for next iteration
        if (mip_width > 1) mip_width /= 2;
        if (mip_height > 1) mip_height /= 2;
    }

    // Transition last bitmap layer to read only optimal layout
    barrier.subresourceRange.setBaseMipLevel(mip_levels - 1);
    barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
    barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
    barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::DependencyFlags(),
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    end_single_time_commands(command_buffer);
}

// MSAA CODE
vk::SampleCountFlagBits VulkanBackend::get_maximum_usable_sample_count() {
    auto count = _physical_device_info.framebuffer_color_sample_counts &
        _physical_device_info.framebuffer_depth_sample_counts;

    if (count & vk::SampleCountFlagBits::e64) return vk::SampleCountFlagBits::e64;
    if (count & vk::SampleCountFlagBits::e32) return vk::SampleCountFlagBits::e32;
    if (count & vk::SampleCountFlagBits::e16) return vk::SampleCountFlagBits::e16;
    if (count & vk::SampleCountFlagBits::e8) return vk::SampleCountFlagBits::e8;
    if (count & vk::SampleCountFlagBits::e4) return vk::SampleCountFlagBits::e4;
    if (count & vk::SampleCountFlagBits::e2) return vk::SampleCountFlagBits::e2;

    return vk::SampleCountFlagBits::e1;
}

void VulkanBackend::create_color_resource() {
    vk::Format color_format = _swapchain_format;

    create_image(
        _swapchain_extent.width, _swapchain_extent.height, 1,
        _msaa_samples,
        color_format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransientAttachment |
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        _color_image, _color_image_memory
    );
    _color_image_view = create_image_view(_color_image, color_format, vk::ImageAspectFlagBits::eColor, 1);
}