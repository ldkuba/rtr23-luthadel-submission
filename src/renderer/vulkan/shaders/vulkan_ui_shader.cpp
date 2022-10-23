#include "renderer/vulkan/shaders/vulkan_ui_shader.hpp"

#include "resources/datapack.hpp"

// UBO
struct GlobalUBO {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 project;
};

struct InstanceVertexUBO {
    alignas(16) glm::mat4 model;
};

struct InstanceFragmentUBO {
    alignas(16) glm::vec4 diffuse_color;
};

// Constructor & Destructor
VulkanUIShader::VulkanUIShader(
    const VulkanDevice* const            device,
    const vk::AllocationCallbacks* const allocator,
    const VulkanRenderPass* const        render_pass,
    ResourceSystem*                      resource_system
)
    : VulkanShader(device, allocator) {
    Logger::trace(RENDERER_VULKAN_LOG, "Creating UI shader.");

    // === Create shader modules ===
    // Load raw binary data
    Result<Resource*, RuntimeError> result;
    result = resource_system->load(
        "shaders/builtin.ui_shader.vert.spv", ResourceType::Binary
    );
    if (result.has_error())
        Logger::fatal(RENDERER_VULKAN_LOG, result.error().what());
    auto vertex_code = (ByteArrayData*) result.value();

    result = resource_system->load(
        "shaders/builtin.ui_shader.frag.spv", ResourceType::Binary
    );
    if (result.has_error())
        Logger::fatal(RENDERER_VULKAN_LOG, result.error().what());
    auto fragment_code = (ByteArrayData*) result.value();

    // Create shader modules
    vk::ShaderModule vertex_shader_module =
        create_shader_module(vertex_code->data);
    vk::ShaderModule fragment_shader_module =
        create_shader_module(fragment_code->data);

    // Release resources
    resource_system->unload(vertex_code);
    resource_system->unload(fragment_code);

    // === Shader stages ===
    Vector<vk::PipelineShaderStageCreateInfo> shader_stages(2);

    // Vertex shader stage
    shader_stages[0].setStage(vk::ShaderStageFlagBits::eVertex);
    // Shader module containing the code
    shader_stages[0].setModule(vertex_shader_module);
    // Function to invoke as an entrypoint
    shader_stages[0].setPName("main");
    // Initial shader constants
    shader_stages[0].setPSpecializationInfo(nullptr);

    // Fragment shader stage
    shader_stages[1].setStage(vk::ShaderStageFlagBits::eFragment);
    shader_stages[1].setModule(fragment_shader_module);
    shader_stages[1].setPName("main");
    shader_stages[1].setPSpecializationInfo(nullptr);

    // === Create descriptor pools and set layouts ===
    // Global descriptor
    auto global_descriptor = new VulkanDescriptor(_device, _allocator);
    global_descriptor->add_uniform_buffer(
        vk::ShaderStageFlagBits::eVertex,
        VulkanSettings::max_frames_in_flight,
        sizeof(GlobalUBO)
    );
    add_descriptor(global_descriptor, VulkanSettings::max_frames_in_flight);

    // Local descriptor
    uint32 total_entity_count = VulkanSettings::max_material_count *
                                VulkanSettings::max_frames_in_flight;
    auto local_descriptor = new VulkanDescriptor(_device, _allocator);
    local_descriptor->add_uniform_buffer(
        vk::ShaderStageFlagBits::eVertex,
        total_entity_count,
        total_entity_count * sizeof(InstanceVertexUBO)
    );
    local_descriptor->add_uniform_buffer(
        vk::ShaderStageFlagBits::eFragment,
        total_entity_count,
        total_entity_count * sizeof(InstanceFragmentUBO)
    );
    local_descriptor->add_image_sampler(
        vk::ShaderStageFlagBits::eFragment,
        total_entity_count * _material_sampler_count
    );
    add_descriptor(local_descriptor, total_entity_count, true);
    _sampler_uses[0] = TextureUse::MapDiffuse;

    // === Vertex input state info ===
    // Vertex bindings
    Vector<vk::VertexInputBindingDescription> binding_descriptions(1);
    binding_descriptions[0].setBinding(0);
    binding_descriptions[0].setStride(sizeof(Vertex2D));
    binding_descriptions[0].setInputRate(vk::VertexInputRate::eVertex);

    // Vertex attributes
    Vector<vk::VertexInputAttributeDescription> attribute_descriptions(2);
    // Position
    attribute_descriptions[0].setBinding(0);
    attribute_descriptions[0].setLocation(0);
    attribute_descriptions[0].setFormat(vk::Format::eR32G32Sfloat);
    attribute_descriptions[0].setOffset(offsetof(Vertex2D, position));
    // Texture coordinates
    attribute_descriptions[1].setBinding(0);
    attribute_descriptions[1].setLocation(1);
    attribute_descriptions[1].setFormat(vk::Format::eR32G32Sfloat);
    attribute_descriptions[1].setOffset(offsetof(Vertex2D, texture_coord));

    vk::PipelineVertexInputStateCreateInfo vertex_input_info {};
    vertex_input_info.setVertexBindingDescriptionCount(1);
    vertex_input_info.setVertexBindingDescriptions(binding_descriptions);
    vertex_input_info.setVertexAttributeDescriptions(attribute_descriptions);

    // === Create pipeline ===
    create_pipeline(shader_stages, vertex_input_info, render_pass);

    // === Free unused objects ===
    _device->handle().destroyShaderModule(vertex_shader_module, _allocator);
    _device->handle().destroyShaderModule(fragment_shader_module, _allocator);

    // === Create global descriptor sets ===
    create_global_descriptor_sets();

    Logger::trace(RENDERER_VULKAN_LOG, "UI shader created.");
}

