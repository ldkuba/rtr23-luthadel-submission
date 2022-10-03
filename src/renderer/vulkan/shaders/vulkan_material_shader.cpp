#include "renderer/vulkan/shaders/vulkan_material_shader.hpp"

#include "renderer/renderer_types.hpp"
#include "file_system.hpp"

// Constructor & Destructor
VulkanMaterialShader::VulkanMaterialShader(
    const VulkanDevice* const device,
    const vk::AllocationCallbacks* const allocator,
    const vk::RenderPass render_pass,
    const vk::SampleCountFlagBits number_of_msaa_samples
) : VulkanShader(device, allocator) {
    Logger::trace(RENDERER_VULKAN_LOG, "Creating material shader.");

    // === Create shader modules ===
    vk::ShaderModule vertex_shader_module;
    vk::ShaderModule fragment_shader_module;
    try {
        auto vertex_code = FileSystem::read_file_bytes("assets/shaders/builtin.material_shader.vert.spv");
        auto fragment_code = FileSystem::read_file_bytes("assets/shaders/builtin.material_shader.frag.spv");
        vertex_shader_module = create_shader_module(vertex_code);
        fragment_shader_module = create_shader_module(fragment_code);
    } catch (FileSystemException e) { Logger::fatal(RENDERER_VULKAN_LOG, e.what()); }

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

    // === Create descriptor pools and set layouts ===
    // Global descriptor
    std::vector<DescriptorInfo> global_descriptor_infos(1);
    global_descriptor_infos[0] = {
        vk::DescriptorType::eUniformBuffer,
        vk::ShaderStageFlagBits::eVertex,
        VulkanSettings::max_frames_in_flight
    };

    _global_descriptor_set_layout = create_descriptor_set_layout(global_descriptor_infos);
    _global_descriptor_pool =
        create_descriptor_pool(global_descriptor_infos, VulkanSettings::max_frames_in_flight);

    // Local descriptor
    std::vector<DescriptorInfo> local_descriptor_infos(2);
    local_descriptor_infos[0] = {
        vk::DescriptorType::eUniformBuffer,
        vk::ShaderStageFlagBits::eVertex,
        VulkanSettings::max_material_count
    };
    local_descriptor_infos[1] = {
        vk::DescriptorType::eCombinedImageSampler,
        vk::ShaderStageFlagBits::eFragment,
        _material_sampler_count * VulkanSettings::max_material_count
    };

    _local_descriptor_set_layout = create_descriptor_set_layout(local_descriptor_infos);
    _local_descriptor_pool = create_descriptor_pool(
        local_descriptor_infos,
        VulkanSettings::max_material_count,
        true
    );

    _sampler_uses[0] = TextureUse::MapDiffuse;

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
    std::vector<vk::DescriptorSetLayout> description_set_layouts{
        _global_descriptor_set_layout,
        _local_descriptor_set_layout
    };
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
    _device->handle().destroyShaderModule(vertex_shader_module, _allocator);
    _device->handle().destroyShaderModule(fragment_shader_module, _allocator);

    // === Create uniform buffers ===
    vk::DeviceSize buffer_size;

    // Global ubo
    buffer_size = sizeof(GlobalUniformObject);
    _global_uniform_buffers.resize(VulkanSettings::max_frames_in_flight);
    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        _global_uniform_buffers[i] = new VulkanBuffer(_device, _allocator);
        _global_uniform_buffers[i]->create(
            buffer_size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent
        );
    }

    create_global_descriptor_sets();

    // Local ubo
    buffer_size = VulkanSettings::max_material_count * sizeof(LocalUniformObject);
    _local_uniform_buffers.resize(VulkanSettings::max_frames_in_flight);
    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        _local_uniform_buffers[i] = new VulkanBuffer(_device, _allocator);
        _local_uniform_buffers[i]->create(
            buffer_size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent
        );
    }

    Logger::trace(RENDERER_VULKAN_LOG, "Material shader created.");
}

VulkanMaterialShader::~VulkanMaterialShader() {
    // Uniforms
    for (uint32 i = 0; i < _global_uniform_buffers.size(); i++)
        delete _global_uniform_buffers[i];
    for (uint32 i = 0; i < _local_uniform_buffers.size(); i++)
        delete _local_uniform_buffers[i];

    // Descriptors
    if (_global_descriptor_pool)
        _device->handle().destroyDescriptorPool(_global_descriptor_pool, _allocator);
    if (_global_descriptor_set_layout)
        _device->handle().destroyDescriptorSetLayout(_global_descriptor_set_layout, _allocator);
    if (_local_descriptor_pool)
        _device->handle().destroyDescriptorPool(_local_descriptor_pool, _allocator);
    if (_local_descriptor_set_layout)
        _device->handle().destroyDescriptorSetLayout(_local_descriptor_set_layout, _allocator);

    // Pipeline
    _device->handle().destroyPipeline(_pipeline, _allocator);
    _device->handle().destroyPipelineLayout(_pipeline_layout, _allocator);
}

