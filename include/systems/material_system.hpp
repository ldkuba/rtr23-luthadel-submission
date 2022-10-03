#pragma once

#include "resources/material.hpp"
#include "texture_system.hpp"

class MaterialSystem {
public:
    Property<Material*> default_material{ Get { return _default_material; } };

    MaterialSystem(Renderer* const renderer, TextureSystem* const texture_system);
    ~MaterialSystem();

    Material* acquire(const String name);
    Material* acquire(
        const String name,
        const bool auto_release,
        const glm::vec4 diffuse_color,
        const String diffuse_map_name
    );
    void release(const String name);

private:
    struct MaterialRef {
        Material* handle;
        uint64 reference_count;
        bool auto_release;
    };

    Renderer* _renderer;
    TextureSystem* _texture_system;

    const uint64 _max_material_count = 1024;
    const String _default_material_name = "default";

    Material* _default_material = nullptr;
    std::map<const String, MaterialRef> _registered_materials;

    void create_default_material();

    Material* crete_material(
        const String name,
        const String diffuse_material_name,
        const glm::vec4 diffuse_color
    );
};
