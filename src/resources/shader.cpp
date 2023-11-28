#include "resources/shader.hpp"

#include "systems/texture_system.hpp"

namespace ENGINE_NAMESPACE {

#define SHADER_LOG "Shader :: "

ByteRange get_aligned_range(uint64 offset, uint64 size, uint64 granularity);

// Statics values
const uint32 Shader::max_name_length;

// Constructor & Destructor
Shader::Shader(const ShaderConfig config)
    : _texture_system(config.texture_system), _name(config.name),
      _use_instances(config.use_instances), _use_locals(config.use_locals),
      _bound_instance_id(0) {
    // Process attributes
    for (const auto attribute : config.attributes) {
        _attribute_stride += attribute.size;
    }
    _attributes = config.attributes;

    // Process bindings
    for (const auto& binding : config.bindings) {
        add_binding(binding);
    }

    // Process push constants
    for (const auto& push_constant_config : config.push_constants) {
        ShaderUniform push_constant {};
        push_constant.index    = _push_constants.size();
        push_constant.type     = push_constant_config.type;
        push_constant.location = push_constant.index;

        push_constant.binding = -1;
        push_constant.scope   = ShaderScope::Local;

        // Push a new aligned range (align to 4, as required by Vulkan spec)
        push_constant.byte_range = get_aligned_range(
            _push_constant_size, push_constant_config.size, 4
        );

        // Increase the push constant's size by the total value.
        _push_constant_size += push_constant.byte_range.size;

        size_t uniform_index = _uniforms.size();
        _uniforms.push_back(push_constant);
        _uniforms_hash[push_constant_config.name] = uniform_index;

        _push_constants.push_back(uniform_index);
    }
}
Shader::~Shader() {
    for (uint64 i = 0; i < _global_texture_maps.size(); i++) {
        _texture_system->release(_global_texture_maps[i]->texture->name());
        del(_global_texture_maps[i]);
    }
    _global_texture_maps.clear();
}

// ///////////////////// //
// SHADER PUBLIC METHODS //
// ///////////////////// //

void Shader::reload() {}

void Shader::use() {}
void Shader::bind_globals() {}
void Shader::bind_instance(const uint32 id) { _bound_instance_id = id; }

void Shader::apply_global() {}
void Shader::apply_instance() {}

uint32 Shader::acquire_instance_resources(const Vector<TextureMap*>& maps) {
    return -1;
}
void Shader::release_instance_resources(uint32 instance_id) {}

void Shader::acquire_texture_map_resources(TextureMap* texture_map) {}
void Shader::release_texture_map_resources(TextureMap* texture_map) {}

Result<uint16, InvalidArgument> Shader::get_uniform_index(const String name) {
    auto it = _uniforms_hash.find(name);
    if (it == _uniforms_hash.end())
        return Failure(InvalidArgument(
            String::build("No uniform named \"", name, "\" exists.")
        ));
    return it->second;
}

Result<void, InvalidArgument> Shader::set_sampler(
    const String name, const TextureMap* const texture_map
) {
    return set_uniform<const TextureMap>(name, texture_map);
}
Result<void, InvalidArgument> Shader::set_sampler(
    const uint16 id, const TextureMap* const texture_map
) {
    return set_uniform<const TextureMap>(id, texture_map);
}

// //////////////////////// //
// SHADER PROTECTED METHODS //
// //////////////////////// //

Outcome Shader::set_uniform(const uint16 id, void* value) {
    return Outcome::Successful;
}

ShaderBinding* Shader::get_binding(ShaderScope scope, uint32 index) {
    if (scope == ShaderScope::Global) {
        return &_global_descriptor_set.bindings[index];
    } else {
        return &_instance_descriptor_set.bindings[index];
    }
}

// ////////////////////// //
// SHADER PRIVATE METHODS //
// ////////////////////// //

void Shader::add_binding(const ShaderBindingConfig& config) {
    ShaderBinding binding {};
    binding.scope = config.scope;

    auto& descriptor_set = (binding.scope == ShaderScope::Global)
                               ? _global_descriptor_set
                               : _instance_descriptor_set;

    binding.type          = config.type;
    binding.set_index     = config.set_index; // Vulkan (set) index
    binding.count         = config.count;
    binding.shader_stages = config.shader_stages;

    // Uniforms
    uint32 binding_size = 0;
    for (const auto& uniform_config : config.uniforms) {
        ShaderUniform uniform {};

        uniform.type  = uniform_config.type;
        uniform.index = binding.uniforms.size(); // Index into binding array

        // This will allow identifying the binding that owns this uniform
        uniform.binding = descriptor_set.bindings.size();
        uniform.scope   = binding.scope;

        // Sampler
        if (binding.type == ShaderBindingType::Sampler) {
            if (uniform_config.type != ShaderUniformType::sampler) {
                Logger::fatal(SHADER_LOG
                              "Sampler binding uniform must be of type sampler."
                );
            }

            // If global, push into the global list.
            if (config.scope == ShaderScope::Global) {
                uniform.location = _global_texture_maps.size();

                // Create default texture map
                // NOTE: Can always be updated later
                TextureMap default_map { nullptr,
                                         TextureUse::Unknown,
                                         TextureFilter::BiLinear,
                                         TextureFilter::BiLinear,
                                         TextureRepeat::Repeat,
                                         TextureRepeat::Repeat,
                                         TextureRepeat::Repeat,
                                         nullptr };
                acquire_texture_map_resources(&default_map);

                // Allocate and push new global texture map
                // NOTE: Allocation within shader only done here (for globals)
                TextureMap* map =
                    new (MemoryTag::Renderer) TextureMap(default_map);
                map->texture = _texture_system->default_texture;
                _global_texture_maps.push_back(map);
            } else {
                // Otherwise, it's instance-level, so keep count of how many
                // need to be added during the resource acquisition.
                uniform.location = _instance_texture_count++;
            }
        } else { // Not a sampler
            uniform.location        = uniform.index;
            uniform.byte_range.size = uniform_config.size;
            
            binding_size += uniform_config.size;
        }

        // Add uniform to vector and hash
        size_t uniform_index                = _uniforms.size();
        _uniforms_hash[uniform_config.name] = uniform_index;
        _uniforms.push_back(uniform);

        // Add uniform to binding
        binding.uniforms.push_back(uniform_index);

    } // End uniforms

    binding.byte_range.size = binding_size;

    descriptor_set.bindings.push_back(binding);
}

// /////////////////////// //
// SHADER HELPER FUNCTIONS //
// /////////////////////// //

ByteRange get_aligned_range(uint64 offset, uint64 size, uint64 granularity) {
    return { get_aligned(offset, granularity),
             (uint16) get_aligned(size, granularity) };
}

} // namespace ENGINE_NAMESPACE