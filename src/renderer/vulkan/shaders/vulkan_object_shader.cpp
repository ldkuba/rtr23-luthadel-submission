#include "renderer/vulkan/shaders/vulkan_object_shader.hpp"

#include "renderer/vulkan/vulkan_settings.hpp"
#include "renderer/renderer_types.hpp"
#include "file_system.hpp"

// Constructor & Destructor
VulkanObjectShader::VulkanObjectShader(
    const VulkanDevice* const device,
    const vk::AllocationCallbacks* const allocator,
    const vk::RenderPass render_pass,
    const vk::SampleCountFlagBits number_of_msaa_samples
) : VulkanShader(device, allocator) {
    // === Create shader modules ===
    auto vertex_code = FileSystem::read_file_bytes("assets/shaders/builtin.object_shader.vert.spv");
    auto fragment_code = FileSystem::read_file_bytes("assets/shaders/builtin.object_shader.frag.spv");
    auto vertex_shader_module = create_shader_module(vertex_code);
    auto fragment_shader_module = create_shader_module(fragment_code);

    // === Shader stages ===
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages(2);

    // Vertex shader stage
    shader_stages[0].setStage(vk::ShaderStageFlagBits::eVertex);
    shader_stages[0].setModule(vertex_shader_module); // Shader module containing the code
    shader_stages[0].setPName("main");                // Function to invoke as an entrypoint
    shader_stages[0].setPSpecializationInfo(nullptr); // Initial shader constants

    // Fragment shader stage
    shader_stages[1].setStage(vk::ShaderStageFlagBits::eFragment);
    shader_stages[1].setModule(fragment_shader_module);
    shader_stages[1].setPName("main");
    shader_stages[1].setPSpecializationInfo(nullptr);

    // === Global descriptors ===
    create_descriptor_set_layout();
    create_descriptor_pool();

    // === Vertex input state info ===
    // Vertex bindings
    std::vector<vk::VertexInputBindingDescription> binding_descriptions(1);
    binding_descriptions[0].setBinding(0);
    binding_descriptions[0].setStride(sizeof(Vertex));
    binding_descriptions[0].setInputRate(vk::VertexInputRate::eVertex);

    // Vertex attributes
    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions(3);
    // Position
    attribute_descriptions[0].setBinding(0);
    attribute_descriptions[0].setLocation(0);
    attribute_descriptions[0].setFormat(vk::Format::eR32G32B32Sfloat);
    attribute_descriptions[0].setOffset(offsetof(Vertex, position));
    // Color
    attribute_descriptions[1].setBinding(0);
    attribute_descriptions[1].setLocation(1);
    attribute_descriptions[1].setFormat(vk::Format::eR32G32B32Sfloat);
    attribute_descriptions[1].setOffset(offsetof(Vertex, color));
    // Texture coordinates
    attribute_descriptions[2].setBinding(0);
    attribute_descriptions[2].setLocation(2);
    attribute_descriptions[2].setFormat(vk::Format::eR32G32Sfloat);
    attribute_descriptions[2].setOffset(offsetof(Vertex, texture_coord));

    vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.setVertexBindingDescriptionCount(1);
    vertex_input_info.setVertexBindingDescriptions(binding_descriptions);
    vertex_input_info.setVertexAttributeDescriptions(attribute_descriptions);

    // === Pipeline layout info ===
    vk::PipelineLayoutCreateInfo layout_info{};
    std::vector<vk::DescriptorSetLayout> description_set_layouts{ _descriptor_set_layout };
    // Used for UNIFORM values
    layout_info.setSetLayouts(description_set_layouts); // Layout of used descriptor sets
    layout_info.setPushConstantRangeCount(0);           // Push constant ranges used
    layout_info.setPPushConstantRanges(nullptr);

    // === Create pipeline ===
    create_pipeline(
        shader_stages,
        vertex_input_info,
        layout_info,
        render_pass,
        number_of_msaa_samples
    );

    // === Free unused objects ===
    _device->handle.destroyShaderModule(vertex_shader_module, _allocator);
    _device->handle.destroyShaderModule(fragment_shader_module, _allocator);

    // Create uniform buffers
    vk::DeviceSize buffer_size = sizeof(UniformBufferObject);
    _uniform_buffers.resize(VulkanSettings::max_frames_in_flight);

    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        _uniform_buffers[i] = new VulkanBuffer(_device, _allocator);
        _uniform_buffers[i]->create(
            buffer_size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent
        );
    }
}

