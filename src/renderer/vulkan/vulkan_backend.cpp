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

    // SURFACE
    _surface = surface;
    _vulkan_surface = surface->get_vulkan_surface(_vulkan_instance, _allocator);

    // DEVICE CODE
    create_device();

    // SWAPCHAIN
    _swapchain = new VulkanSwapchain(_vulkan_surface, _surface, _device, _allocator);

    // TODO: TEMP UNIFORM CODE
    create_descriptor_set_layout();

    // TODO: TEMP PIPELINE CODE
    create_render_pass();
    create_pipeline();

    // TODO: TEMP COMMAND CODE
    _command_pool = new VulkanCommandPool(
        &_device->handle,
        _allocator,
        &_device->graphics_queue,
        _device->queue_family_indices.graphics_family.value()
    );

    // TODO: TEMP FRAMEBUFFER CODE
    _swapchain->initialize_framebuffer(&_render_pass);

    // TODO: TEMP IMAGE TEXTURE CODE
    create_texture_image();
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
    _command_buffers = _command_pool->allocate_command_buffers(VulkanSettings::max_frames_in_flight);
}

VulkanBackend::~VulkanBackend() {
    delete _swapchain;


    // TODO: TEMP IMAGE TEXTURE CODE
    _device->handle.destroySampler(_texture_sampler, _allocator);
    delete _texture_image;


    // TODO: TEMP UNIFORM CODE
    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        _device->handle.destroyBuffer(_uniform_buffers[i]);
        _device->handle.freeMemory(_uniform_buffers_memory[i]);
    }
    _device->handle.destroyDescriptorPool(_descriptor_pool, _allocator);
    _device->handle.destroyDescriptorSetLayout(_descriptor_set_layout, _allocator);


    // TODO: TEMP INDEX BUFFER CODE
    _device->handle.destroyBuffer(_index_buffer, _allocator);
    _device->handle.freeMemory(_vertex_buffer_memory, _allocator);


    // TODO: TEMP VERTEX BUFFER CODE
    _device->handle.destroyBuffer(_vertex_buffer, _allocator);
    _device->handle.freeMemory(_vertex_buffer_memory, _allocator);


    // TODO: TEMP PIPELINE CODE
    _device->handle.destroyPipeline(_graphics_pipeline, _allocator);
    _device->handle.destroyPipelineLayout(_pipeline_layout, _allocator);
    _device->handle.destroyRenderPass(_render_pass, _allocator);


    // TODO: TEMP COMMAND CODE
    delete _command_pool;


    delete _device;
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
    try {
        _vulkan_instance = vk::createInstance(create_info, _allocator);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }

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
    try {
        _debug_messenger = _vulkan_instance.createDebugUtilsMessengerEXT(create_info, _allocator, dispatcher);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }
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

    auto result = _device->handle.createBuffer(&buffer_info, _allocator, &buffer);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create vertex buffer.");

    // Allocate memory to the buffer
    auto memory_requirements = _device->handle.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocation_info{};
    allocation_info.setAllocationSize(memory_requirements.size);
    allocation_info.setMemoryTypeIndex(_device->find_memory_type(memory_requirements.memoryTypeBits, properties));

    result = _device->handle.allocateMemory(&allocation_info, _allocator, &buffer_memory);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to allocate vertex buffer memory.");

    // Bind allocated memory to buffer
    _device->handle.bindBufferMemory(buffer, buffer_memory, 0);
}

void VulkanBackend::copy_buffer(vk::Buffer source_buffer, vk::Buffer destination_buffer, vk::DeviceSize size) {
    auto command_buffer = _command_pool->begin_single_time_commands();

    // Issue transfer
    vk::BufferCopy copy_region{};
    copy_region.setSrcOffset(0);
    copy_region.setDstOffset(0);
    copy_region.setSize(size);
    command_buffer.copyBuffer(source_buffer, destination_buffer, 1, &copy_region);

    _command_pool->end_single_time_commands(command_buffer);
}

void VulkanBackend::copy_buffer_to_image(vk::Buffer buffer, vk::Image image, uint32 width, uint32 height) {
    auto command_buffer = _command_pool->begin_single_time_commands();

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

    _command_pool->end_single_time_commands(command_buffer);
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
// DEVICE
void VulkanBackend::create_device() {
    // Create device
    _device = new VulkanDevice(_vulkan_instance, _vulkan_surface, _allocator);

    // Create vulkan images
    _texture_image = new VulkanImage(_device, _allocator);
}

// COMMAND BUFFER CODE
void VulkanBackend::record_command_buffer(vk::CommandBuffer command_buffer, uint32 image_index) {
    // Begin recoding
    vk::CommandBufferBeginInfo begin_info{};
    /* begin_info.setFlags(0); */
    begin_info.setPInheritanceInfo(nullptr);

    command_buffer.begin(begin_info);

    // Begin render pass
    std::array<float, 4> clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    std::array<vk::ClearValue, 2>clear_values{};
    clear_values[0].setColor({ clear_color });
    clear_values[1].setDepthStencil({ 1.0f, 0 });


    vk::RenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.setRenderPass(_render_pass);
    render_pass_begin_info.setFramebuffer(_swapchain->framebuffers[image_index]);
    render_pass_begin_info.renderArea.setOffset({ 0, 0 });
    render_pass_begin_info.renderArea.setExtent(_swapchain->extent);
    render_pass_begin_info.setClearValues(clear_values);

    command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

    // Bind graphics pipeline
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _graphics_pipeline);

    // Bind vertex buffer
    std::vector<vk::Buffer>vertex_buffers = { _vertex_buffer };
    std::vector<vk::DeviceSize> offsets = { 0 };
    command_buffer.bindVertexBuffers(0, vertex_buffers, offsets);

    // Bind index buffer
    command_buffer.bindIndexBuffer(_index_buffer, 0, vk::IndexType::eUint32);

    // Dynamic states
    vk::Viewport viewport{};
    viewport.setX(0.0f);
    viewport.setY(0.0f);
    viewport.setWidth(static_cast<float32>(_swapchain->extent.width));
    viewport.setHeight(static_cast<float32>(_swapchain->extent.height));
    viewport.setMinDepth(0.0f);
    viewport.setMaxDepth(1.0f);

    command_buffer.setViewport(0, 1, &viewport);

    vk::Rect2D scissor{};
    scissor.setOffset({ 0, 0 });
    scissor.setExtent(_swapchain->extent);

    command_buffer.setScissor(0, 1, &scissor);

    // Bind description sets
    command_buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        _pipeline_layout, 0,
        1, &_descriptor_sets[current_frame],
        0, nullptr
    );

    // Draw command
    command_buffer.drawIndexed(static_cast<uint32>(indices.size()), 1, 0, 0, 0);

    // End render pass
    command_buffer.endRenderPass();

    // End recording
    command_buffer.end();
}

