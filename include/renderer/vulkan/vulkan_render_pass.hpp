#pragma once

#include "vulkan_swapchain.hpp"
#include "renderer/render_pass.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Vulkan implementation of a render pass.
 */
class VulkanRenderPass : public RenderPass {
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
     * @param id Unique vulkan render pass identifier
     * @param config Render pass configurations
     */
    VulkanRenderPass(
        const vk::Device* const              device,
        const vk::AllocationCallbacks* const allocator,
        VulkanSwapchain* const               swapchain,
        const uint16                         id,
        const Config&                        config
    );
    ~VulkanRenderPass();

    /// @brief Begin render pass
    /// @param render_target Targeted to be used
    /// @param command_buffer Buffer to store begin command
    void begin(
        RenderTarget* const      render_target,
        const vk::CommandBuffer& command_buffer
    );
    /// @brief End render pass
    /// @param command_buffer  Buffer to store end command
    void end(const vk::CommandBuffer& command_buffer);

  private:
    const vk::Device*                    _device;
    const vk::AllocationCallbacks* const _allocator;
    VulkanSwapchain* const               _swapchain;

    // Internal data
    vk::RenderPass _handle;
    float32        _depth;
    float32        _stencil;
    bool           _has_prev;
    bool           _has_next;

    Vector<vk::ClearValue> _clear_values { 1 };
};

} // namespace ENGINE_NAMESPACE