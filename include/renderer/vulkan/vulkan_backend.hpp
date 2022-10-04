#pragma once

#include "renderer/renderer_types.hpp"
#include "renderer/renderer_backend.hpp"
#include "vulkan_settings.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_command_pool.hpp"
#include "shaders/vulkan_material_shader.hpp"

#include <map>

class VulkanBackend : public RendererBackend {
public:
    VulkanBackend(Platform::Surface* surface);
    ~VulkanBackend();

    void resized(const uint32 width, const uint32 height);
    bool begin_frame(const float32 delta_time);
    bool end_frame(const float32 delta_time);
    void update_global_state(
        const glm::mat4 projection,
        const glm::mat4 view,
        const glm::vec3 view_position,
        const glm::vec4 ambient_color,
        const int32 mode
    );
    void draw_geometry(const GeometryRenderData data);

    void create_texture(Texture* texture, const byte* const data);
    void destroy_texture(Texture* texture);

    void create_material(Material* const material);
    void destroy_material(Material* const material);

    void create_geometry(
        Geometry* geometry,
        const std::vector<Vertex> vertices,
        const std::vector<uint32> indices
    );
    void destroy_geometry(Geometry* geometry);

private:
    // TODO: Custom allocator
    const vk::AllocationCallbacks* const _allocator = nullptr;

    vk::Instance _vulkan_instance;
    vk::DebugUtilsMessengerEXT _debug_messenger;

    vk::Instance create_vulkan_instance() const;
    vk::DebugUtilsMessengerEXT setup_debug_messenger() const;

    bool all_validation_layers_are_available() const;
    vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_create_info() const;

    // Surface
    vk::SurfaceKHR _vulkan_surface;

    // Synchronization objects
    std::vector<vk::Semaphore> _semaphores_image_available;
    std::vector<vk::Semaphore> _semaphores_render_finished;
    std::vector<vk::Fence> _fences_in_flight;

    void create_sync_objects();

    // DEVICE CODE
    VulkanDevice* _device;

    // SWAPCHAIN CODE
    VulkanSwapchain* _swapchain;
    uint32 _current_frame = 0;
    uint32 _current_image = 0;

    // RENDER PASS
    VulkanRenderPass* _render_pass;

    // OBJECT SHADER
    VulkanMaterialShader* _material_shader;

    // GEOMETRY CODE
    std::map<uint32, VulkanGeometryData> _geometries;

    uint32 generate_geometry_id();

    // TODO: TEMP COMMAND CODE
    VulkanCommandPool* _command_pool;
    std::vector<vk::CommandBuffer> _command_buffers;

    // TODO: TEMP BUFFER CODE
    VulkanBuffer* _vertex_buffer;
    VulkanBuffer* _index_buffer;

    void create_buffers();
    void upload_data_to_buffer(
        const void* data,
        vk::DeviceSize size,
        vk::DeviceSize offset,
        VulkanBuffer* buffer
    );
};