#include "resources/shader.hpp"

#include "systems/texture_system.hpp"

#define SHADER_LOG "Shader :: "

PushConstantRange get_aligned_range(
    uint64 offset, uint64 size, uint64 granularity
);

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

    // Process uniforms
    for (const auto& uniform : config.uniforms) {
        // Sanity checks
        if (uniform.name.compare("") == 0)
            Logger::fatal(SHADER_LOG, "Uniform name not given.");
        if (_uniforms_hash.contains(uniform.name))
            Logger::fatal(
                SHADER_LOG,
                "Uniform by the name ",
                uniform.name,
                " already exists for shader ",
                _name,
                "."
            );
        if (uniform.scope == ShaderScope::Instance && !_use_instances)
            Logger::fatal(
                SHADER_LOG,
                "Adding instance uniform \"",
                uniform.name,
                "\" for a shader that doesn't use instances isn't possible."
            );
        if (uniform.scope == ShaderScope::Local && !_use_locals)
            Logger::fatal(
                SHADER_LOG,
                "Adding local uniform \"",
                uniform.name,
                "\" for a shader that doesn't use locals isn't possible."
            );

        // Add uniform correctly
        switch (uniform.type) {
        case ShaderUniformType::sampler: add_sampler(uniform); break;
        case ShaderUniformType::custom: // TODO: Implement
            Logger::fatal(SHADER_LOG, "Custom uniform usage unimplemented.");
            break;
        default: add_uniform(uniform); break;
        }
    }
}
Shader::~Shader() {
    for (auto texture : _global_textures)
        _texture_system->release(texture->name());
}

// ///////////////////// //
// SHADER PUBLIC METHODS //
// ///////////////////// //

void Shader::use() {}
void Shader::bind_globals() {}
void Shader::bind_instance(const uint32 id) { _bound_instance_id = id; }

void Shader::apply_global() {}
void Shader::apply_instance() {}

uint32 Shader::acquire_instance_resources() { return -1; }
void   Shader::release_instance_resources(uint32 instance_id) {}

Result<uint16, InvalidArgument> Shader::get_uniform_index(const String name) {
    auto it = _uniforms_hash.find(name);
    if (it == _uniforms_hash.end())
        return Failure(InvalidArgument(
            String::build("No uniform named \"", name, "\" exists.")
        ));
    return it->second;
}

Result<void, InvalidArgument> Shader::set_sampler(
    const String name, const Texture* const texture
) {
    return set_uniform<const Texture>(name, texture);
}
Result<void, InvalidArgument> Shader::set_sampler(
    const uint16 id, const Texture* const texture
) {
    return set_uniform<const Texture>(id, texture);
}

// //////////////////////// //
// SHADER PROTECTED METHODS //
// //////////////////////// //

bool Shader::set_uniform(const uint16 id, void* value) { return true; }

// ////////////////////// //
// SHADER PRIVATE METHODS //
// ////////////////////// //

void Shader::add_sampler(const ShaderUniformConfig& config) {
    if (config.scope == ShaderScope::Local)
        Logger::fatal(
            SHADER_LOG,
            "Error while adding sampler \"",
            config.name,
            "\".Local samplers not possible."
        );

    // If global, push into the global list.
    uint32 location = 0;
    if (config.scope == ShaderScope::Global) {
        location = _global_textures.size();
        _global_textures.push_back(_texture_system->default_texture);
    } else {
        // Otherwise, it's instance-level, so keep count of how many need to be
        // added during the resource acquisition.
        location = _instance_texture_count++;
    }

    // Treat it like a uniform.
    add_uniform(config, location);
}
void Shader::add_uniform(
    const ShaderUniformConfig& config, const std::optional<uint32> location
) {
    ShaderUniform entry {};
    entry.index = _uniforms.size();
    entry.scope = config.scope;
    entry.type  = config.type;

    // Use passed location if given
    if (location.has_value()) entry.location = location.value();
    else entry.location = entry.index;

    if (config.scope == ShaderScope::Local) {
        // Push a new aligned range (align to 4, as required by Vulkan spec)
        entry.set_index = -1;
        PushConstantRange range =
            get_aligned_range(_push_constant_size, config.size, 4);
        // utilize the aligned offset/range
        entry.offset = range.offset;
        entry.size   = range.size;

        // Track in configuration for use in initialization.
        _push_constant_ranges.push_back(range);

        // Increase the push constant's size by the total value.
        _push_constant_size += range.size;
    } else {
        entry.set_index = (uint32) config.scope;
        // If this is sampler size & offset are implicitly 0
        if (!location.has_value() /* NOT a sampler */) {
            entry.size = config.size;
            if (entry.scope == ShaderScope::Global) {
                entry.offset = _global_ubo_size;
                _global_ubo_size += entry.size;
            } else {
                entry.offset = _ubo_size;
                _ubo_size += entry.size;
            }
        }
    }

    _uniforms.push_back(entry);
    _uniforms_hash[config.name] = entry.index;
}

// /////////////////////// //
// SHADER HELPER FUNCTIONS //
// /////////////////////// //

PushConstantRange get_aligned_range(
    uint64 offset, uint64 size, uint64 granularity
) {
    return { get_aligned(offset, granularity),
             (uint16) get_aligned(size, granularity) };
}