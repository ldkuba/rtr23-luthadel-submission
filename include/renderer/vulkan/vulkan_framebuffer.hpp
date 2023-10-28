#pragma once

#include "property.hpp"
#include "logger.hpp"
#include <vulkan/vulkan.hpp>

namespace ENGINE_NAMESPACE {

class VulkanRenderPass;

/**
 * @brief Vulkan implementation of framebuffer object.
 */
class VulkanFramebuffer {
  public:
    /// @brief Handle to vk::Framebuffer
    Property<vk::Framebuffer> handle {
        GET { return _handle; }
    };

    /**
     * @brief Construct a new Vulkan Framebuffer object
     *
     * @param device Vulkan device reference
     * @param allocator Allocation callback used
     * @param render_pass Renderpass used for framebuffer
     * @param width Framebuffer width in pixels
     * @param height Framebuffer height in pixels
     * @param attachments List of attached Image views
     */
    VulkanFramebuffer(
        const vk::Device* const              device,
        const vk::AllocationCallbacks* const allocator,
        const VulkanRenderPass* const        render_pass,
        const uint32                         width,
        const uint32                         height,
        const Vector<vk::ImageView>&         attachments
    );
    ~VulkanFramebuffer();

    /**
     * @brief Recreates framebuffer object
     *
     * @param width New width in pixels
     * @param height New height in pixels
     * @param attachments New list of attached Image views
     */
    void recreate(
        const uint32                 width,
        const uint32                 height,
        const Vector<vk::ImageView>& attachments
    );

  private:
    const vk::Device*                    _device;
    const vk::AllocationCallbacks* const _allocator;

    vk::Framebuffer         _handle {};
    const VulkanRenderPass* _render_pass;

    void create(
        const uint32                 width,
        const uint32                 height,
        const Vector<vk::ImageView>& attachments
    );
};

} // namespace ENGINE_NAMESPACE