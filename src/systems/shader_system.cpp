#include "systems/shader_system.hpp"

#define SHADER_SYS_LOG "ShaderSystem :: "

// Constructor & Destructor
ShaderSystem::ShaderSystem(
    Renderer* const       renderer,
    ResourceSystem* const resource_system,
    TextureSystem* const  texture_system
)
    : _renderer(renderer), _resource_system(resource_system),
      _texture_system(texture_system) {
    Logger::trace(SHADER_SYS_LOG, "Creating shader system.");
    Logger::trace(SHADER_SYS_LOG, "Shader system created.");
}
ShaderSystem::~ShaderSystem() {
    for (auto& shader : _registered_shaders)
        _renderer->destroy_shader(shader.second);
    _registered_shaders.clear();
    Logger::trace(SHADER_SYS_LOG, "Shader system destroyed.");
}

// //////////////////////////// //
// SHADER SYSTEM PUBLIC METHODS //
// //////////////////////////// //

Result<Shader*, bool> ShaderSystem::create(ShaderConfig config) {
    Logger::trace(SHADER_SYS_LOG, "Creating shader \"", config.name, "\".");

    if (config.name().length() > Shader::max_name_length) {
        Logger::error(
            SHADER_SYS_LOG,
            "Shader creation failed. Maximum name length of a shader is ",
            Shader::max_name_length,
            " characters but ",
            config.name().length(),
            " character long name was passed. Creation unsuccessful."
        );
        return Failure(false);
    }
    config.texture_system = _texture_system;

    if (_registered_shaders.contains(config.name)) {
        Logger::warning(
            SHADER_SYS_LOG,
            "Shader \"",
            config.name,
            "\" overriden by the ShaderSystem::create method"
        );
        _renderer->destroy_shader(_registered_shaders[config.name]);
    }
    auto shader                      = _renderer->create_shader(config);
    _registered_shaders[config.name] = shader;

    Logger::trace(SHADER_SYS_LOG, "Shader \"", config.name, "\" created.");
    return shader;
}

Result<Shader*, bool> ShaderSystem::acquire(const String name) {
    Logger::trace(SHADER_SYS_LOG, "Shader \"", name, "\" requested.");

    if (name.length() > Shader::max_name_length) {
        Logger::error(
            SHADER_SYS_LOG,
            "Shader acquisition failed. Maximum name length of a shader is ",
            Shader::max_name_length,
            " characters but ",
            name.length(),
            " character long name was passed. Acquisition unsuccessful."
        );
        return Failure(false);
    }

    auto it = _registered_shaders.find(name);

    if (it == _registered_shaders.end()) {
        // Shader not found. Load required
        auto result = _resource_system->load(name, "Shader");
        if (result.has_error()) {
            Logger::error(SHADER_SYS_LOG, result.error().what());
            return Failure(false);
        }
        auto config = (ShaderConfig*) result.value();
        auto shader = create(*config).expect(
            "Shader creation failed. Something went wrong."
        );
        _resource_system->unload(config);

        Logger::trace(SHADER_SYS_LOG, "Shader \"", name, "\" acquired.");
        return shader;
    }

    Logger::trace(SHADER_SYS_LOG, "Shader \"", name, "\" acquired.");
    return it->second;
}