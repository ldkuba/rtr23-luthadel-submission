#pragma once

#include "renderer/renderer_backend.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_command_pool.hpp"
#include "shaders/vulkan_material_shader.hpp"
#include "shaders/vulkan_ui_shader.hpp"

#include <map>

class VulkanBackend : public RendererBackend {
  public:
    VulkanBackend(
        Platform::Surface* const surface, ResourceSystem* const resource_system
    );
    ~VulkanBackend();

    void resized(const uint32 width, const uint32 height);

    Result<void, RuntimeError> begin_frame(const float32 delta_time);
    Result<void, RuntimeError> end_frame(const float32 delta_time);

    void begin_render_pass(uint8 render_pass_id);
    void end_render_pass(uint8 render_pass_id);

    void update_global_world_state(
        const glm::mat4 projection,
        const glm::mat4 view,
        const glm::vec3 view_position,
        const glm::vec4 ambient_color,
        const int32     mode
    );
    void update_global_ui_state(
        const glm::mat4 projection, const glm::mat4 view, const int32 mode
    );

    void draw_geometry(const GeometryRenderData data);

    void create_texture(Texture* texture, const byte* const data);
    void destroy_texture(Texture* texture);

    void create_material(Material* const material);
    void destroy_material(Material* const material);

    void create_geometry(
        Geometry*             geometry,
        const Vector<Vertex>& vertices,
        const Vector<uint32>& indices
    );
    void create_geometry(
        Geometry*               geometry,
        const Vector<Vertex2D>& vertices,
        const Vector<uint32>&   indices
    );
    void destroy_geometry(Geometry* geometry);

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

    // MATERIAL SHADER
    VulkanMaterialShader* _material_shader;

    // UI SHADER
    VulkanUIShader* _ui_shader;

    // GEOMETRY CODE
    std::map<uint32, VulkanGeometryData> _geometries;

    uint32 generate_geometry_id();

    // TODO: TEMP COMMAND CODE
    VulkanCommandPool*        _command_pool;
    Vector<vk::CommandBuffer> _command_buffers;

    // TODO: TEMP BUFFER CODE
    VulkanBuffer* _vertex_buffer;
    VulkanBuffer* _index_buffer;

    void create_buffers();
    void upload_data_to_buffer(
        const void*    data,
        vk::DeviceSize size,
        vk::DeviceSize offset,
        VulkanBuffer*  buffer
    );

    // GEOMETRY
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