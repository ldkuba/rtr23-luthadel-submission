#include "renderer/vulkan/vulkan_framebuffer.hpp"

#include "renderer/vulkan/vulkan_render_pass.hpp"

namespace ENGINE_NAMESPACE {

Vector<vk::ImageView> get_view_attachments(const Vector<Texture*> attachments);

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

// ///////////////////////////////// //
// VULKAN FRAMEBUFFER PUBLIC METHODS //
// ///////////////////////////////// //

void VulkanFramebuffer::recreate(
    const uint32 width, const uint32 height, const Vector<Texture*>& attachments
) {
    _device->destroyFramebuffer(_handle, _allocator);
    const auto view_att = get_view_attachments(attachments);
    create(width, height, view_att);
}

// ////////////////////////////////// //
// VULKAN FRAMEBUFFER PRIVATE METHODS //
// ////////////////////////////////// //

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

// /////////////////////////////////// //
// VULKAN FRAMEBUFFER HELPER FUNCTIONS //
// /////////////////////////////////// //

Vector<vk::ImageView> get_view_attachments(const Vector<Texture*> attachments) {
    Vector<vk::ImageView> view_attachments { attachments.size() };
    for (uint32 i = 0; i < attachments.size(); i++) {
        const auto data = (VulkanTextureData*) attachments[i]->internal_data();
        view_attachments[i] = data->image->view;
    }
    return view_attachments;
}

} // namespace ENGINE_NAMESPACE