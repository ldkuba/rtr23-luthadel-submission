#pragma once

#include "texture_system.hpp"

class MaterialSystem {
  public:
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

    Material* acquire(const String name);
    Material* acquire(const MaterialConfig config);
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
        const String    name,
        const String    diffuse_material_name,
        const glm::vec4 diffuse_color
    );
};
