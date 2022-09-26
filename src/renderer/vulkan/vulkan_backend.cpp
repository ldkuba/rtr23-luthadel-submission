#include "renderer/vulkan/vulkan_backend.hpp"

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
    // Create instance
    _vulkan_instance = create_vulkan_instance();

    // Create debug messenger
    _debug_messenger = setup_debug_messenger();

    // Get vulkan surface
    _vulkan_surface = surface->get_vulkan_surface(_vulkan_instance, _allocator);

    // Create device
    _device = new VulkanDevice(
        _vulkan_instance,
        _vulkan_surface,
        _allocator
    );

    // Create swapchain
    _swapchain = new VulkanSwapchain(
        surface->get_width_in_pixels(),
        surface->get_height_in_pixels(),
        _vulkan_surface,
        _device,
        _allocator
    );

    // Create render pass
    _render_pass = new VulkanRenderPass(
        &_device->handle(),
        _allocator,
        _swapchain
    );

    // Create object shader
    _object_shader = new VulkanObjectShader(
        _device,
        _allocator,
        _render_pass->handle,
        _swapchain->msaa_samples
    );

    // Create command pool
    _command_pool = new VulkanCommandPool(
        &_device->handle(),
        _allocator,
        &_device->graphics_queue,
        _device->queue_family_indices.graphics_family.value()
    );

    // Initialize framebuffers
    _swapchain->initialize_framebuffers(&_render_pass->handle());

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
    create_descriptor_sets();

    // TODO: TEMP COMMAND CODE
    _command_buffers = _command_pool->allocate_command_buffers(VulkanSettings::max_frames_in_flight);

    // Synchronization
    create_sync_objects();
}

VulkanBackend::~VulkanBackend() {
    _device->handle().waitIdle();

    // Swapchain
    delete _swapchain;


    // TODO: TEMP IMAGE TEXTURE CODE
    _device->handle().destroySampler(_texture_sampler, _allocator);
    delete _texture_image;


    // TODO: TEMP UNIFORM CODE
    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++)
        delete _uniform_buffers[i];


    // TODO: TEMP INDEX BUFFER CODE
    delete _index_buffer;


    // TODO: TEMP VERTEX BUFFER CODE
    delete _vertex_buffer;


    delete _object_shader;
    delete _render_pass;
    delete _command_pool;

    // Synchronization code
    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        _device->handle().destroySemaphore(_semaphores_image_available[i], _allocator);
        _device->handle().destroySemaphore(_semaphores_render_finished[i], _allocator);
        _device->handle().destroyFence(_fences_in_flight[i], _allocator);
    }

    delete _device;

    if (VulkanSettings::enable_validation_layers)
        _vulkan_instance.destroyDebugUtilsMessengerEXT(_debug_messenger, _allocator,
            vk::DispatchLoaderDynamic{ _vulkan_instance, vkGetInstanceProcAddr });
    _vulkan_instance.destroy(_allocator);
}

// ////////////////////////////// //
// VULKAN RENDERER PUBLIC METHODS //
// ////////////////////////////// //

void VulkanBackend::resized(const uint32 width, const uint32 height) {
    if (_swapchain != nullptr) {
        _swapchain->change_extent(width, height);
    }
}

bool VulkanBackend::begin_frame(const float32 delta_time) {
    return true;
}

bool VulkanBackend::end_frame(const float32 delta_time) {
    // Wait for previous frame to finish drawing
    std::vector<vk::Fence> fences = { _fences_in_flight[_current_frame] };
    try {
        auto result = _device->handle().waitForFences(fences, true, UINT64_MAX);
        if (result != vk::Result::eSuccess) throw std::runtime_error("End of frame fence timed-out.");
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }

    // Acquire next swapchain image index
    auto image_index = _swapchain->acquire_next_image_index(_semaphores_image_available[_current_frame]);

    // Reset fence
    try {
        _device->handle().resetFences(fences);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }

    // Record commands
    _command_buffers[_current_frame].reset();
    record_command_buffer(_command_buffers[_current_frame], image_index);

    // Submit command buffer
    vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    std::vector<vk::Semaphore> wait_semaphores = { _semaphores_image_available[_current_frame] };
    std::vector<vk::Semaphore> signal_semaphores = { _semaphores_render_finished[_current_frame] };

    vk::SubmitInfo submit_info{};
    submit_info.setPWaitDstStageMask(wait_stages);
    submit_info.setWaitSemaphores(wait_semaphores);
    submit_info.setSignalSemaphores(signal_semaphores);
    submit_info.setCommandBufferCount(1);
    submit_info.setPCommandBuffers(&_command_buffers[_current_frame]);
    std::array<vk::SubmitInfo, 1> submits = { submit_info };

    try {
        _device->graphics_queue.submit(submits, fences[0]);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }

    // Present swapchain
    _swapchain->present(image_index, signal_semaphores);

    // Advance current frame
    _current_frame = (_current_frame + 1) % VulkanSettings::max_frames_in_flight;
    return true;
}

