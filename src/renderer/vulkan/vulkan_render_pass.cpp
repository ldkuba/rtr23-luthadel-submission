#include "renderer/vulkan/vulkan_render_pass.hpp"

#include "renderer/vulkan/vulkan_settings.hpp"

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
VulkanRenderPass::VulkanRenderPass(
    const uint16                         id,
    const Config&                        config,
    const vk::Device* const              device,
    const vk::AllocationCallbacks* const allocator,
    const VulkanCommandBuffer* const     command_buffer,
    VulkanSwapchain* const               swapchain
)
    : RenderPass(id, config), _device(device), _command_buffer(command_buffer),
      _swapchain(swapchain), _allocator(allocator),
      // TODO: Configurable
      _depth(1.0), _stencil(0) {

    Logger::trace(RENDERER_VULKAN_LOG, "Creating render pass.");

    // Compute state
    const auto sample_count = (_multisampling_enabled)
                                  ? _swapchain->msaa_samples
                                  : vk::SampleCountFlagBits::e1;
    const auto clear_depth  = _clear_flags & ClearFlags::Depth;

    // Compute position
    _has_prev = !config.prev_name.empty();
    _has_next = !config.next_name.empty();

    // === Get all attachment descriptions ===
    Vector<vk::AttachmentDescription> attachments {};

    // Color attachment
    auto color_load_op        = ((_clear_flags & ClearFlags::Color) != 0)
                                    ? vk::AttachmentLoadOp::eClear
                                    : vk::AttachmentLoadOp::eLoad;
    auto color_initial_layout = (_has_prev)
                                    ? vk::ImageLayout::eColorAttachmentOptimal
                                    : vk::ImageLayout::eUndefined;
    auto color_final_layout   = (_has_next || _multisampling_enabled)
                                    ? vk::ImageLayout::eColorAttachmentOptimal
                                    : vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentDescription color_attachment {};
    color_attachment.setFormat(_swapchain->get_color_attachment_format()
    ); // TODO: Configure
    color_attachment.setSamples(sample_count);
    color_attachment.setLoadOp(color_load_op);
    color_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    color_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    color_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    color_attachment.setInitialLayout(color_initial_layout);
    color_attachment.setFinalLayout(color_final_layout);

    vk::AttachmentReference* color_attachment_ref =
        new (MemoryTag::Temp) vk::AttachmentReference(
            attachments.size(), vk::ImageLayout::eColorAttachmentOptimal
        );
    attachments.push_back(color_attachment);

    // Depth attachment
    vk::AttachmentReference* depth_attachment_ref = nullptr;
    if (_depth_testing_enabled) {
        vk::AttachmentDescription depth_attachment {};
        depth_attachment.setFormat(_swapchain->get_depth_attachment_format());
        depth_attachment.setSamples(sample_count);
        depth_attachment.setLoadOp(
            (!_has_prev)    ? vk::AttachmentLoadOp::eDontCare
            : (clear_depth) ? vk::AttachmentLoadOp::eClear
                            : vk::AttachmentLoadOp::eLoad
        );
        depth_attachment.setStoreOp(vk::AttachmentStoreOp::eDontCare);
        depth_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        depth_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        depth_attachment.setInitialLayout(vk::ImageLayout::eUndefined);
        depth_attachment.setFinalLayout(
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        );

        depth_attachment_ref = new (MemoryTag::Temp) vk::AttachmentReference(
            attachments.size(), vk::ImageLayout::eDepthStencilAttachmentOptimal
        );
        attachments.push_back(depth_attachment);
    }

    // Resolve attachment
    vk::AttachmentReference* color_attachment_resolve_ref = nullptr;
    if (_multisampling_enabled) {
        auto resolve_final_layout =
            (_has_next) ? vk::ImageLayout::eColorAttachmentOptimal
                        : vk::ImageLayout::ePresentSrcKHR;
        vk::AttachmentDescription resolve_attachment {};
        resolve_attachment.setFormat(_swapchain->get_color_attachment_format());
        resolve_attachment.setSamples(vk::SampleCountFlagBits::e1);
        resolve_attachment.setLoadOp(vk::AttachmentLoadOp::eDontCare);
        resolve_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
        resolve_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        resolve_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        resolve_attachment.setInitialLayout(vk::ImageLayout::eUndefined);
        resolve_attachment.setFinalLayout(resolve_final_layout);

        color_attachment_resolve_ref =
            new (MemoryTag::Temp) vk::AttachmentReference(
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

    // === Subpass dependencies === TODO: MAKE CONFIGURABLE
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
    std::array<vk::SubpassDescription, 1> subpasses { subpass };
    std::array<vk::SubpassDependency, 1>  dependencies { dependency };

    vk::RenderPassCreateInfo create_info {};
    create_info.setAttachments(attachments);
    create_info.setSubpasses(subpasses);
    create_info.setDependencies(dependencies);

    try {
        _handle = _device->createRenderPass(create_info, _allocator);
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    // === Cleanup ===
    if (color_attachment_ref) delete color_attachment_ref;
    if (depth_attachment_ref) delete depth_attachment_ref;
    if (color_attachment_resolve_ref) delete color_attachment_resolve_ref;

    // === Compute default clear values ===
    // Default background values of color and depth stencil for rendered area of
    // the render pass
    if (_clear_flags & ClearFlags::Color)
        _clear_values[0].setColor(
            { _clear_color.x, _clear_color.y, _clear_color.z, _clear_color.w }
        );
    if (clear_depth) {
        _clear_values.push_back({});
        if (_clear_flags & ClearFlags::Depth)
            _clear_values[1].depthStencil.setDepth(_depth);
        if (_clear_flags & ClearFlags::Stencil)
            _clear_values[1].depthStencil.setStencil(_stencil);
    }

    Logger::trace(RENDERER_VULKAN_LOG, "Render pass created.");
}
VulkanRenderPass::~VulkanRenderPass() {
    clear_render_targets();
    _device->destroyRenderPass(handle, _allocator);
    Logger::trace(RENDERER_VULKAN_LOG, "Render pass destroyed.");
}

// ///////////////////////////////// //
// VULKAN RENDER PASS PUBLIC METHODS //
// ///////////////////////////////// //

void VulkanRenderPass::begin(RenderTarget* const render_target) {
    // Get relevant framebuffer
    const auto framebuffer =
        dynamic_cast<VulkanFramebuffer*>(render_target->framebuffer());

    // Begin render pass
    vk::RenderPassBeginInfo render_pass_begin_info {};
    render_pass_begin_info.setRenderPass(_handle);
    render_pass_begin_info.setFramebuffer(framebuffer->handle());
    render_pass_begin_info.setClearValues(_clear_values);
    // Area of the surface to render to (here full surface)
    render_pass_begin_info.renderArea.setOffset({ (int32) _render_offset.x,
                                                  (int32) _render_offset.y });
    render_pass_begin_info.renderArea.setExtent({ render_target->width(),
                                                  render_target->height() });

    _command_buffer->handle->beginRenderPass(
        render_pass_begin_info, vk::SubpassContents::eInline
    );
}

void VulkanRenderPass::end() { _command_buffer->handle->endRenderPass(); }

void VulkanRenderPass::add_window_as_render_target() {
    const auto count = _swapchain->get_render_texture_count();
    for (uint8 i = 0; i < count; i++) {
        // Gather render target attachments
        Vector<Texture*> attachments {};

        if (_multisampling_enabled)
            attachments.push_back(_swapchain->get_color_texture());
        else attachments.push_back(_swapchain->get_render_texture(i));
        if (_depth_testing_enabled)
            attachments.push_back(_swapchain->get_depth_texture());
        if (_multisampling_enabled)
            attachments.push_back(_swapchain->get_render_texture(i));

        // Add render target
        add_render_target(
            _swapchain->extent().width, _swapchain->extent().height, attachments
        );
    }
}
void VulkanRenderPass::add_render_target(
    const uint32 width, const uint32 height, const Vector<Texture*>& attachments
) {
    // Scan for image views needed for framebuffer
    Vector<vk::ImageView> view_attachments { attachments.size() };
    for (uint32 i = 0; i < attachments.size(); i++) {
        const auto v_texture = (VulkanTexture*) attachments[i];
        view_attachments[i]  = v_texture->image()->view;
    }

    // Create render target appropriate framebuffer
    const auto framebuffer = new (MemoryTag::GPUBuffer) VulkanFramebuffer(
        _device, _allocator, this, width, height, view_attachments
    );

    // Store to render target
    const auto target = new (MemoryTag::GPUBuffer)
        RenderTarget(attachments, framebuffer, width, height);

    // Sync to window resize
    // TODO: Use `_sync_to_window_resize` bool
    _swapchain->recreate_event.subscribe(target, &RenderTarget::resize);

    // Add this target
    _render_targets.push_back(target);
}

void VulkanRenderPass::clear_render_targets() {
    for (const auto& target : _render_targets) {
        const auto vk_framebuffer =
            dynamic_cast<VulkanFramebuffer*>(target->framebuffer());
        delete vk_framebuffer;
        delete target;
    }
}

} // namespace ENGINE_NAMESPACE