VulkanUIShader::~VulkanUIShader() {
    Logger::trace(RENDERER_VULKAN_LOG, "UI shader destroyed.");
}

// /////////////////////////////// //
// VULKAN UI SHADER PUBLIC METHODS //
// /////////////////////////////// //

void VulkanUIShader::use(const vk::CommandBuffer& command_buffer) {
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);
}

void VulkanUIShader::bind_global_description_set(
    const vk::CommandBuffer& command_buffer, const uint32 current_frame
) {
    command_buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        _pipeline_layout,
        0,
        1,
        &_global_descriptor_sets[current_frame],
        0,
        nullptr
    );
}

void VulkanUIShader::bind_material(
    const vk::CommandBuffer& command_buffer,
    const uint32             current_frame,
    const uint32             material_id
) {
    command_buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        _pipeline_layout,
        1,
        1,
        &_instance_states[material_id].descriptor_sets[current_frame],
        0,
        nullptr
    );
}

void VulkanUIShader::update_global_state(
    const glm::mat4 projection,
    const glm::mat4 view,
    const int32     mode,
    const uint32    current_frame
) {
    // Define ubo transformations
    GlobalUBO global_ubo {};
    global_ubo.view    = view;
    global_ubo.project = projection;

    // Copy ubo data to the buffer
    descriptors()[0]
        ->get_buffer(0, current_frame)
        ->load_data(&global_ubo, 0, sizeof(global_ubo));
}

void VulkanUIShader::set_model(
    const glm::mat4 model, const uint64 obj_id, const uint32 current_frame
) {
    uint32            size   = sizeof(InstanceVertexUBO);
    uint64            offset = sizeof(InstanceVertexUBO) * obj_id;
    InstanceVertexUBO instance_ubo;

    instance_ubo.model = model;

    // Get local uniform buffer (located at set=1, binding=0)
    auto uniform_buffer = descriptors()[1]->get_buffer(0, current_frame);

    MaterialInstanceState material_state = _instance_states[obj_id];
    if (material_state.descriptor_states[0].ids[current_frame].has_value() ==
        false) {
        vk::DescriptorBufferInfo buffer_info = {};
        buffer_info.setBuffer(uniform_buffer->handle);
        buffer_info.setOffset(offset);
        buffer_info.setRange(size);

        vk::WriteDescriptorSet descriptor_write = {};
        descriptor_write.setDstSet(material_state.descriptor_sets[current_frame]
        );
        descriptor_write.setDstBinding(0);
        descriptor_write.setDstArrayElement(0);
        descriptor_write.setDescriptorType(vk::DescriptorType::eUniformBuffer);
        descriptor_write.setDescriptorCount(1);
        descriptor_write.setPBufferInfo(&buffer_info);

        _device->handle().updateDescriptorSets(
            1, &descriptor_write, 0, nullptr
        );

        material_state.descriptor_states[0].ids[current_frame] = 0;
    }

    uniform_buffer->load_data(&instance_ubo, offset, size);
}

