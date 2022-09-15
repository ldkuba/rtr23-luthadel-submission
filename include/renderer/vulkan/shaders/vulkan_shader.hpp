#pragma once

#include "../vulkan_buffer.hpp"

class VulkanShader {
public:
    /// @brief Pipeline configuration for this shader
    vk::Pipeline pipeline;

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

    // VulkanBuffer* _uniform_buffer;
    vk::DescriptorPool _descriptor_pool;
    vk::DescriptorSetLayout _descriptor_set_layout;
    std::vector<vk::DescriptorSet> _descriptor_sets;
    vk::PipelineLayout _pipeline_layout;

    vk::ShaderModule create_shader_module(const std::vector<byte> code) const;
    void create_pipeline(
        const std::vector<vk::PipelineShaderStageCreateInfo>& shader_stages,
        const std::vector<vk::VertexInputBindingDescription>& binding_descriptions,
        const std::vector<vk::VertexInputAttributeDescription>& attribute_descriptions,
        const std::vector<vk::DescriptorSetLayout>& descriptor_set_layouts,
        const vk::RenderPass render_pass,
        const vk::SampleCountFlagBits number_of_msaa_samples = vk::SampleCountFlagBits::e1,
        const bool is_wire_frame = false
    );
};
