#pragma once

#include "vulkan_swapchain.hpp"

class VulkanRenderPass {
private:
    VulkanSwapchain* _swapchain;
    vk::Device* _device;
    vk::AllocationCallbacks* _allocator;

public:
    vk::RenderPass handle;

    VulkanRenderPass(
        VulkanSwapchain* swapchain,
        vk::Device* device,
        vk::AllocationCallbacks* allocator
    );
    ~VulkanRenderPass();

    void begin(const vk::CommandBuffer& command_buffer, const vk::Framebuffer& framebuffer);
    void end(const vk::CommandBuffer& command_buffer);
};
