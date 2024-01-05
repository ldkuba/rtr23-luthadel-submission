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
      _depth(1.0), _stencil(0) {}
VulkanRenderPass::~VulkanRenderPass() {
    clear_render_targets();

    if (_handle) _device->destroyRenderPass(_handle, _allocator);
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
            attachments.push_back(_swapchain->get_ms_color_texture());
        else attachments.push_back(_swapchain->get_render_texture(i));
        if (_depth_testing_enabled) {
            if (_multisampling_enabled)
                attachments.push_back(_swapchain->get_ms_depth_texture());
            else attachments.push_back(_swapchain->get_depth_texture());
        }
        if (_multisampling_enabled)
            attachments.push_back(_swapchain->get_render_texture(i));

        // Add render target
        add_render_target({ _swapchain->extent().width,
                            _swapchain->extent().height,
                            attachments });
    }
}

void VulkanRenderPass::clear_render_targets() {
    for (const auto& target : _render_targets) {
        const auto vk_framebuffer =
            dynamic_cast<VulkanFramebuffer*>(target->framebuffer());
        del(vk_framebuffer);
        del(target);
    }
}

uint8 VulkanRenderPass::get_color_index() { return _color_output ? 0 : -1; }
uint8 VulkanRenderPass::get_depth_index() {
    return _depth_testing_enabled ? (_color_output ? 1 : 0) : -1;
}
uint8 VulkanRenderPass::get_resolve_index() {
    return _multisampling_enabled ? (_depth_testing_enabled ? 2 : 1) : -1;
}

// //////////////////////////////////// //
// VULKAN RENDER PASS PROTECTED METHODS //
// //////////////////////////////////// //

// Helper for initialization
VulkanTexture* get_texture(
    const Vector<Texture*>& attachments, const uint32 index
) {
    if (index >= attachments.size()) return nullptr;
    const auto texture = attachments[index];
    if (texture->is_render_target()) {
        const auto pack = static_cast<PackedTexture*>(texture);
        return static_cast<VulkanTexture*>(pack->get_at(0));
    }
    return static_cast<VulkanTexture*>(texture);
}

