#pragma once

#include "../vulkan_buffer.hpp"

class VulkanShader {
private:

protected:
    VulkanDevice* _device;
    vk::AllocationCallbacks* _allocator;

    VulkanBuffer* _uniform_buffer;
    vk::DescriptorPool _descriptor_pool;
    vk::DescriptorSetLayout _descriptor_set_layout;
    std::vector<vk::DescriptorSet> _descriptor_sets;

    vk::ShaderModule create_shader_module(std::vector<byte> code);
    void create_pipeline(
        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages,
        std::vector<vk::VertexInputBindingDescription> binding_descriptions,
        std::vector<vk::VertexInputAttributeDescription> attribute_descriptions,
        std::vector<vk::DescriptorSetLayout> descriptor_set_layouts,
        vk::RenderPass render_pass,
        vk::SampleCountFlagBits number_of_msaa_samples = vk::SampleCountFlagBits::e1,
        bool is_wire_frame = false
    );

public:
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline pipeline;

    VulkanShader(VulkanDevice* device, vk::AllocationCallbacks* allocator);
    ~VulkanShader();

    virtual void use(const vk::CommandBuffer& command_buffer) {};
};
