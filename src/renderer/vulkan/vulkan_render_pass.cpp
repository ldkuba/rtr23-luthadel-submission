#include "renderer/vulkan/vulkan_render_pass.hpp"

#include "renderer/vulkan/vulkan_settings.hpp"

// Constructor & Destructor
VulkanRenderPass::VulkanRenderPass(
    const vk::Device* const              device,
    const vk::AllocationCallbacks* const allocator,
    VulkanSwapchain* const               swapchain,
    const std::array<float32, 4>         clear_color,
    const RenderPassPosition             position,
    const uint8                          clear_flags,
    const bool                           multisampling
)
    : _device(device), _swapchain(swapchain), _allocator(allocator),
      _clear_color(clear_color), _clear_flags(clear_flags),
      _multisampling_enabled(multisampling) {

    Logger::trace(RENDERER_VULKAN_LOG, "Creating render pass.");

    // Compute state
    auto sample_count = (multisampling) ? _swapchain->msaa_samples
                                        : vk::SampleCountFlagBits::e1;
    _has_depth        = clear_flags & RenderPassClearFlags::Depth;

    // === Get all attachment descriptions ===
    std::vector<vk::AttachmentDescription> attachments = {};

    // Color attachment
    auto color_load_op = ((clear_flags & RenderPassClearFlags::Color) != 0)
                             ? vk::AttachmentLoadOp::eClear
                             : vk::AttachmentLoadOp::eLoad;
    auto color_initial_layout = ((position == RenderPassPosition::Beginning ||
                                  position == RenderPassPosition::Only))
                                    ? vk::ImageLayout::eUndefined
                                    : vk::ImageLayout::eColorAttachmentOptimal;
    auto color_final_layout   = ((position == RenderPassPosition::End ||
                                position == RenderPassPosition::Only) &&
                               !multisampling)
                                    ? vk::ImageLayout::ePresentSrcKHR
                                    : vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentDescription color_attachment {};
    color_attachment.setFormat(_swapchain->get_color_attachment_format());
    color_attachment.setSamples(sample_count);
    color_attachment.setLoadOp(color_load_op);
    color_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    color_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    color_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    color_attachment.setInitialLayout(color_initial_layout);
    color_attachment.setFinalLayout(color_final_layout);

    vk::AttachmentReference* color_attachment_ref = new vk::AttachmentReference(
        attachments.size(), vk::ImageLayout::eColorAttachmentOptimal
    );
    attachments.push_back(color_attachment);

    // Depth attachment
    vk::AttachmentReference* depth_attachment_ref = nullptr;
    if (_has_depth) {
        vk::AttachmentDescription depth_attachment {};
        depth_attachment.setFormat(_swapchain->get_depth_attachment_format());
        depth_attachment.setSamples(sample_count);
        depth_attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        depth_attachment.setStoreOp(vk::AttachmentStoreOp::eDontCare);
        depth_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        depth_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        depth_attachment.setInitialLayout(vk::ImageLayout::eUndefined);
        depth_attachment.setFinalLayout(
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        );

        depth_attachment_ref = new vk::AttachmentReference(
            attachments.size(), vk::ImageLayout::eDepthStencilAttachmentOptimal
        );
        attachments.push_back(depth_attachment);
    }

    // Resolve attachment
    vk::AttachmentReference* color_attachment_resolve_ref = nullptr;
    if (multisampling) {
        vk::AttachmentDescription resolve_attachment {};
        resolve_attachment.setFormat(_swapchain->get_color_attachment_format());
        resolve_attachment.setSamples(vk::SampleCountFlagBits::e1);
        resolve_attachment.setLoadOp(vk::AttachmentLoadOp::eDontCare);
        resolve_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
        resolve_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        resolve_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        resolve_attachment.setInitialLayout(vk::ImageLayout::eUndefined);
        resolve_attachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

        color_attachment_resolve_ref = new vk::AttachmentReference(
            attachments.size(), vk::ImageLayout::eColorAttachmentOptimal
        );
        attachments.push_back(resolve_attachment);
    }

    // === Subpass ===
    vk::SubpassDescription subpass {};
    // Used for Graphics or Compute
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpass.setColorAttachmentCount(1);
    subpass.setPColorAttachments(color_attachment_ref);
    subpass.setPDepthStencilAttachment(depth_attachment_ref);
    subpass.setPResolveAttachments(color_attachment_resolve_ref);
    // Preserve and input attachments not used

    // === Subpass dependencies ===
    // Controls image layout transitions between subpasses
    vk::SubpassDependency dependency {};
    // Refers to the implicit subpass before current render pass
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
    // Subpass at index 0 (the only one)
    dependency.setDstSubpass(0);
    // Operations to wait on before transitioning (Memory access we want)
    dependency.setSrcAccessMask(vk::AccessFlagBits::eNone);
    // Operations that wait for transition (Memory access we want)
    dependency.setDstAccessMask(
        vk::AccessFlagBits::eColorAttachmentWrite |
        vk::AccessFlagBits::eDepthStencilAttachmentWrite
    );
    // Stages at which the above mentioned operations occur
    dependency.setSrcStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
        vk::PipelineStageFlagBits::eEarlyFragmentTests
    );
    dependency.setDstStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
        vk::PipelineStageFlagBits::eEarlyFragmentTests
    );

    // === Create render pass ===
    std::array<vk::SubpassDescription, 1> subpasses    = { subpass };
    std::array<vk::SubpassDependency, 1>  dependencies = { dependency };

    vk::RenderPassCreateInfo create_info {};
    create_info.setAttachments(attachments);
    create_info.setSubpasses(subpasses);
    create_info.setDependencies(dependencies);

    try {
        _handle = _device->createRenderPass(create_info, _allocator);
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    // === Create register framebuffers ===
    _framebuffer_set_index =
        _swapchain->create_framebuffers(this, multisampling, _has_depth);

    Logger::trace(RENDERER_VULKAN_LOG, "Render pass created.");
}
VulkanRenderPass::~VulkanRenderPass() {
    _device->destroyRenderPass(handle, _allocator);
    Logger::trace(RENDERER_VULKAN_LOG, "Render pass destroyed.");
}