void VulkanRenderPass::initialize() {
    if (_initialized) {
        Logger::debug(
            RENDERER_VULKAN_LOG,
            "Render pass '",
            _name,
            "' will be reinitialized. [Make sure no errors can come from this]"
        );
        if (_handle) _device->destroyRenderPass(_handle, _allocator);
    }
    Logger::trace(
        RENDERER_VULKAN_LOG, "Initializing '", _name, "' render pass."
    );

    // Update formats from render targets (if any)
    if (_render_target_configs.size() != 0) {
        const auto attachments = _render_target_configs[0].attachments;

        if (_color_output) {
            const auto ci = get_color_index();
            const auto ct = get_texture(attachments, ci);
            if (ct) _color_format = ct->get_vulkan_format();
        }
        if (_depth_testing_enabled) {
            const auto di = get_depth_index();
            const auto dt = get_texture(attachments, di);
            if (dt) _depth_format = dt->get_vulkan_format();
        }
        if (_multisampling_enabled) {
            const auto ri = get_resolve_index();
            const auto rt = get_texture(attachments, ri);
            if (rt) _resolve_format = rt->get_vulkan_format();
        }
    }

    // Compute state
    const auto sample_count = (_multisampling_enabled)
                                  ? _swapchain->msaa_samples
                                  : vk::SampleCountFlagBits::e1;

    // Compute position
    _has_prev = !_prev.empty();
    _has_next = !_next.empty();

    // === Get all attachment descriptions ===
    Vector<vk::AttachmentDescription> attachments {};

    // Declare reference pointers
    vk::AttachmentReference* color_attachment_ref   = nullptr;
    vk::AttachmentReference* depth_attachment_ref   = nullptr;
    vk::AttachmentReference* resolve_attachment_ref = nullptr;

    if (_color_output) {
        color_attachment_ref = new (MemoryTag::Temp) vk::AttachmentReference(
            attachments.size(), vk::ImageLayout::eColorAttachmentOptimal
        );
        attachments.push_back(get_color_attachment());
    }
    if (_depth_testing_enabled) {
        depth_attachment_ref = new (MemoryTag::Temp) vk::AttachmentReference(
            attachments.size(), vk::ImageLayout::eDepthStencilAttachmentOptimal
        );
        attachments.push_back(get_depth_attachment());
    }
    if (_color_output && _multisampling_enabled) {
        resolve_attachment_ref = new (MemoryTag::Temp) vk::AttachmentReference(
            attachments.size(), vk::ImageLayout::eColorAttachmentOptimal
        );
        attachments.push_back(get_resolve_attachment());
    }

    // === Subpass ===
    vk::SubpassDescription subpass {};
    // Used for Graphics or Compute
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpass.setColorAttachmentCount(_color_output ? 1 : 0);
    subpass.setPColorAttachments(color_attachment_ref);
    subpass.setPDepthStencilAttachment(depth_attachment_ref);
    subpass.setPResolveAttachments(resolve_attachment_ref);
    // Preserve and input attachments not used

    // === Subpass dependencies === TODO: MAKE CONFIGURABLE
    auto destination_access_mask = vk::AccessFlagBits::eColorAttachmentWrite |
                                   vk::AccessFlagBits::eColorAttachmentRead;
    // if (_depth_testing_enabled) TODO: Maybe we add this... we will see
    //     destination_access_mask |=
    //         vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    // Controls image layout transitions between subpasses
    vk::SubpassDependency dependency {};
    // Refers to the implicit subpass before current render pass
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
    // Subpass at index 0 (the only one)
    dependency.setDstSubpass(0);
    // Operations to wait on before transitioning (Memory access we want)
    dependency.setSrcAccessMask(vk::AccessFlagBits::eNone);
    // Operations that wait for transition (Memory access we want)
    dependency.setDstAccessMask(destination_access_mask);
    // Stages at which the above mentioned operations occur
    dependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput
    );
    dependency.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput
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
    if (resolve_attachment_ref) del(resolve_attachment_ref);
    if (depth_attachment_ref) del(depth_attachment_ref);
    if (color_attachment_ref) del(color_attachment_ref);

    // === Compute default clear values ===
    // Default background values of color and depth stencil for rendered area of
    // the render pass
    if (_clear_flags & ClearFlags::Color)
        _clear_values[0].setColor(
            { _clear_color.x, _clear_color.y, _clear_color.z, _clear_color.w }
        );
    if (_clear_flags & ClearFlags::Depth) {
        _clear_values.push_back({});
        if (_clear_flags & ClearFlags::Depth)
            _clear_values[1].depthStencil.setDepth(_depth);
        if (_clear_flags & ClearFlags::Stencil)
            _clear_values[1].depthStencil.setStencil(_stencil);
    }

    Logger::trace(
        RENDERER_VULKAN_LOG,
        "Render pass '",
        _name,
        "' initialized [handle: ",
        (uint64) &_handle,
        "]."
    );
    _initialized = true;
}

void VulkanRenderPass::initialize_render_targets() {
    for (const auto& config : _render_target_configs) {
        // Check how many frame buffers need to be initialized
        if (config.one_per_frame_in_flight) {
            // Scan for image views needed for framebuffer
            for (uint8 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
                // For attachment list
                Vector<Texture*> attachments {};
                for (const auto& attachment : config.attachments) {
                    if (attachment->is_render_target()) {
                        const auto pack =
                            static_cast<PackedTexture*>(attachment);
                        attachments.push_back(pack->get_at(i));
                    } else attachments.push_back(attachment);
                }

                // Create target under these settings
                const auto target = create_render_target(
                    config.width, config.height, attachments
                );

                // Add this target
                _render_targets.push_back(target);
            }
        } else {
            // Create target under these settings
            const auto target = create_render_target(
                config.width, config.height, config.attachments
            );

            // Add this target
            _render_targets.push_back(target);
        }
    }
}

