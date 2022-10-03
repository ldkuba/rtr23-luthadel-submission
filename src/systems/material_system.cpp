#include "systems/material_system.hpp"

#define MATERIAL_SYS_LOG "MaterialSystem :: "

MaterialSystem::MaterialSystem(
    Renderer* const renderer,
    TextureSystem* const texture_system
) : _renderer(renderer), _texture_system(texture_system) {
    Logger::trace(MATERIAL_SYS_LOG, "Creating material system.");

    if (_max_material_count == 0)
        Logger::fatal(MATERIAL_SYS_LOG, "Const _max_material_count must be greater than 0.");
    create_default_material();

    Logger::trace(MATERIAL_SYS_LOG, "Material system created.");
}
MaterialSystem::~MaterialSystem() {
    for (auto material : _registered_materials) {
        _renderer->destroy_material(material.second.handle);
        delete material.second.handle;
    }
    _registered_materials.clear();
    _renderer->destroy_material(_default_material);
    delete _default_material;

    Logger::trace(MATERIAL_SYS_LOG, "Material system destroyed.");
}

// ////////////////////////////// //
// MATERIAL SYSTEM PUBLIC METHODS //
// ////////////////////////////// //

#define MATERIAL_FOLDER "./assets/materials/"
#include "file_system.hpp"

#define M_SETTING_version "version"
#define M_SETTING_name "name"
#define M_SETTING_diffuse_color "diffuse_color"
#define M_SETTING_diffuse_map_name "diffuse_map_name"

Material* MaterialSystem::acquire(const String name) {
    // TODO: TEMP CODE BEGIN
    // === Material configuration ===
    String mat_name;
    bool mat_auto_release = true;
    glm::vec4 mat_diffuse_color;
    String mat_diffuse_map_name;

    // === Load material configuration from file ===
    String file_name = name + ".mat"; file_name.to_lower();
    String file_path = MATERIAL_FOLDER + file_name;

    std::vector<String> material_settings;
    try {
        material_settings = FileSystem::read_file_lines(file_path);
    } catch (FileSystemException e) { Logger::fatal(MATERIAL_SYS_LOG, e.what()); }

    // === Parse loaded config ==
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
            Logger::warning(MATERIAL_SYS_LOG, "Potential formatting issue ",
                "with the number of = tokens found. ", "Skipping line ", line_number,
                " of file ", file_path, ".");
            line_number++;
            continue;
        }

        auto setting_var = setting[0]; setting_var.trim(); setting_var.to_lower();
        auto setting_val = setting[1]; setting_val.trim();

        // Process variable and its argument
        if (setting_var.compare(M_SETTING_version) == 0) {
            // TODO: Versions
        } else if (setting_var.compare(M_SETTING_name) == 0) {
            if (setting_val.length() > Material::max_name_length)
                Logger::warning(MATERIAL_SYS_LOG, "Couldn't load material name at line ",
                    line_number, " of file ", file_path, ". Name is too long (", setting_val.length(),
                    " characters, while maximum name length is ", Material::max_name_length,
                    " characters).");
            else mat_name = setting_val;
        } else if (setting_var.compare(M_SETTING_diffuse_map_name) == 0) {
            mat_diffuse_map_name = setting_val;
        } else if (setting_var.compare(M_SETTING_diffuse_color) == 0) {
            // Parse vec4
            auto str_floats = setting_val.split(' ');
            if (str_floats.size() != 4) {
                Logger::warning(MATERIAL_SYS_LOG, "Couldn't parse vec4 at line ",
                    line_number, " of file ", file_path, ". Wrong argument count.");
            }
            std::vector<float32> floats(str_floats.size());
            try {
                for (uint32 i = 0; i < str_floats.size(); i++)
                    floats[i] = str_floats[i].parse_as_float32();
            } catch (std::invalid_argument e) {
                Logger::warning(MATERIAL_SYS_LOG, "Couldn't parse vec4 at line ",
                    line_number, " of file ", file_path, ". Couldn't parse floats.");
            }
            // Assign to diffuse color
            mat_diffuse_color = glm::vec4(floats[0], floats[1], floats[2], floats[3]);
        } else {
            Logger::warning(MATERIAL_SYS_LOG, " Invalid variable : \"",
                setting_var, "\" at line ", line_number, " of file ", file_path, ".");
        }

        line_number++;
    }
    // TODO: TEMP CODE END

    return acquire(
        mat_name,
        mat_auto_release,
        mat_diffuse_color,
        mat_diffuse_map_name
    );
}
Material* MaterialSystem::acquire(
    const String name,
    const bool auto_release,
    const glm::vec4 diffuse_color,
    const String diffuse_map_name
) {
    Logger::trace(MATERIAL_SYS_LOG, "Material requested.");

    if (name.length() > Material::max_name_length) {
        Logger::error(MATERIAL_SYS_LOG, "Material couldn't be acquired. ",
            "Maximum name length of a material is ", Material::max_name_length,
            " characters but ", name.length(), " character long name was passed. ",
            "Default material acquired instead.");
        return _default_material;
    }
    if (name.compare_ci(_default_material_name) == 0) {
        return _default_material;
    }

    String s = name; s.to_lower();
    auto ref = _registered_materials[s];

    if (ref.handle == nullptr) {
        ref.handle = crete_material(
            name,
            diffuse_map_name,
            diffuse_color
        );
        ref.handle->id = (uint64) ref.handle;
        ref.auto_release = auto_release;
        ref.reference_count = 0;

        // Upload material to GPU
        _renderer->create_material(ref.handle);
    }
    ref.reference_count++;

    Logger::trace(MATERIAL_SYS_LOG, "Material acquired.");
    return ref.handle;
}

