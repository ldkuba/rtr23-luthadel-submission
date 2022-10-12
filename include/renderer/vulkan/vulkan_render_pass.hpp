#pragma once

#include "vulkan_swapchain.hpp"

enum RenderPassClearFlags : uint8 {
    None    = 0x0,
    Color   = 0x1,
    Depth   = 0x2,
    Stencil = 0x4
};
enum class RenderPassPosition { Beginning, Middle, End, Only };

class VulkanRenderPass {
  public:
    /// @brief Handle to the vk::RenderPass object
    Property<vk::RenderPass> handle {
        Get { return _handle; }
    };
    /// @brief Number of sampled used for Multisample anti-aliasing
    Property<vk::SampleCountFlagBits> sample_count {
        Get {
            if (_multisampling_enabled) return _swapchain->msaa_samples();
            static auto one_sample = vk::SampleCountFlagBits::e1;
            return one_sample;
        }
    };

    VulkanRenderPass(
        const vk::Device* const              device,
        const vk::AllocationCallbacks* const allocator,
        VulkanSwapchain* const               swapchain,
        const std::array<float32, 4>         clear_color,
        const RenderPassPosition             position,
        const uint8                          clear_flags,
        const bool                           multisampling
    );
    ~VulkanRenderPass();

    /// @brief Begin render pass
    /// @param command_buffer Buffer to store begin command
    /// @param framebuffer Relevant framebuffer
    void begin(const vk::CommandBuffer& command_buffer);
    /// @brief End render pass
    /// @param command_buffer  Buffer to store end command
    void end(const vk::CommandBuffer& command_buffer);

  private:
    const vk::Device*                    _device;
    const vk::AllocationCallbacks* const _allocator;
    VulkanSwapchain* const               _swapchain;

    vk::RenderPass _handle;
    uint32         _framebuffer_set_index;

    bool                   _multisampling_enabled;
    bool                   _has_depth;
    std::array<float32, 4> _clear_color;
    uint8                  _clear_flags;
};