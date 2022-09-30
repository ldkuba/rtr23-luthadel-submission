#pragma once

#include "../vulkan_buffer.hpp"

class VulkanShader {
public:

    VulkanShader(
        const VulkanDevice* const device,
        const vk::AllocationCallbacks* const allocator
    );
    ~VulkanShader();

    /// @brief Select shader for rendering
    /// @param command_buffer Buffer to store bind commands
    virtual void use(const vk::CommandBuffer& command_buffer) {};

protected:
    const VulkanDevice* _device;
    const vk::AllocationCallbacks* const _allocator;

    vk::Pipeline _pipeline;
    vk::PipelineLayout _pipeline_layout;

    vk::ShaderModule create_shader_module(const std::vector<byte> code) const;
    void create_pipeline(
        const std::vector<vk::PipelineShaderStageCreateInfo>& shader_stages,
        const vk::PipelineVertexInputStateCreateInfo& vertex_input_info,
        const vk::PipelineLayoutCreateInfo& layout_info,
        const vk::RenderPass render_pass,
        const vk::SampleCountFlagBits number_of_msaa_samples = vk::SampleCountFlagBits::e1,
        const bool is_wire_frame = false
    );

    vk::DescriptorPool create_descriptor_pool(
        const std::vector<DescriptorInfo>& descriptor_info,
        const uint32 max_sets
    ) const;
    vk::DescriptorSetLayout create_descriptor_set_layout(
        const std::vector<DescriptorInfo>& descriptor_info
    ) const;
};
