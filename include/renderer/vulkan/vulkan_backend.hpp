#pragma once

#include "renderer/renderer_backend.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_command_pool.hpp"
#include "vulkan_managed_buffer.hpp"
#include "vulkan_shader.hpp"
#include "vulkan_settings.hpp"

#include "map.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Vulkan implementation of RendererBackend abstract class. Central class
 * tasked with initializing and controlling all Vulkan renderer functionalities.
 */
class VulkanBackend : public RendererBackend {
  public:
    VulkanBackend(Platform::Surface* const surface);
    ~VulkanBackend() override;

    Result<void, RuntimeError> begin_frame(const float32 delta_time) override;
    Result<void, RuntimeError> end_frame(const float32 delta_time) override;

    void resized(const uint32 width, const uint32 height) override;

    // Texture
    void create_texture(Texture* const texture, const byte* const data)
        override;
    void create_writable_texture(Texture* const texture) override;
    void destroy_texture(Texture* const texture) override;
    void resize_texture(
        Texture* const texture, const uint32 width, const uint32 height
    ) override;
    void texture_write_data(
        Texture* const    texture,
        const byte* const data,
        const uint32      size,
        const uint32      offset
    ) override;

    // Geometry
    void create_geometry(
        Geometry* const         geometry,
        const Vector<Vertex3D>& vertices,
        const Vector<uint32>&   indices
    ) override;
    void create_geometry(
        Geometry* const         geometry,
        const Vector<Vertex2D>& vertices,
        const Vector<uint32>&   indices
    ) override;
    void destroy_geometry(Geometry* const geometry) override;
    void draw_geometry(Geometry* const geometry) override;

    // Shader
    Shader* create_shader(const ShaderConfig config) override;
    void    destroy_shader(Shader* shader) override;

    // Render targets
    RenderTarget* create_render_target(
        RenderPass* const       pass,
        const uint32            width,
        const uint32            height,
        const Vector<Texture*>& attachments
    ) override;
    void destroy_render_target(
        RenderTarget* const render_target, const bool free_internal_data = true
    ) override;

    // Render pass
    RenderPass* create_render_pass(const RenderPass::Config& config) override;
    void        destroy_render_pass(RenderPass* const pass) override;
    Result<RenderPass*, RuntimeError> get_render_pass(const String& name
    ) const override;

    // Attachments
    uint8    get_current_window_attachment_index() const override;
    uint8    get_window_attachment_count() const override;
    Texture* get_window_attachment(const uint8 index) const override;
    Texture* get_depth_attachment() const override;
    Texture* get_color_attachment() const override;

  private:
    // TODO: Custom allocator
    const vk::AllocationCallbacks* const _allocator = nullptr;

    vk::Instance               _vulkan_instance;
    vk::DebugUtilsMessengerEXT _debug_messenger;

    // Surface
    vk::SurfaceKHR _vulkan_surface;

    // Synchronization objects
    std::array<vk::Semaphore, VulkanSettings::max_frames_in_flight>
        _semaphores_image_available;
    std::array<vk::Semaphore, VulkanSettings::max_frames_in_flight>
        _semaphores_render_finished;
    std::array<vk::Fence, VulkanSettings::max_frames_in_flight>
        _fences_in_flight;

    // DEVICE CODE
    VulkanDevice* _device;

    // SWAPCHAIN CODE
    VulkanSwapchain* _swapchain;
    uint32           _current_frame = 0;

    // RENDER PASS
    UnorderedMap<String, uint16> _render_pass_table {};
    Vector<VulkanRenderPass*>    _registered_passes {};
    static constexpr const uint8 initial_renderpass_count = 32;

    // GEOMETRY CODE
    Map<uint32, VulkanGeometryData> _geometries;

    // COMMAND CODE
    VulkanCommandPool*   _command_pool;
    VulkanCommandBuffer* _command_buffer;

    // TODO: TEMP BUFFER CODE
    VulkanManagedBuffer* _vertex_buffer;
    VulkanManagedBuffer* _index_buffer;

    // General methods
    vk::Instance create_vulkan_instance() const;

    // Debugger
    vk::DebugUtilsMessengerEXT           setup_debug_messenger() const;
    vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_create_info() const;

    // Sync method
    void create_sync_objects();

    // Utility buffer methods
    void create_buffers();
    void upload_data_to_buffer(
        const void*          data,
        vk::DeviceSize       size,
        vk::DeviceSize       offset,
        VulkanManagedBuffer* buffer
    );

    // Utility geometry methods
    uint32 generate_geometry_id();
    void   create_geometry_internal(
          Geometry* const   geometry,
          const uint32      vertex_size,
          const uint32      vertex_count,
          const void* const vertex_data,
          const uint32      index_size,
          const uint32      index_count,
          const void* const index_data
      );
};

} // namespace ENGINE_NAMESPACE