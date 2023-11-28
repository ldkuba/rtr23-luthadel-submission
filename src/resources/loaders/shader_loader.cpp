#include "resources/loaders/shader_loader.hpp"

#include "systems/resource_system.hpp"
#include "resources/shader.hpp"
#include "systems/file_system.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace ENGINE_NAMESPACE {

Result<ShaderAttribute, RuntimeErrorCode> parse_attribute_config(
    json attribute_settings
);
Result<ShaderUniformConfig, RuntimeErrorCode> parse_uniform_config(
    json uniform_settings
);
Result<uint8, RuntimeErrorCode> parse_shader_stage_flags(
    Vector<String> shader_stages
);

// Constructor & Destructor
ShaderLoader::ShaderLoader() {
    _type      = ResourceType::Shader;
    _type_path = "shaders/config";
}
ShaderLoader::~ShaderLoader() {}

// //////////////////////////// //
// SHADER LOADER PUBLIC METHODS //
// //////////////////////////// //

Result<Resource*, RuntimeError> ShaderLoader::load(const String name) {
    // Material configuration defaults
    String                      shader_name             = name;
    String                      shader_render_pass_name = "";
    uint8                       shader_stages           = 0;
    Vector<ShaderAttribute>     shader_attributes       = {};
    Vector<ShaderBindingConfig> shader_bindings         = {};
    Vector<ShaderUniformConfig> shader_push_constants   = {};
    bool                        shader_has_instances    = false;
    bool                        shader_has_locals       = false;

    // Load material configuration from file
    String file_name = name + ".shadercfg";
    file_name.to_lower();
    String file_path =
        ResourceSystem::base_path + "/" + _type_path + "/" + file_name;

    auto shader_settings = FileSystem::read_json(file_path);
    if (shader_settings.has_error()) {
        Logger::error(RESOURCE_LOG, shader_settings.error().what());
        return Failure(shader_settings.error().what());
    }

    auto shader_settings_json = shader_settings.value();

    // === Version ===
    // TODO: versions

    // === Name ===
    String shader_settings_name = shader_settings_json.at("name");
    if (shader_settings_name.length() <= Shader::max_name_length)
        shader_name = shader_settings_name;
    else {
        Logger::warning(
            RESOURCE_LOG,
            "Couldn't load shader name in file",
            file_path,
            ". Name is too long (",
            shader_settings_name.length(),
            " characters, while maximum name length is ",
            Shader::max_name_length,
            " characters)."
        );
    }

    // === Render Pass ===
    shader_render_pass_name = shader_settings_json.at("renderpass");

    // === Shader Stages ===
    Vector<String> shader_settings_stages =
        shader_settings_json.at("stages");
    auto stage_flags = parse_shader_stage_flags(shader_settings_stages);

    if (shader_settings.has_error()) {
        Logger::warning(
            RESOURCE_LOG,
            "Couldn't parse shader stages in file ",
            file_name,
            ". Invalid shader stage \"",
            shader_settings.error().what(),
            "\" passed."
        );
    } else {
        shader_stages = stage_flags.value();
    }

    // === Attributes ===
    Vector<json> shader_settings_attributes =
        shader_settings_json.at("attributes");
    for (auto attribute_settings : shader_settings_attributes) {
        auto attribute = parse_attribute_config(attribute_settings);

        match_error_code(attribute) {
            Err(0) Logger::warning(
                RESOURCE_LOG,
                "Couldn't parse attribute in file ",
                file_name,
                ". Wrong attribute argument format passed."
            );
            Err(1) Logger::warning(
                RESOURCE_LOG,
                "Invalid attribute type \"",
                attribute.error().what(),
                "\" passed."
            );
        }
        else { shader_attributes.push_back(attribute.value()); }
    }

    // === Bindings ===
    Vector<json> shader_settings_bindings =
        shader_settings_json.at("bindings");
    for (auto binding_settings : shader_settings_bindings) {
        ShaderBindingConfig binding_config {};

        // Scope
        String binding_scope = binding_settings.at("scope");
        if(binding_scope.compare_ci("global") == 0) {
            binding_config.scope = ShaderScope::Global;
        } else if(binding_scope.compare_ci("instance") == 0) {
            binding_config.scope = ShaderScope::Instance;
            shader_has_instances = true;
        } else {
            Logger::warning(
                RESOURCE_LOG,
                "Invalid binding scope \"",
                binding_scope,
                "\" passed."
            );
        }

        // Type
        String binding_type = binding_settings.at("type");
        if(binding_type.compare_ci("uniform") == 0) {
            binding_config.type = ShaderBindingType::Uniform;
        } else if(binding_type.compare_ci("sampler") == 0) {
            binding_config.type = ShaderBindingType::Sampler;
        } else if(binding_type.compare_ci("storage") == 0) {
            binding_config.type = ShaderBindingType::Storage;
        } else {
            Logger::warning(
                RESOURCE_LOG,
                "Invalid binding type \"",
                binding_type,
                "\" passed."
            );
        }

        // Index
        size_t binding_set_index = binding_settings.value("set_index", 0);
        binding_config.set_index = binding_set_index;

        // Count
        size_t binding_count = binding_settings.value("count", 1);
        binding_config.count = binding_count;

        // Stages
        Vector<String> binding_stages = binding_settings.at("stages");
        auto binding_stage_flags_result = parse_shader_stage_flags(binding_stages);
        match_error_code(binding_stage_flags_result) {
            Err(0) Logger::warning(
                    RESOURCE_LOG,
                    "Invalid shader stage \"",
                    binding_stage_flags_result.error().what(),
                    "\" passed."
                );
            Ok() {
                binding_config.shader_stages = binding_stage_flags_result.value();
            }
        }

        // Uniforms
        Vector<json> binding_settings_uniforms =
            binding_settings.at("uniforms");
        for(auto& uniform_settings : binding_settings_uniforms) {
            auto uniform_config_result = parse_uniform_config(uniform_settings);
            match_error_code(uniform_config_result) {
                Err(0) Logger::warning(
                    RESOURCE_LOG,
                    "Couldn't parse uniform in file ",
                    file_name,
                    ". Invalid format."
                );
                Err(1) Logger::warning(
                    RESOURCE_LOG,
                    "Couldn't parse uniform in file ",
                    file_name,
                    ". Invalid uniform type \"",
                    uniform_config_result.error().what(),
                    "\" passed."
                );
                Err(2) Logger::warning(
                    RESOURCE_LOG,
                    "Couldn't parse uniform in file ",
                    file_name,
                    ". Custom uniforms must have a size."
                );
                Ok() {
                    auto uniform = uniform_config_result.value();
                    binding_config.uniforms.push_back(uniform);
                }
            }
        }

        shader_bindings.push_back(binding_config);
    }

    // === Push Constants ===
    Vector<json> shader_settings_push_constants =
        shader_settings_json.at("push_constants");
    for (auto push_constant_settings : shader_settings_push_constants) {
        auto push_constant_result = parse_uniform_config(push_constant_settings);

        match_error_code(push_constant_result) {
            Err(0) Logger::warning(
                RESOURCE_LOG,
                "Couldn't parse push constant in file ",
                file_name,
                ". Invalid format."
            );
            Err(1) Logger::warning(
                RESOURCE_LOG,
                "Couldn't parse push constant in file ",
                file_name,
                ". Invalid push constant type \"",
                push_constant_result.error().what(),
                "\" passed."
            );
            Err(2) Logger::warning(
                RESOURCE_LOG,
                "Couldn't parse push constant in file ",
                file_name,
                ". Custom push constants must have a size."
            );
            Ok() {
                auto push_constant = push_constant_result.value();
                shader_push_constants.push_back(push_constant);
            }
        }
    }

    if (shader_push_constants.size() > 0) shader_has_locals = true;

    // Create shader config
    auto shader_config = new (MemoryTag::Resource) ShaderConfig(
        shader_name,
        shader_render_pass_name,
        shader_stages,
        shader_attributes,
        shader_bindings,
        shader_push_constants,
        shader_has_instances,
        shader_has_locals
    );
    shader_config->full_path   = file_path;
    shader_config->loader_type = ResourceType::Shader;
    return shader_config;
}