VulkanObjectShader::~VulkanObjectShader() {
    // Uniforms
    for (uint32 i = 0; i < _uniform_buffers.size(); i++)
        delete _uniform_buffers[i];

    // Descriptors
    if (_descriptor_pool)
        _device->handle.destroyDescriptorPool(_descriptor_pool, _allocator);
    if (_descriptor_set_layout)
        _device->handle.destroyDescriptorSetLayout(_descriptor_set_layout, _allocator);

    // Pipeline
    _device->handle.destroyPipeline(_pipeline, _allocator);
    _device->handle.destroyPipelineLayout(_pipeline_layout, _allocator);
}

// /////////////////////////////////// //
// VULKAN OBJECT SHADER PUBLIC METHODS //
// /////////////////////////////////// //

void VulkanObjectShader::use(const vk::CommandBuffer& command_buffer) {
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);
}

void VulkanObjectShader::create_descriptor_pool() {
    std::array<vk::DescriptorPoolSize, 2> pool_sizes{};
    pool_sizes[0].setType(vk::DescriptorType::eUniformBuffer);
    pool_sizes[0].setDescriptorCount(VulkanSettings::max_frames_in_flight);
    pool_sizes[1].setType(vk::DescriptorType::eCombinedImageSampler);
    pool_sizes[1].setDescriptorCount(VulkanSettings::max_frames_in_flight);

    vk::DescriptorPoolCreateInfo create_info{};
    create_info.setPoolSizes(pool_sizes);
    create_info.setMaxSets(VulkanSettings::max_frames_in_flight);

    try {
        _descriptor_pool = _device->handle.createDescriptorPool(create_info, _allocator);
    } catch (vk::SystemError e) { Logger::fatal(e.what()); }
}

void VulkanObjectShader::create_descriptor_sets(
    const std::vector<vk::DescriptorBufferInfo>& buffer_infos,
    const vk::DescriptorImageInfo& image_info
) {
    std::vector<vk::DescriptorSetLayout> layouts(VulkanSettings::max_frames_in_flight, _descriptor_set_layout);
    vk::DescriptorSetAllocateInfo allocation_info{};
    allocation_info.setDescriptorPool(_descriptor_pool);
    allocation_info.setSetLayouts(layouts);

    try {
        _descriptor_sets = _device->handle.allocateDescriptorSets(allocation_info);
    } catch (vk::SystemError e) { Logger::fatal(e.what()); }

    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        // Combined descriptor
        std::array<vk::WriteDescriptorSet, 2> descriptor_writes{};

        descriptor_writes[0].setDstSet(_descriptor_sets[i]);
        descriptor_writes[0].setDstBinding(0);
        descriptor_writes[0].setDstArrayElement(0);
        descriptor_writes[0].setDescriptorType(vk::DescriptorType::eUniformBuffer);
        descriptor_writes[0].setDescriptorCount(1);
        descriptor_writes[0].setPBufferInfo(&buffer_infos[i]);

        descriptor_writes[1].setDstSet(_descriptor_sets[i]);
        descriptor_writes[1].setDstBinding(1);
        descriptor_writes[1].setDstArrayElement(0);
        descriptor_writes[1].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
        descriptor_writes[1].setDescriptorCount(1);
        descriptor_writes[1].setPImageInfo(&image_info);

        _device->handle.updateDescriptorSets(descriptor_writes, nullptr);
    }
}

void VulkanObjectShader::bind_descriptor_set(
    const vk::CommandBuffer& command_buffer,
    const uint32 current_frame
) {
    command_buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        _pipeline_layout, 0,
        1, &_descriptor_sets[current_frame],
        0, nullptr
    );
}

// //////////////////////////////////// //
// VULKAN OBJECT SHADER PRIVATE METHODS //
// //////////////////////////////////// //

void VulkanObjectShader::create_descriptor_set_layout() {
    vk::DescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.setBinding(0);
    ubo_layout_binding.setDescriptorType(vk::DescriptorType::eUniformBuffer);
    ubo_layout_binding.setDescriptorCount(1);
    ubo_layout_binding.setStageFlags(vk::ShaderStageFlagBits::eVertex);
    ubo_layout_binding.setPImmutableSamplers(nullptr);

    vk::DescriptorSetLayoutBinding sampler_layout_binding{};
    sampler_layout_binding.setBinding(1);
    sampler_layout_binding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
    sampler_layout_binding.setDescriptorCount(1);
    sampler_layout_binding.setStageFlags(vk::ShaderStageFlagBits::eFragment);
    sampler_layout_binding.setPImmutableSamplers(nullptr);

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
        ubo_layout_binding,
        sampler_layout_binding
    };

    vk::DescriptorSetLayoutCreateInfo layout_info{};
    layout_info.setBindings(bindings);

    try {
        _descriptor_set_layout = _device->handle.createDescriptorSetLayout(layout_info, _allocator);
    } catch (vk::SystemError e) { Logger::fatal(e.what()); }
}