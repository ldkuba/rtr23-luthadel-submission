#pragma once

#include "texture_system.hpp"

class MaterialSystem {
  public:
    /// @brief Default fallback material
    Property<Material*> default_material {
        Get { return _default_material; }
    };

    MaterialSystem(
        Renderer* const       renderer,
        ResourceSystem* const resource_system,
        TextureSystem* const  texture_system
    );
    ~MaterialSystem();

    // Prevent accidental copying
    MaterialSystem(MaterialSystem const&)            = delete;
    MaterialSystem& operator=(MaterialSystem const&) = delete;

    /// @brief Acquire material resource from the material system. Materia
    /// system will load requested material from the appropriate location if
    /// it isn't already loaded.
    /// @param name Name of the requested material
    /// @param auto_release If enabled material system will automaticaly release
    /// the material resource from memory if no references to it are detected.
    /// Can only be set if the material resource isn't loaded yet.
    /// @returns Requested material resource
    Material* acquire(const String name);
    /// @brief Acquire material resource from the material system. Materia
    /// system will create requested material with the given settings if
    /// material with config.name isn't already loaded.
    /// @param config Material configuration
    /// @returns Requested material resource
    Material* acquire(const MaterialConfig config);
    /// @brief Releases material resource. Material system will automatically
    /// release this material from memory if no other references to it are
    /// detected and auto release flag is set to true.
    /// @param name Name of the released material
    void      release(const String name);

  private:
    struct MaterialRef {
        Material* handle;
        uint64    reference_count;
        bool      auto_release;
    };

    Renderer*       _renderer;
    ResourceSystem* _resource_system;
    TextureSystem*  _texture_system;

    const uint64 _max_material_count    = 1024;
    const String _default_material_name = "default";

    Material*                               _default_material     = nullptr;
    std::unordered_map<String, MaterialRef> _registered_materials = {};

    void create_default_material();

    Material* crete_material(
        const String       name,
        const MaterialType type,
        const String       diffuse_material_name,
        const glm::vec4    diffuse_color
    );
    void destroy_material(Material* material);
};
