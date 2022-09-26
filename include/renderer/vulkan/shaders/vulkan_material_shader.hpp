#pragma once

#include "vulkan_shader.hpp"

class VulkanMaterialShader : public VulkanShader {
public:
    VulkanMaterialShader(
        const VulkanDevice* const device,
        const vk::AllocationCallbacks* const allocator,
        const vk::RenderPass render_pass,
        const vk::SampleCountFlagBits number_of_msaa_samples
    );
    ~VulkanMaterialShader();

    /// @brief Select shader for rendering
    /// @param command_buffer Buffer to store bind commands
    void use(const vk::CommandBuffer& command_buffer);

    /// @brief Create descriptor sets specific to this shader
    /// @param buffer_infos Array of buffer infos used for UBO
    /// @param image_info Image info used for sampler
    void create_descriptor_sets(
        const std::vector<vk::DescriptorBufferInfo>& buffer_infos,
        const vk::DescriptorImageInfo& image_info
    );

    /// @brief Bind descriptor sets specific to this shader
    /// @param command_buffer Buffer to store bind command
    /// @param current_frame Frame on which to use UBO buffer
    void bind_descriptor_set(
        const vk::CommandBuffer& command_buffer,
        const uint32 current_frame
    );

private:
    void create_descriptor_pool();
    void create_descriptor_set_layout();

    std::vector<VulkanBuffer*> _uniform_buffers;
};