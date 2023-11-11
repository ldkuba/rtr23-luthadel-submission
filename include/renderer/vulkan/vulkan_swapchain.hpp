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

    /// @brief Event that is evoked after swapchain recreation completes
    Event<void(uint32, uint32)> recreate_event {};

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

    /// @brief Current image index
    uint8 get_current_index() const;
    /// @brief Image / Render texture count
    uint8 get_render_texture_count() const;

    /// @brief Active render texture at @p index
    Texture* get_render_texture(const uint8 index) const;
    /// @brief Active depth texture for depth testing
    Texture* get_depth_texture() const;
    /// @brief Active color / resolve texture for multisampling
    Texture* get_color_texture() const;

    /// @return Format currently used by the color attachment
    vk::Format get_color_attachment_format() const;
    /// @return Format currently used by the depth attachment
    vk::Format get_depth_attachment_format() const;

    /// @brief Change swapchain image extent
    /// @param width New width
    /// @param height New height
    void change_extent(const uint32 width, const uint32 height);

    /// @brief Compute the index of the next swapchain image for rendering
    /// @param signal_semaphore Semaphore to signal after acquisition
    void compute_next_image_index(const vk::Semaphore& signal_semaphore);

    /// @brief Present render results
    /// @param image_index Index of the image to present
    /// @param wait_for_semaphores Semaphores to wait on before presenting
    void present(
        const vk::ArrayProxyNoTemporaries<vk::Semaphore>& wait_for_semaphores
    );

  private:
    const VulkanDevice*                  _device;
    const vk::AllocationCallbacks* const _allocator;
    const vk::SurfaceKHR                 _vulkan_surface;
    const vk::RenderPass*                _render_pass;

    vk::SwapchainKHR        _handle;
    vk::Format              _format;
    vk::Format              _depth_format;
    uint32                  _depth_format_channel_count;
    vk::Extent2D            _extent;
    vk::SampleCountFlagBits _msaa_samples;

    uint32 _current_image_index = 0;

    uint32 _width;
    uint32 _height;
    bool   _should_resize = false;

    // Image resources
    Vector<Texture*> _render_textures {};
    Texture*         _depth_attachment {};
    Texture*         _color_attachment {};

    void create();
    void destroy();
    void recreate();

    void create_color_resource();
    void create_depth_resources();
    void find_depth_format();
};

} // namespace ENGINE_NAMESPACE