// ///////////////////////////////////// //
// VULKAN MATERIAL SHADER PUBLIC METHODS //
// ///////////////////////////////////// //

void VulkanMaterialShader::use(const vk::CommandBuffer& command_buffer) {
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);
}

void VulkanMaterialShader::bind_descriptor_set(
    const vk::CommandBuffer& command_buffer,
    const uint32 current_frame
) {
    command_buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        _pipeline_layout, 0,
        1, &_global_descriptor_sets[current_frame],
        0, nullptr
    );
}

void VulkanMaterialShader::bind_object(
    const vk::CommandBuffer& command_buffer,
    const uint32 current_frame,
    const uint32 object_id
) {
    command_buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        _pipeline_layout, 1,
        1, &_instance_states[object_id].descriptor_sets[current_frame],
        0, nullptr
    );
}

void VulkanMaterialShader::update_global_state(
    const glm::mat4 projection,
    const glm::mat4 view,
    const glm::vec3 view_position,
    const glm::vec4 ambient_color,
    const int32 mode,
    const uint32 current_frame
) {
    // Define ubo transformations
    static float32 rotation = 0.0f;
    rotation += 0.01f;
    GlobalUniformObject ubo{};
    ubo.view = view;
    ubo.project = projection;

    // Copy ubo data to the buffer
    _global_uniform_buffers[current_frame]->load_data(&ubo, 0, sizeof(ubo));
}

void VulkanMaterialShader::update_object_state(
    const GeometryRenderData data,
    uint32 current_frame
) {
    // Obtain material data.
    MaterialInstanceState object_state = _instance_states[data.material->internal_id.value()];
    if (object_state.allocated == false) {
        Logger::error(RENDERER_VULKAN_LOG, "Requested object is not allocated.");
        return;
    }
    vk::DescriptorSet object_descriptor_set = object_state.descriptor_sets[current_frame];

    // Load data to buffer
    uint32 size = sizeof(LocalUniformObject);
    uint64 offset = sizeof(LocalUniformObject) * data.material->internal_id.value();
    LocalUniformObject lub;

    lub.model = data.model;
    lub.diffuse_color = data.material->diffuse_color;

    _local_uniform_buffers[current_frame]->load_data(&lub, offset, size);

    // Update descriptor set if needed
    // Combined descriptor
    std::vector<vk::WriteDescriptorSet> descriptor_writes{};
    uint32 descriptor_index = 0;

    // Uniform buffer
    if (object_state.descriptor_states[0].ids[current_frame].has_value() == false) {
        vk::DescriptorBufferInfo buffer_info = {};
        buffer_info.setBuffer(_local_uniform_buffers[current_frame]->handle);
        buffer_info.setOffset(offset);
        buffer_info.setRange(size);

        vk::WriteDescriptorSet descriptor_write = {};
        descriptor_write.setDstSet(object_descriptor_set);
        descriptor_write.setDstBinding(descriptor_index);
        descriptor_write.setDstArrayElement(0);
        descriptor_write.setDescriptorType(vk::DescriptorType::eUniformBuffer);
        descriptor_write.setDescriptorCount(1);
        descriptor_write.setPBufferInfo(&buffer_info);
        descriptor_writes.push_back(descriptor_write);

        object_state.descriptor_states[0].ids[current_frame] = 0;
    }
    descriptor_index++;

    // Samplers
    for (uint32 i = 0; i < _material_sampler_count; i++) {
        const Texture* texture = nullptr;

        switch (_sampler_uses[i]) {
        case TextureUse::MapDiffuse:
            texture = data.material->diffuse_map().texture;
            break;

        default:
            Logger::fatal(RENDERER_VULKAN_LOG, "Unable to bind sampler to a known use.");
            break;
        }

        auto& descriptor_id = object_state.descriptor_states[descriptor_index].ids[current_frame];

        // If the texture hasn't been loaded yet, use the default.
        if (texture == nullptr || !texture->id.has_value()) {
            // texture = texture_system_get_default_texture();
            continue;
        }

        // Update sampler if needed
        if (!descriptor_id.has_value() || descriptor_id.value() != texture->id) {
            VulkanTextureData* texture_data =
                reinterpret_cast<VulkanTextureData*>(texture->internal_data());

            vk::DescriptorImageInfo image_info = {};
            image_info.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
            image_info.setImageView(texture_data->image->view);
            image_info.setSampler(texture_data->sampler);

            vk::WriteDescriptorSet descriptor_write = {};
            descriptor_write.setDstSet(object_descriptor_set);
            descriptor_write.setDstBinding(descriptor_index);
            descriptor_write.setDstArrayElement(0);
            descriptor_write.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
            descriptor_write.setDescriptorCount(1);
            descriptor_write.setPImageInfo(&image_info);
            descriptor_writes.push_back(descriptor_write);

            descriptor_id = texture->id;
        }
        descriptor_index++;
    }

    if (descriptor_writes.size() > 0)
        _device->handle().updateDescriptorSets(descriptor_writes, nullptr);
}

