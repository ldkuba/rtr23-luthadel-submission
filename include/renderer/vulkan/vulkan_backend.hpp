#pragma once

#include "renderer/renderer_backend.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_command_pool.hpp"
#include "vulkan_managed_buffer.hpp"
#include "vulkan_shader.hpp"
#include "vulkan_settings.hpp"

#include "map.hpp"

/**
 * @brief Vulkan implementation of RendererBackend abstract class. Central class
 * tasked with initializing and controlling all Vulkan renderer functionalities.
 */
class VulkanBackend : public RendererBackend {
  public:
    VulkanBackend(Platform::Surface* const surface);
    ~VulkanBackend();

    void resized(const uint32 width, const uint32 height) override;

    Result<void, RuntimeError> begin_frame(const float32 delta_time) override;
    Result<void, RuntimeError> end_frame(const float32 delta_time) override;

    void begin_render_pass(uint8 render_pass_id) override;
    void end_render_pass(uint8 render_pass_id) override;

    void draw_geometry(const GeometryRenderData data) override;

    void create_texture(Texture* texture, const byte* const data) override;
    void destroy_texture(Texture* texture) override;

    void create_geometry(
        Geometry*             geometry,
        const Vector<Vertex>& vertices,
        const Vector<uint32>& indices
    ) override;
    void create_geometry(
        Geometry*               geometry,
        const Vector<Vertex2D>& vertices,
        const Vector<uint32>&   indices
    ) override;
    void destroy_geometry(Geometry* geometry) override;

    Shader* create_shader(const ShaderConfig config) override;
    void    destroy_shader(Shader* shader) override;

  private:
    // TODO: Custom allocator
    const vk::AllocationCallbacks* const _allocator = nullptr;

    vk::Instance               _vulkan_instance;
    vk::DebugUtilsMessengerEXT _debug_messenger;

    vk::Instance               create_vulkan_instance() const;
    vk::DebugUtilsMessengerEXT setup_debug_messenger() const;

    bool all_validation_layers_are_available() const;
    vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_create_info() const;

    // Surface
    vk::SurfaceKHR _vulkan_surface;

    // Synchronization objects
    std::array<vk::Semaphore, VulkanSettings::max_frames_in_flight>
        _semaphores_image_available;
    std::array<vk::Semaphore, VulkanSettings::max_frames_in_flight>
        _semaphores_render_finished;
    std::array<vk::Fence, VulkanSettings::max_frames_in_flight>
        _fences_in_flight;

    void create_sync_objects();

    // DEVICE CODE
    VulkanDevice* _device;

    // SWAPCHAIN CODE
    VulkanSwapchain* _swapchain;
    uint32           _current_frame = 0;

    // RENDER PASS
    VulkanRenderPass* _main_render_pass;
    VulkanRenderPass* _ui_render_pass;

    // GEOMETRY CODE
    Map<uint32, VulkanGeometryData> _geometries;

    uint32 generate_geometry_id();

    // COMMAND CODE
    VulkanCommandPool*   _command_pool;
    VulkanCommandBuffer* _command_buffer;

    // TODO: TEMP BUFFER CODE
    VulkanManagedBuffer* _vertex_buffer;
    VulkanManagedBuffer* _index_buffer;

    // Utility buffer methods
    void create_buffers();
    void upload_data_to_buffer(
        const void*          data,
        vk::DeviceSize       size,
        vk::DeviceSize       offset,
        VulkanManagedBuffer* buffer
    );

    // Utility geometry methods
    void create_geometry_internal(
        Geometry*         geometry,
        const uint32      vertex_size,
        const uint32      vertex_count,
        const void* const vertex_data,
        const uint32      index_size,
        const uint32      index_count,
        const void* const index_data
    );
};