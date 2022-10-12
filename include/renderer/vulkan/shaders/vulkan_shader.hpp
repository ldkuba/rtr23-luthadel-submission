#pragma once

#include "vulkan_descriptor.hpp"
#include "../vulkan_render_pass.hpp"

class VulkanShader {
  public:

    VulkanShader(
        const VulkanDevice* const            device,
        const vk::AllocationCallbacks* const allocator
    );
    virtual ~VulkanShader();

    /// @brief Select shader for rendering
    /// @param command_buffer Buffer to store bind commands
    virtual void use(const vk::CommandBuffer& command_buffer) {};

  protected:
    const VulkanDevice*                  _device;
    const vk::AllocationCallbacks* const _allocator;

    Property<std::vector<VulkanDescriptor*>> descriptors {
        Get { return _descriptors; }
    };

    vk::Pipeline       _pipeline;
    vk::PipelineLayout _pipeline_layout;

    vk::ShaderModule create_shader_module(const std::vector<byte> code) const;
    void             create_pipeline(
                    const std::vector<vk::PipelineShaderStageCreateInfo>& shader_stages,
                    const vk::PipelineVertexInputStateCreateInfo&         vertex_input_info,
                    const VulkanRenderPass* const                         render_pass,
                    const bool depth_testing_enabled = false,
                    const bool is_wire_frame         = false
                );

    void add_descriptor(
        VulkanDescriptor* const descriptor,
        const uint32            max_sets,
        const bool              can_free = false
    );

  private:
    std::vector<VulkanDescriptor*> _descriptors;
};