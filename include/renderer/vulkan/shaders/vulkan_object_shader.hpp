#pragma once

#include "vulkan_shader.hpp"

class VulkanObjectShader : public VulkanShader {
private:
    void create_descriptor_pool();
    void create_descriptor_set_layout();

    std::vector<VulkanBuffer*> _uniform_buffers;

public:
    VulkanObjectShader(
        VulkanDevice* device,
        vk::AllocationCallbacks* allocator,
        vk::RenderPass render_pass,
        vk::SampleCountFlagBits number_of_msaa_samples
    );
    ~VulkanObjectShader();

    void use(const vk::CommandBuffer& command_buffer);
    void create_descriptor_sets(
        vk::DescriptorBufferInfo buffer_info,
        vk::DescriptorImageInfo image_info
    );
};