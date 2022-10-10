#include "systems/material_system.hpp"

#define MATERIAL_SYS_LOG "MaterialSystem :: "

MaterialSystem::MaterialSystem(
    Renderer* const       renderer,
    ResourceSystem* const resource_system,
    TextureSystem* const  texture_system
)
    : _renderer(renderer), _resource_system(resource_system),
      _texture_system(texture_system) {
    Logger::trace(MATERIAL_SYS_LOG, "Creating material system.");

    if (_max_material_count == 0)
        Logger::fatal(
            MATERIAL_SYS_LOG,
            "Const _max_material_count must be greater than 0."
        );
    create_default_material();

    Logger::trace(MATERIAL_SYS_LOG, "Material system created.");
}
MaterialSystem::~MaterialSystem() {
    for (auto& material : _registered_materials)
        destroy_material(material.second.handle);
    _registered_materials.clear();
    _renderer->destroy_material(_default_material);
    delete _default_material;

    Logger::trace(MATERIAL_SYS_LOG, "Material system destroyed.");
}

// ////////////////////////////// //
// MATERIAL SYSTEM PUBLIC METHODS //
// ////////////////////////////// //

Material* MaterialSystem::acquire(const String name) {
    Logger::trace(MATERIAL_SYS_LOG, "Material requested.");

    // Check name validity
    if (name.length() > Material::max_name_length) {
        Logger::error(
            MATERIAL_SYS_LOG,
            "Material couldn't be acquired. ",
            "Maximum name length of a material is ",
            Material::max_name_length,
            " characters but ",
            name.length(),
            " character long name was passed. ",
            "Default material acquired instead."
        );
        Logger::trace(MATERIAL_SYS_LOG, "Default material acquired.");
        return _default_material;
    }
    if (name.compare_ci(_default_material_name) == 0) {
        return _default_material;
        Logger::trace(MATERIAL_SYS_LOG, "Default material acquired.");
    }

    String s = name;
    s.to_lower();
    auto ref = _registered_materials.find(s);

    if (ref == _registered_materials.end()) {
        // No material under this name found; load form resource system
        MaterialConfig* material_config = static_cast<MaterialConfig*>(
            _resource_system->load(name, ResourceType::Material)
        );
        auto material = acquire(*material_config);
        _resource_system->unload(material_config);
        Logger::trace(MATERIAL_SYS_LOG, "Material acquired.");
        return material;
    }

    ref->second.reference_count++;
    Logger::trace(MATERIAL_SYS_LOG, "Material acquired.");
    return ref->second.handle;
}
Material* MaterialSystem::acquire(const MaterialConfig config) {
    Logger::trace(MATERIAL_SYS_LOG, "Material requested.");

    // Check name validity
    String name = config.name;
    name.to_lower();
    if (name.length() > Material::max_name_length) {
        Logger::error(
            MATERIAL_SYS_LOG,
            "Material couldn't be acquired. ",
            "Maximum name length of a material is ",
            Material::max_name_length,
            " characters but ",
            name.length(),
            " character long name was passed. ",
            "Default material acquired instead."
        );
        Logger::trace(MATERIAL_SYS_LOG, "Default material acquired.");
        return _default_material;
    }
    if (name.compare(_default_material_name) == 0) {
        Logger::trace(MATERIAL_SYS_LOG, "Default material acquired.");
        return _default_material;
    }

    // Get reference
    auto& ref = _registered_materials[name];
    if (ref.handle == nullptr) {
        // Material was just added
        ref.handle =
            crete_material(name, config.diffuse_map_name, config.diffuse_color);
        ref.handle->id      = (uint64) ref.handle;
        ref.auto_release    = config.auto_release;
        ref.reference_count = 0;
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

    String s = name;
    s.to_lower();
    auto ref = _registered_materials.find(s);

    if (ref == _registered_materials.end() ||
        ref->second.reference_count == 0) {
        Logger::warning(
            MATERIAL_SYS_LOG, "Tried to release a non-existent material: ", name
        );
        return;
    }
    ref->second.reference_count--;

    // Release resource if it isn't needed
    if (ref->second.reference_count == 0 && ref->second.auto_release == true) {
        destroy_material(ref->second.handle);
        _registered_materials.erase(s);
    }

    Logger::trace(MATERIAL_SYS_LOG, "Material released.");
}

// /////////////////////////////// //
// MATERIAL SYSTEM PRIVATE METHODS //
// /////////////////////////////// //

void MaterialSystem::create_default_material() {
    _default_material = new Material(_default_material_name, glm::vec4(1.0f));
    TextureMap diffuse_map         = { _texture_system->default_texture,
                                       TextureUse::MapDiffuse };
    _default_material->diffuse_map = diffuse_map;

    // TODO: Set other maps

    // Upload material to GPU
    _renderer->create_material(_default_material);
}

Material* MaterialSystem::crete_material(
    const String    name,
    const String    diffuse_material_name,
    const glm::vec4 diffuse_color
) {
    auto material = new Material(name, diffuse_color);

    TextureMap diffuse_map = {};
    if (diffuse_material_name.length() > 0) {
        diffuse_map.use = TextureUse::MapDiffuse;
        diffuse_map.texture =
            _texture_system->acquire(diffuse_material_name, true);
    } else {
        // Note: Not needed. Set explicit for readability
        diffuse_map.use     = TextureUse::Unknown;
        diffuse_map.texture = nullptr;
    }
    material->diffuse_map = diffuse_map;
    // TODO: Set other maps

    // Upload material to GPU
    _renderer->create_material(material);

    return material;
}

void MaterialSystem::destroy_material(Material* material) {
    String texture_name = material->diffuse_map().texture->name;
    _texture_system->release(texture_name);
    _renderer->destroy_material(material);
    delete material;
}
