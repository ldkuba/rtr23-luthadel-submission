#include "renderer/vulkan/vulkan_backend.hpp"

#include "renderer/vulkan/vulkan_framebuffer.hpp"
#include "resources/geometry.hpp"

namespace ENGINE_NAMESPACE {

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback_function(
    VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
    VkDebugUtilsMessageTypeFlagsEXT             message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void*                                       user_data
);

// Constructor
VulkanBackend::VulkanBackend(Platform::Surface* const surface)
    : RendererBackend(surface) {
    // Create instance
    _vulkan_instance = create_vulkan_instance();

    // Create debug messenger
    _debug_messenger = setup_debug_messenger();

    // Get vulkan surface
    auto result = surface->get_vulkan_surface(_vulkan_instance, _allocator);
    if (result.has_error())
        Logger::fatal(RENDERER_VULKAN_LOG, result.error().what());
    _vulkan_surface = result.value();

    // Create device
    _device = new (MemoryTag::Renderer)
        VulkanDevice(_vulkan_instance, _vulkan_surface, _allocator);

    // Create swapchain
    _swapchain = new (MemoryTag::Renderer) VulkanSwapchain(
        _device,
        _allocator,
        surface->get_width_in_pixels(),
        surface->get_height_in_pixels(),
        _vulkan_surface
    );

    // Create render pass
    _main_render_pass = new (MemoryTag::Renderer) VulkanRenderPass(
        &_device->handle(),
        _allocator,
        _swapchain,
        { 0.0f, 0.0f, 0.0f, 1.0f }, // Clear color
        RenderPassPosition::Beginning,
        RenderPassClearFlags::Color | RenderPassClearFlags::Depth |
            RenderPassClearFlags::Stencil,
        true, // Multisampling
        true  // Depth testing
    );
    _ui_render_pass = new (MemoryTag::Renderer) VulkanRenderPass(
        &_device->handle(),
        _allocator,
        _swapchain,
        { 0.0f, 0.0f, 0.0f, 0.0f }, // Clear color
        RenderPassPosition::End,
        RenderPassClearFlags::None,
        false, // Multisampling
        false  // Depth testing
    );

    // Create command pool
    _command_pool = new (MemoryTag::Renderer) VulkanCommandPool(
        &_device->handle(),
        _allocator,
        &_device->graphics_queue,
        _device->queue_family_indices.graphics_family.value()
    );

    // TODO: TEMP VERTEX & INDEX BUFFER CODE
    create_buffers();

    // TODO: TEMP COMMAND CODE
    // TODO: One per swapchain image, instead of one per frame; maybe
    _command_buffer = _command_pool->allocate_managed_command_buffer();

    // Synchronization
    create_sync_objects();
}
// Destructor
VulkanBackend::~VulkanBackend() {
    _device->handle().waitIdle();

    // Swapchain
    del(_swapchain);

    // TODO: TEMP VERTEX & INDEX BUFFER CODE
    del(_index_buffer);
    del(_vertex_buffer);

    // Render pass
    del(_ui_render_pass);
    del(_main_render_pass);

    // Command pool
    del(_command_pool);

    // Synchronization code
    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        _device->handle().destroySemaphore(
            _semaphores_image_available[i], _allocator
        );
        _device->handle().destroySemaphore(
            _semaphores_render_finished[i], _allocator
        );
        _device->handle().destroyFence(_fences_in_flight[i], _allocator);
    }
    Logger::trace(RENDERER_VULKAN_LOG, "Synchronization objects destroyed.");

    // Device
    del(_device);
    // Surface
    _vulkan_instance.destroySurfaceKHR(_vulkan_surface);
    // Validation layer
    if (VulkanSettings::enable_validation_layers) {
        _vulkan_instance.destroyDebugUtilsMessengerEXT(
            _debug_messenger,
            _allocator,
            vk::DispatchLoaderDynamic { _vulkan_instance,
                                        vkGetInstanceProcAddr }
        );
        Logger::trace(RENDERER_VULKAN_LOG, "Debug messenger destroyed.");
    }
    // instance
    _vulkan_instance.destroy(_allocator);

