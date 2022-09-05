#pragma once

#include "vulkan_device.hpp"
#include "platform/platform.hpp"

class VulkanSwapchain {
private:
    VulkanDevice* _device;
    Platform::Surface* _surface;
    vk::RenderPass* _render_pass;
    vk::AllocationCallbacks* _allocator;

    vk::SurfaceKHR _vulkan_surface;
    vk::SwapchainKHR _handle;
    std::vector<vk::ImageView> _image_views;

    void create();
    void destroy();
    void recreate();

    void create_framebuffers();

    // Image resources
    VulkanImage* _depth_image;
    VulkanImage* _color_image;

    void create_color_resource();
    void create_depth_resources();
    vk::Format find_depth_format();

    // Synchronization objects
    std::vector<vk::Semaphore> _semaphores_image_available;
    std::vector<vk::Semaphore> _semaphores_render_finished;
    std::vector<vk::Fence> _fences_in_flight;

    void create_sync_objects();

public:
    vk::Format format;
    vk::Extent2D extent;
    std::vector<vk::Framebuffer> framebuffers;
    vk::SampleCountFlagBits msaa_samples;

    VulkanSwapchain(
        vk::SurfaceKHR vulkan_surface,
        Platform::Surface* surface,
        VulkanDevice* device,
        vk::AllocationCallbacks* allocator
    );
    ~VulkanSwapchain();

    void initialize_framebuffer(vk::RenderPass* render_pass);

    vk::AttachmentDescription get_depth_attachment();
    vk::AttachmentDescription get_color_attachment();
    vk::AttachmentDescription get_color_attachment_resolve();

    uint32 acquire_next_image_index(uint32 current_frame);
    void present(
        uint32 current_frame,
        uint32 image_index,
        std::vector<vk::CommandBuffer> command_buffers
    );
};
