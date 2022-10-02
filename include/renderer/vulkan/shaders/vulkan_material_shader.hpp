#pragma once

#include "math_libs.hpp"
#include "vulkan_shader.hpp"
#include "renderer/vulkan/vulkan_settings.hpp"
#include "renderer/renderer_types.hpp"

#include <map>

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

    /// @brief Bind descriptor sets specific to this shader
    /// @param command_buffer Buffer to store bind command
    /// @param current_frame Frame on which to use UBO buffer
    void bind_descriptor_set(
        const vk::CommandBuffer& command_buffer,
        const uint32 current_frame
    );

    void bind_object(
        const vk::CommandBuffer& command_buffer,
        const uint32 current_frame,
        const uint32 object_id
    );

    void update_global_state(
        const glm::mat4 projection,
        const glm::mat4 view,
        const glm::vec3 view_position,
        const glm::vec4 ambient_color,
        const int32 mode,
        const uint32 current_frame
    );
    void update_object_state(
        const GeometryRenderData data,
        uint32 current_frame
    );

    void acquire_resource(Material* const);
    void release_resource(Material* const);

private:
    static const uint32 _material_descriptor_count = 2;
    static const uint32 _material_sampler_count = 1;
    struct DescriptorState {
        std::array<std::optional<uint32>,
            VulkanSettings::max_frames_in_flight> ids;
    };
    struct MaterialInstanceState {
        bool allocated = false;
        std::vector<vk::DescriptorSet> descriptor_sets;
        std::array<DescriptorState,
            VulkanMaterialShader::_material_descriptor_count> descriptor_states;
    };

    vk::DescriptorPool _global_descriptor_pool;
    vk::DescriptorSetLayout _global_descriptor_set_layout;
    std::vector<vk::DescriptorSet> _global_descriptor_sets;
    std::vector<VulkanBuffer*> _global_uniform_buffers;

    vk::DescriptorPool _local_descriptor_pool;
    vk::DescriptorSetLayout _local_descriptor_set_layout;
    std::vector<VulkanBuffer*> _local_uniform_buffers;

    std::map<int, MaterialInstanceState> _instance_states;
    TextureUse _sampler_uses[_material_sampler_count];

    void create_global_descriptor_sets();
    uint32 get_next_available_ubo_index();
};