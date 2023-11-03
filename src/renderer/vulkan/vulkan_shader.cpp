#include "renderer/vulkan/vulkan_shader.hpp"

#include "systems/file_system.hpp"
#include "systems/texture_system.hpp"
#include "resources/datapack.hpp"

namespace ENGINE_NAMESPACE {

vk::SamplerAddressMode convert_repeat_type(const TextureRepeat repeat);
vk::Filter             convert_filter_type(const TextureFilter filter);

// Constructor & Destructor
VulkanShader::VulkanShader(
    const ShaderConfig                   config,
    const VulkanDevice* const            device,
    const vk::AllocationCallbacks* const allocator,
    const VulkanRenderPass* const        render_pass,
    const VulkanCommandBuffer* const     command_buffer
)
    : Shader(config), _device(device), _allocator(allocator),
      _render_pass(render_pass), _command_buffer(command_buffer) {

    // === Process shader config ===
    // Translate stage info to vulkan flags
    Vector<vk::ShaderStageFlagBits> shader_stages {};
    if (config.shader_stages & (uint8) ShaderStage::Vertex)
        shader_stages.push_back(vk::ShaderStageFlagBits::eVertex);
    if (config.shader_stages & (uint8) ShaderStage::Geometry)
        shader_stages.push_back(vk::ShaderStageFlagBits::eGeometry);
    if (config.shader_stages & (uint8) ShaderStage::Fragment)
        shader_stages.push_back(vk::ShaderStageFlagBits::eFragment);
    if (config.shader_stages & (uint8) ShaderStage::Compute)
        shader_stages.push_back(vk::ShaderStageFlagBits::eCompute);

    // Compute shader stage infos
    auto shader_stage_infos = compute_stage_infos(shader_stages);
    // Compute attributes
    auto attributes         = compute_attributes();
    // Get descriptor set configs from uniforms
    _descriptor_set_configs = compute_uniforms(shader_stages);

    // === Create Descriptor pool. ===
    // Compute pool sizes
    // For now, shaders will only ever have these 2 types of descriptor pools.
    std::array<vk::DescriptorPoolSize, 2> pool_sizes;

    // Calculate total number of samplers & non-local non-sampler uniforms used
    auto sampler_count = 0;
    auto uniform_count = 0;
    for (const auto& uniform : _uniforms) {
        if (uniform.type == ShaderUniformType::sampler) sampler_count++;
        else if (uniform.scope != ShaderScope::Local) uniform_count++;
    }

    // Assign
    auto instance_count =
        Shader::max_instance_count * VulkanSettings::max_frames_in_flight;
    pool_sizes[_bind_index_ubo].setType( //
        vk::DescriptorType::eUniformBuffer
    );
    pool_sizes[_bind_index_ubo].setDescriptorCount(
        uniform_count * instance_count
    );
    pool_sizes[_bind_index_sampler].setType(
        vk::DescriptorType::eCombinedImageSampler
    );
    pool_sizes[_bind_index_sampler].setDescriptorCount(
        sampler_count * instance_count
    );

    // Setup pool
    vk::DescriptorPoolCreateInfo pool_info {};
    pool_info.setPoolSizes(pool_sizes);
    pool_info.setMaxSets(VulkanShader::max_descriptor_sets);
    pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

    // Create pool
    try {
        _descriptor_pool =
            _device->handle().createDescriptorPool(pool_info, _allocator);
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    // === Create descriptor set layouts ===
    for (uint32 i = 0; i < _descriptor_set_configs.size(); ++i) {
        vk::DescriptorSetLayoutCreateInfo layout_info {};
        layout_info.setBindings(_descriptor_set_configs[i]->bindings);
        try {
            _descriptor_set_configs[i]->layout =
                _device->handle().createDescriptorSetLayout(
                    layout_info, _allocator
                );
        } catch (vk::SystemError e) {
            Logger::fatal(RENDERER_VULKAN_LOG, e.what());
        }
    }

    // === Vertex input state info ===
    // Vertex bindings
    Vector<vk::VertexInputBindingDescription> binding_descriptions(1);
    binding_descriptions[0].setBinding(0);
    binding_descriptions[0].setStride(_attribute_stride);
    binding_descriptions[0].setInputRate(vk::VertexInputRate::eVertex);

    vk::PipelineVertexInputStateCreateInfo vertex_input_info {};
    vertex_input_info.setVertexBindingDescriptions(binding_descriptions);
    vertex_input_info.setVertexAttributeDescriptions(attributes);

    // === Create pipeline ===
    create_pipeline(shader_stage_infos, vertex_input_info);

    // === Cleanup temp resources ===
    for (auto shader_stage_info : shader_stage_infos)
        _device->handle().destroyShaderModule(shader_stage_info.module);

    // === Compute required buffer alignment ===
    // Grab the UBO alignment requirement from the device.
    _required_ubo_alignment = _device->info().min_ubo_alignment;

    // Make sure the UBO is aligned according to device requirements.
    _global_ubo_stride = get_aligned(_global_ubo_size, _required_ubo_alignment);
    _ubo_stride        = get_aligned(_ubo_size, _required_ubo_alignment);

    // === Uniform  buffer ===
    vk::MemoryPropertyFlagBits device_local_bits {};
    if (_device->info().supports_device_local_host_visible_memory)
        device_local_bits = vk::MemoryPropertyFlagBits::eDeviceLocal;
    // TODO: max count should be configurable, or perhaps long term support of
    // buffer resizing.
    uint64 total_buffer_size = /* global + (locals) */
        _global_ubo_stride + (_ubo_stride * Shader::max_instance_count);
    _uniform_buffer = new VulkanManagedBuffer(_device, _allocator);
    _uniform_buffer->create(
        total_buffer_size,
        vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent | device_local_bits
    );

    // Allocate space for the global UBO, which should occupy the _stride_
    // space, _not_ the actual size used.
    _global_ubo_offset =
        _uniform_buffer->allocate(_global_ubo_size, _required_ubo_alignment);

    // Map the entire buffer's memory.
    _uniform_buffer_offset =
        (uint64) _uniform_buffer->lock_memory(0, VK_WHOLE_SIZE);

    // === Allocate global descriptor sets ===
    // One per frame. Global is always the first set.
    std::array<vk::DescriptorSetLayout, VulkanSettings::max_frames_in_flight>
        global_layouts;
    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++)
        global_layouts[i] =
            _descriptor_set_configs[_desc_set_index_global]->layout;

    vk::DescriptorSetAllocateInfo alloc_info {};
    alloc_info.setDescriptorPool(_descriptor_pool);
    alloc_info.setSetLayouts(global_layouts);
    try {
        _global_descriptor_sets =
            _device->handle().allocateDescriptorSets(alloc_info);
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }
}
VulkanShader::~VulkanShader() {
    // Ubo
    _uniform_buffer_offset = 0;
    _uniform_buffer->unlock_memory();
    delete _uniform_buffer;

    // Pipeline
    if (_pipeline) _device->handle().destroyPipeline(_pipeline, _allocator);
    if (_pipeline_layout)
        _device->handle().destroyPipelineLayout(_pipeline_layout, _allocator);

    // Instances
    for (auto instance_state : _instance_states) {
        if (instance_state) {
            auto instance_state_v = (VulkanInstanceState*) instance_state;
            _device->handle().freeDescriptorSets(
                _descriptor_pool, instance_state_v->descriptor_set
            );
            delete instance_state_v;
        }
    }
    _instance_states.clear();

    // Descriptors
    if (_descriptor_pool)
        _device->handle().destroyDescriptorPool(_descriptor_pool, _allocator);
    for (auto config : _descriptor_set_configs) {
        _device->handle().destroyDescriptorSetLayout(
            config->layout, _allocator
        );
        delete config;
    }
    _descriptor_set_configs.clear();
}

// //////////////////////////// //
// VULKAN SHADER PUBLIC METHODS //
// //////////////////////////// //

void VulkanShader::reload() {
    // === Process shader config ===
    // Translate stage info to vulkan flags
    Vector<vk::ShaderStageFlagBits> shader_stages {};
    shader_stages.push_back(vk::ShaderStageFlagBits::eVertex);
    shader_stages.push_back(vk::ShaderStageFlagBits::eFragment);

    // Compute shader stage infos
    auto shader_stage_infos = compute_stage_infos(shader_stages);
    // Compute attributes
    auto attributes         = compute_attributes();

    // === Vertex input state info ===
    // Vertex bindings
    Vector<vk::VertexInputBindingDescription> binding_descriptions(1);
    binding_descriptions[0].setBinding(0);
    binding_descriptions[0].setStride(_attribute_stride);
    binding_descriptions[0].setInputRate(vk::VertexInputRate::eVertex);

    vk::PipelineVertexInputStateCreateInfo vertex_input_info {};
    vertex_input_info.setVertexBindingDescriptions(binding_descriptions);
    vertex_input_info.setVertexAttributeDescriptions(attributes);

    // === Destroy previous pipeline info ===
    _device->handle().waitIdle();
    _device->handle().destroyPipelineLayout(_pipeline_layout);
    _device->handle().destroyPipeline(_pipeline);

    // === Create pipeline ===
    create_pipeline(shader_stage_infos, vertex_input_info);

    // === Cleanup temp resources ===
    for (auto shader_stage_info : shader_stage_infos)
        _device->handle().destroyShaderModule(shader_stage_info.module);
}

void VulkanShader::use() {
    _command_buffer->handle->bindPipeline(
        vk::PipelineBindPoint::eGraphics, _pipeline
    );
    _bound_ubo_offset = _global_ubo_offset;
}

void VulkanShader::bind_globals() { _bound_ubo_offset = _global_ubo_offset; }

void VulkanShader::bind_instance(const uint32 id) {
    if (id >= _instance_states.size() || _instance_states[id] == nullptr) {
        Logger::error(
            RENDERER_VULKAN_LOG,
            "Invalid instance id given for binding. Task impossible."
        );
        return;
    }
    _bound_instance_id = id;
    _bound_ubo_offset  = _instance_states[id]->offset;
}

void VulkanShader::apply_global() {
    vk::DescriptorSet& global_descriptor =
        _global_descriptor_sets[_command_buffer->current_frame];

    static Vector<vk::WriteDescriptorSet> descriptor_writes {};
    descriptor_writes.clear();

    // Apply UBO first
    vk::DescriptorBufferInfo buffer_info {};
    buffer_info.setBuffer(_uniform_buffer->handle);
    buffer_info.setOffset(_global_ubo_offset);
    buffer_info.setRange(_global_ubo_stride);

    // Update descriptor sets.
    vk::WriteDescriptorSet ubo_write {};
    ubo_write.setDstSet(global_descriptor);
    ubo_write.setDstBinding(_bind_index_ubo);
    ubo_write.setDescriptorType(vk::DescriptorType::eUniformBuffer);
    ubo_write.setDstArrayElement(0);
    ubo_write.setDescriptorCount(1);
    ubo_write.setPBufferInfo(&buffer_info);
    descriptor_writes.push_back(ubo_write);

    if (_descriptor_set_configs[_desc_set_index_global]->bindings.size() > 1) {
        // Iterate samplers.
        const auto& image_infos = get_image_infos(_global_texture_maps);

        vk::WriteDescriptorSet sampler_descriptor {};
        sampler_descriptor.setDstSet(global_descriptor);
        sampler_descriptor.setDstBinding(_bind_index_sampler);
        sampler_descriptor.setDescriptorType(
            vk::DescriptorType::eCombinedImageSampler
        );
        ubo_write.setDstArrayElement(0);
        sampler_descriptor.setImageInfo(image_infos);
        descriptor_writes.push_back(sampler_descriptor);
    }

    // Throws no exceptions
    _device->handle().updateDescriptorSets(descriptor_writes, nullptr);

    // Bind the global descriptor set to be updated.
    _command_buffer->handle->bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        _pipeline_layout,
        0,
        1,
        &global_descriptor,
        0,
        0
    );
}

