#include "systems/material_system.hpp"

MaterialSystem::MaterialSystem(
    Renderer* const renderer,
    TextureSystem* const texture_system
) : _renderer(renderer), _texture_system(texture_system) {
    if (_max_material_count == 0)
        Logger::fatal("MaterialSystem :: _max_material_count must be greater than 0.");
    create_default_material();
}
MaterialSystem::~MaterialSystem() {
    for (auto material : _registered_materials) {
        _renderer->destroy_material(material.second.handle);
        delete material.second.handle;
    }
    _registered_materials.clear();
    _renderer->destroy_material(_default_material);
    delete _default_material;
}

// ////////////////////////////// //
// MATERIAL SYSTEM PUBLIC METHODS //
// ////////////////////////////// //

#define MATERIAL_FOLDER "./assets/textures/"
#include "file_system.hpp"

#define M_SETTING_version "version"
#define M_SETTING_name "name"
#define M_SETTING_diffuse_color "diffuse_color"
#define M_SETTING_diffuse_map_name "diffuse_map_name"

Material* MaterialSystem::acquire(const String name) {
    // TODO: TEMP CODE BEGIN
    // === Material configuration ===
    String mat_name;
    bool mat_auto_release;
    glm::vec4 mat_diffuse_color;
    String mat_diffuse_map_name;

    // === Load material configuration from file ===
    String file_name = name + ".mat"; file_name.to_lower();
    String file_path = MATERIAL_FOLDER + file_name;

    std::vector<String> material_settings;
    try {
        material_settings = FileSystem::read_file_lines(file_path);
    } catch (FileSystemException e) { Logger::fatal(e.what()); }

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
            Logger::warning("MaterialSystem :: Potential formatting issue ",
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
                Logger::warning("MaterialSystem :: Couldn't load material name at line ",
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
                Logger::warning("MaterialSystem :: Couldn't parse vec4 at line ",
                    line_number, " of file ", file_path, ". Wrong argument count.");
            }
            std::vector<float32> floats(str_floats.size());
            try {
                for (uint32 i = 0; i < str_floats.size(); i++)
                    floats[i] = str_floats[i].parse_as_float32();
            } catch (std::invalid_argument e) {
                Logger::warning("MaterialSystem :: Couldn't parse vec4 at line ",
                    line_number, " of file ", file_path, ". Couldn't parse floats.");
            }
            // Assign to diffuse color
            mat_diffuse_color = glm::vec4(floats[0], floats[1], floats[2], floats[3]);
        } else {
            Logger::warning("Material System :: Invalid variable : \"",
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
    if (name.length() > Material::max_name_length) {
        Logger::error("MaterialSystem :: Material couldn't be acquired. ",
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
        ref.handle = new Material(
            name,
            diffuse_color,
            diffuse_map_name,
            _texture_system
        );
        ref.handle->id = (uint64) ref.handle;
        ref.auto_release = auto_release;
        ref.reference_count = 0;

        // Upload material to GPU
        _renderer->create_material(ref.handle);
    }
    ref.reference_count++;

    return ref.handle;
}

void MaterialSystem::release(const String name) {
    if (name.compare_ci(_default_material_name) == 0) {
        Logger::warning("MaterialSystem :: Cannot release default material.");
        return;
    }

    String s = name; s.to_lower();
    auto ref = _registered_materials.find(s);

    if (ref == _registered_materials.end() || ref->second.reference_count == 0) {
        Logger::warning("MaterialSystem :: Tried to release a non-existent material: ", name);
        return;
    }
    ref->second.reference_count--;
    if (ref->second.reference_count == 0 && ref->second.auto_release == true) {
        _renderer->destroy_material(ref->second.handle);
        delete ref->second.handle;
        _registered_materials.erase(s);
    }
}

// /////////////////////////////// //
// MATERIAL SYSTEM PRIVATE METHODS //
// /////////////////////////////// //

void MaterialSystem::create_default_material() {
    _default_material = new Material(
        _default_material_name,
        glm::vec4(1.0f),
        _texture_system->default_texture()->name,
        _texture_system,
        true
    );
    // Upload material to GPU
    _renderer->create_material(_default_material);
}