    Logger::trace(RENDERER_VULKAN_LOG, "Vulkan instance destroyed.");
}

// ////////////////////////////// //
// VULKAN RENDERER PUBLIC METHODS //
// ////////////////////////////// //

void VulkanBackend::resized(const uint32 width, const uint32 height) {
    if (_swapchain != nullptr) _swapchain->change_extent(width, height);
}

// Frame
Result<void, RuntimeError> VulkanBackend::begin_frame(const float32 delta_time
) {
    // Wait for previous frame to finish drawing
    std::array<vk::Fence, 1> fences { _fences_in_flight[_current_frame] };
    try {
        auto result = _device->handle().waitForFences(fences, true, uint64_max);
        if (result != vk::Result::eSuccess) {
            Logger::error(RENDERER_VULKAN_LOG, "End of frame fence timed-out.");
            return Failure("Failed initialization failed.");
        }
    } catch (const vk::SystemError& e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    // Compute next swapchain image index
    _swapchain->compute_next_image_index(
        _semaphores_image_available[_current_frame]
    );

    // Reset fence
    try {
        _device->handle().resetFences(fences);
    } catch (const vk::SystemError& e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    // Begin recording commands
    _command_buffer->reset(_current_frame);
    auto command_buffer = _command_buffer->handle;

    // Begin recoding
    vk::CommandBufferBeginInfo begin_info {};
    begin_info.setPInheritanceInfo(nullptr);

    command_buffer->begin(begin_info);

    // Set dynamic states
    // Viewport
    vk::Viewport viewport {};
    viewport.setX(0.0f);
    viewport.setY(static_cast<float32>(_swapchain->extent().height));
    viewport.setWidth(static_cast<float32>(_swapchain->extent().width));
    viewport.setHeight(-static_cast<float32>(_swapchain->extent().height));
    viewport.setMinDepth(0.0f);
    viewport.setMaxDepth(1.0f);

    command_buffer->setViewport(0, 1, &viewport);

    // Scissors
    vk::Rect2D scissor {};
    scissor.setOffset({ 0, 0 });
    scissor.setExtent(_swapchain->extent);

    command_buffer->setScissor(0, 1, &scissor);

    return {};
}

Result<void, RuntimeError> VulkanBackend::end_frame(const float32 delta_time) {
    // End recording
    auto command_buffer = _command_buffer->handle;
    command_buffer->end();

    // Submit command buffer
    vk::PipelineStageFlags wait_stages[] {
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    std::array<vk::Semaphore, 1> wait_semaphores {
        _semaphores_image_available[_current_frame]
    };
    std::array<vk::Semaphore, 1> signal_semaphores {
        _semaphores_render_finished[_current_frame]
    };

    vk::SubmitInfo submit_info {};
    submit_info.setPWaitDstStageMask(wait_stages);
    submit_info.setWaitSemaphores(wait_semaphores);
    submit_info.setSignalSemaphores(signal_semaphores);
    submit_info.setCommandBufferCount(1);
    submit_info.setPCommandBuffers(command_buffer);
    std::array<vk::SubmitInfo, 1> submits { submit_info };

    try {
        _device->graphics_queue.submit(
            submits, _fences_in_flight[_current_frame]
        );
    } catch (const vk::SystemError& e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    // Present swapchain
    _swapchain->present(signal_semaphores);

    // Advance current frame
    _current_frame =
        (_current_frame + 1) % VulkanSettings::max_frames_in_flight;
    return {};
}

// Render pass
void VulkanBackend::begin_render_pass(uint8 render_pass_id) {
    // Begin render pass
    switch (render_pass_id) {
    case BuiltinRenderPass::World:
        _main_render_pass->begin(*_command_buffer->handle);
        break;
    case BuiltinRenderPass::UI:
        _ui_render_pass->begin(*_command_buffer->handle);
        break;
    default:
        Logger::error(
            RENDERER_VULKAN_LOG,
            "Method begin_render_pass called on unrecognized render pass id: ",
            render_pass_id,
            "."
        );
        return;
    }
}
void VulkanBackend::end_render_pass(uint8 render_pass_id) {
    // End render pass
    switch (render_pass_id) {
    case BuiltinRenderPass::World:
        _main_render_pass->end(*_command_buffer->handle);
        break;
    case BuiltinRenderPass::UI:
        _ui_render_pass->end(*_command_buffer->handle);
        break;
    default:
        Logger::error(
            RENDERER_VULKAN_LOG,
            "Method end_render_pass called on unrecognized render pass id: ",
            render_pass_id,
            "."
        );
        return;
    }
}

void VulkanBackend::draw_geometry(Geometry* const geometry) {
    // Check if geometry data is valid
    if (!geometry || !geometry->internal_id.has_value()) return;

    auto buffer_data    = _geometries[geometry->internal_id.value()];
    auto command_buffer = _command_buffer->handle;

    // Bind vertex buffer
    std::array<vk::Buffer, 1>     vertex_buffers { _vertex_buffer->handle };
    std::array<vk::DeviceSize, 1> offsets { buffer_data.vertex_offset };
    command_buffer->bindVertexBuffers(0, vertex_buffers, offsets);

    // Issue draw command
    if (buffer_data.index_count > 0) {
        // Bind index buffer
        command_buffer->bindIndexBuffer(
            _index_buffer->handle,
            buffer_data.index_offset,
            vk::IndexType::eUint32 // TODO: Might need to be configurable
        );
        // Draw command indexed
        command_buffer->drawIndexed(buffer_data.index_count, 1, 0, 0, 0);
    } else {
        // Draw command non-indexed
        command_buffer->draw(buffer_data.vertex_count, 1, 0, 0);
    }
}

// Textures
void VulkanBackend::create_texture(Texture* texture, const byte* const data) {
    Logger::trace(RENDERER_VULKAN_LOG, "Creating texture.");

    // Image format
    // NOTE: assumes 8 bits per channel
    auto texture_format = vk::Format::eR8G8B8A8Srgb;

    // Create staging buffer
    auto staging_buffer =
        new (MemoryTag::Temp) VulkanBuffer(_device, _allocator);
    staging_buffer->create(
        texture->total_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent
    );

    // Fill created memory with data
    staging_buffer->load_data(data, 0, texture->total_size);

    // Create device side image
    // NOTE: Lots of assumptions here
    auto texture_image =
        new (MemoryTag::GPUTexture) VulkanImage(_device, _allocator);
    texture_image->create(
        texture->width,
        texture->height,
        texture->mip_level_count,
        vk::SampleCountFlagBits::e1,
        texture_format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eColor
    );

    auto command_buffer = _command_pool->begin_single_time_commands();

    // Transition image to a layout optimal for data transfer
    auto result = texture_image->transition_image_layout(
        command_buffer,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal
    );
    if (result.has_error())
        Logger::fatal(RENDERER_VULKAN_LOG, result.error().what());

    // Copy buffer data to image
    staging_buffer->copy_data_to_image(command_buffer, texture_image);

    // Generate mipmaps, this also transitions image to a layout optimal for
    // sampling
    texture_image->generate_mipmaps(command_buffer);
    _command_pool->end_single_time_commands(command_buffer);

    // Cleanup
    del(staging_buffer);

    // Save internal data
    VulkanTextureData* vulkan_texture_data =
        new (MemoryTag::GPUTexture) VulkanTextureData();
    vulkan_texture_data->image = texture_image;
    texture->internal_data     = vulkan_texture_data;

    Logger::trace(RENDERER_VULKAN_LOG, "Texture created.");
}
void VulkanBackend::destroy_texture(Texture* texture) {
    if (texture == nullptr) return;
    if (texture->internal_data == nullptr) return;
    auto data = reinterpret_cast<VulkanTextureData*>(texture->internal_data());

    _device->handle().waitIdle();
    if (data->image) del(data->image);

    Logger::trace(RENDERER_VULKAN_LOG, "Texture destroyed.");
}

// Geometry
void VulkanBackend::create_geometry(
    Geometry*             geometry,
    const Vector<Vertex>& vertices,
    const Vector<uint32>& indices
) {
    create_geometry_internal(
        geometry,
        sizeof(Vertex),
        vertices.size(),
        vertices.data(),
        sizeof(uint32),
        indices.size(),
        indices.data()
    );
}
void VulkanBackend::create_geometry(
    Geometry*               geometry,
    const Vector<Vertex2D>& vertices,
    const Vector<uint32>&   indices
) {
    create_geometry_internal(
        geometry,
        sizeof(Vertex2D),
        vertices.size(),
        vertices.data(),
        sizeof(uint32),
        indices.size(),
        indices.data()
    );
}
void VulkanBackend::destroy_geometry(Geometry* geometry) {
    if (!geometry) {
        Logger::warning(
            RENDERER_VULKAN_LOG,
            "Method destroy_geometry called with nullptr. Nothing was done."
        );
        return;
    }
    if (!geometry->internal_id.has_value()) {
        Logger::warning(
            RENDERER_VULKAN_LOG,
            "Method destroy_geometry called for a geometry which was already ",
            "destroyed. Nothing was done."
        );
        return;
    }

    auto& internal_data = _geometries[geometry->internal_id.value()];

    // TODO: FREE VERTEX & INDEX DATA

    _geometries.erase(geometry->internal_id.value());
}

// Shader
Shader* VulkanBackend::create_shader(const ShaderConfig config) {
    Logger::trace(RENDERER_VULKAN_LOG, "Creating shader.");

    // TODO: Render pass switch
    auto render_pass = _main_render_pass;
    if (config.render_pass_name.compare_ci("Renderpass.Builtin.UI") == 0)
        render_pass = _ui_render_pass;

    // Create shader
    auto shader = new (MemoryTag::Shader) /**/ VulkanShader(
        config, _device, _allocator, render_pass, _command_buffer
    );

    Logger::trace(RENDERER_VULKAN_LOG, "Shader created.");
    return shader;
}
void VulkanBackend::destroy_shader(Shader* shader) {
    del(shader);
    Logger::trace(RENDERER_VULKAN_LOG, "Shader destroyed.");
}

// /////////////////////////////// //
// VULKAN RENDERER PRIVATE METHODS //
// /////////////////////////////// //

vk::Instance VulkanBackend::create_vulkan_instance() const {
    Logger::trace(RENDERER_VULKAN_LOG, "Creating a vulkan instance.");

    if (VulkanSettings::enable_validation_layers &&
        !all_validation_layers_are_available())
        Logger::fatal(
            RENDERER_VULKAN_LOG,
            "Validation layer was requested, but not available."
        );

    // Optional application info
    vk::ApplicationInfo app_info {};
    app_info.setPApplicationName(APP_NAME); // Application name
    app_info.setApplicationVersion(1);      // Application version
    app_info.setPEngineName(ENGINE_NAME);   // Engine name
    app_info.setEngineVersion(1);           // Engine version
    app_info.setApiVersion(VulkanSettings::vulkan_version
    ); // Set vulkan version

    // Mandatory vulkan info info
    vk::InstanceCreateInfo create_info {};
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
        create_info.setPNext(
            (vk::DebugUtilsMessengerCreateInfoEXT*) &debug_create_info
        );
        create_info.setPEnabledLayerNames(VulkanSettings::validation_layers);
    }

    // Create instance
    vk::Instance instance;
    try {
        instance = vk::createInstance(create_info, _allocator);
    } catch (const vk::SystemError& e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

#define PRINT_EXTENSIONS 0
#if PRINT_EXTENSIONS
    // Get extension data
    auto extensions = vk::enumerateInstanceExtensionProperties();
    for (const auto& extension : extensions)
        std::cout << '\t' << extension.extensionName << '\n';
#endif

    Logger::trace(RENDERER_VULKAN_LOG, "Vulkan instance created.");
    return instance;
}

vk::DebugUtilsMessengerEXT VulkanBackend::setup_debug_messenger() const {
    if (!VulkanSettings::enable_validation_layers) return _debug_messenger;

    Logger::trace(RENDERER_VULKAN_LOG, "Creating debug messenger.");

    auto create_info = debug_messenger_create_info();

    // Dispatcher needed for vulkan extension functions
    auto dispatcher =
        vk::DispatchLoaderDynamic { _vulkan_instance, vkGetInstanceProcAddr };

    // Create debugger
    vk::DebugUtilsMessengerEXT messenger;
    try {
        messenger = _vulkan_instance.createDebugUtilsMessengerEXT(
            create_info, _allocator, dispatcher
        );
    } catch (const vk::SystemError& e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    Logger::trace(RENDERER_VULKAN_LOG, "Debug messenger created.");
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
        if (validation_layer_available == false) return false;
    }

    return true;
}

vk::DebugUtilsMessengerCreateInfoEXT VulkanBackend::debug_messenger_create_info(
) const {
    vk::DebugUtilsMessengerCreateInfoEXT create_info {};
    create_info.setMessageSeverity(
        VulkanSettings::enabled_message_security_levels
    ); // Severity levels enabled
    create_info.setMessageType(VulkanSettings::enabled_message_types
    ); // Message types enabled
    create_info.setPfnUserCallback(debug_callback_function
    ); // User defined debug callback
    create_info.setPUserData(nullptr
    ); // (Optional) Additional user data (can be anything)

    return create_info;
}

// SYNCH
void VulkanBackend::create_sync_objects() {
    Logger::trace(RENDERER_VULKAN_LOG, "Creating synchronization objects.");

    // Create synchronization objects
    vk::SemaphoreCreateInfo semaphore_info {};
    vk::FenceCreateInfo     fence_info {};
    // Fence becomes signaled on initialization
    fence_info.setFlags(vk::FenceCreateFlagBits::eSignaled);

    try {
        for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
            _semaphores_image_available[i] =
                _device->handle().createSemaphore(semaphore_info, _allocator);
            _semaphores_render_finished[i] =
                _device->handle().createSemaphore(semaphore_info, _allocator);
            _fences_in_flight[i] =
                _device->handle().createFence(fence_info, _allocator);
        }
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    Logger::trace(RENDERER_VULKAN_LOG, "All synchronization objects created.");
}

uint32 VulkanBackend::generate_geometry_id() {
    static uint32 id = 0;
    return id++;
}

void VulkanBackend::create_geometry_internal(
    Geometry*         geometry,
    const uint32      vertex_size,
    const uint32      vertex_count,
    const void* const vertex_data,
    const uint32      index_size,
    const uint32      index_count,
    const void* const index_data
) {
    if (!geometry) {
        Logger::error(
            RENDERER_VULKAN_LOG,
            "Method create_geometry called with nullptr. Creation failed."
        );
        return;
    }

    if (vertex_count < 1) {
        Logger::error(RENDERER_VULKAN_LOG, "No vertex data passed.");
        return;
    }

    auto               is_reupload = geometry->internal_id.has_value();
    VulkanGeometryData old_data;

    VulkanGeometryData* internal_data = nullptr;
    if (is_reupload) {
        internal_data          = &_geometries[geometry->internal_id.value()];
        old_data.vertex_count  = internal_data->vertex_count;
        old_data.vertex_size   = internal_data->vertex_size;
        old_data.vertex_offset = internal_data->vertex_offset;
        old_data.index_count   = internal_data->index_count;
        old_data.index_size    = internal_data->index_size;
        old_data.index_offset  = internal_data->index_offset;
    } else {
        uint32 id             = generate_geometry_id();
        geometry->internal_id = id;
        internal_data         = &_geometries[id];
    }

    if (internal_data == nullptr)
        Logger::fatal(
            RENDERER_VULKAN_LOG, "Geometry internal data somehow nullptr."
        );

    // Upload vertex data
    vk::DeviceSize buffer_size   = vertex_size * vertex_count;
    vk::DeviceSize buffer_offset = _vertex_buffer->allocate(buffer_size);

    internal_data->vertex_count  = vertex_count;
    internal_data->vertex_size   = vertex_size;
    internal_data->vertex_offset = buffer_offset;

    upload_data_to_buffer(
        vertex_data, buffer_size, buffer_offset, _vertex_buffer
    );

    // Upload index data
    if (index_count > 0) {
        buffer_size   = index_size * index_count;
        buffer_offset = _index_buffer->allocate(buffer_size);

        internal_data->index_count  = index_count;
        internal_data->index_size   = index_size;
        internal_data->index_offset = buffer_offset;

        upload_data_to_buffer(
            index_data, buffer_size, buffer_offset, _index_buffer
        );
    }

    if (is_reupload) {
        _vertex_buffer->deallocate(old_data.vertex_offset);
        if (old_data.index_count > 0)
            _index_buffer->deallocate(old_data.index_offset);
    }
}

// /////////////////////////////// //
// VULKAN BACKEND HELPER FUNCTIONS //
// /////////////////////////////// //

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback_function(
    VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
    VkDebugUtilsMessageTypeFlagsEXT             message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void*                                       user_data
) {
    switch (message_severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        Logger::log("VULKAN :: ", callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        Logger::trace("VULKAN :: ", callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        Logger::warning("VULKAN :: ", callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        if (message_type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
            Logger::fatal("VULKAN :: ", callback_data->pMessage);
        else Logger::error("VULKAN :: ", callback_data->pMessage);
        break;
    default: break;
    }

    return VK_FALSE;
}

/// TODO: TEMP CODE BELOW
void VulkanBackend::create_buffers() {
    // Create vertex buffer
    // TODO: NOT LIKE THIS, values choosen arbitrarily
    vk::DeviceSize vertex_buffer_size = sizeof(Vertex) * 1024 * 1024;
    _vertex_buffer =
        new (MemoryTag::GPUBuffer) VulkanManagedBuffer(_device, _allocator);
    _vertex_buffer->create(
        vertex_buffer_size,
        vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    // Create index buffer
    vk::DeviceSize index_buffer_size = sizeof(uint32) * 1024 * 1024;
    _index_buffer =
        new (MemoryTag::GPUBuffer) VulkanManagedBuffer(_device, _allocator);
    _index_buffer->create(
        index_buffer_size,
        vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
}

void VulkanBackend::upload_data_to_buffer(
    const void*          data,
    vk::DeviceSize       size,
    vk::DeviceSize       offset,
    VulkanManagedBuffer* buffer
) {
    // Create staging buffer
    auto staging_buffer =
        new (MemoryTag::Temp) VulkanBuffer(_device, _allocator);
    staging_buffer->create(
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent
    );

    // Fill created memory with data
    staging_buffer->load_data(data, 0, size);

    auto command_buffer = _command_pool->begin_single_time_commands();
    staging_buffer->copy_data_to_buffer(
        command_buffer, buffer->handle, 0, offset, size
    );
    _command_pool->end_single_time_commands(command_buffer);

    // Cleanup
    del(staging_buffer);
}
// TODO: TEMP CODE END

} // namespace ENGINE_NAMESPACE