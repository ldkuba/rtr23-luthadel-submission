#pragma once

#include "vulkan_image.hpp"
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
    vk::Format _format;

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

public:
    /// @brief Swapchain image extent
    vk::Extent2D extent;
    /// @brief Framebuffers used for each swapchain image
    std::vector<vk::Framebuffer> framebuffers;
    /// @brief Number of sampled used for Multisample anti-aliasing
    vk::SampleCountFlagBits msaa_samples;

    VulkanSwapchain(
        vk::SurfaceKHR vulkan_surface,
        Platform::Surface* surface,
        VulkanDevice* device,
        vk::AllocationCallbacks* allocator
    );
    ~VulkanSwapchain();

    /// @brief Used for initial creation of framebuffers
    /// @param render_pass Render pass for which to create the framebuffers
    void initialize_framebuffers(vk::RenderPass* render_pass);

    /// @return Appropriately filled vk::AttachmentDescription object for depth attachment
    vk::AttachmentDescription get_depth_attachment();
    /// @return Appropriately filled vk::AttachmentDescription object for color attachment
    vk::AttachmentDescription get_color_attachment();
    /// @return Appropriately filled vk::AttachmentDescription object for color attachment resolve
    vk::AttachmentDescription get_color_attachment_resolve();

    /// @brief Compute the index of the next swapchain image to be rendered to
    /// @param signal_semaphore Semaphore to signal after acquisition
    /// @return Index of next image to be rendered to
    uint32 acquire_next_image_index(vk::Semaphore signal_semaphore);
    /// @brief Present render results
    /// @param image_index Index of the image to present
    /// @param wait_for_semaphores Semaphores to wait on before presenting
    void present(uint32 image_index, std::vector<vk::Semaphore> wait_for_semaphores);
};
