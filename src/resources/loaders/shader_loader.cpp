#include "resources/loaders/shader_loader.hpp"

#include "systems/resource_system.hpp"
#include "resources/shader.hpp"
#include "systems/file_system.hpp"

namespace ENGINE_NAMESPACE {

struct ShaderVars {
    STRING_CONST(version);
    STRING_CONST(name);
    STRING_CONST(renderpass);
    STRING_CONST(stages);
    STRING_CONST(attribute);
    STRING_CONST(uniform);
};

Result<ShaderAttribute, RuntimeErrorCode> parse_attribute_config(
    String attribute_str
);
Result<ShaderUniformConfig, RuntimeErrorCode> parse_uniform_config(
    String uniform_str
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
    Vector<ShaderUniformConfig> shader_uniforms         = {};
    bool                        shader_has_instances    = false;
    bool                        shader_has_locals       = false;

    // Load material configuration from file
    String file_name = name + ".shadercfg";
    file_name.to_lower();
    String file_path =
        ResourceSystem::base_path + "/" + _type_path + "/" + file_name;

    auto shader_settings = FileSystem::read_lines(file_path);
    if (shader_settings.has_error()) {
        Logger::error(RESOURCE_LOG, shader_settings.error().what());
        return Failure(shader_settings.error().what());
    }

    // Parse loaded config
    uint32 line_number = 1;
    for (auto setting_line : shader_settings.value()) {
        setting_line.trim();

        // Skip blank and comment lines
        if (setting_line.length() < 1 || setting_line[0] == '#') {
            line_number++;
            continue;
        }

        // Split line by = into a var/value pair
        auto setting = setting_line.split('=');
        if (setting.size() != 2) {
            Logger::warning(
                RESOURCE_LOG,
                "Potential formatting issue with the number of = tokens found. "
                "Skipping line ",
                line_number,
                " of file ",
                file_path,
                "."
            );
            line_number++;
            continue;
        }

        auto setting_var = setting[0];
        auto setting_val = setting[1];

        setting_var.trim();
        setting_var.to_lower();
        setting_val.trim();

        // === Process variable and its argument ===
        // VERSION
        if (setting_var.compare(ShaderVars::version) == 0) {
            // TODO: Versions
        }
        // NAME
        else if (setting_var.compare(ShaderVars::name) == 0) {
            if (setting_val.length() <= Shader::max_name_length)
                shader_name = setting_val;
            else {
                Logger::warning(
                    RESOURCE_LOG,
                    "Couldn't load shader name at line ",
                    line_number,
                    " of file ",
                    file_path,
                    ". Name is too long (",
                    setting_val.length(),
                    " characters, while maximum name length is ",
                    Shader::max_name_length,
                    " characters)."
                );
            }
        }
        // RENDER PASS
        else if (setting_var.compare(ShaderVars::renderpass) == 0) {
            shader_render_pass_name = setting_val;
        }
        // SHADER STAGES
        else if (setting_var.compare(ShaderVars::stages) == 0) {
            for (auto stage : setting_val.split(',')) {
                stage.trim();
                if (stage.compare_ci("vertex") == 0)
                    shader_stages |= (uint8) ShaderStage::Vertex;
                else if (stage.compare_ci("geometry") == 0)
                    shader_stages |= (uint8) ShaderStage::Geometry;
                else if (stage.compare_ci("fragment") == 0)
                    shader_stages |= (uint8) ShaderStage::Fragment;
                else if (stage.compare_ci("compute") == 0)
                    shader_stages |= (uint8) ShaderStage::Compute;
                else
                    Logger::warning(
                        RESOURCE_LOG,
                        "Couldn't parse line ",
                        line_number,
                        " of file ",
                        file_name,
                        ". Invalid shader stage \"",
                        stage,
                        "\" passed."
                    );
            }
        }
        // ATTRIBUTES
        else if (setting_var.compare(ShaderVars::attribute) == 0) {
            auto attribute = parse_attribute_config(setting_val);

            match_error_code(attribute) {
                Err(0) Logger::warning(
                    RESOURCE_LOG,
                    "Couldn't parse line ",
                    line_number,
                    " of file ",
                    file_name,
                    ". Wrong attribute argument format passed."
                );
                Err(1) Logger::warning(
                    RESOURCE_LOG,
                    "Couldn't parse line ",
                    line_number,
                    " of file ",
                    file_name,
                    ". Invalid attribute type \"",
                    attribute.error().what(),
                    "\" passed."
                );
            }
            else { shader_attributes.push_back(attribute.value()); }

        }
        // UNIFORMS
        else if (setting_var.compare(ShaderVars::uniform) == 0) {
            auto uniform_res = parse_uniform_config(setting_val);

            match_error_code(uniform_res) {
                Err(0) Logger::warning(
                    RESOURCE_LOG,
                    "Couldn't parse line ",
                    line_number,
                    " of file ",
                    file_name,
                    ". Invalid argument count for uniform passed."
                );
                Err(1) Logger::warning(
                    RESOURCE_LOG,
                    "Couldn't parse line ",
                    line_number,
                    " of file ",
                    file_name,
                    ". Invalid uniform type \"",
                    uniform_res.error().what(),
                    "\" passed."
                );
                Err(2) Logger::warning(
                    RESOURCE_LOG,
                    "Invalid scope passed in line ",
                    line_number,
                    " of file ",
                    file_name,
                    ". Only uniform scopes 0, 1 and 2 are allowed."
                );
                Ok() {
                    auto uniform = uniform_res.value();
                    shader_uniforms.push_back(uniform);
                    if (uniform.scope == ShaderScope::InstanceVert || uniform.scope == ShaderScope::InstanceFrag)
                        shader_has_instances = true;
                    if (uniform.scope == ShaderScope::Local)
                        shader_has_locals = true;
                }
            }
        }
        // WRONG VAR
        else {
            Logger::warning(
                RESOURCE_LOG,
                " Invalid variable : \"",
                setting_var,
                "\" at line ",
                line_number,
                " of file ",
                file_path,
                "."
            );
        }
        line_number++;
    }

    // Create shader config
    auto shader_config = new (MemoryTag::Resource) ShaderConfig(
        shader_name,
        shader_render_pass_name,
        shader_stages,
        shader_attributes,
        shader_uniforms,
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
    String attribute_str
) {
    auto attribute = attribute_str.split(',');

    if (attribute.size() != 2) return Failure(RuntimeErrorCode(0));

    // Parse name
    ShaderAttribute attribute_config {};
    attribute_config.name = attribute[1];
    attribute_config.name.trim();

    // Parse type
    auto attribute_type = attribute[0];
    attribute_type.trim();
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
    String uniform_line
) {
    auto uniform = uniform_line.split(',');

    // Custom uniforms have additional size parameter
    if (uniform.size() < 3 || uniform.size() > 4) return Failure(RuntimeErrorCode(0));

    auto uniform_type  = uniform[0];
    auto uniform_scope = uniform[1];
    auto uniform_name  = uniform[2];

    uniform_type.trim();
    uniform_scope.trim();
    uniform_name.trim();
    
    // Parse name
    ShaderUniformConfig uniform_config {};
    uniform_config.name = uniform_name;

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
    } else if (uniform_type.compare_ci("sampler") == 0) {
        uniform_config.type = ShaderUniformType::sampler;
        uniform_config.size = 0; // Samplers dont have a size
    } else if (uniform_type.compare_ci("custom") == 0) {
        uniform_config.type = ShaderUniformType::custom;
        auto uniform_size = uniform[3];
        uniform_size.trim();
        uniform_config.size = uniform_size.parse_as_uint8().value_or(0); 
    } else return Failure(RuntimeErrorCode(1, uniform_type));

    // Parse scope
    auto scope = uniform_scope.parse_as_uint8().value_or(-1);

    switch (scope) {
    case 0: uniform_config.scope = ShaderScope::GlobalVert; break;
    case 1: uniform_config.scope = ShaderScope::GlobalFrag; break;
    case 2: uniform_config.scope = ShaderScope::InstanceVert; break;
    case 3: uniform_config.scope = ShaderScope::InstanceFrag; break;
    case 4: uniform_config.scope = ShaderScope::Local; break;
    default: return Failure(RuntimeErrorCode(2));
    };

    return uniform_config;
}

} // namespace ENGINE_NAMESPACE