void ShaderLoader::unload(Resource* resource) {
    can_unload(ResourceType::Shader, resource);

    ShaderConfig* res = (ShaderConfig*) resource;
    del(res);
}

Result<ShaderAttribute, RuntimeErrorCode> parse_attribute_config(
    json attribute_settings
) {
    // Parse name
    ShaderAttribute attribute_config {};
    try {
        attribute_config.name = attribute_settings.at("name");
    } catch (nlohmann::json::out_of_range& e) {
        return Failure(RuntimeErrorCode(0));
    }

    // Parse type
    String attribute_type = attribute_settings.at("type");
    if (attribute_type.compare_ci("float32") == 0) {
        attribute_config.type = ShaderAttributeType::float32;
        attribute_config.size = sizeof(float32);
    } else if (attribute_type.compare_ci("vec2") == 0) {
        attribute_config.type = ShaderAttributeType::vec2;
        attribute_config.size = 2 * sizeof(float32);
    } else if (attribute_type.compare_ci("vec3") == 0) {
        attribute_config.type = ShaderAttributeType::vec3;
        attribute_config.size = 3 * sizeof(float32);
    } else if (attribute_type.compare_ci("vec4") == 0) {
        attribute_config.type = ShaderAttributeType::vec4;
        attribute_config.size = 4 * sizeof(float32);
    } else if (attribute_type.compare_ci("int8") == 0) {
        attribute_config.type = ShaderAttributeType::int8;
        attribute_config.size = sizeof(int8);
    } else if (attribute_type.compare_ci("int16") == 0) {
        attribute_config.type = ShaderAttributeType::int16;
        attribute_config.size = sizeof(int16);
    } else if (attribute_type.compare_ci("int32") == 0) {
        attribute_config.type = ShaderAttributeType::int32;
        attribute_config.size = sizeof(int32);
    } else if (attribute_type.compare_ci("uint8") == 0) {
        attribute_config.type = ShaderAttributeType::uint8;
        attribute_config.size = sizeof(uint8);
    } else if (attribute_type.compare_ci("uint16") == 0) {
        attribute_config.type = ShaderAttributeType::uint16;
        attribute_config.size = sizeof(uint16);
    } else if (attribute_type.compare_ci("uint32") == 0) {
        attribute_config.type = ShaderAttributeType::uint32;
        attribute_config.size = sizeof(uint32);
    } else return Failure(RuntimeErrorCode(1, attribute_type));

    return attribute_config;
}

