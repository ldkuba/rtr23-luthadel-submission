#include "renderer/vulkan/vulkan_framebuffer.hpp"

#include "renderer/vulkan/vulkan_render_pass.hpp"

// Constructor & Destructor
VulkanFramebuffer::VulkanFramebuffer(
    const vk::Device* const              device,
    const vk::AllocationCallbacks* const allocator,
    const VulkanRenderPass* const        render_pass,
    const uint32                         width,
    const uint32                         height,
    const Vector<vk::ImageView>&         attachments
)
    : _device(device), _allocator(allocator), _render_pass(render_pass) {
    create(width, height, attachments);
}
VulkanFramebuffer::~VulkanFramebuffer() {
    _device->destroyFramebuffer(_handle, _allocator);
}

void VulkanFramebuffer::recreate(
    const uint32                 width,
    const uint32                 height,
    const Vector<vk::ImageView>& attachments
) {
    _device->destroyFramebuffer(_handle, _allocator);
    create(width, height, attachments);
}

// ///////////////////////////////// //
// VULKAN FRAMEBUFFER PUBLIC METHODS //
// ///////////////////////////////// //

void VulkanFramebuffer::create(
    const uint32                 width,
    const uint32                 height,
    const Vector<vk::ImageView>& attachments
) {
    // Create framebuffer
    vk::FramebufferCreateInfo framebuffer_info {};
    // Render pass with which framebuffer need to be compatible
    framebuffer_info.setRenderPass(_render_pass->handle);
    // List of objects bound to the corresponding attachment descriptions
    // render pass
    framebuffer_info.setAttachments(attachments);
    framebuffer_info.setWidth(width);   // Framebuffer width
    framebuffer_info.setHeight(height); // Framebuffer height
    framebuffer_info.setLayers(1);      // Number of layers in image array

    try {
        _handle = _device->createFramebuffer(framebuffer_info, _allocator);
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }
}