void VulkanBackend::update_global_uniform_buffer_state(
    const glm::mat4 projection,
    const glm::mat4 view,
    const glm::vec3 view_position,
    const glm::vec4 ambient_color,
    const int32 mode
) {
    // Define ubo transformations
    static float32 rotation = 0.0f;
    rotation += 0.01f;
    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = view;
    ubo.project = projection;

    // Copy ubo data to the buffer
    _uniform_buffers[_current_frame]->load_data(&ubo, 0, sizeof(ubo));
}

void VulkanBackend::create_texture(Texture* texture) {

}
void VulkanBackend::destroy_texture(Texture* texture) {

}

// /////////////////////////////// //
// VULKAN RENDERER PRIVATE METHODS //
// /////////////////////////////// //

vk::Instance VulkanBackend::create_vulkan_instance() const {
    if (VulkanSettings::enable_validation_layers && !all_validation_layers_are_available())
        throw std::runtime_error("Validation layer was requested, but not available.");

    // Optional application info
    vk::ApplicationInfo app_info{};
    app_info.setPApplicationName(APP_NAME);                   // Application name
    app_info.setApplicationVersion(1);                        // Application version
    app_info.setPEngineName(ENGINE_NAME);                     // Engine name
    app_info.setEngineVersion(1);                             // Engine version
    app_info.setApiVersion(VulkanSettings::vulkan_version);   // Set vulkan version

    // Mandatory vulkan info info
    vk::InstanceCreateInfo create_info{};
    create_info.setPApplicationInfo(&app_info);

    // Specifying required extensions:
    // - Platform extensions
    // - Debug message extension
    auto extensions = Platform::get_required_vulkan_extensions();
    if (VulkanSettings::enable_validation_layers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    create_info.setPEnabledExtensionNames(extensions);

    // Validation layers debugging
    vk::DebugUtilsMessengerCreateInfoEXT debug_create_info;
    if (VulkanSettings::enable_validation_layers) {
        debug_create_info = debug_messenger_create_info();
        create_info.setPNext((vk::DebugUtilsMessengerCreateInfoEXT*) &debug_create_info);
        create_info.setPEnabledLayerNames(VulkanSettings::validation_layers);
    }

    // Create instance
    vk::Instance instance;
    try {
        instance = vk::createInstance(create_info, _allocator);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }

#define PRINT_EXTENSIONS 0
#if PRINT_EXTENSIONS
    // Get extension data
    auto extensions = vk::enumerateInstanceExtensionProperties();
    for (const auto& extension : extensions) std::cout << '\t' << extension.extensionName << '\n';
#endif

    return instance;
}

vk::DebugUtilsMessengerEXT VulkanBackend::setup_debug_messenger() const {
    if (!VulkanSettings::enable_validation_layers) return _debug_messenger;

    auto create_info = debug_messenger_create_info();

    // Dispatcher needed for vulkan extension functions
    auto dispatcher = vk::DispatchLoaderDynamic{ _vulkan_instance, vkGetInstanceProcAddr };

    // Create debugger
    vk::DebugUtilsMessengerEXT messenger;
    try {
        messenger = _vulkan_instance.createDebugUtilsMessengerEXT(create_info, _allocator, dispatcher);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }
    return messenger;
}

// Debug messenger
bool VulkanBackend::all_validation_layers_are_available() const {
    auto available_layers = vk::enumerateInstanceLayerProperties();

    for (auto validation_layer : VulkanSettings::validation_layers) {
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

vk::DebugUtilsMessengerCreateInfoEXT VulkanBackend::debug_messenger_create_info() const {
    vk::DebugUtilsMessengerCreateInfoEXT create_info{};
    create_info.setMessageSeverity(VulkanSettings::enabled_message_security_levels); // Severity levels enabled
    create_info.setMessageType(VulkanSettings::enabled_message_types);               // Message types enabled
    create_info.setPfnUserCallback(debug_callback_function); // User defined debug callback
    create_info.setPUserData(nullptr);                       // (Optional) Additional user data (can be anything)

    return create_info;
}

// /////////////////////////////// //
// VULKAN BACKEND HELPER FUNCTIONS //
// /////////////////////////////// //

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

/// TODO: TEMP CODE BELOW

// COMMAND BUFFER CODE
void VulkanBackend::record_command_buffer(vk::CommandBuffer command_buffer, uint32 image_index) {
    // Begin recoding
    vk::CommandBufferBeginInfo begin_info{};
    begin_info.setPInheritanceInfo(nullptr);

    command_buffer.begin(begin_info);

    // Begin render pass
    std::vector<vk::Framebuffer> framebuffers = _swapchain->framebuffers;
    _render_pass->begin(command_buffer, framebuffers[image_index]);

    // Bind object shader
    _object_shader->use(command_buffer);
    _object_shader->bind_descriptor_set(command_buffer, _current_frame);

    // Bind vertex buffer
    std::vector<vk::Buffer>vertex_buffers = { _vertex_buffer->handle };
    std::vector<vk::DeviceSize> offsets = { 0 };
    command_buffer.bindVertexBuffers(0, vertex_buffers, offsets);

    // Bind index buffer
    command_buffer.bindIndexBuffer(_index_buffer->handle, 0, vk::IndexType::eUint32);

    // Dynamic states
    // Viewport
    vk::Viewport viewport{};
    viewport.setX(0.0f);
    viewport.setY(static_cast<float32>(_swapchain->extent().height));
    viewport.setWidth(static_cast<float32>(_swapchain->extent().width));
    viewport.setHeight(-static_cast<float32>(_swapchain->extent().height));
    viewport.setMinDepth(0.0f);
    viewport.setMaxDepth(1.0f);

    command_buffer.setViewport(0, 1, &viewport);

    // Scissors
    vk::Rect2D scissor{};
    scissor.setOffset({ 0, 0 });
    scissor.setExtent(_swapchain->extent);

    command_buffer.setScissor(0, 1, &scissor);

    // Draw command
    command_buffer.drawIndexed(static_cast<uint32>(indices.size()), 1, 0, 0, 0);

    // End render pass
    _render_pass->end(command_buffer);

    // End recording
    command_buffer.end();
}

// SYNCH

void VulkanBackend::create_sync_objects() {
    _semaphores_image_available.resize(VulkanSettings::max_frames_in_flight);
    _semaphores_render_finished.resize(VulkanSettings::max_frames_in_flight);
    _fences_in_flight.resize(VulkanSettings::max_frames_in_flight);

    // Create synchronization objects
    vk::SemaphoreCreateInfo semaphore_info{};
    vk::FenceCreateInfo fence_info{};
    // Fence becomes signaled on initialization
    fence_info.setFlags(vk::FenceCreateFlagBits::eSignaled);

    try {
        for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
            _semaphores_image_available[i] = _device->handle().createSemaphore(semaphore_info, _allocator);
            _semaphores_render_finished[i] = _device->handle().createSemaphore(semaphore_info, _allocator);
            _fences_in_flight[i] = _device->handle().createFence(fence_info, _allocator);
        }
    } catch (vk::SystemError e) { Logger::fatal(e.what()); }
}

// VERTEX BUFFER
void VulkanBackend::create_vertex_buffer() {
    vk::DeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

    // Create staging buffer
    auto staging_buffer = new VulkanBuffer(_device, _allocator);
    staging_buffer->create(
        buffer_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent
    );

    // Fill created memory with data
    staging_buffer->load_data(vertices.data(), 0, buffer_size);

    // Create vertex buffer
    _vertex_buffer = new VulkanBuffer(_device, _allocator);
    _vertex_buffer->create(
        buffer_size,
        vk::BufferUsageFlagBits::eTransferDst |
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    staging_buffer->copy_data_to_buffer(_command_pool, _vertex_buffer->handle, 0, 0, buffer_size);

    // Cleanup
    delete staging_buffer;
}

// INDEX BUFFER

void VulkanBackend::create_index_buffer() {
    vk::DeviceSize buffer_size = sizeof(indices[0]) * indices.size();

    // Create staging buffer
    auto staging_buffer = new VulkanBuffer(_device, _allocator);
    staging_buffer->create(
        buffer_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent
    );

    // Fill created memory with data
    staging_buffer->load_data(indices.data(), 0, buffer_size);

    // Create index buffer
    _index_buffer = new VulkanBuffer(_device, _allocator);
    _index_buffer->create(
        buffer_size,
        vk::BufferUsageFlagBits::eTransferDst |
        vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    staging_buffer->copy_data_to_buffer(_command_pool, _index_buffer->handle, 0, 0, buffer_size);

    // Cleanup
    delete staging_buffer;
}

// UNIFORM CODE
void VulkanBackend::create_uniform_buffers() {
    vk::DeviceSize buffer_size = sizeof(UniformBufferObject);

    _uniform_buffers.resize(VulkanSettings::max_frames_in_flight);

    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        _uniform_buffers[i] = new VulkanBuffer(_device, _allocator);
        _uniform_buffers[i]->create(
            buffer_size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent
        );
    }
}

void VulkanBackend::create_descriptor_sets() {
    // UBO info
    std::vector<vk::DescriptorBufferInfo> buffer_infos(VulkanSettings::max_frames_in_flight);
    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        buffer_infos[i].setBuffer(_uniform_buffers[i]->handle);
        buffer_infos[i].setOffset(0);
        buffer_infos[i].setRange(sizeof(UniformBufferObject));
    }

    // Texture info
    vk::DescriptorImageInfo image_info{};
    image_info.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    image_info.setImageView(_texture_image->view);
    image_info.setSampler(_texture_sampler);

    _object_shader->create_descriptor_sets(buffer_infos, image_info);
}

// TEXTURE IMAGE
void VulkanBackend::create_texture_image() {
    // Load image
    Texture* texture = new Texture("viking_room", "png");
    vk::DeviceSize texture_size = texture->total_size;

    // Calculate mip levels
    _mip_levels = std::floor(std::log2(std::max(texture->width(), texture->height()))) + 1;

    // Create staging buffer
    auto staging_buffer = new VulkanBuffer(_device, _allocator);
    staging_buffer->create(
        texture_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent
    );

    // Fill created memory with data
    staging_buffer->load_data(texture->pixels, 0, texture_size);

    // Create device side image
    _texture_image = new VulkanImage(_device, _allocator);
    _texture_image->create(
        texture->width, texture->height, _mip_levels,
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
    staging_buffer->copy_data_to_image(_command_pool, _texture_image);

    // Generate mipmaps, this also transitions image to a layout optimal for sampling
    generate_mipmaps(
        _texture_image->handle,
        vk::Format::eR8G8B8A8Srgb,
        texture->width,
        texture->height,
        _mip_levels
    );

    // Cleanup
    delete staging_buffer;
}

void VulkanBackend::create_texture_sampler() {
    vk::SamplerCreateInfo sampler_info{};
    sampler_info.setAddressModeU(vk::SamplerAddressMode::eRepeat);
    sampler_info.setAddressModeV(vk::SamplerAddressMode::eRepeat);
    sampler_info.setAddressModeW(vk::SamplerAddressMode::eRepeat);
    sampler_info.setAnisotropyEnable(true);
    sampler_info.setMaxAnisotropy(_device->info().max_sampler_anisotropy);
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
    sampler_info.setMaxLod(static_cast<float32>(_mip_levels));

    try {
        _texture_sampler = _device->handle().createSampler(sampler_info, _allocator);
    } catch (vk::SystemError e) { Logger::fatal(e.what()); }
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
    auto properties = _device->info().get_format_properties(format);
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
// TODO: TEMP CODE END