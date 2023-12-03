#include "resources/loaders/shader_loader.hpp"

#include "systems/resource_system.hpp"
#include "resources/shader.hpp"
#include "systems/file_system.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace ENGINE_NAMESPACE {

Result<Shader::Attribute, RuntimeErrorCode> parse_attribute_config(
    json attribute_settings
);
Result<Shader::Uniform::Config, RuntimeErrorCode> parse_uniform_config(
    json uniform_settings
);
Result<uint8, RuntimeErrorCode> parse_shader_stage_flags(
    Vector<String> shader_stages
);

struct ShaderVars {
    STRING_ENUM(version);
    STRING_ENUM(name);
    STRING_ENUM(renderpass);
    STRING_ENUM(stages);
    STRING_ENUM(cull_mode);
    STRING_ENUM(attributes);
    STRING_ENUM(descriptor_sets);
    STRING_ENUM(set_index);
    STRING_ENUM(bindings);
    STRING_ENUM(scope);
    STRING_ENUM(type);
    STRING_ENUM(binding_index);
    STRING_ENUM(uniforms);
    STRING_ENUM(count);
    STRING_ENUM(push_constants);
};

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
    String                          shader_name             = name;
    String                          shader_render_pass_name = "";
    uint8                           shader_stages           = 0;
    Vector<Shader::Attribute>       shader_attributes       = {};
    Vector<Shader::DescriptorSet::Config> shader_sets             = {};
    Vector<Shader::Uniform::Config> shader_push_constants   = {};
    Shader::CullMode                shader_cull_mode = Shader::CullMode::Back;

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
    String shader_settings_name = shader_settings_json.at(ShaderVars::name);
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
    shader_render_pass_name = shader_settings_json.at(ShaderVars::renderpass);

    // === Shader Stages ===
    Vector<String> shader_settings_stages =
        shader_settings_json.at(ShaderVars::stages);
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

    // === Cull Mode ===
    String culling_mode = shader_settings_json.at(ShaderVars::cull_mode);
    if (culling_mode.compare("none")) shader_cull_mode = Shader::CullMode::None;
    else if (culling_mode.compare("front"))
        shader_cull_mode = Shader::CullMode::Back;
    else if (culling_mode.compare("back"))
        shader_cull_mode = Shader::CullMode::Front;
    else if (culling_mode.compare("both"))
        shader_cull_mode = Shader::CullMode::Both;

    // === Attributes ===
    Vector<json> shader_settings_attributes =
        shader_settings_json.at(ShaderVars::attributes);
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

    // === Descriptor Sets ===
    Vector<json> shader_settings_descriptor_sets =
        shader_settings_json.at(ShaderVars::descriptor_sets);
    for (auto descriptor_set_settings : shader_settings_descriptor_sets) {
        Shader::DescriptorSet::Config descriptor_set_config {};

        // Scope
        String descriptor_set_scope =
            descriptor_set_settings.at(ShaderVars::scope);
        if (descriptor_set_scope.compare_ci("global") == 0) {
            descriptor_set_config.scope = Shader::Scope::Global;
        } else if (descriptor_set_scope.compare_ci("instance") == 0) {
            descriptor_set_config.scope = Shader::Scope::Instance;
        } else {
            Logger::warning(
                RESOURCE_LOG,
                "Invalid descriptor set scope \"",
                descriptor_set_scope,
                "\" passed."
            );
        }

        // Index
        uint32 descriptor_set_index =
            descriptor_set_settings.value(ShaderVars::set_index, 0);
        descriptor_set_config.set_index = descriptor_set_index;

        // === Bindings ===
        Vector<json> shader_settings_bindings =
            shader_settings_json.at(ShaderVars::bindings);
        for (auto binding_settings : shader_settings_bindings) {
            Shader::Binding::Config binding_config {};

            // Type
            String binding_type = binding_settings.at(ShaderVars::type);
            if (binding_type.compare_ci("uniform") == 0) {
                binding_config.type = Shader::Binding::Type::Uniform;
            } else if (binding_type.compare_ci("sampler") == 0) {
                binding_config.type = Shader::Binding::Type::Sampler;
            } else if (binding_type.compare_ci("storage") == 0) {
                binding_config.type = Shader::Binding::Type::Storage;
            } else {
                Logger::warning(
                    RESOURCE_LOG,
                    "Invalid binding type \"",
                    binding_type,
                    "\" passed."
                );
            }

            // Index
            size_t binding_index =
                binding_settings.value(ShaderVars::binding_index, 0);
            binding_config.binding_index = binding_index;

            // Count
            size_t binding_count = binding_settings.value(ShaderVars::count, 1);
            binding_config.count = binding_count;

            // Stages
            Vector<String> binding_stages =
                binding_settings.at(ShaderVars::stages);
            auto binding_stage_flags_result =
                parse_shader_stage_flags(binding_stages);
            match_error_code(binding_stage_flags_result) {
                Err(0) Logger::warning(
                    RESOURCE_LOG,
                    "Invalid shader stage \"",
                    binding_stage_flags_result.error().what(),
                    "\" passed."
                );
                Ok() {
                    binding_config.shader_stages =
                        binding_stage_flags_result.value();
                }
            }

            // Uniforms
            Vector<json> binding_settings_uniforms =
                binding_settings.at("uniforms");
            for (auto& uniform_settings : binding_settings_uniforms) {
                auto uniform_config_result =
                    parse_uniform_config(uniform_settings);
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

            descriptor_set_config.bindings.push_back(binding_config);
        }

        shader_sets.push_back(descriptor_set_config);
    }

    // === Push Constants ===
    Vector<json> shader_settings_push_constants =
        shader_settings_json.at("push_constants");
    for (auto push_constant_settings : shader_settings_push_constants) {
        auto push_constant_result =
            parse_uniform_config(push_constant_settings);

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

    // Create shader config
    auto shader_config = new (MemoryTag::Resource) Shader::Config(
        shader_name,
        shader_render_pass_name,
        shader_stages,
        shader_attributes,
        shader_sets,
        shader_push_constants,
        shader_cull_mode
    );
    shader_config->full_path   = file_path;
    shader_config->loader_type = ResourceType::Shader;
    return shader_config;
}

void ShaderLoader::unload(Resource* resource) {
    can_unload(ResourceType::Shader, resource);

    Shader::Config* res = (Shader::Config*) resource;
    del(res);
}

Result<Shader::Attribute, RuntimeErrorCode> parse_attribute_config(
    json attribute_settings
) {
    // Parse name
    Shader::Attribute attribute_config {};
    try {
        attribute_config.name = attribute_settings.at("name");
    } catch (nlohmann::json::out_of_range& e) {
        return Failure(RuntimeErrorCode(0));
    }

    // Parse type
    String attribute_type = attribute_settings.at("type");
    if (attribute_type.compare_ci("float32") == 0) {
        attribute_config.type = Shader::AttributeType::float32;
        attribute_config.size = sizeof(float32);
    } else if (attribute_type.compare_ci("vec2") == 0) {
        attribute_config.type = Shader::AttributeType::vec2;
        attribute_config.size = 2 * sizeof(float32);
    } else if (attribute_type.compare_ci("vec3") == 0) {
        attribute_config.type = Shader::AttributeType::vec3;
        attribute_config.size = 3 * sizeof(float32);
    } else if (attribute_type.compare_ci("vec4") == 0) {
        attribute_config.type = Shader::AttributeType::vec4;
        attribute_config.size = 4 * sizeof(float32);
    } else if (attribute_type.compare_ci("int8") == 0) {
        attribute_config.type = Shader::AttributeType::int8;
        attribute_config.size = sizeof(int8);
    } else if (attribute_type.compare_ci("int16") == 0) {
        attribute_config.type = Shader::AttributeType::int16;
        attribute_config.size = sizeof(int16);
    } else if (attribute_type.compare_ci("int32") == 0) {
        attribute_config.type = Shader::AttributeType::int32;
        attribute_config.size = sizeof(int32);
    } else if (attribute_type.compare_ci("uint8") == 0) {
        attribute_config.type = Shader::AttributeType::uint8;
        attribute_config.size = sizeof(uint8);
    } else if (attribute_type.compare_ci("uint16") == 0) {
        attribute_config.type = Shader::AttributeType::uint16;
        attribute_config.size = sizeof(uint16);
    } else if (attribute_type.compare_ci("uint32") == 0) {
        attribute_config.type = Shader::AttributeType::uint32;
        attribute_config.size = sizeof(uint32);
    } else return Failure(RuntimeErrorCode(1, attribute_type));

    return attribute_config;
}

Result<Shader::Uniform::Config, RuntimeErrorCode> parse_uniform_config(
    json uniform_settings
) {
    Shader::Uniform::Config uniform_config {};
    String                  uniform_type = "";

    try {
        uniform_config.name = uniform_settings.at("name");
        uniform_type        = uniform_settings.at("type");
    } catch (nlohmann::json::out_of_range& e) {
        return Failure(RuntimeErrorCode(0));
    }

    // Parse type
    if (uniform_type.compare_ci("float32") == 0) {
        uniform_config.type = Shader::UniformType::float32;
        uniform_config.size = sizeof(float32);
    } else if (uniform_type.compare_ci("vec2") == 0) {
        uniform_config.type = Shader::UniformType::vec2;
        uniform_config.size = 2 * sizeof(float32);
    } else if (uniform_type.compare_ci("vec3") == 0) {
        uniform_config.type = Shader::UniformType::vec3;
        uniform_config.size = 3 * sizeof(float32);
    } else if (uniform_type.compare_ci("vec4") == 0) {
        uniform_config.type = Shader::UniformType::vec4;
        uniform_config.size = 4 * sizeof(float32);
    } else if (uniform_type.compare_ci("int8") == 0) {
        uniform_config.type = Shader::UniformType::int8;
        uniform_config.size = sizeof(int8);
    } else if (uniform_type.compare_ci("int16") == 0) {
        uniform_config.type = Shader::UniformType::int16;
        uniform_config.size = sizeof(int16);
    } else if (uniform_type.compare_ci("int32") == 0) {
        uniform_config.type = Shader::UniformType::int32;
        uniform_config.size = sizeof(int32);
    } else if (uniform_type.compare_ci("uint8") == 0) {
        uniform_config.type = Shader::UniformType::uint8;
        uniform_config.size = sizeof(uint8);
    } else if (uniform_type.compare_ci("uint16") == 0) {
        uniform_config.type = Shader::UniformType::uint16;
        uniform_config.size = sizeof(uint16);
    } else if (uniform_type.compare_ci("uint32") == 0) {
        uniform_config.type = Shader::UniformType::uint32;
        uniform_config.size = sizeof(uint32);
    } else if (uniform_type.compare_ci("mat4") == 0) {
        uniform_config.type = Shader::UniformType::matrix4;
        uniform_config.size = 16 * sizeof(float32);
    } else if (uniform_type.compare_ci("sampler2D") == 0) {
        uniform_config.type = Shader::UniformType::sampler;
        uniform_config.size = 0; // Samplers dont have a size
    } else if (uniform_type.compare_ci("custom") == 0) {
        uniform_config.type = Shader::UniformType::custom;
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
            stage_flags |= (uint8) Shader::Stage::Vertex;
        else if (stage.compare_ci("geometry") == 0)
            stage_flags |= (uint8) Shader::Stage::Geometry;
        else if (stage.compare_ci("fragment") == 0)
            stage_flags |= (uint8) Shader::Stage::Fragment;
        else if (stage.compare_ci("compute") == 0)
            stage_flags |= (uint8) Shader::Stage::Compute;
        else return Failure(RuntimeErrorCode(0));
    }

    return stage_flags;
}

} // namespace ENGINE_NAMESPACE