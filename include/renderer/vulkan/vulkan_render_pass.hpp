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
        GET {
            if (!_initialized) {
                Logger::warning(
                    RENDERER_VULKAN_LOG,
                    "Vulkan render pass handle used before initialization. "
                    "Orphan initialization preformed inplace ['",
                    _name,
                    "']."
                );
                initialize();
            }
            return _handle;
        }
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
     * @param id Unique vulkan render pass identifier
     * @param config Render pass configurations
     * @param device Vulkan device reference
     * @param allocator Allocation callback used
     * @param command_buffer Buffer on which commands will be issued
     * @param swapchain Swapchain reference
     */
    VulkanRenderPass(
        const uint16                         id,
        const Config&                        config,
        const vk::Device* const              device,
        const vk::AllocationCallbacks* const allocator,
        const VulkanCommandBuffer* const     command_buffer,
        VulkanSwapchain* const               swapchain
    );
    ~VulkanRenderPass() override;

    /// @brief Begin render pass
    /// @param render_target Targeted to be used
    void begin(RenderTarget* const render_target) override;
    /// @brief End render pass
    void end() override;

    void add_window_as_render_target() override;
    void clear_render_targets() override;

    uint8 get_color_index() override;
    uint8 get_depth_index() override;
    uint8 get_resolve_index() override;

  protected:
    void initialize() override;
    void initialize_render_targets() override;

    RenderTarget* create_render_target(
        uint32 width, uint32 height, const Vector<Texture*>& attachments
    );

  private:
    const vk::Device*                    _device;
    const vk::AllocationCallbacks* const _allocator;
    const VulkanCommandBuffer* const     _command_buffer;
    VulkanSwapchain* const               _swapchain;

    // Internal data
    vk::RenderPass _handle;
    float32        _depth;
    float32        _stencil;
    bool           _has_prev;
    bool           _has_next;

    // Internal state
    bool _initialized = false;

    // Format
    vk::Format _color_format;
    vk::Format _depth_format;
    vk::Format _resolve_format;

    Vector<vk::ClearValue> _clear_values { 1 };

    vk::AttachmentDescription get_color_attachment();
    vk::AttachmentDescription get_depth_attachment();
    vk::AttachmentDescription get_resolve_attachment();
};

} // namespace ENGINE_NAMESPACE