// DRAW CODE
void VulkanBackend::draw_frame() {
    // Acquire next image index
    auto image_index = _swapchain->acquire_next_image_index(current_frame);
    if (image_index == -1) return;

    // Record commands
    _command_buffers[current_frame].reset();
    record_command_buffer(_command_buffers[current_frame], image_index);

    // Update uniform buffer data
    update_uniform_buffer(current_frame);

    // Present swapchain
    std::vector<vk::CommandBuffer> command_buffers = { _command_buffers[current_frame] };
    _swapchain->present(current_frame, image_index, command_buffers);

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
    auto data = _device->handle.mapMemory(staging_buffer_memory, 0, buffer_size);
    memcpy(data, vertices.data(), (size_t) buffer_size);
    _device->handle.unmapMemory(staging_buffer_memory);

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
    _device->handle.destroyBuffer(staging_buffer, _allocator);
    _device->handle.freeMemory(staging_buffer_memory, _allocator);
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
    auto data = _device->handle.mapMemory(staging_buffer_memory, 0, buffer_size);
    memcpy(data, indices.data(), (size_t) buffer_size);
    _device->handle.unmapMemory(staging_buffer_memory);

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
    _device->handle.destroyBuffer(staging_buffer, _allocator);
    _device->handle.freeMemory(staging_buffer_memory, _allocator);
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

    auto result = _device->handle.createDescriptorSetLayout(&layout_info, _allocator, &_descriptor_set_layout);
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
    float32 screen_ratio = _swapchain->extent.width / (float32) _swapchain->extent.height;
    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), delta_time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.project = glm::perspective(glm::radians(45.0f), screen_ratio, 0.1f, 100.0f);
    ubo.project[1][1] *= -1; // Flip Y axis

    // Copy ubo data to the buffer
    auto data = _device->handle.mapMemory(_uniform_buffers_memory[current_image], 0, sizeof(ubo));
    memcpy(data, &ubo, sizeof(ubo));
    _device->handle.unmapMemory(_uniform_buffers_memory[current_image]);
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

    auto result = _device->handle.createDescriptorPool(&create_info, _allocator, &_descriptor_pool);
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

    auto result = _device->handle.allocateDescriptorSets(&allocation_info, _descriptor_sets.data());
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
        image_info.setImageView(_texture_image->view);
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

        _device->handle.updateDescriptorSets(
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
    auto data = _device->handle.mapMemory(staging_buffer_memory, 0, image_size);
    memcpy(data, pixels, (size_t) image_size);
    _device->handle.unmapMemory(staging_buffer_memory);
    stbi_image_free(pixels);

    // Create device side image
    _texture_image->create(
        width, height, _mip_levels,
        vk::SampleCountFlagBits::e1,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc |
        vk::ImageUsageFlagBits::eTransferDst |
        vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eColor
    );

    // Transition image to a layout optimal for data transfer
    _texture_image->transition_image_layout(
        _command_pool,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        _mip_levels
    );

    // Copy buffer data to image
    copy_buffer_to_image(staging_buffer, _texture_image->handle, width, height);

    // Generate mipmaps, this also transitions image to a layout optimal for sampling
    generate_mipmaps(_texture_image->handle, vk::Format::eR8G8B8A8Srgb, width, height, _mip_levels);

    // Cleanup
    _device->handle.destroyBuffer(staging_buffer, _allocator);
    _device->handle.freeMemory(staging_buffer_memory, _allocator);
}

void VulkanBackend::create_texture_sampler() {
    vk::SamplerCreateInfo sampler_info{};
    sampler_info.setAddressModeU(vk::SamplerAddressMode::eRepeat);
    sampler_info.setAddressModeV(vk::SamplerAddressMode::eRepeat);
    sampler_info.setAddressModeW(vk::SamplerAddressMode::eRepeat);
    sampler_info.setAnisotropyEnable(true);
    sampler_info.setMaxAnisotropy(_device->info.max_sampler_anisotropy);
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

    auto result = _device->handle.createSampler(&sampler_info, _allocator, &_texture_sampler);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create texture sampler.");
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
    auto properties = _device->info.get_format_properties(format);
    if (!(properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        throw std::runtime_error("Texture image format does not support linear blitting.");

    // Generate mipmaps
    auto command_buffer = _command_pool->begin_single_time_commands();

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

    _command_pool->end_single_time_commands(command_buffer);
}