Result<ShaderUniformConfig, RuntimeErrorCode> parse_uniform_config(
    json uniform_settings
) {
    ShaderUniformConfig uniform_config {};
    String              uniform_type = "";

    try {
        uniform_config.name = uniform_settings.at("name");
        uniform_type        = uniform_settings.at("type");
    } catch (nlohmann::json::out_of_range& e) {
        return Failure(RuntimeErrorCode(0));
    }

    // Parse type
    if (uniform_type.compare_ci("float32") == 0) {
        uniform_config.type = ShaderUniformType::float32;
        uniform_config.size = sizeof(float32);
    } else if (uniform_type.compare_ci("vec2") == 0) {
        uniform_config.type = ShaderUniformType::vec2;
        uniform_config.size = 2 * sizeof(float32);
    } else if (uniform_type.compare_ci("vec3") == 0) {
        uniform_config.type = ShaderUniformType::vec3;
        uniform_config.size = 3 * sizeof(float32);
    } else if (uniform_type.compare_ci("vec4") == 0) {
        uniform_config.type = ShaderUniformType::vec4;
        uniform_config.size = 4 * sizeof(float32);
    } else if (uniform_type.compare_ci("int8") == 0) {
        uniform_config.type = ShaderUniformType::int8;
        uniform_config.size = sizeof(int8);
    } else if (uniform_type.compare_ci("int16") == 0) {
        uniform_config.type = ShaderUniformType::int16;
        uniform_config.size = sizeof(int16);
    } else if (uniform_type.compare_ci("int32") == 0) {
        uniform_config.type = ShaderUniformType::int32;
        uniform_config.size = sizeof(int32);
    } else if (uniform_type.compare_ci("uint8") == 0) {
        uniform_config.type = ShaderUniformType::uint8;
        uniform_config.size = sizeof(uint8);
    } else if (uniform_type.compare_ci("uint16") == 0) {
        uniform_config.type = ShaderUniformType::uint16;
        uniform_config.size = sizeof(uint16);
    } else if (uniform_type.compare_ci("uint32") == 0) {
        uniform_config.type = ShaderUniformType::uint32;
        uniform_config.size = sizeof(uint32);
    } else if (uniform_type.compare_ci("mat4") == 0) {
        uniform_config.type = ShaderUniformType::matrix4;
        uniform_config.size = 16 * sizeof(float32);
    } else if (uniform_type.compare_ci("sampler2D") == 0) {
        uniform_config.type = ShaderUniformType::sampler;
        uniform_config.size = 0; // Samplers dont have a size
    } else if (uniform_type.compare_ci("custom") == 0) {
        uniform_config.type = ShaderUniformType::custom;
        try {
            uniform_config.size = uniform_settings.at("size");
        } catch (nlohmann::json::out_of_range& e) {
            return Failure(RuntimeErrorCode(2));
        }
    } else return Failure(RuntimeErrorCode(1, uniform_type));

    return uniform_config;
}

Result<uint8, RuntimeErrorCode> parse_shader_stage_flags(
    Vector<String> shader_stages
) {
    uint8 stage_flags = 0;
    for (auto stage : shader_stages) {
        if (stage.compare_ci("vertex") == 0)
            stage_flags |= (uint8) ShaderStage::Vertex;
        else if (stage.compare_ci("geometry") == 0)
            stage_flags |= (uint8) ShaderStage::Geometry;
        else if (stage.compare_ci("fragment") == 0)
            stage_flags |= (uint8) ShaderStage::Fragment;
        else if (stage.compare_ci("compute") == 0)
            stage_flags |= (uint8) ShaderStage::Compute;
        else return Failure(RuntimeErrorCode(0));
    }

    return stage_flags;
}

} // namespace ENGINE_NAMESPACE