void VulkanShader::apply_instance() {
    if (!_use_instances) {
        Logger::error(
            RENDERER_VULKAN_LOG, "This shader does not use instances."
        );
        return;
    }
    if (_instance_states[_bound_instance_id] == nullptr)
        Logger::fatal(RENDERER_VULKAN_LOG, "No instance is bound.");

    // Obtain instance data.
    const auto current_frame = _command_buffer->current_frame;
    auto       object_state =
        (VulkanInstanceState*) _instance_states[_bound_instance_id];
    auto& object_descriptor_set = object_state->descriptor_set[current_frame];
    auto& descriptor_set_id = object_state->descriptor_set_ids[current_frame];

    bool should_update =
        object_state->should_update || !descriptor_set_id.has_value();

    // Descriptor 0 - Uniform buffer
    // Only do this if the descriptor has not yet been updated.
    if (should_update) {
        static Vector<vk::WriteDescriptorSet> descriptor_writes {};
        descriptor_writes.clear();

        vk::DescriptorBufferInfo buffer_info {};
        buffer_info.setBuffer(_uniform_buffer->handle);
        buffer_info.setOffset(object_state->offset);
        buffer_info.setRange(_ubo_stride);

        vk::WriteDescriptorSet ubo_descriptor {};
        ubo_descriptor.setDstSet(object_descriptor_set);
        ubo_descriptor.setDstBinding(_bind_index_ubo);
        ubo_descriptor.setDescriptorType(vk::DescriptorType::eUniformBuffer);
        ubo_descriptor.setDescriptorCount(1);
        ubo_descriptor.setPBufferInfo(&buffer_info);

        descriptor_writes.push_back(ubo_descriptor);
        descriptor_set_id = 0; // TODO: Implement better id

        // Samplers will always be in the binding. If the binding count is less
        // than 2, there are no samplers.
        if (_descriptor_set_configs[_desc_set_index_instance]->bindings.size() >
            1) {
            // Iterate samplers.
            const auto& image_infos =
                get_image_infos(object_state->instance_texture_maps);

            vk::WriteDescriptorSet sampler_descriptor {};
            sampler_descriptor.setDstSet(object_descriptor_set);
            sampler_descriptor.setDstBinding(_bind_index_sampler);
            sampler_descriptor.setDescriptorType(
                vk::DescriptorType::eCombinedImageSampler
            );
            sampler_descriptor.setImageInfo(image_infos);
            descriptor_writes.push_back(sampler_descriptor);
        }

        // No throws
        if (descriptor_writes.size() > 0)
            _device->handle().updateDescriptorSets(descriptor_writes, nullptr);
        object_state->should_update = false;
    }

    // Bind the descriptor set to be updated, or in case the shader changed.
    // Bind the global descriptor set to be updated.
    _command_buffer->handle->bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        _pipeline_layout,
        1,
        1,
        &object_descriptor_set,
        0,
        0
    );
}