void VulkanUIShader::apply_material(
    const Material* const material, const uint32 current_frame
) {
    if (!material || !material->internal_id.has_value()) {
        Logger::error(
            RENDERER_VULKAN_LOG,
            "Application of a non-existant material attempted. Nothing was "
            "done."
        );
        return;
    }

    // Obtain material data.
    MaterialInstanceState material_state =
        _instance_states[material->internal_id.value()];
    if (material_state.allocated == false) {
        Logger::error(
            RENDERER_VULKAN_LOG, "Requested material is not allocated."
        );
        return;
    }
    vk::DescriptorSet object_descriptor_set =
        material_state.descriptor_sets[current_frame];

    // Load data to buffer
    uint32 size   = sizeof(InstanceFragmentUBO);
    uint64 offset = sizeof(InstanceFragmentUBO) * material->internal_id.value();
    InstanceFragmentUBO instance_ubo;

    instance_ubo.diffuse_color = material->diffuse_color;

    // Get local uniform buffer (located at set=1, binding=1)
    auto uniform_buffer = descriptors()[1]->get_buffer(1, current_frame);
    uniform_buffer->load_data(&instance_ubo, offset, size);

    // Update descriptor set if needed
    // Combined descriptor
    Vector<vk::WriteDescriptorSet> descriptor_writes {};
    uint32                         descriptor_index = 1;

    // Uniform buffer
    if (material_state.descriptor_states[descriptor_index]
            .ids[current_frame]
            .has_value() == false) {
        vk::DescriptorBufferInfo buffer_info = {};
        buffer_info.setBuffer(uniform_buffer->handle);
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

        material_state.descriptor_states[descriptor_index].ids[current_frame] =
            0;
    }
    descriptor_index++;

    // Samplers
    for (uint32 i = 0; i < _material_sampler_count; i++) {
        const Texture* texture = nullptr;

        switch (_sampler_uses[i]) {
        case TextureUse::MapDiffuse:
            texture = material->diffuse_map().texture;
            break;

        default:
            Logger::fatal(
                RENDERER_VULKAN_LOG, "Unable to bind sampler to a known use."
            );
            break;
        }

        auto& descriptor_id = material_state.descriptor_states[descriptor_index]
                                  .ids[current_frame];

        // If the texture hasn't been loaded yet, use the default.
        if (texture == nullptr || !texture->id.has_value()) {
            // texture = texture_system_get_default_texture();
            continue;
        }

        // Update sampler if needed
        if (!descriptor_id.has_value() ||
            descriptor_id.value() != texture->id) {
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
            descriptor_write.setDescriptorType(
                vk::DescriptorType::eCombinedImageSampler
            );
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

void VulkanUIShader::acquire_resource(Material* const material) {
    // Acquire next object slot on gpu
    material->internal_id = get_next_available_ubo_index();

    // Reset object descriptor info
    MaterialInstanceState& state =
        _instance_states[material->internal_id.value()];
    for (auto descriptor_state : state.descriptor_states) {
        for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
            descriptor_state.ids[i].reset();
        }
    }

    // Local descriptor is at set=1
    auto local_descriptor = descriptors()[1];

    // Allocate descriptor sets.
    Vector<vk::DescriptorSetLayout> layouts(
        VulkanSettings::max_frames_in_flight, local_descriptor->set_layout
    );
    vk::DescriptorSetAllocateInfo allocation_info {};
    allocation_info.setDescriptorPool(local_descriptor->pool);
    allocation_info.setSetLayouts(layouts);

    try {
        state.descriptor_sets =
            _device->handle().allocateDescriptorSets(allocation_info);
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }
    state.allocated = true;
}

void VulkanUIShader::release_resource(Material* const material) {
    if (!material->internal_id.has_value()) return;
    MaterialInstanceState& state =
        _instance_states[material->internal_id.value()];

    // Local descriptor is at set=1
    auto local_descriptor = descriptors()[1];

    // Release object descriptor sets.
    _device->handle().waitIdle();
    try {
        _device->handle().freeDescriptorSets(
            local_descriptor->pool, state.descriptor_sets
        );
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    // Reset object descriptor info
    for (auto descriptor_state : state.descriptor_states) {
        for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
            descriptor_state.ids[i].reset();
        }
    }

    material->internal_id.reset();

    // TODO: add the material_id to the free list
}

// //////////////////////////////// //
// VULKAN UI SHADER PRIVATE METHODS //
// //////////////////////////////// //

void VulkanUIShader::create_global_descriptor_sets() {
    // Global descriptor is at set=0
    auto global_descriptor = descriptors()[0];

    Vector<vk::DescriptorSetLayout> layouts(
        VulkanSettings::max_frames_in_flight, global_descriptor->set_layout
    );
    vk::DescriptorSetAllocateInfo allocation_info {};
    allocation_info.setDescriptorPool(global_descriptor->pool);
    allocation_info.setSetLayouts(layouts);

    try {
        _global_descriptor_sets =
            _device->handle().allocateDescriptorSets(allocation_info);
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        // Combined descriptor
        std::array<vk::WriteDescriptorSet, 1> descriptor_writes {};

        vk::DescriptorBufferInfo buffer_info = {};
        buffer_info.setBuffer(global_descriptor->get_buffer(0, i)->handle);
        buffer_info.setOffset(0);
        buffer_info.setRange(sizeof(GlobalUBO));

        descriptor_writes[0].setDstSet(_global_descriptor_sets[i]);
        descriptor_writes[0].setDstBinding(0);
        descriptor_writes[0].setDstArrayElement(0);
        descriptor_writes[0].setDescriptorType(
            vk::DescriptorType::eUniformBuffer
        );
        descriptor_writes[0].setDescriptorCount(1);
        descriptor_writes[0].setPBufferInfo(&buffer_info);

        _device->handle().updateDescriptorSets(descriptor_writes, nullptr);
    }
}

uint32 VulkanUIShader::get_next_available_ubo_index() {
    static uint32 index = 0;
    return index++;
}