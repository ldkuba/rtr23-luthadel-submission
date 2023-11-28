#include "renderer/vulkan/vulkan_backend.hpp"

#include "renderer/vulkan/vulkan_framebuffer.hpp"
#include "resources/geometry.hpp"

namespace ENGINE_NAMESPACE {

// Helper functions
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback_function(
    VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
    VkDebugUtilsMessageTypeFlagsEXT             message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void*                                       user_data
);
bool all_validation_layers_are_available();

vk::SamplerAddressMode convert_repeat_type(const Texture::Repeat repeat);
vk::Filter             convert_filter_type(const Texture::Filter filter);

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

    // Create render pass registry
    _registered_passes.reserve(initial_renderpass_count);

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
    delete _swapchain;

    // TODO: TEMP VERTEX & INDEX BUFFER CODE
    delete _index_buffer;
    delete _vertex_buffer;

    // Render pass
    for (auto& pass : _registered_passes)
        destroy_render_pass(pass);
    _registered_passes.clear();
    _render_pass_table.clear();

    // Command pool
    delete _command_pool;

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
    delete _device;
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

void VulkanBackend::resized(const uint32 width, const uint32 height) {
    if (_swapchain != nullptr) _swapchain->change_extent(width, height);
}

// -----------------------------------------------------------------------------
// Textures
// -----------------------------------------------------------------------------

Texture* VulkanBackend::create_texture(
    const Texture::Config& config, const byte* const data
) {
    Logger::trace(RENDERER_VULKAN_LOG, "Creating texture.");

    // Get format
    const auto texture_format =
        VulkanTexture::channel_count_to_UNORM(config.channel_count);

    // Create device side image
    // NOTE: Lots of assumptions here
    auto texture_image =
        new (MemoryTag::GPUTexture) VulkanImage(_device, _allocator);
    if (config.type == Texture::Type::T2D) {
        texture_image->create_2d(
            config.width,
            config.height,
            config.mip_level_count,
            vk::SampleCountFlagBits::e1,
            texture_format,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eTransferDst |
                vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            vk::ImageAspectFlagBits::eColor
        );
    } else if (config.type == Texture::Type::TCube) {
        texture_image->create_cube(
            config.width,
            config.height,
            config.mip_level_count,
            vk::SampleCountFlagBits::e1,
            texture_format,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eTransferDst |
                vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            vk::ImageAspectFlagBits::eColor
        );
    } else {
        Logger::fatal(
            RENDERER_VULKAN_LOG,
            "Creation of this texture type is unimplemented."
        );
    }

    // Create texture
    const auto texture = new (MemoryTag::Texture
    ) VulkanTexture(config, texture_image, _command_pool, _device, _allocator);

    // Write data
    texture->write(data, texture->total_size, 0);

    Logger::trace(RENDERER_VULKAN_LOG, "Texture created.");
    return texture;
}

Texture* VulkanBackend::create_writable_texture(const Texture::Config& config) {
    Logger::trace(RENDERER_VULKAN_LOG, "Creating texture.");

    // Get format
    const auto texture_format =
        VulkanTexture::channel_count_to_UNORM(config.channel_count);

    // Create device side image
    // NOTE: Lots of assumptions here
    auto texture_image =
        new (MemoryTag::GPUTexture) VulkanImage(_device, _allocator);
    texture_image->create_2d(
        config.width,
        config.height,
        config.mip_level_count,
        vk::SampleCountFlagBits::e1,
        texture_format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eColorAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eColor
    );

    // Create texture
    const auto texture = new (MemoryTag::Texture
    ) VulkanTexture(config, texture_image, _command_pool, _device, _allocator);

    Logger::trace(RENDERER_VULKAN_LOG, "Texture created.");
    return texture;
}

void VulkanBackend::destroy_texture(Texture* const texture) {
    if (texture == nullptr) return;

    auto vt = reinterpret_cast<VulkanTexture*>(texture);

    _device->handle().waitIdle();
    del(vt);

    Logger::trace(RENDERER_VULKAN_LOG, "Texture destroyed.");
}

// -----------------------------------------------------------------------------
// Texture map
// -----------------------------------------------------------------------------

Texture::Map* VulkanBackend::create_texture_map(
    const Texture::Map::Config& config
) {
    Logger::trace(RENDERER_VULKAN_LOG, "Creating texture map.");

    // TODO: Additional configurable settings
    // Create sampler
    vk::SamplerCreateInfo sampler_info {};
    sampler_info.setAddressModeU(convert_repeat_type(config.repeat_u));
    sampler_info.setAddressModeV(convert_repeat_type(config.repeat_v));
    sampler_info.setAddressModeW(convert_repeat_type(config.repeat_w));
    sampler_info.setMagFilter(convert_filter_type(config.filter_magnify));
    sampler_info.setMinFilter(convert_filter_type(config.filter_minify));
    sampler_info.setAnisotropyEnable(true);
    sampler_info.setMaxAnisotropy(_device->info().max_sampler_anisotropy);
    sampler_info.setBorderColor(vk::BorderColor::eIntOpaqueBlack);
    sampler_info.setUnnormalizedCoordinates(false);
    sampler_info.setCompareEnable(false);
    sampler_info.setCompareOp(vk::CompareOp::eAlways);
    // Mipmap settings
    sampler_info.setMipmapMode(vk::SamplerMipmapMode::eLinear);
    sampler_info.setMipLodBias(0.0f);
    sampler_info.setMinLod(0.0f);
    sampler_info.setMaxLod( //
        static_cast<float32>(config.texture->mip_level_count)
    );

    vk::Sampler texture_sampler;
    try {
        texture_sampler =
            _device->handle().createSampler(sampler_info, _allocator);
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    // Create texture map
    const auto texture_map =
        new (MemoryTag::TextureMap) VulkanTexture::Map(config, texture_sampler);

    Logger::trace(RENDERER_VULKAN_LOG, "Texture map created.");
    return texture_map;
}
void VulkanBackend::destroy_texture_map(Texture::Map* map) {
    if (!map) return;
    auto v_map = static_cast<VulkanTexture::Map*>(map);
    if (v_map->sampler) _device->handle().destroySampler(v_map->sampler);
    del(v_map);
}

// -----------------------------------------------------------------------------
// Geometry
// -----------------------------------------------------------------------------

void VulkanBackend::create_geometry(
    Geometry* const         geometry,
    const Vector<Vertex3D>& vertices,
    const Vector<uint32>&   indices
) {
    create_geometry_internal(
        geometry,
        sizeof(Vertex3D),
        vertices.size(),
        vertices.data(),
        sizeof(uint32),
        indices.size(),
        indices.data()
    );
}
void VulkanBackend::create_geometry(
    Geometry* const         geometry,
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
void VulkanBackend::destroy_geometry(Geometry* const geometry) {
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

// -----------------------------------------------------------------------------
// Shader
// -----------------------------------------------------------------------------

Shader* VulkanBackend::create_shader(
    Renderer* const       renderer,
    TextureSystem* const  texture_system,
    const Shader::Config& config
) {
    Logger::trace(RENDERER_VULKAN_LOG, "Creating shader.");

    // Get render pass
    const auto render_pass_res = get_render_pass(config.render_pass_name);
    if (render_pass_res.has_error())
        Logger::fatal(RENDERER_VULKAN_LOG, render_pass_res.error().what());
    const auto render_pass =
        dynamic_cast<VulkanRenderPass*>(render_pass_res.value());

    // Create shader
    auto shader = new (MemoryTag::Shader) VulkanShader(
        renderer,
        texture_system,
        config,
        _device,
        _allocator,
        render_pass,
        _command_buffer
    );

    Logger::trace(RENDERER_VULKAN_LOG, "Shader created.");
    return shader;
}
void VulkanBackend::destroy_shader(Shader* const shader) {
    del(shader);
    Logger::trace(RENDERER_VULKAN_LOG, "Shader destroyed.");
}

// -----------------------------------------------------------------------------
// Render pass
// -----------------------------------------------------------------------------

RenderPass* VulkanBackend::create_render_pass(const RenderPass::Config& config
) {
    // Name is not empty
    if (config.name.empty())
        Logger::fatal(
            RENDERER_VULKAN_LOG,
            "Empty renderpass name passed. Initialization failed."
        );

    // There mustn't be a name collision
    if (_render_pass_table.find(config.name) != _render_pass_table.end())
        Logger::fatal(
            RENDERER_VULKAN_LOG,
            "Two renderpass configurations given with the same renderpass "
            "name; `",
            config.name,
            "`. Initialization failed."
        );

    // Generate new pass id
    const auto new_id = _registered_passes.size();

    // Register pass
    _render_pass_table[config.name] = new_id;

    // Create and add this pass
    const auto renderpass = new (MemoryTag::GPUBuffer) VulkanRenderPass(
        new_id,
        config,
        &_device->handle(),
        _allocator,
        _command_buffer,
        _swapchain
    );
    _registered_passes.push_back(renderpass);

    return renderpass;
}
void VulkanBackend::destroy_render_pass(RenderPass* pass) {
    const auto vulkan_pass = dynamic_cast<VulkanRenderPass*>(pass);
    del(vulkan_pass);
}

Result<RenderPass*, RuntimeError> VulkanBackend::get_render_pass(
    const String& name
) const {
    const auto pass_id = _render_pass_table.find(name);
    if (pass_id == _render_pass_table.end())
        return Failure(
            String::build("No registered renderpass is named `", name, "`")
        );
    return _registered_passes[pass_id->second];
}

// -----------------------------------------------------------------------------
// Attachments
// -----------------------------------------------------------------------------

uint8 VulkanBackend::get_current_window_attachment_index() const {
    return _swapchain->get_current_index();
}
uint8 VulkanBackend::get_window_attachment_count() const {
    return _swapchain->get_render_texture_count();
}

Texture* VulkanBackend::get_window_attachment(const uint8 index) const {
    return _swapchain->get_render_texture(index);
}
Texture* VulkanBackend::get_depth_attachment() const {
    return _swapchain->get_depth_texture();
}
Texture* VulkanBackend::get_color_attachment() const {
    return _swapchain->get_color_texture();
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

// -----------------------------------------------------------------------------
// Debug messenger
// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------
// Synch methods
// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------
// Buffer methods
// -----------------------------------------------------------------------------

// TODO: TEMP CODE BELOW
void VulkanBackend::create_buffers() {
    // Create vertex buffer
    // TODO: NOT LIKE THIS, values choosen arbitrarily
    vk::DeviceSize vertex_buffer_size = sizeof(Vertex3D) * 1024 * 1024;
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
    delete staging_buffer;
}
// TODO: TEMP CODE END

// -----------------------------------------------------------------------------
// Geometry methods
// -----------------------------------------------------------------------------

uint32 VulkanBackend::generate_geometry_id() {
    static uint32 id = 0;
    return id++;
}

void VulkanBackend::create_geometry_internal(
    Geometry* const   geometry,
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

bool all_validation_layers_are_available() {
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

vk::SamplerAddressMode convert_repeat_type(const Texture::Repeat repeat) {
    switch (repeat) {
    case Texture::Repeat::Repeat: //
        return vk::SamplerAddressMode::eRepeat;
    case Texture::Repeat::MirroredRepeat:
        return vk::SamplerAddressMode::eMirroredRepeat;
    case Texture::Repeat::ClampToEdge:
        return vk::SamplerAddressMode::eClampToEdge;
    case Texture::Repeat::ClampToBorder:
        return vk::SamplerAddressMode::eClampToBorder;
    default:
        Logger::warning(
            RENDERER_VULKAN_LOG,
            "Conversion of repeat type ",
            (int32) repeat,
            " not supported. Function `convert_repeat_type` will default to "
            "repeat."
        );
        return vk::SamplerAddressMode::eRepeat;
    }
}

vk::Filter convert_filter_type(const Texture::Filter filter) {
    switch (filter) {
    case Texture::Filter::NearestNeighbour: return vk::Filter::eNearest;
    case Texture::Filter::BiLinear: return vk::Filter::eLinear;
    default:
        Logger::warning(
            RENDERER_VULKAN_LOG,
            "Conversion of filter mode ",
            (int32) filter,
            " not supported. Function `convert_filter_type` will default to "
            "linear."
        );
        return vk::Filter::eLinear;
    }
}

} // namespace ENGINE_NAMESPACE