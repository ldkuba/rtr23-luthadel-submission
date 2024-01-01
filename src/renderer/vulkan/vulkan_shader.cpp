#include "renderer/vulkan/vulkan_shader.hpp"

#include "systems/file_system.hpp"
#include "systems/texture_system.hpp"
#include "resources/datapack.hpp"

namespace ENGINE_NAMESPACE {

vk::DescriptorType   get_descriptor_type(Shader::Binding::Type type);
vk::ShaderStageFlags get_shader_stages(uint8 stages);

// Constructor & Destructor
VulkanShader::VulkanShader(
    TextureSystem* const                 texture_system,
    const Config&                        config,
    const VulkanDevice* const            device,
    const vk::AllocationCallbacks* const allocator,
    const VulkanRenderPass* const        render_pass,
    const VulkanCommandBuffer* const     command_buffer
)
    : Shader(texture_system, config), _device(device), _allocator(allocator),
      _render_pass(render_pass), _command_buffer(command_buffer) {

    // === Process shader config ===
    // Translate stage info to vulkan flags
    Vector<vk::ShaderStageFlagBits> shader_stages {};
    if (config.shader_stages & (uint8) Stage::Vertex)
        shader_stages.push_back(vk::ShaderStageFlagBits::eVertex);
    if (config.shader_stages & (uint8) Stage::Geometry)
        shader_stages.push_back(vk::ShaderStageFlagBits::eGeometry);
    if (config.shader_stages & (uint8) Stage::Fragment)
        shader_stages.push_back(vk::ShaderStageFlagBits::eFragment);
    if (config.shader_stages & (uint8) Stage::Compute)
        shader_stages.push_back(vk::ShaderStageFlagBits::eCompute);

    // Grab the UBO alignment requirement from the device.
    _required_ubo_alignment = _device->info().min_ubo_alignment;

    // Compute the buffer offsets of bindings and uniforms with required
    // alignment.
    for (auto& set : _descriptor_sets) {
        for (auto& binding : set.bindings) {
            if (binding.type == Binding::Type::Sampler) continue;

            // Fill in missing uniform offsets
            binding.total_size = 0;
            for (size_t uniform_id : binding.uniforms) {
                auto& uniform             = _uniforms[uniform_id];
                uniform.byte_range.offset = set.total_size + binding.total_size;
                binding.total_size += uniform.byte_range.size;
            }

            binding.byte_range.offset = set.total_size;
            binding.byte_range.size =
                get_aligned(binding.total_size, _required_ubo_alignment);

            set.total_size += binding.byte_range.size;
        }

        set.stride = get_aligned(set.total_size, _required_ubo_alignment);
    }

    // Compute shader stage infos
    auto shader_stage_infos = compute_stage_infos(shader_stages);
    // Compute attributes
    auto attributes         = compute_attributes();
    // Set descriptor set configs from uniforms
    compute_uniforms(shader_stages);

    // === Create Descriptor pool. ===
    // Compute pool sizes
    // For now, shaders will only ever have these 3 types of descriptor pools.
    std::array<vk::DescriptorPoolSize, 3> pool_sizes;

    // Calculate total number of samplers & non-local non-sampler uniforms used
    auto sampler_count = 0;
    auto uniform_count = 0;
    auto storage_count = 0;
    for (const auto& uniform : _uniforms) {
        if (uniform.scope == Scope::Local) continue;
        auto* binding = get_binding(uniform.set_index, uniform.binding_index);

        if (binding->type == Binding::Type::Uniform) uniform_count++;
        else if (binding->type == Binding::Type::Sampler) sampler_count++;
        else if (binding->type == Binding::Type::Storage) storage_count++;
    }

    // TODO: Hack, pool sized cannot be 0
    sampler_count = sampler_count == 0 ? 1 : sampler_count;
    uniform_count = uniform_count == 0 ? 1 : uniform_count;
    storage_count = storage_count == 0 ? 1 : storage_count;

    // Assign
    auto instance_count =
        Shader::max_instance_count * VulkanSettings::max_frames_in_flight;
    pool_sizes[0].setType(vk::DescriptorType::eUniformBuffer);
    pool_sizes[0].setDescriptorCount(uniform_count * instance_count);
    pool_sizes[1].setType(vk::DescriptorType::eCombinedImageSampler);
    pool_sizes[1].setDescriptorCount(sampler_count * instance_count);
    pool_sizes[2].setType(vk::DescriptorType::eStorageBuffer);
    pool_sizes[2].setDescriptorCount(storage_count * instance_count);

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
    for (auto& set : _descriptor_sets) {
        const auto backend_data =
            static_cast<VulkanDescriptorSetBackendData*>(set.backend_data);
        if (backend_data == nullptr)
            Logger::fatal(
                RENDERER_VULKAN_LOG,
                "No backend data found for descriptor set. This should never "
                "happen."
            );

        vk::DescriptorSetLayoutCreateInfo layout_info {};
        layout_info.setBindings(backend_data->vulkan_bindings);
        try {
            backend_data->layout = _device->handle().createDescriptorSetLayout(
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

    // === Uniform  buffer ===
    vk::MemoryPropertyFlagBits device_local_bits {};
    if (_device->info().supports_device_local_host_visible_memory)
        device_local_bits = vk::MemoryPropertyFlagBits::eDeviceLocal;
    // TODO: max count should be configurable, or perhaps long term support of
    // buffer resizing.
    uint64 total_global_buffers_size   = 0;
    uint64 total_instance_buffers_size = 0;
    for (auto& set : _descriptor_sets) {
        if (set.scope == Scope::Global) {
            total_global_buffers_size += set.stride;
        } else if (set.scope == Scope::Instance) {
            total_instance_buffers_size += set.stride;
        }
    }
    uint64 total_buffer_size =
        total_global_buffers_size +
        total_instance_buffers_size * Shader::max_instance_count;
    if (total_buffer_size == 0) total_buffer_size = _required_ubo_alignment;

    _uniform_buffer =
        new (MemoryTag::GPUBuffer) VulkanManagedBuffer(_device, _allocator);
    _uniform_buffer->create(
        total_buffer_size,
        vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent | device_local_bits
    );

    // Allocate space for the global UBO, which should occupy the _stride_
    // space, _not_ the actual size used.
    for (auto& set : _descriptor_sets) {
        if (set.scope == Scope::Global) {
            VulkanDescriptorSetState* global_state =
                new (MemoryTag::Shader) VulkanDescriptorSetState();
            global_state->offset =
                _uniform_buffer->allocate(set.stride, _required_ubo_alignment);
            set.states.push_back(global_state);
        }
    }

    // Map the entire buffer's memory.
    _uniform_buffer_offset =
        (uint64) _uniform_buffer->lock_memory(0, VK_WHOLE_SIZE);

    // === Acquire global resources ===
    acquire_global_resources();

    // === Allocate global descriptor sets ===
    for (auto& set : _descriptor_sets) {
        if (set.scope != Scope::Global) continue;
        // One per frame. Global is always the first set.
        std::
            array<vk::DescriptorSetLayout, VulkanSettings::max_frames_in_flight>
                   global_layouts;
        const auto backend_data =
            static_cast<VulkanDescriptorSetBackendData*>(set.backend_data);
        for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
            global_layouts[i] = backend_data->layout;
        }

        vk::DescriptorSetAllocateInfo alloc_info {};
        alloc_info.setDescriptorPool(_descriptor_pool);
        alloc_info.setSetLayouts(global_layouts);

        try {
            auto result = _device->handle().allocateDescriptorSets(alloc_info);
            auto state  = static_cast<VulkanDescriptorSetState*>(set.states[0]);
            for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++)
                state->descriptor_set[i] = result[i];
        } catch (const vk::SystemError& e) {
            Logger::fatal(RENDERER_VULKAN_LOG, e.what());
        }
    }
}

VulkanShader::~VulkanShader() {
    // Ubo
    _uniform_buffer_offset = 0;
    _uniform_buffer->unlock_memory();
    del(_uniform_buffer);

    // Pipeline
    if (_pipeline) _device->handle().destroyPipeline(_pipeline, _allocator);
    if (_pipeline_layout)
        _device->handle().destroyPipelineLayout(_pipeline_layout, _allocator);

    // Global texture maps
    release_global_resources();

    // Descriptor states
    for (auto& set : _descriptor_sets) {
        for (auto& state : set.states) {
            if (state) {
                auto state_v = static_cast<VulkanDescriptorSetState*>(state);
                _device->handle().freeDescriptorSets(
                    _descriptor_pool, state_v->descriptor_set
                );
                del(state_v);
            }
        }
        set.states.clear();
    }

    // Descriptor pools
    if (_descriptor_pool)
        _device->handle().destroyDescriptorPool(_descriptor_pool, _allocator);

    // Descriptor backend data
    for (auto& set : _descriptor_sets) {
        auto backend_data =
            static_cast<VulkanDescriptorSetBackendData*>(set.backend_data);
        _device->handle().destroyDescriptorSetLayout(
            backend_data->layout, _allocator
        );
        del(backend_data);
    }
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
}

void VulkanShader::apply_descriptor_set(DescriptorSet& set, uint32 state_id) {
    const auto state =
        static_cast<VulkanDescriptorSetState*>(set.states[state_id]);
    auto& descriptor_set_id =
        state->descriptor_set_ids[_command_buffer->current_frame];
    auto vk_descriptor_set =
        state->descriptor_set[_command_buffer->current_frame];

    // Update only if necessary or if the descriptor set is not set yet
    bool should_update = state->should_update || !descriptor_set_id.has_value();

    if (should_update) {
        // All descriptor writes for this set
        static Vector<vk::WriteDescriptorSet> descriptor_writes {};
        descriptor_writes.clear();

        // Keep track of temporary allocated objects
        Vector<std::pair<vk::DescriptorBufferInfo*, vk::DescriptorImageInfo*>>
            allocated_infos {};

        // Iterate bindings
        for (auto& binding : set.bindings) {
            // descriptor_set_id is a hack for initializing all frames in flight
            if (!binding.was_modified && descriptor_set_id.has_value())
                continue;

            // Add new write
            vk::WriteDescriptorSet binding_write {};
            binding_write //
                .setDstSet(vk_descriptor_set)
                .setDstBinding(binding.binding_index)
                .setDescriptorType(get_descriptor_type(binding.type))
                .setDstArrayElement(0)
                .setDescriptorCount(binding.count);

            // Samplers & other uniforms treated differently
            if (binding.type == Binding::Type::Sampler) {
                const auto& texture_maps =
                    state->texture_maps[binding.binding_index];

                // Allocate temporary info for initialization
                const auto image_infos = new (MemoryTag::Temp)
                    vk::DescriptorImageInfo[texture_maps.size()];

                // Keep track of these objects
                allocated_infos.push_back({ nullptr, image_infos });
                binding_write.setPImageInfo(image_infos);

                // Set all image infos
                for (uint32 i = 0; i < texture_maps.size(); ++i) {
                    // TODO: only update in the list if actually needing an
                    // update.
                    const auto tm = static_cast<const VulkanTexture::Map*>(
                        texture_maps[i] ? texture_maps[i]
                                        : _texture_system->default_map
                    );
                    auto texture =
                        static_cast<const VulkanTexture*>(tm->texture);

                    if (tm->texture->is_render_target()) {
                        const auto pt =
                            static_cast<const PackedTexture*>(tm->texture);
                        texture = static_cast<const VulkanTexture*>(
                            pt->get_at(_command_buffer->current_frame)
                        );
                    }

                    image_infos[i]
                        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                        .setImageView(texture->image()->view)
                        .setSampler(tm->sampler);

                    // TODO: change up descriptor state to handle this properly.
                    // Sync frame generation if not using a default texture.
                    // if (t->generation != INVALID_ID) {
                    //     *descriptor_generation = t->generation;
                    //     *descriptor_id = t->id;
                    // }
                }
            } else /* Uniform or storage buffer */ {
                // Allocate temporary info for initialization
                const auto buffer_info =
                    new (MemoryTag::Temp) vk::DescriptorBufferInfo {};

                // Keep track of these objects
                allocated_infos.push_back({ buffer_info, nullptr });
                binding_write.setPBufferInfo(buffer_info);

                // Set values
                buffer_info->setBuffer(_uniform_buffer->handle())
                    .setOffset(state->offset + binding.byte_range.offset)
                    .setRange(binding.byte_range.size);
            }

            // Add this write
            descriptor_writes.push_back(binding_write);
            binding.was_modified = false;
        }

        state->should_update = false;
        descriptor_set_id    = 884;

        // Preform update
        // Throws no exceptions
        if (descriptor_writes.size() > 0)
            _device->handle().updateDescriptorSets(descriptor_writes, nullptr);

        // Free temporary memory
        if (!allocated_infos.empty()) {
            if (allocated_infos[0].first) del(allocated_infos[0].first);
            if (allocated_infos[0].second) del(allocated_infos[0].second);
        }
    }

    // Bind the global descriptor set to be updated.
    _command_buffer->handle->bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        _pipeline_layout,
        set.set_index,
        1,
        &vk_descriptor_set,
        0,
        0
    );
}

void VulkanShader::apply_global() {
    for (auto& set : _descriptor_sets) {
        if (set.scope != Scope::Global) continue;

        apply_descriptor_set(set, 0);
    }
}

void VulkanShader::apply_instance() {
    for (auto& set : _descriptor_sets) {
        if (set.scope != Scope::Instance) continue;

        apply_descriptor_set(set, _bound_instance_id);
    }
}

uint32 VulkanShader::acquire_instance_resources( //
    const Vector<Texture::Map*>& maps
) {
    uint32 instance_id = -1;
    for (auto& set : _descriptor_sets) {
        if (set.scope != Scope::Instance) continue;

        // All state vector should be same size
        if (instance_id == -1) instance_id = set.states.size();

        // Create instance state for this descriptor set
        auto instance_state =
            new (MemoryTag::Shader) VulkanDescriptorSetState();

        // === Samplers ===
        auto& texture_maps = instance_state->texture_maps;

        // Count texture maps in bindings
        uint32 instance_texture_count = 0;
        for (auto& binding : set.bindings) {
            if (binding.type != Binding::Type::Sampler) continue;

            // Validate if too little maps passed
            if (maps.size() < instance_texture_count + binding.count) {
                Logger::fatal(
                    RENDERER_VULKAN_LOG,
                    "Not enough texture maps provided for shader instance."
                );
            }

            // Initialize texture maps
            texture_maps[binding.binding_index] =
                Vector<Texture::Map*>(binding.count);
            auto& binding_maps = texture_maps[binding.binding_index];
            for (uint32 i = 0; i < binding.count; i++) {
                // Get map
                const auto& map = maps[instance_texture_count + i];

                // Set data
                binding_maps[i] = map;
                if (!map->texture)
                    binding_maps[i]->texture = _texture_system->default_texture;
            }

            // Update total instance count
            instance_texture_count += binding.count;
        }

        // Validate if too many maps passed
        if (maps.size() > instance_texture_count) {
            Logger::error(
                RENDERER_VULKAN_LOG,
                "Too many texture maps provided for shader instance."
            );
        }

        // === Uniforms ===
        // Allocate space in the UBO
        if (set.stride > 0) {
            instance_state->offset =
                _uniform_buffer->allocate(set.stride, _required_ubo_alignment);
        }

        // Allocate one descriptor set per frame in flight.
        std::array< //
            vk::DescriptorSetLayout,
            VulkanSettings::max_frames_in_flight>
            layouts;

        auto* backend_data =
            static_cast<VulkanDescriptorSetBackendData*>(set.backend_data);
        for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++)
            layouts[i] = backend_data->layout;

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

        set.states.push_back(instance_state);
    }

