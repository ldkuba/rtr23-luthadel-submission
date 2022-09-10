#include "renderer/vulkan/vulkan_render_pass.hpp"


VulkanRenderPass::VulkanRenderPass(
    VulkanSwapchain* swapchain,
    vk::Device* device,
    vk::AllocationCallbacks* allocator
) : _device(device), _swapchain(swapchain), _allocator(allocator) {
    // Color attachment
    vk::AttachmentDescription color_attachment = _swapchain->get_color_attachment();

    vk::AttachmentReference color_attachment_ref{};
    color_attachment_ref.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    color_attachment_ref.setAttachment(0);

    // Depth attachment
    vk::AttachmentDescription depth_attachment = _swapchain->get_depth_attachment();

    vk::AttachmentReference depth_attachment_ref{};
    depth_attachment_ref.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
    depth_attachment_ref.setAttachment(1);

    // Resolve attachment
    vk::AttachmentDescription color_attachment_resolve = _swapchain->get_color_attachment_resolve();

    vk::AttachmentReference color_attachment_resolve_ref{};
    color_attachment_resolve_ref.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    color_attachment_resolve_ref.setAttachment(2);

    // Subpass
    vk::SubpassDescription subpass{};
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics); // Used for Graphics or Compute
    subpass.setColorAttachmentCount(1);
    subpass.setPColorAttachments(&color_attachment_ref);
    subpass.setPDepthStencilAttachment(&depth_attachment_ref);
    subpass.setPResolveAttachments(&color_attachment_resolve_ref);
    // Preserve and input attachments not used

    // Subpass dependencies
    // Controls image layout transitions between subpasses
    vk::SubpassDependency dependency{};
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL); // Refers to the implicit subpass before current render pass
    dependency.setDstSubpass(0); // Subpass at index 0 (the only one)
    // Operations to wait on before transitioning (Memory access we want)
    dependency.setSrcAccessMask(vk::AccessFlagBits::eNone);
    // Operations that wait for transition (Memory access we want)
    dependency.setDstAccessMask(
        vk::AccessFlagBits::eColorAttachmentWrite |
        vk::AccessFlagBits::eDepthStencilAttachmentWrite);
    // Stages at which the above mentioned operations occur
    dependency.setSrcStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
        vk::PipelineStageFlagBits::eEarlyFragmentTests);
    dependency.setDstStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
        vk::PipelineStageFlagBits::eEarlyFragmentTests);

    // Create render pass
    std::array<vk::AttachmentDescription, 3> attachments = {
        color_attachment,
        depth_attachment,
        color_attachment_resolve
    };
    std::array<vk::SubpassDescription, 1> subpasses = { subpass };
    std::array<vk::SubpassDependency, 1> dependencies = { dependency };

    vk::RenderPassCreateInfo create_info{};
    create_info.setAttachments(attachments);
    create_info.setSubpasses(subpasses);
    create_info.setDependencies(dependencies);

    try {
        handle = _device->createRenderPass(create_info, _allocator);
    } catch (vk::SystemError e) { Logger::fatal(e.what()); }
}
VulkanRenderPass::~VulkanRenderPass() {
    _device->destroyRenderPass(handle, _allocator);
}

void VulkanRenderPass::begin(const vk::CommandBuffer& command_buffer, const vk::Framebuffer& framebuffer) {
    // Default background values of color and depth stencil for rendered area of the render pass
    std::array<float32, 4> clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    std::array<vk::ClearValue, 2>clear_values{};
    clear_values[0].setColor({ clear_color });
    clear_values[1].setDepthStencil({ 1.0f, 0 });


    // Begin render pass
    vk::RenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.setRenderPass(handle);
    render_pass_begin_info.setFramebuffer(framebuffer);
    render_pass_begin_info.setClearValues(clear_values);
    // Area of the surface to render to (here full surface)
    render_pass_begin_info.renderArea.setOffset({ 0, 0 });
    render_pass_begin_info.renderArea.setExtent(_swapchain->extent);

    command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
}

void VulkanRenderPass::end(const vk::CommandBuffer& command_buffer) {
    command_buffer.endRenderPass();
}