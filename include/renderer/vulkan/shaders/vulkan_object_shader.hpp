#pragma once

#include "vulkan_shader.hpp"

class VulkanObjectShader : public VulkanShader {
public:
    VulkanObjectShader(
        const VulkanDevice* const device,
        const vk::AllocationCallbacks* const allocator,
        const vk::RenderPass render_pass,
        const vk::SampleCountFlagBits number_of_msaa_samples
    );
    ~VulkanObjectShader();

    void use(const vk::CommandBuffer& command_buffer);
    void create_descriptor_sets(
        const std::vector<vk::DescriptorBufferInfo>& buffer_infos,
        const vk::DescriptorImageInfo& image_info
    );
    void bind_descriptor_set(
        const vk::CommandBuffer& command_buffer,
        const uint32 current_frame
    );

private:
    void create_descriptor_pool();
    void create_descriptor_set_layout();

    std::vector<VulkanBuffer*> _uniform_buffers;
};