    if (instance_id == -1) {
        Logger::fatal(
            RENDERER_VULKAN_LOG,
            "Attempting to acquire instance shader resources, for shader with "
            "no instance descriptor sets."
        );
    }

    return instance_id;
}
void VulkanShader::release_instance_resources(uint32 instance_id) {
    // Wait for any pending operations using the descriptor set to finish.
    _device->handle().waitIdle();

    for (auto& set : _descriptor_sets) {
        if (set.scope != Scope::Instance) continue;

        auto instance_state =
            static_cast<VulkanDescriptorSetState*>(set.states[instance_id]);

        _device->handle().freeDescriptorSets(
            _descriptor_pool, instance_state->descriptor_set
        );

        _uniform_buffer->deallocate(instance_state->offset);

        del(instance_state);
        set.states[instance_id] = nullptr;
    }
}

// /////////////////////////////// //
// VULKAN SHADER PROTECTED METHODS //
// /////////////////////////////// //

Outcome VulkanShader::set_uniform(const uint16 id, void* value) {
    auto uniform = _uniforms[id];

    if (uniform.scope == Shader::Scope::Local) {
        _command_buffer->handle->pushConstants(
            _pipeline_layout,
            vk::ShaderStageFlagBits::eVertex |
                vk::ShaderStageFlagBits::eFragment, // TODO: Dynamic
            uniform.byte_range.offset,
            uniform.byte_range.size,
            value
        );
        return Outcome::Successful;
    }

    // Get binding, set and state
    auto* binding = get_binding(uniform.set_index, uniform.binding_index);
    auto& set     = _descriptor_sets[uniform.set_index];

    uint32 state_id = 0;
    if (set.scope == Scope::Global) {
        state_id = 0;
    } else if (set.scope == Scope::Instance) {
        state_id = _bound_instance_id;
    }
    auto state = static_cast<VulkanDescriptorSetState*>(set.states[state_id]);

    if (state == nullptr)
        Logger::fatal(
            RENDERER_VULKAN_LOG, "No descriptor set state found for uniform."
        );

    // Inform the need for uniform reapplication
    binding->was_modified = true;

    // If sampler
    if (uniform.type == UniformType::sampler) {
        state->texture_maps[binding->binding_index][uniform.array_index] =
            (Texture::Map*) value;
        state->should_update = true;
    } else {
        auto address =
            _uniform_buffer_offset + state->offset + uniform.byte_range.offset;
        memcpy((void*) address, value, uniform.byte_range.size);
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
    static vk::Format  t[(uint8) AttributeType::COUNT];
    if (!types) {
        t[(uint8) AttributeType::int8]    = vk::Format::eR8Sint;
        t[(uint8) AttributeType::int16]   = vk::Format::eR16Sint;
        t[(uint8) AttributeType::int32]   = vk::Format::eR32Sint;
        t[(uint8) AttributeType::uint8]   = vk::Format::eR8Uint;
        t[(uint8) AttributeType::uint16]  = vk::Format::eR16Uint;
        t[(uint8) AttributeType::uint32]  = vk::Format::eR32Uint;
        t[(uint8) AttributeType::float32] = vk::Format::eR32Sfloat;
        t[(uint8) AttributeType::vec2]    = vk::Format::eR32G32Sfloat;
        t[(uint8) AttributeType::vec3]    = vk::Format::eR32G32B32Sfloat;
        t[(uint8) AttributeType::vec4]    = vk::Format::eR32G32B32A32Sfloat;

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

void VulkanShader::compute_uniforms(
    const Vector<vk::ShaderStageFlagBits>& shader_stages
) {
    for (auto& set : _descriptor_sets) {
        auto desc_set_backend_data =
            new (MemoryTag::Shader) VulkanDescriptorSetBackendData();

        for (const auto& binding : set.bindings) {
            vk::DescriptorType   type = get_descriptor_type(binding.type);
            vk::ShaderStageFlags binding_stages =
                get_shader_stages(binding.shader_stages);

            vk::DescriptorSetLayoutBinding vulkan_binding {};
            vulkan_binding.setBinding(binding.binding_index);
            vulkan_binding.setDescriptorCount(binding.count);
            vulkan_binding.setDescriptorType(type);
            vulkan_binding.setStageFlags(binding_stages);
            desc_set_backend_data->vulkan_bindings.push_back(vulkan_binding);
        }
        set.backend_data = desc_set_backend_data;
    }
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
    // If enabled allows breakup of strip topologies (Used for line and
    // triangle strips)
    input_assembly_info.setPrimitiveRestartEnable(false);

    // === Viewport and scissors ===
    // Dynamically allocated, so nothing goes here
    vk::PipelineViewportStateCreateInfo viewport_state_info {};
    viewport_state_info.setViewportCount(1);
    viewport_state_info.setScissorCount(1);

    // === Rasterizer ===
    vk::PipelineRasterizationStateCreateInfo rasterization_info {};
    // Clamp values beyond far/near planes instead of discarding them
    // (feature required for enabling)
    rasterization_info.setDepthClampEnable(false);
    // Disable output to framebuffer (feature required for enabling)
    rasterization_info.setRasterizerDiscardEnable(false);
    // Determines how fragments are generated for geometry (feature required
    // for changing)
    rasterization_info.setPolygonMode(
        is_wire_frame ? vk::PolygonMode::eLine : vk::PolygonMode::eFill
    );
    // Line thickness (feature required for values above 1)
    rasterization_info.setLineWidth(1.0f);
    // Triangle face we dont want to render
    switch (_cull_mode) {
    case Shader::CullMode::None:
        rasterization_info.setCullMode(vk::CullModeFlagBits::eNone);
        break;
    case Shader::CullMode::Front:
        rasterization_info.setCullMode(vk::CullModeFlagBits::eFront);
        break;
    case Shader::CullMode::Back:
        rasterization_info.setCullMode(vk::CullModeFlagBits::eBack);
        break;
    case Shader::CullMode::Both:
        rasterization_info.setCullMode(vk::CullModeFlagBits::eFrontAndBack);
        break;
    }
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
        // Should depth buffer be updated with new depth values (depth
        // values of fragments that passed the depth test)
        depth_stencil.setDepthWriteEnable(true);
        // Comparison operation preformed for depth test
        depth_stencil.setDepthCompareOp(vk::CompareOp::eLess);
        // Should depth bounds test be preformed
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
    color_blend_attachments[0].setBlendEnable(_enable_blending);
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
    // NOTE: 32 is the max number of ranges we can ever have, since spec
    // only guarantees 128 bytes with 4-byte alignment.
    if (_push_constants.size() > _push_constant_stride / 4)
        Logger::fatal(
            RENDERER_VULKAN_LOG,
            "Vulkan graphics pipeline cannot have more than ",
            _push_constant_stride / 4,
            " push constant ranges. Passed count: ",
            _push_constants.size(),
            "."
        );

    Vector<vk::PushConstantRange> ranges {};
    for (uint32 i = 0; i < _push_constants.size(); i++) {
        ranges.push_back({});
        ranges[i].setStageFlags(
            vk::ShaderStageFlagBits::eVertex |
            vk::ShaderStageFlagBits::eFragment // TODO: Dynamic
        );
        ranges[i].setOffset(_uniforms[_push_constants[i]].byte_range.offset);
        ranges[i].setSize(_uniforms[_push_constants[i]].byte_range.size);
    }

    // === Create pipeline layout ===
    Vector<vk::DescriptorSetLayout> descriptor_set_layouts {
        _descriptor_sets.size()
    };
    for (uint32 i = 0; i < _descriptor_sets.size(); i++) {
        const auto backend_data = static_cast<VulkanDescriptorSetBackendData*>(
            _descriptor_sets[i].backend_data
        );
        descriptor_set_layouts[i] = backend_data->layout;
    }

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
    create_info.setSubpass(0); // The index of the subpass in the render
                               // pass where this pipeline will be used
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
                "Failed to create graphics pipeline (Unexpected "
                "compilation)."
            );
        _pipeline = result.value;
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    Logger::trace(RENDERER_VULKAN_LOG, "Graphics pipeline created.");
}

Vector<vk::DescriptorImageInfo>& VulkanShader::get_image_infos(
    const Vector<Texture::Map*>& texture_maps
) const {
    static Vector<vk::DescriptorImageInfo> image_infos {};
    image_infos.clear();

    for (uint32 i = 0; i < texture_maps.size(); ++i) {
        // TODO: only update in the list if actually needing an update.
        const auto tm = static_cast<const VulkanTexture::Map*>(
            texture_maps[i] ? texture_maps[i] : _texture_system->default_map
        );
        auto texture = static_cast<const VulkanTexture*>(tm->texture);

        if (tm->texture->is_render_target()) {
            const auto pt = static_cast<const PackedTexture*>(tm->texture);
            texture       = static_cast<const VulkanTexture*>(
                pt->get_at(_command_buffer->current_frame)
            );
        }

        vk::DescriptorImageInfo image_info {};
        image_info.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        image_info.setImageView(texture->image()->view);
        image_info.setSampler(tm->sampler);
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

vk::DescriptorType get_descriptor_type(Shader::Binding::Type type) {
    switch (type) {
    case Shader::Binding::Type::Uniform:
        return vk::DescriptorType::eUniformBuffer;
    case Shader::Binding::Type::Sampler:
        return vk::DescriptorType::eCombinedImageSampler;
    case Shader::Binding::Type::Storage:
        return vk::DescriptorType::eStorageBuffer;
    default:
        Logger::fatal(
            RENDERER_VULKAN_LOG,
            "Shader binding type ",
            (int32) type,
            " not supported."
        );
        return vk::DescriptorType::eUniformBuffer;
    }
}

vk::ShaderStageFlags get_shader_stages(uint8 stages) {
    vk::ShaderStageFlags stage_flags {};
    if ((stages & (uint8) Shader::Stage::Vertex) != 0) {
        stage_flags |= vk::ShaderStageFlagBits::eVertex;
    }
    if ((stages & (uint8) Shader::Stage::Fragment) != 0) {
        stage_flags |= vk::ShaderStageFlagBits::eFragment;
    }
    if ((stages & (uint8) Shader::Stage::Compute) != 0) {
        stage_flags |= vk::ShaderStageFlagBits::eCompute;
    }
    if ((stages & (uint8) Shader::Stage::Geometry) != 0) {
        stage_flags |= vk::ShaderStageFlagBits::eGeometry;
    }
    return stage_flags;
}

} // namespace ENGINE_NAMESPACE