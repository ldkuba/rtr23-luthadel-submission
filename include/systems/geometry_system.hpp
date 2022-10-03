#pragma once

#include "material_system.hpp"

class GeometrySystem {
public:
    Property<Geometry*> default_geometry{ Get { return _default_geometry; } };

    GeometrySystem(Renderer* const renderer, MaterialSystem* const material_system);
    ~GeometrySystem();

    Geometry* acquire(const uint32 id);
    Geometry* acquire(
        const std::vector<Vertex> vertices,
        const std::vector<uint32> indices,
        const String name,
        const String material_name,
        bool auto_release = true
    );
    void release(Geometry* geometry);

private:
    struct GeometryRef {
        Geometry* handle;
        uint64 reference_count;
        bool auto_release;
    };

    Renderer* _renderer;
    MaterialSystem* _material_system;

    // Number of geometries that can be loaded at once.
    // NOTE: Should be significantly higher than the maximum number of static meshes.
    const uint32 _max_geometry_count = 1024 * 8;
    const String _default_geometry_name = "default";

    Geometry* _default_geometry = nullptr;

    std::map<uint32, GeometryRef> _registered_geometries;

    void create_default_geometry();
};