// ///////////////////////////////// //
// VULKAN RENDER PASS PUBLIC METHODS //
// ///////////////////////////////// //

void VulkanRenderPass::begin(const vk::CommandBuffer& command_buffer) {
    // Default background values of color and depth stencil for rendered area of
    // the render pass
    std::vector<vk::ClearValue> clear_values { 1 };
    if (_clear_flags & RenderPassClearFlags::Color)
        clear_values[0].setColor({ _clear_color });
    if (_has_depth) {
        clear_values.push_back({});
        if (_clear_flags & RenderPassClearFlags::Depth)
            clear_values[1].depthStencil.setDepth(1.0f);
        if (_clear_flags & RenderPassClearFlags::Stencil)
            clear_values[1].depthStencil.setStencil(0);
    }

    // Get relevant framebuffer
    auto framebuffer =
        _swapchain->get_currently_used_framebuffer(_framebuffer_set_index);

    // Begin render pass
    vk::RenderPassBeginInfo render_pass_begin_info {};
    render_pass_begin_info.setRenderPass(handle);
    render_pass_begin_info.setFramebuffer(framebuffer->handle());
    render_pass_begin_info.setClearValues(clear_values);
    // Area of the surface to render to (here full surface)
    render_pass_begin_info.renderArea.setOffset({ 0, 0 });
    render_pass_begin_info.renderArea.setExtent(_swapchain->extent);

    command_buffer.beginRenderPass(
        render_pass_begin_info, vk::SubpassContents::eInline
    );
}

void VulkanRenderPass::end(const vk::CommandBuffer& command_buffer) {
    command_buffer.endRenderPass();
}