void VulkanMaterialShader::acquire_resource(Material* const material) {
    // Acquire next object slot on gpu
    material->internal_id = get_next_available_ubo_index();

    // Reset object descriptor info
    MaterialInstanceState& state = _instance_states[material->internal_id.value()];
    for (auto descriptor_state : state.descriptor_states) {
        for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
            descriptor_state.ids[i].reset();
        }
    }

    // Allocate descriptor sets.
    std::vector<vk::DescriptorSetLayout> layouts(
        VulkanSettings::max_frames_in_flight, _local_descriptor_set_layout);
    vk::DescriptorSetAllocateInfo allocation_info{};
    allocation_info.setDescriptorPool(_local_descriptor_pool);
    allocation_info.setSetLayouts(layouts);

    try {
        state.descriptor_sets = _device->handle().allocateDescriptorSets(allocation_info);
    } catch (vk::SystemError e) { Logger::fatal(RENDERER_VULKAN_LOG, e.what()); }
    state.allocated = true;
}

void VulkanMaterialShader::release_resource(Material* const material) {
    if (!material->internal_id.has_value()) return;
    MaterialInstanceState& state = _instance_states[material->internal_id.value()];

    // Release object descriptor sets.
    try {
        _device->handle().freeDescriptorSets(_local_descriptor_pool, state.descriptor_sets);
    } catch (vk::SystemError e) { Logger::fatal(RENDERER_VULKAN_LOG, e.what()); }

    // Reset object descriptor info
    for (auto descriptor_state : state.descriptor_states) {
        for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
            descriptor_state.ids[i].reset();
        }
    }

    material->internal_id.reset();

    // TODO: add the object_id to the free list
}

// ////////////////////////////////////// //
// VULKAN MATERIAL SHADER PRIVATE METHODS //
// ////////////////////////////////////// //

void VulkanMaterialShader::create_global_descriptor_sets() {
    std::vector<vk::DescriptorSetLayout> layouts(
        VulkanSettings::max_frames_in_flight, _global_descriptor_set_layout);
    vk::DescriptorSetAllocateInfo allocation_info{};
    allocation_info.setDescriptorPool(_global_descriptor_pool);
    allocation_info.setSetLayouts(layouts);

    try {
        _global_descriptor_sets = _device->handle().allocateDescriptorSets(allocation_info);
    } catch (vk::SystemError e) { Logger::fatal(RENDERER_VULKAN_LOG, e.what()); }

    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        // Combined descriptor
        std::array<vk::WriteDescriptorSet, 1> descriptor_writes{};

        vk::DescriptorBufferInfo buffer_info = {};
        buffer_info.setBuffer(_global_uniform_buffers[i]->handle);
        buffer_info.setOffset(0);
        buffer_info.setRange(sizeof(GlobalUniformObject));

        descriptor_writes[0].setDstSet(_global_descriptor_sets[i]);
        descriptor_writes[0].setDstBinding(0);
        descriptor_writes[0].setDstArrayElement(0);
        descriptor_writes[0].setDescriptorType(vk::DescriptorType::eUniformBuffer);
        descriptor_writes[0].setDescriptorCount(1);
        descriptor_writes[0].setPBufferInfo(&buffer_info);

        _device->handle().updateDescriptorSets(descriptor_writes, nullptr);
    }
}

uint32 VulkanMaterialShader::get_next_available_ubo_index() {
    static uint32 index = 0;
    return index++;
}