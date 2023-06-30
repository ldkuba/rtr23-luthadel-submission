#include "resources/loaders/material_loader.hpp"

#include "systems/resource_system.hpp"
#include "resources/material.hpp"
#include "systems/file_system.hpp"

struct MSVars {
    STRING_CONST(version);
    STRING_CONST(name);
    STRING_CONST(shader);
    STRING_CONST(diffuse_color);
    STRING_CONST(shininess);
    STRING_CONST(diffuse_map_name);
    STRING_CONST(specular_map_name);
    STRING_CONST(normal_map_name);
};

Result<glm::vec4, uint8> load_vector(const String vector_str);

// Constructor & Destructor
MaterialLoader::MaterialLoader() {
    _type      = ResourceType::Material;
    _type_path = "materials";
}
MaterialLoader::~MaterialLoader() {}

// ////////////////////////////// //
// MATERIAL LOADER PUBLIC METHODS //
// ////////////////////////////// //

Result<Resource*, RuntimeError> MaterialLoader::load(const String name) {
    // Material configuration defaults
    String    mat_name              = name;
    String    mat_shader            = "";
    bool      mat_auto_release      = true;
    glm::vec4 mat_diffuse_color     = glm::vec4(1.0f);
    float32   mat_shininess         = 32.0f;
    String    mat_diffuse_map_name  = "";
    String    mat_specular_map_name = "";
    String    mat_normal_map_name   = "";

    // Load material configuration from file
    String file_name = name + ".mat";
    file_name.to_lower();
    String file_path =
        ResourceSystem::base_path + "/" + _type_path + "/" + file_name;

    auto material_settings = FileSystem::read_lines(file_path);
    if (material_settings.has_error()) {
        Logger::error(RESOURCE_LOG, material_settings.error().what());
        return Failure(material_settings.error().what());
    }

    // Parse loaded config
    uint32 line_number = 1;
    for (auto setting_line : material_settings.value()) {
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
        if (setting_var.compare(MSVars::version) == 0) {
            // TODO: Versions
        }
        // NAME
        else if (setting_var.compare(MSVars::name) == 0) {
            if (setting_val.length() <= Material::max_name_length)
                mat_name = setting_val;
            else
                Logger::warning(
                    RESOURCE_LOG,
                    "Couldn't load material name at line ",
                    line_number,
                    " of file ",
                    file_path,
                    ". Name is too long (",
                    setting_val.length(),
                    " characters, while maximum name length is ",
                    Material::max_name_length,
                    " characters)."
                );
        }
        // SHADER NAME
        else if (setting_var.compare(MSVars::shader) == 0) {
            mat_shader = setting_val;
        }
        // DIFFUSE MAP NAME
        else if (setting_var.compare(MSVars::diffuse_map_name) == 0) {
            mat_diffuse_map_name = setting_val;
        }
        // SPECULAR MAP NAME
        else if (setting_var.compare(MSVars::specular_map_name) == 0) {
            mat_specular_map_name = setting_val;
        }
        // NORMAL MAP NAME
        else if (setting_var.compare(MSVars::normal_map_name) == 0) {
            mat_normal_map_name = setting_val;
        }
        // DIFFUSE COLOR
        else if (setting_var.compare(MSVars::diffuse_color) == 0) {
            auto result = load_vector(setting_val);
            match_error(result) {
                Err(1) {
                    Logger::warning(
                        RESOURCE_LOG,
                        "Couldn't parse vec4 at line ",
                        line_number,
                        " of file ",
                        file_path,
                        ". Wrong argument count."
                    );
                }
                Err(2) {
                    Logger::warning(
                        RESOURCE_LOG,
                        "Couldn't parse vec4 at line ",
                        line_number,
                        " of file ",
                        file_path,
                        ". Couldn't parse floats."
                    );
                }
            }
            else { mat_diffuse_color = result.value(); }
        }
        // SHININESS
        else if (setting_var.compare(MSVars::shininess) == 0) {
            auto parse_res = setting_val.parse_as_float32();
            if (!parse_res.has_error()) mat_shininess = parse_res.value();
            else
                Logger::warning(
                    RESOURCE_LOG,
                    "Couldn't parse shininess at line ",
                    line_number,
                    " of file ",
                    file_path,
                    ". Couldn't parse floats."
                );
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

    // Create material config
    auto material_config = new (MemoryTag::Resource) MaterialConfig(
        mat_name,
        mat_shader,
        mat_diffuse_map_name,
        mat_specular_map_name,
        mat_normal_map_name,
        mat_diffuse_color,
        mat_shininess,
        mat_auto_release
    );
    material_config->full_path   = file_path;
    material_config->loader_type = ResourceType::Material;
    return material_config;
}

void MaterialLoader::unload(Resource* resource) {
    can_unload(ResourceType::Material, resource);

    MaterialConfig* res = (MaterialConfig*) resource;
    delete res;
}

// //////////////////////////////// //
// MATERIAL LOADER HELPER FUNCTIONS //
// //////////////////////////////// //

Result<glm::vec4, uint8> load_vector(const String vector_str) { // Parse vec4
    auto str_floats = vector_str.split(' ');
    // Check vector dim size
    if (str_floats.size() != 4) return Failure(1);

    // convert to float
    Vector<float32> floats(str_floats.size());
    for (uint32 i = 0; i < str_floats.size(); i++) {
        auto result = str_floats[i].parse_as_float32();
        if (result.has_error()) return Failure(2);
        floats[i] = result.value();
    }

    // Assign to diffuse color
    return glm::vec4(floats[0], floats[1], floats[2], floats[3]);
}