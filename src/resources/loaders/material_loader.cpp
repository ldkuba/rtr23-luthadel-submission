#include "resources/loaders/material_loader.hpp"

#include "systems/resource_system.hpp"
#include "resources/material.hpp"
#include "file_system.hpp"

#define M_SETTING_version "version"
#define M_SETTING_name "name"
#define M_SETTING_diffuse_color "diffuse_color"
#define M_SETTING_diffuse_map_name "diffuse_map_name"

// Constructor & Destructor
MaterialLoader::MaterialLoader() {
    _type      = ResourceType::Material;
    _type_path = "materials";
}
MaterialLoader::~MaterialLoader() {}

// ////////////////////////////// //
// MATERIAL LOADER PUBLIC METHODS //
// ////////////////////////////// //

Resource* MaterialLoader::load(const String name) {
    // Material configuration defaults
    String    mat_name             = name;
    bool      mat_auto_release     = true;
    glm::vec4 mat_diffuse_color    = glm::vec4(1.0f);
    String    mat_diffuse_map_name = "";

    // Load material configuration from file
    String file_name = name + ".mat";
    file_name.to_lower();
    String file_path =
        ResourceSystem::base_path + "/" + _type_path + "/" + file_name;

    std::vector<String> material_settings;
    try {
        material_settings = FileSystem::read_file_lines(file_path);
    } catch (const FileSystemException& e) {
        Logger::error(RESOURCE_LOG, e.what());
        throw std::runtime_error(e.what());
    }

    // Parse loaded config
    uint32 line_number = 1;
    for (auto setting_line : material_settings) {
        setting_line.trim();

        // Skip blank and comment lines
        if (setting_line.length() < 1 || setting_line[0] == '#') {
            continue;
            line_number++;
        }

        // Split line by = int a var/value pair
        auto setting = setting_line.split('=');
        if (setting.size() != 2) {
            Logger::warning(
                RESOURCE_LOG,
                "Potential formatting issue ",
                "with the number of = tokens found. ",
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
        setting_var.trim();
        setting_var.to_lower();
        auto setting_val = setting[1];
        setting_val.trim();

        // Process variable and its argument
        if (setting_var.compare(M_SETTING_version) == 0) {
            // TODO: Versions
        } else if (setting_var.compare(M_SETTING_name) == 0) {
            if (setting_val.length() > Material::max_name_length)
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
            else mat_name = setting_val;
        } else if (setting_var.compare(M_SETTING_diffuse_map_name) == 0) {
            mat_diffuse_map_name = setting_val;
        } else if (setting_var.compare(M_SETTING_diffuse_color) == 0) {
            // Parse vec4
            auto str_floats = setting_val.split(' ');
            if (str_floats.size() != 4) {
                Logger::warning(
                    RESOURCE_LOG,
                    "Couldn't parse vec4 at line ",
                    line_number,
                    " of file ",
                    file_path,
                    ". Wrong argument count."
                );
            }
            std::vector<float32> floats(str_floats.size());
            try {
                for (uint32 i = 0; i < str_floats.size(); i++)
                    floats[i] = str_floats[i].parse_as_float32();
            } catch (std::invalid_argument e) {
                Logger::warning(
                    RESOURCE_LOG,
                    "Couldn't parse vec4 at line ",
                    line_number,
                    " of file ",
                    file_path,
                    ". Couldn't parse floats."
                );
            }
            // Assign to diffuse color
            mat_diffuse_color =
                glm::vec4(floats[0], floats[1], floats[2], floats[3]);
        } else {
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
    auto material_config = new MaterialConfig(
        mat_name, mat_diffuse_map_name, mat_diffuse_color, mat_auto_release
    );
    material_config->full_path   = file_path;
    material_config->loader_type = ResourceType::Material;
    return material_config;
}

void MaterialLoader::unload(Resource* resource) {
    CAN_UNLOAD(Material, resource);

    MaterialConfig* res = (MaterialConfig*) resource;
    delete res;
}