void MaterialSystem::release(const String name) {
    if (name.compare_ci(_default_material_name) == 0) {
        Logger::warning(MATERIAL_SYS_LOG, "Cannot release default material.");
        return;
    }

    String s = name; s.to_lower();
    auto ref = _registered_materials.find(s);

    if (ref == _registered_materials.end() || ref->second.reference_count == 0) {
        Logger::warning(MATERIAL_SYS_LOG, "Tried to release a non-existent material: ", name);
        return;
    }
    ref->second.reference_count--;
    if (ref->second.reference_count == 0 && ref->second.auto_release == true) {
        _renderer->destroy_material(ref->second.handle);
        delete ref->second.handle;
        _registered_materials.erase(s);
    }

    Logger::trace(MATERIAL_SYS_LOG, "Material released.");
}

// /////////////////////////////// //
// MATERIAL SYSTEM PRIVATE METHODS //
// /////////////////////////////// //

void MaterialSystem::create_default_material() {
    _default_material = new Material(
        _default_material_name,
        glm::vec4(1.0f)
    );
    TextureMap diffuse_map = { _texture_system->default_texture, TextureUse::MapDiffuse };
    _default_material->diffuse_map = diffuse_map;

    // TODO: Set other maps

    // Upload material to GPU
    _renderer->create_material(_default_material);
}

Material* MaterialSystem::crete_material(
    const String name,
    const String diffuse_material_name,
    const glm::vec4 diffuse_color
) {
    auto material = new Material(
        name,
        diffuse_color
    );

    TextureMap diffuse_map;
    if (diffuse_material_name.length() > 0) {
        diffuse_map.use = TextureUse::MapDiffuse;
        Logger::debug(diffuse_material_name);
        diffuse_map.texture = _texture_system->acquire(diffuse_material_name, true);
    } else {
        diffuse_map.use = TextureUse::Unknown;
        diffuse_map.texture = nullptr;
    }
    material->diffuse_map = diffuse_map;
    // TODO: Set other maps

    // Upload material to GPU
    _renderer->create_material(material);

    return material;
}