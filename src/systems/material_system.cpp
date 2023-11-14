#include "systems/material_system.hpp"

namespace ENGINE_NAMESPACE {

#define MATERIAL_SYS_LOG "MaterialSystem :: "

// Constructor & Destructor
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
    _default_material->release_map_resources();
    _renderer->destroy_texture_map(_default_material->diffuse_map);
    _renderer->destroy_texture_map(_default_material->specular_map);
    _renderer->destroy_texture_map(_default_material->normal_map);
    del(_default_material);

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

    const auto key = name.lower_c();
    auto       ref = _registered_materials.find(key);
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
    auto material_config = (Material::Config*) config_result.value();

    // Create material with loaded configs
    auto material_result = create_material(*material_config);

    // Check for errors
    if (material_result.has_error()) return default_material;
    auto material_ref = material_result.value();

    // Register created material
    _registered_materials[key] = material_ref;

    // Free config
    _resource_system->unload(material_config);

    Logger::trace(MATERIAL_SYS_LOG, "Material \"", name, "\" acquired.");
    return material_ref.handle;
}
Material* MaterialSystem::acquire(const Material::Config& config) {
    Logger::trace(
        MATERIAL_SYS_LOG, "Material \"", config.name, "\" requested."
    );

    // Check name validity
    const auto name = config.name().lower_c();
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

    const auto key = name.lower_c();
    auto       ref = _registered_materials.find(key);

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
        _registered_materials.erase(key);
    }

    Logger::trace(MATERIAL_SYS_LOG, "Material \"", name, "\" released.");
}

// /////////////////////////////// //
// MATERIAL SYSTEM PRIVATE METHODS //
// /////////////////////////////// //

void MaterialSystem::create_default_material() {
    // Get shader
    auto shader = _shader_system->acquire(Shader::BuiltIn::MaterialShader)
                      .value_or(nullptr);
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
    _default_material->diffuse_map = _renderer->create_texture_map(
        { _texture_system->default_diffuse_texture,
          Texture::Use::MapDiffuse,
          Texture::Filter::BiLinear,
          Texture::Filter::BiLinear,
          Texture::Repeat::Repeat,
          Texture::Repeat::Repeat,
          Texture::Repeat::Repeat }
    );
    // Specular
    _default_material->specular_map = _renderer->create_texture_map(
        { _texture_system->default_specular_texture,
          Texture::Use::MapSpecular,
          Texture::Filter::BiLinear,
          Texture::Filter::BiLinear,
          Texture::Repeat::Repeat,
          Texture::Repeat::Repeat,
          Texture::Repeat::Repeat }
    );
    // Normal
    _default_material->normal_map =
        _renderer->create_texture_map({ _texture_system->default_normal_texture,
                                        Texture::Use::MapNormal,
                                        Texture::Filter::BiLinear,
                                        Texture::Filter::BiLinear,
                                        Texture::Repeat::Repeat,
                                        Texture::Repeat::Repeat,
                                        Texture::Repeat::Repeat });

    // TODO: Set other maps

    // Upload material to GPU
    _default_material->acquire_map_resources();

    _default_material->id = 0;
}

#define acquire_texture(name, or_default)                                      \
    (name.length() > 0) ? _texture_system->acquire(name, true, or_default)     \
                        : or_default

Result<MaterialSystem::MaterialRef, RuntimeError> //
MaterialSystem::create_material(const Material::Config& config) {
    // Get shader
    auto shader = _shader_system->acquire(config.shader).value_or(nullptr);
    if (shader == nullptr) {
        const auto error_message = String::build(
            "Material couldn't be created. Couldn't find \"",
            config.shader,
            "\" shader. Default material returned instead."
        );
        Logger::error(MATERIAL_SYS_LOG, error_message);
        return Failure(error_message);
    }

    auto material = new (MemoryTag::MaterialInstance)
        Material(config.name, shader, config.diffuse_color, config.shininess);

    // TODO: Make filter and repeat configurable
    // Diffuse map
    material->diffuse_map = _renderer->create_texture_map(
        { acquire_texture(
              config.diffuse_map_name, _texture_system->default_diffuse_texture
          ),
          Texture::Use::MapDiffuse,
          Texture::Filter::BiLinear,
          Texture::Filter::BiLinear,
          Texture::Repeat::Repeat,
          Texture::Repeat::Repeat,
          Texture::Repeat::Repeat }
    );
    // Specular map
    material->specular_map = _renderer->create_texture_map(
        { acquire_texture(
              config.specular_map_name,
              _texture_system->default_specular_texture
          ),
          Texture::Use::MapSpecular,
          Texture::Filter::BiLinear,
          Texture::Filter::BiLinear,
          Texture::Repeat::Repeat,
          Texture::Repeat::Repeat,
          Texture::Repeat::Repeat }
    );
    // Normal map
    material->normal_map = _renderer->create_texture_map(
        { acquire_texture(
              config.normal_map_name, _texture_system->default_normal_texture
          ),
          Texture::Use::MapNormal,
          Texture::Filter::BiLinear,
          Texture::Filter::BiLinear,
          Texture::Repeat::Repeat,
          Texture::Repeat::Repeat,
          Texture::Repeat::Repeat }
    );
    // TODO: Set other maps

    // Acquire resource from GPU
    material->acquire_map_resources();

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

    // Release Textures & Texture map resources
    if (material->diffuse_map()) {
        const Texture* diffuse_texture = material->diffuse_map()->texture;
        if (diffuse_texture) _texture_system->release(diffuse_texture->name());
        _renderer->destroy_texture_map(material->diffuse_map());
    }
    if (material->specular_map()) {
        const Texture* specular_texture = material->specular_map()->texture;
        if (specular_texture)
            _texture_system->release(specular_texture->name());
        _renderer->destroy_texture_map(material->specular_map());
    }
    if (material->normal_map()) {
        const Texture* normal_texture = material->normal_map()->texture;
        if (normal_texture) _texture_system->release(normal_texture->name());
        _renderer->destroy_texture_map(material->normal_map());
    }

    // Release GPU map resources
    material->release_map_resources();

    // Delete material
    del(material);
}

} // namespace ENGINE_NAMESPACE