uint32 VulkanShader::acquire_instance_resources( //
    const Vector<TextureMap*>& maps
) {
    uint32 instance_id    = _instance_states.size();
    auto   instance_state = new (MemoryTag::Renderer) VulkanInstanceState();

    // Check if map count fits
    auto instance_texture_count =
        _descriptor_set_configs[_desc_set_index_instance]
            ->bindings[_bind_index_sampler]
            .descriptorCount;
    if (maps.size() < instance_texture_count)
        Logger::fatal(
            RENDERER_VULKAN_LOG,
            "Attempting to acquire internal shader resources, while not "
            "providing all required texture maps [",
            maps.size(),
            "/",
            instance_texture_count,
            "]."
        );
    if (maps.size() > instance_texture_count) {
        Logger::warning(
            RENDERER_VULKAN_LOG,
            "Too many texture maps provided for internal shader resource "
            "acquisition [",
            maps.size(),
            "/",
            instance_texture_count,
            "]. All additional texture maps will be ignored."
        );
    }

    // Initialize texture maps
    auto& texture_maps = instance_state->instance_texture_maps;
    texture_maps.resize(_instance_texture_count);
    for (uint32 i = 0; i < _instance_texture_count; i++) {
        texture_maps[i] = maps[i];
        if (!maps[i]->texture)
            texture_maps[i]->texture = _texture_system->default_texture;
    }

    // Allocate some space in the UBO - by the stride, not the size.
    instance_state->offset =
        _uniform_buffer->allocate(_ubo_stride, _required_ubo_alignment);

    // Allocate one descriptor set per frame in flight.
    std::array<vk::DescriptorSetLayout, VulkanSettings::max_frames_in_flight>
        layouts;
    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++)
        layouts[i] = _descriptor_set_configs[_desc_set_index_instance]->layout;

    vk::DescriptorSetAllocateInfo alloc_info {};
    alloc_info.setDescriptorPool(_descriptor_pool);
    alloc_info.setSetLayouts(layouts);

    try {
        auto result = _device->handle().allocateDescriptorSets(alloc_info);
        for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++)
            instance_state->descriptor_set[i] = result[i];
    } catch (const vk::SystemError& e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    _instance_states.push_back(instance_state);
    return instance_id;
}
void VulkanShader::release_instance_resources(uint32 instance_id) {
    if (instance_id >= _instance_states.size() ||
        _instance_states[instance_id] == nullptr) {
        Logger::warning(
            RENDERER_VULKAN_LOG,
            "Invalid instance id given for resource release. Nothing was done."
        );
        return;
    }

    auto instance_state = (VulkanInstanceState*) _instance_states[instance_id];

    // Wait for any pending operations using the descriptor set to finish.
    _device->handle().waitIdle();

    // Free 3 descriptor sets (one per frame)
    _device->handle().freeDescriptorSets(
        _descriptor_pool, instance_state->descriptor_set
    );

    _uniform_buffer->deallocate(instance_state->offset);

    delete instance_state;
    _instance_states[instance_id] = nullptr;
}

