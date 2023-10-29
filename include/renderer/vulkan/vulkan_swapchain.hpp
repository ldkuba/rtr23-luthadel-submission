#pragma once

#include "vulkan_image.hpp"
#include "vulkan_framebuffer.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Vulkan implementation of a swapchain. Utilizes framebuffers,
 * attachments and a surface to present the (rendered) image to the screen.
 *
 */
class VulkanSwapchain {
  public:
    /// @brief Swapchain image extent
    Property<vk::Extent2D> extent {
        GET { return _extent; }
    };
    /// @brief Number of sampled used for Multisample anti-aliasing
    Property<vk::SampleCountFlagBits> msaa_samples {
        GET { return _msaa_samples; }
    };

    /**
     * @brief Construct a new Vulkan Swapchain object
     *
     * @param device Vulkan device reference
     * @param allocator Allocation callback used
     * @param width Swapchain width in pixels
     * @param height Swapchain height in pixels
     * @param vulkan_surface Surface to which we wish to render
     */
    VulkanSwapchain(
        const VulkanDevice* const            device,
        const vk::AllocationCallbacks* const allocator,
        const uint32                         width,
        const uint32                         height,
        const vk::SurfaceKHR                 vulkan_surface
    );
    ~VulkanSwapchain();

    /// @brief Change swapchain image extent
    /// @param width New width
    /// @param height New height
    void   change_extent(const uint32 width, const uint32 height);
    /// @brief Create used framebuffers
    /// @param render_pass Render pass for which to create the framebuffers
    /// @param multisampling If true enables multisampling
    /// @param depth_testing If true enables depth testing
    /// @returns Framebuffer set identifier
    uint32 create_framebuffers(
        const VulkanRenderPass* const render_pass,
        bool                          multisampling,
        bool                          depth_testing
    );

    /// @return Format currently used by the color attachment
    vk::Format get_color_attachment_format() const;
    /// @return Format currently used by the depth attachment
    vk::Format get_depth_attachment_format() const;

    /// @brief Compute the index of the next swapchain image for rendering
    /// @param signal_semaphore Semaphore to signal after acquisition
    void compute_next_image_index(const vk::Semaphore& signal_semaphore);
    /// @brief Present render results
    /// @param image_index Index of the image to present
    /// @param wait_for_semaphores Semaphores to wait on before presenting
    void present(
        const vk::ArrayProxyNoTemporaries<vk::Semaphore>& wait_for_semaphores
    );
    /// @brief Get the framebuffer used in the current draw
    /// @param index Framebuffer set identifier
    VulkanFramebuffer* get_currently_used_framebuffer(const uint32 index);

  private:
    const VulkanDevice*                  _device;
    const vk::AllocationCallbacks* const _allocator;
    const vk::SurfaceKHR                 _vulkan_surface;
    const vk::RenderPass*                _render_pass;

    vk::SwapchainKHR        _handle;
    vk::Format              _format;
    vk::Format              _depth_format;
    vk::Extent2D            _extent;
    vk::SampleCountFlagBits _msaa_samples;

    uint32 _current_image_index = 0;

    struct FramebufferRef {
        Vector<VulkanFramebuffer*> framebuffers {};
        bool                       multisampling;
        bool                       depth_testing;
    };
    Vector<FramebufferRef> _framebuffer_sets {};

    uint32 _width;
    uint32 _height;
    bool   _should_resize = false;

    void create();
    void destroy();
    void recreate();

    // Image resources
    Vector<Texture*> _render_textures {};
    VulkanImage*     _depth_image;
    VulkanImage*     _color_image;

    void create_color_resource();
    void create_depth_resources();
    void find_depth_format();
};

} // namespace ENGINE_NAMESPACE