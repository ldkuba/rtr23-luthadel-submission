#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>
#include <set>

#include "defines.hpp"

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

class VulkanDevice {
private:
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

    void create_swapchain(const uint32 width, const uint32 height);
    void recreate_swapchain(const uint32 width, const uint32 height);
    void create_image_views();

    QueueFamilyIndices find_queue_families(const vk::PhysicalDevice& device);
    PhysicalDeviceInfo rate_device_suitability(const vk::PhysicalDevice& device);
    SwapchainSupportDetails query_swapchain_support_details(const vk::PhysicalDevice& device);

    // TODO: TEMP PIPELINE CODE
    vk::RenderPass _render_pass;
    vk::DescriptorSetLayout _descriptor_set_layout;
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
    vk::CommandBuffer _command_buffer;

    void create_command_pool();
    void create_command_buffer();

    void record_command_buffer(vk::CommandBuffer command_buffer, uint32 image_index);

    // TODO: TEMP SYNCHRONIZATION
    vk::Semaphore _semaphore_image_available;
    vk::Semaphore _semaphore_render_finished;
    vk::Fence _fence_in_flight;

    void create_sync_objects();

public:
    VulkanDevice(
        vk::Instance* instance,
        vk::AllocationCallbacks* allocator,
        const vk::SurfaceKHR surface,
        const uint32 width,
        const uint32 height
    );
    ~VulkanDevice();

    // TODO: TEMP DRAW CODE
    void draw_frame();

    // TODO: TEMP WAIT IDLE
    void wait_idle() {
        _logical_device.waitIdle();
    }
};