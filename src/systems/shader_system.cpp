#include "systems/shader_system.hpp"

namespace ENGINE_NAMESPACE {

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

Result<Shader*, RuntimeError> ShaderSystem::create(
    Shader::Config config, const String& instance_name
) {
    Logger::trace(SHADER_SYS_LOG, "Creating shader \"", config.name, "\".");

    auto shader                        = _renderer->create_shader(config);
    _registered_shaders[instance_name] = shader;

    Logger::trace(SHADER_SYS_LOG, "Shader \"", config.name, "\" created.");
    return shader;
}

Result<Shader*, RuntimeError> ShaderSystem::acquire(const AcquireConfig& config
) {
    Logger::trace(
        SHADER_SYS_LOG,
        "Shader \"",
        config.instance_name,
        "\" of type \"",
        config.shader_name,
        "\" requested."
    );

    if (config.shader_name.length() > Shader::max_name_length) {
        const auto error_message = String::build(
            "Shader acquisition failed. Maximum name length of a shader is ",
            Shader::max_name_length,
            " characters but ",
            config.shader_name.length(),
            " character long name was passed. Acquisition unsuccessful."
        );
        Logger::error(SHADER_SYS_LOG, error_message);
        return Failure(error_message);
    }

    auto it = _registered_shaders.find(config.instance_name);

    if (it == _registered_shaders.end()) {
        auto result = _resource_system->load(config.shader_name, "Shader");
        if (result.has_error()) {
            Logger::error(SHADER_SYS_LOG, result.error().what());
            return Failure(result.error().what());
        }
        auto shader_config = (Shader::Config*) result.value();
        if (config.renderpass_name.length() > 0)
            shader_config->set_renderpass_name(config.renderpass_name);

        auto shader =
            create(*shader_config, config.instance_name)
                .expect("Shader creation failed. Something went wrong.");
        _resource_system->unload(shader_config);

        Logger::trace(
            SHADER_SYS_LOG, "Shader \"", config.instance_name, "\" acquired."
        );
        return shader;
    }

    Logger::trace(
        SHADER_SYS_LOG, "Shader \"", config.instance_name, "\" acquired."
    );
    return it->second;
}

void ShaderSystem::reload_shaders() {
    for (auto& shader : _registered_shaders) {
        shader.second->reload();
    }
}

} // namespace ENGINE_NAMESPACE