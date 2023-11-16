#pragma once

#include "vulkan_render_pass.hpp"
#include "vulkan_managed_buffer.hpp"
#include "vulkan_settings.hpp"
#include "resources/shader.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief  Vulkan implementation of generic shader. This uses a set of inputs
 * and parameters, as well as the shader programs contained in SPIR-V files to
 * construct a shader for use in rendering.
 */
class VulkanShader : public Shader {
  public:
    /**
     * @brief Configuration for descriptor set. Contains layout and binding
     * description.
     */
    struct VulkanDescriptorSetConfig {
        vk::DescriptorSetLayout                layout;
        Vector<vk::DescriptorSetLayoutBinding> bindings;
        std::optional<uint8>                   uniform_index {};
        std::optional<uint8>                   sampler_index {};
        uint8                                  binding_count = 0;
    };

    /**
     * @brief Vulkan implementation of instance-level shader state.
     */
    struct VulkanInstanceState : public InstanceState {
        std::array<vk::DescriptorSet, VulkanSettings::max_frames_in_flight>
            descriptor_set;
        std::array<std::optional<uint32>, VulkanSettings::max_frames_in_flight>
            descriptor_set_ids;
    };

  public:
    /**
     * @brief Construct a new Vulkan Shader object.
     * @param renderer Reference to renderer which owns this shader
     * @param texture_system Reference to a texture system
     * @param config Shader configuration used
     * @param device Device on which this shader will be used
     * @param allocator Custom allocation callback
     * @param render_pass Render pass used by this shader
     * @param command_buffer Buffer on which commands will be issued
     */
    VulkanShader(
        Renderer* const                      renderer,
        TextureSystem* const                 texture_system,
        const Config&                        config,
        const VulkanDevice* const            device,
        const vk::AllocationCallbacks* const allocator,
        const VulkanRenderPass* const        render_pass,
        const VulkanCommandBuffer* const     command_buffer
    );
    ~VulkanShader();

    void reload() override;

    void use() override;
    void bind_globals() override;
    void bind_instance(const uint32 id) override;
    void apply_global() override;
    void apply_instance() override;

    uint32 acquire_instance_resources(const Vector<Texture::Map*>& maps
    ) override;
    void   release_instance_resources(uint32 instance_id) override;

    const static uint32 max_descriptor_sets = 1024; // TODO: Maybe make dynamic

  protected:
    Outcome set_uniform(const uint16 id, void* value) override;

  private:
    const VulkanDevice*                  _device;
    const vk::AllocationCallbacks* const _allocator;
    const VulkanRenderPass*              _render_pass;
    const VulkanCommandBuffer* const     _command_buffer;

    // Pipeline
    vk::Pipeline       _pipeline;
    vk::PipelineLayout _pipeline_layout;

    // Descriptors
    vk::DescriptorPool                 _descriptor_pool;
    Vector<VulkanDescriptorSetConfig*> _descriptor_set_configs;
    Vector<vk::DescriptorSet>          _global_descriptor_sets;

    std::optional<uint8> global_set_index {};
    std::optional<uint8> instance_set_index {};

    // Buffers
    VulkanManagedBuffer* _uniform_buffer;
    uint64               _uniform_buffer_offset;

    vk::ShaderModule create_shader_module(
        const vk::ShaderStageFlagBits shader_stage
    ) const;
    Vector<vk::PipelineShaderStageCreateInfo> compute_stage_infos(
        const Vector<vk::ShaderStageFlagBits>& shader_stages
    ) const;
    Vector<vk::VertexInputAttributeDescription> compute_attributes() const;
    Vector<VulkanDescriptorSetConfig*>          compute_uniforms(
                 const Vector<vk::ShaderStageFlagBits>& shader_stages
             ) const;

    void create_pipeline(
        const Vector<vk::PipelineShaderStageCreateInfo>& shader_stages,
        const vk::PipelineVertexInputStateCreateInfo&    vertex_input_info,
        const bool                                       is_wire_frame = false
    );

    Vector<vk::DescriptorImageInfo>& get_image_infos(
        const Vector<Texture::Map*>& texture_maps
    ) const;
};

} // namespace ENGINE_NAMESPACE