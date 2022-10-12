#pragma once

#include "vulkan_shader.hpp"
#include "renderer/renderer_types.hpp"

#include "systems/resource_system.hpp"

class VulkanUIShader : public VulkanShader {
  public:
    VulkanUIShader(
        const VulkanDevice* const            device,
        const vk::AllocationCallbacks* const allocator,
        const VulkanRenderPass* const        render_pass,
        ResourceSystem* resource_system // TODO: most likely temporary
    );
    ~VulkanUIShader();

    /// @brief Select shader for rendering
    /// @param command_buffer Buffer to store shader bind commands
    void use(const vk::CommandBuffer& command_buffer);

    /// @brief Bind global ubo data for rendering
    /// @param command_buffer Buffer to store descriptor set bind command
    /// @param current_frame Frame on which to use UBO buffer
    void bind_global_description_set(
        const vk::CommandBuffer& command_buffer, const uint32 current_frame
    );

    /// @brief Bind material ubo data for rendering
    /// @param command_buffer Buffer to store material bind command
    /// @param current_frame Frame on which to use UBO buffer
    /// @param material_id Id of a material we wish to bind
    void bind_material(
        const vk::CommandBuffer& command_buffer,
        const uint32             current_frame,
        const uint32             material_id
    );

    /// @brief Update global ubo data
    /// @param projection Projection matrix
    /// @param view View matrix
    /// @param view_position View position
    /// @param ambient_color Ambient color
    /// @param mode Rendering mode
    /// @param current_frame Currently rendered frame
    void update_global_state(
        const glm::mat4 projection,
        const glm::mat4 view,
        const int32     mode,
        const uint32    current_frame
    );

    void set_model(
        const glm::mat4 model, const uint64 obj_id, const uint32 current_frame
    );

    void apply_material(
        const Material* const material, const uint32 current_frame
    );

    void acquire_resource(Material* const);
    void release_resource(Material* const);

  private:
    static const uint32 _material_descriptor_count = 3;
    static const uint32 _material_sampler_count    = 1;
    struct DescriptorState {
        std::array<std::optional<uint32>, VulkanSettings::max_frames_in_flight>
            ids;
    };
    struct MaterialInstanceState {
        bool                           allocated = false;
        std::vector<vk::DescriptorSet> descriptor_sets;
        std::array<DescriptorState, VulkanUIShader::_material_descriptor_count>
            descriptor_states;
    };

    std::vector<vk::DescriptorSet>                    _global_descriptor_sets;
    std::unordered_map<uint32, MaterialInstanceState> _instance_states;
    TextureUse _sampler_uses[_material_sampler_count];

    void   create_global_descriptor_sets();
    uint32 get_next_available_ubo_index();
};