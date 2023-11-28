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
    struct VulkanDescriptorSetBackendData : public DescriptorSet::BackendData {
        vk::DescriptorSetLayout                layout;
        Vector<vk::DescriptorSetLayoutBinding> vulkan_bindings;
    };

    /**
     * @brief Vulkan implementation of instance-level shader state.
     */
    struct VulkanDescriptorSetState : public DescriptorSet::State {
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

    void bind_instance(const uint32 id) override;
    void apply_global() override;
    void apply_instance() override;

    // maps is a flattened vector of maps for all instance sets -> for all
    // sampler bindings
    // TODO: when sampler binding "names" are implemented properly maybe it's a good
    // idea to change the vector to something that knows which Vector of maps
    // belongs to which set and binding for example: Map<uint32, Map<String,
    // Vector<Texture::Map*>>>
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
    vk::DescriptorPool _descriptor_pool;

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

    // TODO: For now all used shader staged are passed to each binding. Some
    // bindings should only be available in a specific stage
    void compute_uniforms(const Vector<vk::ShaderStageFlagBits>& shader_stages
    );

    void create_pipeline(
        const Vector<vk::PipelineShaderStageCreateInfo>& shader_stages,
        const vk::PipelineVertexInputStateCreateInfo&    vertex_input_info,
        const bool                                       is_wire_frame = false
    );

    Vector<vk::DescriptorImageInfo>& get_image_infos(
        const Vector<Texture::Map*>& texture_maps
    ) const;

    void apply_descriptor_set(
        DescriptorSet& set, uint32 state_id
    );
};

} // namespace ENGINE_NAMESPACE