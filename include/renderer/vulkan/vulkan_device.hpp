#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>
#include <set>

#include "defines.hpp"
#include "platform/platform.hpp"

struct PhysicalDeviceInfo {
    uint32 suitability;
    std::string name;
    std::string type;
    std::string driver_version;
    std::string api_version;
    std::vector<float32> memory_size_in_gb;
    std::vector<bool> memory_is_local;
};

struct QueueFamilyIndices {
    std::optional<uint32> graphics_family;
    std::optional<uint32> compute_family;
    std::optional<uint32> transfer_family;
    std::optional<uint32> present_family;

    bool is_complete();
    std::set<uint32> get_unique_indices();
};

struct SwapchainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentation_modes;

    vk::Extent2D get_extent(uint32 width, uint32 height);
    vk::SurfaceFormatKHR get_surface_format();
    vk::PresentModeKHR get_presentation_mode();
};

struct Vertex;

class VulkanDevice {
private:
    // Surface
    Platform::Surface* _surface;

    vk::Instance* _vulkan_instance;
    vk::AllocationCallbacks* _vulkan_allocator;
    vk::SurfaceKHR _vulkan_surface;

    vk::PhysicalDevice _physical_device = VK_NULL_HANDLE;
    vk::Device _logical_device;

    // Queues
    vk::Queue _graphics_queue;
    vk::Queue _presentation_queue;
    vk::Queue _transfer_queue;
    vk::Queue _compute_queue;

    // Swapchain
    vk::SwapchainKHR _swapchain;
    vk::Format _swapchain_format;
    vk::Extent2D _swapchain_extent;
    std::vector<vk::Image> _swapchain_images;
    std::vector<vk::ImageView> _swapchain_image_views;

    void pick_physical_device();
    void create_logical_device();

    void create_swapchain();
    void recreate_swapchain();
    void cleanup_swapchain();

    vk::ImageView create_image_view(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect_flags);
    void create_image_views();

    QueueFamilyIndices find_queue_families(const vk::PhysicalDevice& device);
    PhysicalDeviceInfo rate_device_suitability(const vk::PhysicalDevice& device);
    SwapchainSupportDetails query_swapchain_support_details(const vk::PhysicalDevice& device);

    // TODO: TEMP PIPELINE CODE
    vk::RenderPass _render_pass;
    vk::PipelineLayout _pipeline_layout;
    vk::Pipeline _graphics_pipeline;

    void create_render_pass();
    void create_pipeline();

    vk::ShaderModule create_shader_module(const std::vector<byte>& code);

    // TODO: TEMP FRAMEBUFFER CODE
    std::vector<vk::Framebuffer> _swapchain_framebuffers;

    void create_framebuffers();

    // TODO: TEMP COMMAND CODE
    vk::CommandPool _command_pool;
    std::vector<vk::CommandBuffer> _command_buffers;

    void create_command_pool();
    void create_command_buffers();

    void record_command_buffer(vk::CommandBuffer command_buffer, uint32 image_index);

    // TODO: TEMP SYNCHRONIZATION CODE
    std::vector<vk::Semaphore> _semaphores_image_available;
    std::vector<vk::Semaphore> _semaphores_render_finished;
    std::vector<vk::Fence> _fences_in_flight;

    void create_sync_objects();

    uint32 current_frame = 0;

    // TODO: TEMP VERTEX BUFFER CODE
    vk::Buffer _vertex_buffer;
    vk::DeviceMemory _vertex_buffer_memory;

    void create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
        vk::Buffer& buffer, vk::DeviceMemory& buffer_memory);
    void create_vertex_buffer();

    uint32 find_memory_type(uint32 type_filter, vk::MemoryPropertyFlags properties);

    vk::CommandBuffer begin_single_time_commands();
    void end_single_time_commands(vk::CommandBuffer command_buffer);

    void copy_buffer(vk::Buffer source_buffer, vk::Buffer destination_buffer, vk::DeviceSize size);

    // TODO: TEMP INDEX BUFFER CODE
    vk::Buffer _index_buffer;
    vk::DeviceMemory _index_buffer_memory;

    void create_index_buffer();

    // TODO: TEMP UNIFORM CODE
    vk::DescriptorSetLayout _descriptor_set_layout;
    vk::DescriptorPool _descriptor_pool;
    std::vector<vk::DescriptorSet> _descriptor_sets;

    std::vector<vk::Buffer> _uniform_buffers;
    std::vector<vk::DeviceMemory> _uniform_buffers_memory;

    void create_descriptor_set_layout();
    void create_uniform_buffers();
    void update_uniform_buffer(uint32 current_image);
    void create_descriptor_pool();
    void create_descriptor_sets();

    // TODO: TEMP IMAGE CODE
    vk::Image _texture_image;
    vk::DeviceMemory _texture_image_memory;
    vk::ImageView _texture_image_view;
    vk::Sampler _texture_sampler;

    void create_image(
        uint32 width,
        uint32 height,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        vk::Image& image, vk::DeviceMemory& image_memory
    );
    void transition_image_layout(
        vk::Image image,
        vk::Format format,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout
    );
    void copy_buffer_to_image(vk::Buffer buffer, vk::Image image, uint32 width, uint32 height);
    void create_texture_image();
    void create_texture_image_view();
    void create_texture_sampler();

    // TODO: TEMP DEPTH BUFFER CODE
    vk::Image _depth_image;
    vk::DeviceMemory _depth_image_memory;
    vk::ImageView _depth_image_view;

    void create_depth_resources();
    vk::Format find_depth_format();
    vk::Format find_supported_formats(
        const std::vector<vk::Format>& candidates,
        vk::ImageTiling tiling,
        vk::FormatFeatureFlags features
    );

    // TODO: TEMP MODEL LOADING CODE
    std::vector<Vertex> vertices;
    std::vector<uint32> indices;

    void load_model();

public:
    VulkanDevice(
        vk::Instance* instance,
        vk::AllocationCallbacks* allocator,
        Platform::Surface* surface
    );
    ~VulkanDevice();

    // TODO: TEMP DRAW CODE
    void draw_frame();

    // TODO: TEMP WAIT IDLE
    void wait_idle() {
        _logical_device.waitIdle();
    }
};