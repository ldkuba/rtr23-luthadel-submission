#include "resources/shader.hpp"

#include "systems/texture_system.hpp"

namespace ENGINE_NAMESPACE {

#define SHADER_LOG "Shader :: "

ByteRange get_aligned_range(uint64 offset, uint64 size, uint64 granularity);

// Statics values
const uint32 Shader::max_name_length;

// Constructor & Destructor
Shader::Shader(
    Renderer* const      renderer,
    TextureSystem* const texture_system,
    const Config&        config
)
    : _renderer(renderer), _texture_system(texture_system), _name(config.name),
      _cull_mode(config.cull_mode), _bound_instance_id(0) {
    // Process attributes
    for (const auto attribute : config.attributes) {
        _attribute_stride += attribute.size;
    }
    _attributes = config.attributes;

    // Process sets
    for (const auto& set : config.sets) {
        DescriptorSet descriptor_set {};
        descriptor_set.scope     = set.scope;
        descriptor_set.set_index = set.set_index;
        _descriptor_sets.push_back(descriptor_set);

        for (const auto& binding : set.bindings) {
            add_binding(binding, set.set_index);
        }
    }

    // Process push constants
    for (const auto& push_constant_config : config.push_constants) {
        Uniform push_constant {};
        push_constant.type        = push_constant_config.type;
        push_constant.array_index = 0;

        push_constant.binding_index = -1;
        push_constant.set_index     = -1;
        push_constant.scope         = Scope::Local;

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
Shader::~Shader() {}

// ///////////////////// //
// SHADER PUBLIC METHODS //
// ///////////////////// //

void Shader::reload() {}

void Shader::use() {}
void Shader::bind_globals() { _bound_scope = Shader::Scope::Global; }
void Shader::bind_instance(const uint32 id) {
    _bound_scope       = Shader::Scope::Instance;
    _bound_instance_id = id;
}

void Shader::apply_global() {}
void Shader::apply_instance() {}

uint32 Shader::acquire_instance_resources(const Vector<Texture::Map*>& maps) {
    return -1;
}
void Shader::release_instance_resources(uint32 instance_id) {}

void Shader::acquire_global_resources() {
    for (auto& set : _descriptor_sets) {
        if (set.scope != Scope::Global) continue;

        for (auto& binding : set.bindings) {
            if (binding.type != Binding::Type::Sampler) continue;

            Vector<Texture::Map*> binding_maps {};
            binding_maps.clear();

            for (auto uniform_index : binding.uniforms) {
                auto& uniform       = _uniforms[uniform_index];
                uniform.array_index = set.states[0]->texture_maps.size();

                // Create default texture map
                // NOTE: Can always be updated later
                // NOTE: Allocation within shader only done here (for globals)
                const auto default_map =
                    _renderer->create_texture_map({ nullptr,
                                                    Texture::Use::Unknown,
                                                    Texture::Filter::BiLinear,
                                                    Texture::Filter::BiLinear,
                                                    Texture::Repeat::Repeat,
                                                    Texture::Repeat::Repeat,
                                                    Texture::Repeat::Repeat });

                // Allocate and push new global texture map
                binding_maps.push_back(default_map);
            }

            set.states[0]->texture_maps[binding.binding_index] = binding_maps;
        }
    }
}

void Shader::release_global_resources() {
    for (auto& set : _descriptor_sets) {
        if (set.scope != Scope::Global) continue;

        for (auto& binding_entry : set.states[0]->texture_maps) {
            auto& binding_maps = binding_entry.second;
            for (uint64 i = 0; i < binding_maps.size(); i++) {
                _texture_system->release(binding_maps[i]->texture->name());
                del(binding_maps[i]);
            }
        }
    }
}

Result<uint16, InvalidArgument> Shader::get_uniform_index(const String& name
) const {
    auto it = _uniforms_hash.find(name);
    if (it == _uniforms_hash.end())
        return Failure(InvalidArgument(
            String::build("No uniform named \"", name, "\" exists.")
        ));
    return it->second;
}

Result<void, InvalidArgument> Shader::set_sampler(
    const String name, const Texture::Map* const texture_map
) {
    return set_uniform<const Texture::Map>(name, texture_map);
}
Result<void, InvalidArgument> Shader::set_sampler(
    const uint16 id, const Texture::Map* const texture_map
) {
    return set_uniform<const Texture::Map>(id, texture_map);
}

// //////////////////////// //
// SHADER PROTECTED METHODS //
// //////////////////////// //

Outcome Shader::set_uniform(const uint16 id, void* value) {
    return Outcome::Successful;
}

Shader::Binding* Shader::get_binding(uint32 set_index, uint32 binding_index) {
    if (set_index >= _descriptor_sets.size())
        Logger::fatal(String::build("No set with index ", set_index, " exists.")
        );
    auto& set = _descriptor_sets[set_index];
    if (binding_index >= set.bindings.size())
        Logger::fatal(String::build(
            "No binding with index ",
            binding_index,
            " exists in set ",
            set_index,
            "."
        ));
    return &set.bindings[binding_index];
}

// ////////////////////// //
// SHADER PRIVATE METHODS //
// ////////////////////// //

void Shader::add_binding(
    const Binding::Config& config, const uint32 set_index
) {
    Binding binding {};
    auto&   descriptor_set = _descriptor_sets[set_index];

    binding.type          = config.type;
    binding.set_index     = set_index;
    binding.binding_index = config.binding_index;
    binding.count         = config.count;
    binding.shader_stages = config.shader_stages;

    // Uniforms
    uint32 binding_size = 0;
    for (const auto& uniform_config : config.uniforms) {
        Uniform uniform {};

        uniform.type = uniform_config.type;

        // This will allow identifying the binding that owns this uniform
        uniform.binding_index = descriptor_set.bindings.size();

        uniform.set_index = set_index;
        uniform.scope     = descriptor_set.scope;

        // Sampler
        if (binding.type == Binding::Type::Sampler) {
            if (uniform_config.type != UniformType::sampler) {
                Logger::fatal(SHADER_LOG
                              "Sampler binding uniform must be of type sampler."
                );
            }

            uniform.array_index = descriptor_set.texture_map_count++;
        } else { // Not a sampler
            // Only set smallest uniform size. This might be alligned later
            uniform.byte_range.size = uniform_config.size;
        }

        // Add uniform to vector and hash
        size_t uniform_index                = _uniforms.size();
        _uniforms_hash[uniform_config.name] = uniform_index;
        _uniforms.push_back(uniform);

        // Add uniform to binding
        binding.uniforms.push_back(uniform_index);

    } // End uniforms

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