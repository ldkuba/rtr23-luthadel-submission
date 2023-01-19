#include "systems/material_system.hpp"

#define MATERIAL_SYS_LOG "MaterialSystem :: "

MaterialSystem::MaterialSystem(
    Renderer* const       renderer,
    ResourceSystem* const resource_system,
    TextureSystem* const  texture_system,
    ShaderSystem* const   shader_system
)
    : _renderer(renderer), _resource_system(resource_system),
      _texture_system(texture_system), _shader_system(shader_system) {
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
    _default_material->shader()->release_instance_resources(
        _default_material->internal_id.value()
    );
    delete _default_material;

    Logger::trace(MATERIAL_SYS_LOG, "Material system destroyed.");
}

// ////////////////////////////// //
// MATERIAL SYSTEM PUBLIC METHODS //
// ////////////////////////////// //

Material* MaterialSystem::acquire(const String name) {
    Logger::trace(MATERIAL_SYS_LOG, "Material \"", name, "\" requested.");

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

    if (ref != _registered_materials.end()) {
        ref->second.reference_count++;
        Logger::trace(MATERIAL_SYS_LOG, "Material acquired.");
        return ref->second.handle;
    }

    // No material under this name found; load form resource system
    auto config_result = _resource_system->load(name, ResourceType::Material);
    if (config_result.has_error()) {
        Logger::error(
            MATERIAL_SYS_LOG,
            "Material load failed. Returning default_material."
        );
        return _default_material;
    }
    auto material_config = (MaterialConfig*) config_result.value();

    // Create material with loaded configs
    auto material_result = create_material(*material_config);

    // Check for errors
    if (material_result.has_error()) return default_material;
    auto material_ref = material_result.value();

    // Register created material
    _registered_materials[name] = material_ref;

    // Free config
    _resource_system->unload(material_config);

    Logger::trace(MATERIAL_SYS_LOG, "Material \"", name, "\" acquired.");
    return material_ref.handle;
}
Material* MaterialSystem::acquire(const MaterialConfig config) {
    Logger::trace(
        MATERIAL_SYS_LOG, "Material \"", config.name, "\" requested."
    );

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
    auto ref = _registered_materials.find(name);
    if (ref == _registered_materials.end()) {
        // Create material
        auto result = create_material(config);

        // Check for errors
        if (result.has_error()) return default_material;
        auto material_ref = result.value();

        // Register created material
        _registered_materials[name] = material_ref;

        Logger::trace(
            MATERIAL_SYS_LOG, "Material \"", config.name, "\" acquired."
        );
        return material_ref.handle;
    }
    ref->second.reference_count++;

    Logger::trace(MATERIAL_SYS_LOG, "Material \"", config.name, "\" acquired.");
    return ref->second.handle;
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

    Logger::trace(MATERIAL_SYS_LOG, "Material \"", name, "\" released.");
}

// /////////////////////////////// //
// MATERIAL SYSTEM PRIVATE METHODS //
// /////////////////////////////// //

#define DEFAULT_MATERIAL_SHADER_NAME "builtin.material_shader"

void MaterialSystem::create_default_material() {
    // Get shader
    auto shader =
        _shader_system->acquire(DEFAULT_MATERIAL_SHADER_NAME).value_or(nullptr);
    if (shader == nullptr)
        Logger::fatal(
            MATERIAL_SYS_LOG,
            "Unable to create default material. Couldn't create default shader."
        );

    // Create default material
    _default_material = new (MemoryTag::MaterialInstance)
        Material(_default_material_name, shader, glm::vec4(1.0f), 32.0f);

    // Set maps:
    // Diffuse
    TextureMap diffuse_map         = { _texture_system->default_texture,
                                       TextureUse::MapDiffuse };
    _default_material->diffuse_map = diffuse_map;
    // Specular
    TextureMap specular_map = { _texture_system->default_specular_texture,
                                TextureUse::MapSpecular };
    _default_material->specular_map = specular_map;
    // Normal
    TextureMap normal_map           = { _texture_system->default_normal_texture,
                                        TextureUse::MapNormal };
    _default_material->normal_map   = normal_map;

    // TODO: Set other maps

    // Upload material to GPU
    _default_material->internal_id = shader->acquire_instance_resources();

    _default_material->id = 0;
}

Result<MaterialSystem::MaterialRef, bool> MaterialSystem::create_material(
    const MaterialConfig config
) {
    // Get shader
    auto shader = _shader_system->acquire(config.shader).value_or(nullptr);
    if (shader == nullptr) {
        Logger::error(
            MATERIAL_SYS_LOG,
            "Material couldn't be created. Couldn't find \"",
            config.shader,
            "\" shader. Default material returned instead."
        );
        return Failure(false);
    }

    auto material = new (MemoryTag::MaterialInstance)
        Material(config.name, shader, config.diffuse_color, config.shininess);

    // Diffuse map
    TextureMap diffuse_map = {};
    if (config.diffuse_map_name.length() > 0) {
        diffuse_map.use = TextureUse::MapDiffuse;
        diffuse_map.texture =
            _texture_system->acquire(config.diffuse_map_name, true);
    } else {
        // Note: Not needed. Set explicit for readability
        diffuse_map.use     = TextureUse::Unknown;
        diffuse_map.texture = nullptr;
    }
    material->diffuse_map = diffuse_map;

    // Specular map
    TextureMap specular_map = {};
    if (config.specular_map_name.length() > 0) {
        specular_map.use = TextureUse::MapSpecular;
        specular_map.texture =
            _texture_system->acquire(config.specular_map_name, true);
    } else {
        // Note: Not needed. Set explicit for readability
        specular_map.use     = TextureUse::Unknown;
        specular_map.texture = nullptr;
    }
    material->specular_map = specular_map;

    // Normal map
    TextureMap normal_map = {};
    if (config.specular_map_name.length() > 0) {
        normal_map.use = TextureUse::MapNormal;
        normal_map.texture =
            _texture_system->acquire(config.normal_map_name, true);
    } else {
        // Note: Not needed. Set explicit for readability
        normal_map.use     = TextureUse::Unknown;
        normal_map.texture = nullptr;
    }
    material->normal_map = normal_map;

    // TODO: Set other maps

    // Acquire resource from GPU
    material->internal_id = shader->acquire_instance_resources();

    // Assign to reference
    MaterialRef material_ref {};
    // Material
    material_ref.handle          = material;
    // Other
    material_ref.handle->id      = (uint64) material_ref.handle;
    material_ref.auto_release    = config.auto_release;
    material_ref.reference_count = 1;

    return material_ref;
}

void MaterialSystem::destroy_material(Material* material) {
    if (!material->internal_id.has_value())
        Logger::fatal(
            MATERIAL_SYS_LOG,
            "Material \"",
            material->name,
            "\" not properly initialized. Internal id not set."
        );

    const Texture* diffuse_texture  = material->diffuse_map().texture;
    const Texture* specular_texture = material->specular_map().texture;
    const Texture* normal_texture   = material->normal_map().texture;
    if (diffuse_texture) _texture_system->release(diffuse_texture->name());
    if (specular_texture) _texture_system->release(specular_texture->name());
    if (normal_texture) _texture_system->release(normal_texture->name());

    material->shader()->release_instance_resources( //
        material->internal_id.value()
    );
    delete material;
}
