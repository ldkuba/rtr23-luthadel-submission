#include "renderer/vulkan/vulkan_backend.hpp"
#include "renderer/vulkan/vulkan_settings.hpp"

// COMMAND BUFFER CODE
void VulkanBackend::create_command_pool() {
    vk::CommandPoolCreateInfo command_pool_info{};
    command_pool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    command_pool_info.setQueueFamilyIndex(_queue_family_indices.graphics_family.value());

    auto result = _device.createCommandPool(&command_pool_info, _allocator, &_command_pool);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create command pool.");
}

void VulkanBackend::create_command_buffers() {
    _command_buffers.resize(VulkanSettings::max_frames_in_flight);

    vk::CommandBufferAllocateInfo alloc_info{};
    alloc_info.setCommandPool(_command_pool);
    alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);
    alloc_info.setCommandBufferCount((uint32) _command_buffers.size());

    auto result = _device.allocateCommandBuffers(&alloc_info, _command_buffers.data());
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to allocate command buffers.");
}


void VulkanBackend::record_command_buffer(vk::CommandBuffer command_buffer, uint32 image_index) {
    // Begin recoding
    vk::CommandBufferBeginInfo begin_info{};
    /* begin_info.setFlags(0); */
    begin_info.setPInheritanceInfo(nullptr);

    command_buffer.begin(begin_info);

    // Begin render pass
    std::array<float, 4> clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    std::array<vk::ClearValue, 2>clear_values{};
    clear_values[0].setColor({ clear_color });
    clear_values[1].setDepthStencil({ 1.0f, 0 });


    vk::RenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.setRenderPass(_render_pass);
    render_pass_begin_info.setFramebuffer(_swapchain_framebuffers[image_index]);
    render_pass_begin_info.renderArea.setOffset({ 0, 0 });
    render_pass_begin_info.renderArea.setExtent(_swapchain_extent);
    render_pass_begin_info.setClearValues(clear_values);

    command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

    // Bind graphics pipeline
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _graphics_pipeline);

    // Bind vertex buffer
    std::vector<vk::Buffer>vertex_buffers = { _vertex_buffer };
    std::vector<vk::DeviceSize> offsets = { 0 };
    command_buffer.bindVertexBuffers(0, vertex_buffers, offsets);

    // Bind index buffer
    command_buffer.bindIndexBuffer(_index_buffer, 0, vk::IndexType::eUint32);

    // Dynamic states
    vk::Viewport viewport{};
    viewport.setX(0.0f);
    viewport.setY(0.0f);
    viewport.setWidth(static_cast<float32>(_swapchain_extent.width));
    viewport.setHeight(static_cast<float32>(_swapchain_extent.height));
    viewport.setMinDepth(0.0f);
    viewport.setMaxDepth(1.0f);

    command_buffer.setViewport(0, 1, &viewport);

    vk::Rect2D scissor{};
    scissor.setOffset({ 0, 0 });
    scissor.setExtent(_swapchain_extent);

    command_buffer.setScissor(0, 1, &scissor);

    // Bind description sets
    command_buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        _pipeline_layout, 0,
        1, &_descriptor_sets[current_frame],
        0, nullptr
    );

    // Draw command
    command_buffer.drawIndexed(static_cast<uint32>(indices.size()), 1, 0, 0, 0);

    // End render pass
    command_buffer.endRenderPass();

    // End recording
    command_buffer.end();
}

vk::CommandBuffer VulkanBackend::begin_single_time_commands() {
    vk::CommandBufferAllocateInfo allocation_info{};
    allocation_info.setLevel(vk::CommandBufferLevel::ePrimary);
    allocation_info.setCommandBufferCount(1);
    allocation_info.setCommandPool(_command_pool);

    vk::CommandBuffer command_buffer;
    command_buffer = _device.allocateCommandBuffers(allocation_info)[0];

    // Begin recording commands
    vk::CommandBufferBeginInfo begin_info{};
    begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    command_buffer.begin(begin_info);

    return command_buffer;
}

void VulkanBackend::end_single_time_commands(vk::CommandBuffer command_buffer) {
    // Finish recording
    command_buffer.end();

    // Execute command buffer
    vk::SubmitInfo submit_info{};
    submit_info.setCommandBufferCount(1);
    submit_info.setPCommandBuffers(&command_buffer);

    _graphics_queue.submit(submit_info);
    _graphics_queue.waitIdle();

    // Free temp command buffer
    _device.freeCommandBuffers(_command_pool, 1, &command_buffer);
}