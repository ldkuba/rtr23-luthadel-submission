#pragma once

#include "renderer/renderer_backend.hpp"
#include "vulkan_settings.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_buffer.hpp"
#include "shaders/vulkan_object_shader.hpp"
#include "../renderer_types.hpp"

class VulkanBackend : public RendererBackend {
private:
    // TODO: Custom allocator
    vk::AllocationCallbacks* _allocator = nullptr;

    vk::Instance _vulkan_instance;
    vk::DebugUtilsMessengerEXT _debug_messenger;

    const std::vector<const char*> _validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    void create_vulkan_instance();
    void setup_debug_messenger();

    bool all_validation_layers_are_available();
    vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_create_info();

    // Surface
    vk::SurfaceKHR _vulkan_surface;

    // Synchronization objects
    std::vector<vk::Semaphore> _semaphores_image_available;
    std::vector<vk::Semaphore> _semaphores_render_finished;
    std::vector<vk::Fence> _fences_in_flight;

    void create_sync_objects();

    // DEVICE CODE
    VulkanDevice* _device;

    void create_device();

    // SWAPCHAIN CODE
    VulkanSwapchain* _swapchain;
    uint32 current_frame = 0;

    // RENDER PASS
    VulkanRenderPass* _render_pass;

    // OBJECT SHADER
    VulkanObjectShader* _object_shader;

    // TODO: TEMP COMMAND CODE
    VulkanCommandPool* _command_pool;
    std::vector<vk::CommandBuffer> _command_buffers;

    void record_command_buffer(vk::CommandBuffer command_buffer, uint32 image_index);

    // TODO: TEMP VERTEX BUFFER CODE
    VulkanBuffer* _vertex_buffer;

    void create_vertex_buffer();

    // TODO: TEMP INDEX BUFFER CODE
    VulkanBuffer* _index_buffer;

    void create_index_buffer();

    // TODO: TEMP UNIFORM CODE
    std::vector<VulkanBuffer*> _uniform_buffers;

    void create_descriptor_sets();
    void create_uniform_buffers();
    void update_uniform_buffer(uint32 current_image);

    // TODO: TEMP IMAGE CODE
    VulkanImage* _texture_image;
    vk::Sampler _texture_sampler;

    void create_texture_image();
    void create_texture_sampler();

    // TODO: TEMP MODEL LOADING CODE
    std::vector<Vertex> vertices;
    std::vector<uint32> indices;

    void load_model();

    // TODO: TEMP MIPMAP CODE
    uint32 _mip_levels;

    void generate_mipmaps(vk::Image image, vk::Format format, uint32 width, uint32 height, uint32 mip_levels);

    // TODO: TEMP MSAA CODE

public:
    VulkanBackend(Platform::Surface* surface);
    ~VulkanBackend();

    void resized(uint32 width, uint32 height);
    bool begin_frame(float32 delta_time);
    bool end_frame(float32 delta_time);

    void wait_for_shutdown() {
        _device->handle.waitIdle();
    }
};