// Helper
RenderTarget* VulkanRenderPass::create_render_target(
    uint32 width, uint32 height, const Vector<Texture*>& attachments
) {
    // Create view attachments for framebuffer initialization
    Vector<vk::ImageView> view_attachments { attachments.size() };
    for (uint32 j = 0; j < attachments.size(); j++) {
        const auto texture   = attachments[j];
        const auto v_texture = (VulkanTexture*) attachments[j];
        view_attachments[j]  = v_texture->image()->view;
    }

    // Create render target appropriate framebuffer
    const auto framebuffer = new (MemoryTag::GPUBuffer) VulkanFramebuffer(
        _device, _allocator, this, width, height, view_attachments
    );

    // Store to render targets
    const auto target = new (MemoryTag::GPUBuffer)
        RenderTarget(attachments, framebuffer, width, height);

    // Sync to window resize
    // TODO: Use `_sync_to_window_resize` bool
    _swapchain->recreate_event.subscribe(target, &RenderTarget::resize);

    return target;
}

// ////////////////////////////////// //
// VULKAN RENDER PASS PRIVATE METHODS //
// ////////////////////////////////// //

vk::AttachmentDescription VulkanRenderPass::get_color_attachment() {
    auto color_load_op        = ((_clear_flags & ClearFlags::Color) != 0)
                                    ? vk::AttachmentLoadOp::eClear
                                : (_init_color) ? vk::AttachmentLoadOp::eDontCare
                                                : vk::AttachmentLoadOp::eLoad;
    auto color_initial_layout = (_init_color)
                                    ? vk::ImageLayout::eUndefined
                                    : vk::ImageLayout::eColorAttachmentOptimal;
    auto color_final_layout   = (_has_next || _multisampling_enabled)
                                    ? vk::ImageLayout::eColorAttachmentOptimal
                                    : vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentDescription color_attachment {};
    color_attachment.setFormat(_color_format);
    color_attachment.setSamples(sample_count);
    color_attachment.setLoadOp(color_load_op);
    color_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    color_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    color_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    color_attachment.setInitialLayout(color_initial_layout);
    color_attachment.setFinalLayout(color_final_layout);

    return color_attachment;
}

vk::AttachmentDescription VulkanRenderPass::get_depth_attachment() {
    auto depth_load_op = ((_clear_flags & ClearFlags::Depth) != 0)
                             ? vk::AttachmentLoadOp::eClear
                         : (_init_depth) ? vk::AttachmentLoadOp::eDontCare
                                         : vk::AttachmentLoadOp::eLoad;
    auto depth_initial_layout =
        (_init_depth) ? vk::ImageLayout::eUndefined
                      : vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentDescription depth_attachment {};
    depth_attachment.setFormat(_depth_format);
    depth_attachment.setSamples(sample_count);
    depth_attachment.setLoadOp(depth_load_op);
    depth_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    depth_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    depth_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    depth_attachment.setInitialLayout(depth_initial_layout);
    depth_attachment.setFinalLayout(
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    );

    return depth_attachment;
}

vk::AttachmentDescription VulkanRenderPass::get_resolve_attachment() {
    auto res_load_op        = ((_clear_flags & ClearFlags::Resolve) != 0)
                                  ? vk::AttachmentLoadOp::eClear
                              : (_init_resolve) ? vk::AttachmentLoadOp::eDontCare
                                                : vk::AttachmentLoadOp::eLoad;
    auto res_initial_layout = (_init_resolve)
                                  ? vk::ImageLayout::eUndefined
                                  : vk::ImageLayout::eColorAttachmentOptimal;
    auto res_final_layout   = (_has_next)
                                  ? vk::ImageLayout::eColorAttachmentOptimal
                                  : vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentDescription resolve_attachment {};
    resolve_attachment.setFormat(_resolve_format);
    resolve_attachment.setSamples(vk::SampleCountFlagBits::e1);
    resolve_attachment.setLoadOp(res_load_op);
    resolve_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    resolve_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    resolve_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    resolve_attachment.setInitialLayout(res_initial_layout);
    resolve_attachment.setFinalLayout(res_final_layout);

    return resolve_attachment;
}

} // namespace ENGINE_NAMESPACE