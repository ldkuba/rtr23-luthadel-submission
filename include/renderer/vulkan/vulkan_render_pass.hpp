#pragma once

#include "vulkan_swapchain.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Types of clearing available to the render pass. Combinations will
 * enable multiple clearing types.
 */
enum RenderPassClearFlags : uint8 {
    None    = 0x0,
    Color   = 0x1,
    Depth   = 0x2,
    Stencil = 0x4
};

/**
 * @brief Relative position of the render pass in execution sequence. Only is
 * reserved if render pass is solitary.
 */
enum class RenderPassPosition { Beginning, Middle, End, Only };

/**
 * @brief Vulkan implementation of a render pass.
 */
class VulkanRenderPass {
  public:
    /// @brief Handle to the vk::RenderPass object
    Property<vk::RenderPass> handle {
        GET { return _handle; }
    };
    /// @brief Number of sampled used for Multisample anti-aliasing
    Property<vk::SampleCountFlagBits> sample_count {
        GET {
            if (_multisampling_enabled) return _swapchain->msaa_samples();
            static auto one_sample = vk::SampleCountFlagBits::e1;
            return one_sample;
        }
    };
    /// @brief Returns true if depth testing is enabled for this render pass
    Property<bool> depth_testing {
        GET { return _depth_testing_enabled; }
    };

    /**
     * @brief Construct a new Vulkan Render Pass object
     *
     * @param device Vulkan device reference
     * @param allocator Allocation callback used
     * @param swapchain Swapchain reference
     * @param clear_color Normalized color vector to which swapchain image is
     * set before this pass
     * @param position Relative position in execution sequence. If this is the
     * only render pass preformed this value should be set to "Only". If this
     * pass is first of many this value should be "Beginning", if its the last
     * of many it should be "End". Otherwise "Middle" is required.
     * @param clear_flags Binary combination of RenderPassClearFlags used for
     * clearing (for example "Color", "Depth", ...).
     * @param multisampling True if this render pass utilizes multisampling
     * @param depth_testing True if this render pass utilizes depth testing
     */
    VulkanRenderPass(
        const vk::Device* const              device,
        const vk::AllocationCallbacks* const allocator,
        VulkanSwapchain* const               swapchain,
        const std::array<float32, 4>         clear_color,
        const RenderPassPosition             position,
        const uint8                          clear_flags,
        const bool                           multisampling,
        const bool                           depth_testing
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
    bool                   _depth_testing_enabled;
    Vector<vk::ClearValue> _clear_values { 1 };
};

} // namespace ENGINE_NAMESPACE