void VulkanShader::acquire_texture_map_resources(TextureMap* texture_map) {
    // TODO: Additional configurable settings
    // Create sampler
    vk::SamplerCreateInfo sampler_info {};
    sampler_info.setAddressModeU(convert_repeat_type(texture_map->repeat_u));
    sampler_info.setAddressModeV(convert_repeat_type(texture_map->repeat_v));
    sampler_info.setAddressModeW(convert_repeat_type(texture_map->repeat_w));
    sampler_info.setMagFilter(convert_filter_type(texture_map->filter_magnify));
    sampler_info.setMinFilter(convert_filter_type(texture_map->filter_minify));
    sampler_info.setAnisotropyEnable(true);
    sampler_info.setMaxAnisotropy(_device->info().max_sampler_anisotropy);
    sampler_info.setBorderColor(vk::BorderColor::eIntOpaqueBlack);
    sampler_info.setUnnormalizedCoordinates(false);
    sampler_info.setCompareEnable(false);
    sampler_info.setCompareOp(vk::CompareOp::eAlways);
    // Mipmap settings
    sampler_info.setMipmapMode(vk::SamplerMipmapMode::eLinear);
    sampler_info.setMipLodBias(0.0f);
    sampler_info.setMinLod(0.0f);
    sampler_info.setMaxLod(
        static_cast<float32>(texture_map->texture->mip_level_count)
    );

    vk::Sampler texture_sampler;
    try {
        texture_sampler =
            _device->handle().createSampler(sampler_info, _allocator);
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    const auto vulkan_texture_map_data =
        new (MemoryTag::GPUTexture) VulkanTextureMapData();
    vulkan_texture_map_data->sampler = texture_sampler;
    texture_map->internal_data       = vulkan_texture_map_data;
}
void VulkanShader::release_texture_map_resources(TextureMap* texture_map) {
    if (!texture_map) return;
    if (!texture_map->internal_data) return;
    auto data =
        reinterpret_cast<VulkanTextureMapData*>(texture_map->internal_data);
    if (data->sampler) _device->handle().destroySampler(data->sampler);
}

// /////////////////////////////// //
// VULKAN SHADER PROTECTED METHODS //
// /////////////////////////////// //

Outcome VulkanShader::set_uniform(const uint16 id, void* value) {
    auto uniform = _uniforms[id];

    // Bind scope
    if (_bound_scope != uniform.scope) {
        if (uniform.scope == ShaderScope::Global)
            _bound_ubo_offset = _global_ubo_offset;
        else if (uniform.scope == ShaderScope::Instance)
            _bound_ubo_offset = _instance_states[_bound_instance_id]->offset;
        _bound_scope = uniform.scope;
    }

    // Inform the need for uniform reapplication
    if (_bound_scope == ShaderScope::Instance)
        _instance_states[_bound_instance_id]->should_update = true;

    // If sampler
    if (uniform.type == ShaderUniformType::sampler) {
        if (uniform.scope == ShaderScope::Global)
            _global_texture_maps[uniform.location] = (TextureMap*) value;
        else
            _instance_states[_bound_instance_id]
                ->instance_texture_maps[uniform.location] = (TextureMap*) value;
        return Outcome::Successful;
    }

    // If other uniform
    if (uniform.scope == ShaderScope::Local) {
        _command_buffer->handle->pushConstants(
            _pipeline_layout,
            vk::ShaderStageFlagBits::eVertex |
                vk::ShaderStageFlagBits::eFragment, // TODO: Dynamic
            uniform.offset,
            uniform.size,
            value
        );
    } else {
        auto address =
            _uniform_buffer_offset + _bound_ubo_offset + uniform.offset;
        memcpy((void*) address, value, uniform.size);
    }
    return Outcome::Successful;
}

// ///////////////////////////// //
// VULKAN SHADER PRIVATE METHODS //
// ///////////////////////////// //

vk::ShaderModule VulkanShader::create_shader_module(
    const vk::ShaderStageFlagBits shader_stage
) const {
    // Process path
    const auto shader_file_ext =
        (shader_stage == vk::ShaderStageFlagBits::eVertex) ? "vert" : "frag";
    const auto shader_file_path = String::build(
        "../assets/shaders/bin/", _name, ".", shader_file_ext, ".spv"
    );

    // Load data
    const auto result = FileSystem::read_bytes(shader_file_path);
    if (result.has_error())
        Logger::fatal(RENDERER_VULKAN_LOG, result.error().what());
    const auto code = result.value();

    // Turns raw shader code into a shader module
    vk::ShaderModuleCreateInfo create_info {};
    create_info.setCodeSize(code.size());
    create_info.setPCode(reinterpret_cast<const uint32*>(code.data()));
    vk::ShaderModule shader_module;
    try {
        shader_module =
            _device->handle().createShaderModule(create_info, _allocator);
    } catch (const vk::SystemError& e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    return shader_module;
}

Vector<vk::PipelineShaderStageCreateInfo> VulkanShader::compute_stage_infos(
    const Vector<vk::ShaderStageFlagBits>& shader_stages
) const {
    Vector<vk::PipelineShaderStageCreateInfo> shader_stage_infos {};
    // Create a module for each stage.
    for (uint32 i = 0; i < shader_stages.size(); i++) {
        // Create module
        auto shader_module = create_shader_module(shader_stages[i]);

        // Add Stage
        shader_stage_infos.push_back({});
        shader_stage_infos[i].setStage(shader_stages[i]);
        // Shader module containing the code
        shader_stage_infos[i].setModule(shader_module);
        // Function to invoke as an entrypoint
        shader_stage_infos[i].setPName("main");
        // Initial shader constants
        shader_stage_infos[i].setPSpecializationInfo(nullptr);
    }
    return shader_stage_infos;
}

Vector<vk::VertexInputAttributeDescription> VulkanShader::compute_attributes(
) const {
    // Static lookup table for our types->Vulkan ones.
    static vk::Format* types = 0;
    static vk::Format  t[(uint8) ShaderAttributeType::COUNT];
    if (!types) {
        t[(uint8) ShaderAttributeType::int8]    = vk::Format::eR8Sint;
        t[(uint8) ShaderAttributeType::int16]   = vk::Format::eR16Sint;
        t[(uint8) ShaderAttributeType::int32]   = vk::Format::eR32Sint;
        t[(uint8) ShaderAttributeType::uint8]   = vk::Format::eR8Uint;
        t[(uint8) ShaderAttributeType::uint16]  = vk::Format::eR16Uint;
        t[(uint8) ShaderAttributeType::uint32]  = vk::Format::eR32Uint;
        t[(uint8) ShaderAttributeType::float32] = vk::Format::eR32Sfloat;
        t[(uint8) ShaderAttributeType::vec2]    = vk::Format::eR32G32Sfloat;
        t[(uint8) ShaderAttributeType::vec3]    = vk::Format::eR32G32B32Sfloat;
        t[(uint8) ShaderAttributeType::vec4] = vk::Format::eR32G32B32A32Sfloat;

        types = t;
    }

    // Process
    Vector<vk::VertexInputAttributeDescription> attributes(_attributes.size());
    uint32                                      offset = 0;
    for (uint32 i = 0; i < _attributes.size(); ++i) {
        // Setup the new attribute.
        attributes[i].setLocation(i);
        attributes[i].setBinding(0);
        attributes[i].setOffset(offset);
        attributes[i].setFormat(types[(uint8) _attributes[i].type]);

        // Add to the stride.
        offset += _attributes[i].size;
    }

    return attributes;
}

Vector<VulkanDescriptorSetConfig*> VulkanShader::compute_uniforms(
    const Vector<vk::ShaderStageFlagBits>& shader_stages
) const {
    Vector<VulkanDescriptorSetConfig*> desc_set_configs {};

    // === Compute stage flags mask ===
    vk::ShaderStageFlags stage_flags {};
    for (auto stage_flag : shader_stages)
        stage_flags |= stage_flag;

    // === Process uniforms ===
    // Global descriptor set config
    auto global_desc_set_config =
        new (MemoryTag::Renderer) VulkanDescriptorSetConfig();

    // UBO is always available and first.
    vk::DescriptorSetLayoutBinding global_ubo_binding {};
    global_ubo_binding.setBinding(_bind_index_ubo);
    global_ubo_binding.setDescriptorCount(1);
    global_ubo_binding.setDescriptorType(vk::DescriptorType::eUniformBuffer);
    global_ubo_binding.setStageFlags(stage_flags);
    global_desc_set_config->bindings.push_back(global_ubo_binding);

    desc_set_configs.push_back(global_desc_set_config);

    if (_use_instances) {
        // If using instances, add a second descriptor set.
        auto instance_desc_set_config =
            new (MemoryTag::Renderer) VulkanDescriptorSetConfig();

        // Add a UBO to it, as instances should always have one available.
        // NOTE: Might be a good idea to only add this if it is going to be
        // used...
        vk::DescriptorSetLayoutBinding instance_ubo_binding {};
        instance_ubo_binding.setBinding(_bind_index_ubo);
        instance_ubo_binding.setDescriptorCount(1);
        instance_ubo_binding.setDescriptorType(
            vk::DescriptorType::eUniformBuffer
        );
        instance_ubo_binding.setStageFlags(stage_flags);
        instance_desc_set_config->bindings.push_back(instance_ubo_binding);

        desc_set_configs.push_back(instance_desc_set_config);
    }

    // === Process samplers ===
    for (uint32 i = 0; i < _uniforms.size(); ++i) {
        // For samplers, the descriptor bindings need to be updated. Other types
        // of uniforms don't need anything to be done here.
        if (_uniforms[i].type == ShaderUniformType::sampler) {
            const uint32 set_index =
                (_uniforms[i].scope == ShaderScope::Global
                     ? _desc_set_index_global
                     : _desc_set_index_instance);
            auto desc_set_config = desc_set_configs[set_index];

            if (desc_set_config->bindings.size() < 2) {
                // There isn't a binding yet, meaning this is the first sampler
                // to be added. Create the binding with a single descriptor for
                // this sampler.
                // Always going to be the second one.
                vk::DescriptorSetLayoutBinding sampler_binding;
                sampler_binding.setBinding(_bind_index_sampler);
                // Default to 1, will increase with each sampler added to the
                // appropriate level.
                sampler_binding.setDescriptorCount(1);
                sampler_binding.setDescriptorType(
                    vk::DescriptorType::eCombinedImageSampler
                );
                sampler_binding.setStageFlags(stage_flags);
                desc_set_config->bindings.push_back(sampler_binding);
            } else {
                // There is already a binding for samplers, so just add a
                // descriptor to it. Take the current descriptor count as the
                // location and increment the number of descriptors.
                desc_set_config->bindings[_bind_index_sampler]
                    .descriptorCount++;
            }
        }
    }

    return desc_set_configs;
}

void VulkanShader::create_pipeline(
    const Vector<vk::PipelineShaderStageCreateInfo>& shader_stages,
    const vk::PipelineVertexInputStateCreateInfo&    vertex_input_info,
    const bool                                       is_wire_frame
) {
    Logger::trace(RENDERER_VULKAN_LOG, "Creating graphics pipeline.");

    // === Input assembly ===
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_info {};
    // What geometry will be drawn from the vertices. Possible values:
    //  - point list
    //  - line list
    //  - line strip
    //  - triangle list
    //  - triangle strip
    input_assembly_info.setTopology(vk::PrimitiveTopology::eTriangleList);
    // If enabled allows breakup of strip topologies (Used for line and triangle
    // strips)
    input_assembly_info.setPrimitiveRestartEnable(false);

    // === Viewport and scissors ===
    // Dynamically allocated, so nothing goes here
    vk::PipelineViewportStateCreateInfo viewport_state_info {};
    viewport_state_info.setViewportCount(1);
    viewport_state_info.setScissorCount(1);

    // === Rasterizer ===
    vk::PipelineRasterizationStateCreateInfo rasterization_info {};
    // Clamp values beyond far/near planes instead of discarding them (feature
    // required for enabling)
    rasterization_info.setDepthClampEnable(false);
    // Disable output to framebuffer (feature required for enabling)
    rasterization_info.setRasterizerDiscardEnable(false);
    // Determines how fragments are generated for geometry (feature required for
    // changing)
    rasterization_info.setPolygonMode(
        is_wire_frame ? vk::PolygonMode::eLine : vk::PolygonMode::eFill
    );
    // Line thickness (feature required for values above 1)
    rasterization_info.setLineWidth(1.0f);
    // Triangle face we dont want to render
    rasterization_info.setCullMode(vk::CullModeFlagBits::eBack);
    // Set vertex order of front-facing triangles
    rasterization_info.setFrontFace(vk::FrontFace::eCounterClockwise);
    // Change depth information in some manner (used for shadow mapping)
    rasterization_info.setDepthBiasEnable(false);
    rasterization_info.setDepthBiasConstantFactor(0.0f);
    rasterization_info.setDepthBiasClamp(0.0f);
    rasterization_info.setDepthBiasSlopeFactor(0.0f);

    // === Multisampling ===
    auto multisampling_enabled =
        _render_pass->sample_count != vk::SampleCountFlagBits::e1;
    vk::PipelineMultisampleStateCreateInfo multisampling_info {};
    // Number of samples used for multisampling
    multisampling_info.setRasterizationSamples(_render_pass->sample_count);
    // Is sample shading enabled
    multisampling_info.setSampleShadingEnable(multisampling_enabled);
    // Min fraction of samples used for sample shading; closer to one is
    // smoother
    multisampling_info.setMinSampleShading(
        (multisampling_enabled) ? 0.2f : 1.0f
    );
    // Sample mask test
    multisampling_info.setPSampleMask(nullptr);
    multisampling_info.setAlphaToCoverageEnable(false);
    multisampling_info.setAlphaToOneEnable(false);

    // === Depth and stencil testing ===
    vk::PipelineDepthStencilStateCreateInfo depth_stencil {};
    if (_render_pass->depth_testing()) {
        // Should depth testing be preformed
        depth_stencil.setDepthTestEnable(true);
        // Should depth buffer be updated with new depth values (depth values of
        // fragments that passed the depth test)
        depth_stencil.setDepthWriteEnable(true);
        // Comparison operation preformed for depth test
        depth_stencil.setDepthCompareOp(vk::CompareOp::eLess);
        // Should depth bods test be preformed
        depth_stencil.setDepthBoundsTestEnable(false);
        // Minimum non-discarded fragment depth value
        depth_stencil.setMinDepthBounds(0.0f);
        // Maximum non-discarded fragment depth value
        depth_stencil.setMaxDepthBounds(1.0f);
        // Stencil buffer operations (here disabled)
        depth_stencil.setStencilTestEnable(false);
        depth_stencil.setFront({});
        depth_stencil.setBack({});
    }

    // === Color blending ===
    std::array<vk::PipelineColorBlendAttachmentState, 1>
        color_blend_attachments;
    // Controls whether blending is enabled for the corresponding color
    // attachment If blending is not enabled, the source fragment’s color is
    // passed through unmodified.
    color_blend_attachments[0].setBlendEnable(true);
    // Specifies which RGBA components are enabled for blending
    color_blend_attachments[0].setColorWriteMask(
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    );
    // Color blend options
    // Factor multiplied with source RGB
    color_blend_attachments[0].setSrcColorBlendFactor( //
        vk::BlendFactor::eSrcAlpha
    );
    // Factor multiplied with destination RGB
    color_blend_attachments[0].setDstColorBlendFactor(
        vk::BlendFactor::eOneMinusSrcAlpha
    );
    // Blend operation used for calculating RGB of color attachment
    color_blend_attachments[0].setColorBlendOp(vk::BlendOp::eAdd);
    // Alpha blend option
    // Factor multiplied with source alpha
    color_blend_attachments[0].setSrcAlphaBlendFactor( //
        vk::BlendFactor::eSrcAlpha
    );
    // Factor multiplied with destination alpha
    color_blend_attachments[0].setDstAlphaBlendFactor(
        vk::BlendFactor::eOneMinusSrcAlpha
    );
    // Blend operation used for calculating alpha of color attachment
    color_blend_attachments[0].setAlphaBlendOp(vk::BlendOp::eAdd);

    vk::PipelineColorBlendStateCreateInfo color_blend_state_info {};
    color_blend_state_info.setAttachments(color_blend_attachments);
    // Should Logical Op be applied
    color_blend_state_info.setLogicOpEnable(false);
    // Which Op to apply if enabled
    color_blend_state_info.setLogicOp(vk::LogicOp::eCopy);

    // === Dynamic state ===
    std::array<vk::DynamicState, 3> dynamic_states {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
        vk::DynamicState::eLineWidth
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state_info {};
    // Pipeline information to be set dynamically at draw time
    dynamic_state_info.setDynamicStates(dynamic_states);

    // === Push constant ranges ===
    // NOTE: 32 is the max number of ranges we can ever have, since spec only
    // guarantees 128 bytes with 4-byte alignment.
    if (_push_constant_ranges.size() > _push_constant_stride / 4)
        Logger::fatal(
            RENDERER_VULKAN_LOG,
            "Vulkan graphics pipeline cannot have more than ",
            _push_constant_stride / 4,
            " push constant ranges. Passed count: ",
            _push_constant_ranges.size(),
            "."
        );

    Vector<vk::PushConstantRange> ranges {};
    for (uint32 i = 0; i < _push_constant_ranges.size(); i++) {
        ranges.push_back({});
        ranges[i].setStageFlags(
            vk::ShaderStageFlagBits::eVertex |
            vk::ShaderStageFlagBits::eFragment // TODO: Dynamic
        );
        ranges[i].setOffset(_push_constant_ranges[i].offset);
        ranges[i].setSize(_push_constant_ranges[i].size);
    }

    // === Create pipeline layout ===
    Vector<vk::DescriptorSetLayout> descriptor_set_layouts {
        _descriptor_set_configs.size()
    };
    for (uint32 i = 0; i < _descriptor_set_configs.size(); i++)
        descriptor_set_layouts[i] = _descriptor_set_configs[i]->layout;

    vk::PipelineLayoutCreateInfo layout_info {};
    // Layout of used descriptor sets
    layout_info.setSetLayouts(descriptor_set_layouts);
    // Push constant ranges used
    layout_info.setPushConstantRanges(ranges);

    try {
        _pipeline_layout =
            _device->handle().createPipelineLayout(layout_info, _allocator);
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    // === Create pipeline object ===
    vk::GraphicsPipelineCreateInfo create_info {};
    // Programable pipeline stages
    create_info.setStages(shader_stages);
    // Fixed-function stages
    create_info.setPVertexInputState(&vertex_input_info);
    create_info.setPInputAssemblyState(&input_assembly_info);
    create_info.setPViewportState(&viewport_state_info);
    create_info.setPRasterizationState(&rasterization_info);
    create_info.setPMultisampleState(&multisampling_info);
    create_info.setPDepthStencilState(&depth_stencil);
    create_info.setPColorBlendState(&color_blend_state_info);
    create_info.setPDynamicState(&dynamic_state_info);
    create_info.setPTessellationState(nullptr); // TODO: Use tessellation
    // Pipeline layout handle
    create_info.setLayout(_pipeline_layout);
    // Render passes
    create_info.setRenderPass(_render_pass->handle());
    create_info.setSubpass(0); // The index of the subpass in the render pass
                               // where this pipeline will be used
    // Pipeline derivation
    create_info.setBasePipelineHandle(VK_NULL_HANDLE);
    create_info.setBasePipelineIndex(-1);

    try {
        auto result = _device->handle().createGraphicsPipeline(
            VK_NULL_HANDLE, create_info, _allocator
        );
        if (result.result != vk::Result::eSuccess)
            Logger::fatal(
                RENDERER_VULKAN_LOG,
                "Failed to create graphics pipeline (Unexpected compilation)."
            );
        _pipeline = result.value;
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    Logger::trace(RENDERER_VULKAN_LOG, "Graphics pipeline created.");
}

Vector<vk::DescriptorImageInfo>& VulkanShader::get_image_infos(
    const Vector<TextureMap*>& texture_maps
) const {
    static Vector<vk::DescriptorImageInfo> image_infos {};
    image_infos.clear();

    for (uint32 i = 0; i < texture_maps.size(); ++i) {
        // TODO: only update in the list if actually needing an update.
        const auto tm = texture_maps[i];

        VulkanTextureData* t_data =
            (VulkanTextureData*) tm->texture->internal_data();
        VulkanTextureMapData* tm_data =
            (VulkanTextureMapData*) tm->internal_data;

        vk::DescriptorImageInfo image_info {};
        image_info.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        image_info.setImageView(t_data->image->view);
        image_info.setSampler(tm_data->sampler);
        image_infos.push_back(image_info);

        // TODO: change up descriptor state to handle this properly.
        // Sync frame generation if not using a default texture.
        // if (t->generation != INVALID_ID) {
        //     *descriptor_generation = t->generation;
        //     *descriptor_id = t->id;
        // }
    }

    return image_infos;
}

// ////////////////////////////// //
// VULKAN SHADER HELPER FUNCTIONS //
// ////////////////////////////// //

vk::SamplerAddressMode convert_repeat_type(const TextureRepeat repeat) {
    switch (repeat) {
    case TextureRepeat::Repeat: //
        return vk::SamplerAddressMode::eRepeat;
    case TextureRepeat::MirroredRepeat:
        return vk::SamplerAddressMode::eMirroredRepeat;
    case TextureRepeat::ClampToEdge:
        return vk::SamplerAddressMode::eClampToEdge;
    case TextureRepeat::ClampToBorder:
        return vk::SamplerAddressMode::eClampToBorder;
    default:
        Logger::warning(
            RENDERER_VULKAN_LOG,
            "Conversion of repeat type ",
            (int32) repeat,
            " not supported. Function `convert_repeat_type` will default to "
            "repeat."
        );
        return vk::SamplerAddressMode::eRepeat;
    }
}

vk::Filter convert_filter_type(const TextureFilter filter) {
    switch (filter) {
    case TextureFilter::NearestNeighbour: return vk::Filter::eNearest;
    case TextureFilter::BiLinear: return vk::Filter::eLinear;
    default:
        Logger::warning(
            RENDERER_VULKAN_LOG,
            "Conversion of filter mode ",
            (int32) filter,
            " not supported. Function `convert_filter_type` will default to "
            "linear."
        );
        return vk::Filter::eLinear;
    }
}

} // namespace ENGINE_NAMESPACE