#pragma once

#include "vulkan_swapchain.hpp"

class VulkanRenderPass {
  public:
    /// @brief Handle to the vk::RenderPass object
    Property<vk::RenderPass> handle {
        Get { return _handle; }
    };

    VulkanRenderPass(
        const vk::Device* const              device,
        const vk::AllocationCallbacks* const allocator,
        VulkanSwapchain* const               swapchain
    );
    ~VulkanRenderPass();

    /// @brief Begin render pass
    /// @param command_buffer Buffer to store begin command
    /// @param framebuffer Relevant framebuffer
    void begin(
        const vk::CommandBuffer& command_buffer,
        const vk::Framebuffer&   framebuffer
    );
    /// @brief End render pass
    /// @param command_buffer  Buffer to store end command
    void end(const vk::CommandBuffer& command_buffer);

  private:
    const vk::Device*                    _device;
    const vk::AllocationCallbacks* const _allocator;
    VulkanSwapchain* const               _swapchain;

    vk::RenderPass _handle;
};