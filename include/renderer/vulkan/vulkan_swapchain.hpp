#pragma once

#include "vulkan_image.hpp"

class VulkanSwapchain {
  public:
    /// @brief Swapchain image extent
    Property<vk::Extent2D> extent {
        Get { return _extent; }
    };
    /// @brief Framebuffers used for each swapchain image
    Property<std::vector<vk::Framebuffer>> framebuffers {
        Get { return _framebuffers; }
    };
    /// @brief Number of sampled used for Multisample anti-aliasing
    Property<vk::SampleCountFlagBits> msaa_samples {
        Get { return _msaa_samples; }
    };

    VulkanSwapchain(
        const uint32                         width,
        const uint32                         height,
        const vk::SurfaceKHR                 vulkan_surface,
        const VulkanDevice* const            device,
        const vk::AllocationCallbacks* const allocator
    );
    ~VulkanSwapchain();

    /// @brief Change swapchain image extent
    /// @param width New width
    /// @param height New height
    void change_extent(const uint32 width, const uint32 height);
    /// @brief Used for initial creation of framebuffers
    /// @param render_pass Render pass for which to create the framebuffers
    void initialize_framebuffers(const vk::RenderPass* const render_pass);

    /// @return Appropriately filled vk::AttachmentDescription object for depth
    /// attachment
    vk::AttachmentDescription get_depth_attachment() const;
    /// @return Appropriately filled vk::AttachmentDescription object for color
    /// attachment
    vk::AttachmentDescription get_color_attachment() const;
    /// @return Appropriately filled vk::AttachmentDescription object for color
    /// attachment resolve
    vk::AttachmentDescription get_color_attachment_resolve() const;

    /// @brief Compute the index of the next swapchain image to be rendered to
    /// @param signal_semaphore Semaphore to signal after acquisition
    /// @return Index of next image to be rendered to
    uint32 acquire_next_image_index(const vk::Semaphore& signal_semaphore);
    /// @brief Present render results
    /// @param image_index Index of the image to present
    /// @param wait_for_semaphores Semaphores to wait on before presenting
    void   present(
          const uint32                      image_index,
          const std::vector<vk::Semaphore>& wait_for_semaphores
      );

  private:
    const VulkanDevice*                  _device;
    const vk::AllocationCallbacks* const _allocator;
    const vk::SurfaceKHR                 _vulkan_surface;
    const vk::RenderPass*                _render_pass;

    vk::SwapchainKHR           _handle;
    std::vector<vk::ImageView> _image_views;
    vk::Format                 _format;

    vk::Extent2D                 _extent;
    std::vector<vk::Framebuffer> _framebuffers;
    vk::SampleCountFlagBits      _msaa_samples;

    uint32 _width;
    uint32 _height;
    bool   _should_resize = false;

    void create();
    void destroy();
    void recreate();
    void create_framebuffers();

    // Image resources
    VulkanImage* _depth_image;
    VulkanImage* _color_image;

    void       create_color_resource();
    void       create_depth_resources();
    vk::Format find_depth_format() const;
};