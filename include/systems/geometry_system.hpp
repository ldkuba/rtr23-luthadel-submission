#pragma once

#include "material_system.hpp"

class GeometrySystem {
  public:
    /// @brief Default fallback geometry
    Property<Geometry*> default_geometry {
        Get { return _default_geometry; }
    };
    /// @brief Default fallback 2D geometry
    Property<Geometry*> default_2d_geometry {
        Get { return _default_2d_geometry; }
    };

    GeometrySystem(
        Renderer* const renderer, MaterialSystem* const material_system
    );
    ~GeometrySystem();

    // Prevent accidental copying
    GeometrySystem(GeometrySystem const&)            = delete;
    GeometrySystem& operator=(GeometrySystem const&) = delete;

    /**
     * @brief Acquire already loaded geometry resource.
     *
     * @param id Requested geometry id
     * @return Geometry*
     */
    Geometry* acquire(const uint32 id);
    /**
     * @brief Creates new geometry resource and load its material
     *
     * @tparam VertexType Vertex (Vertex3D) or Vertex2D
     * @param vertices Vertex data
     * @param indices Index data
     * @param name Geometry's name
     * @param material_name Material to be loaded
     * @param auto_release If enabled geometry system will automaticaly release
     * the geometry resource from memory if no references to it are detected.
     * @returns Created geometry resource
     */
    template<typename VertexType>
    Geometry* acquire(
        const String              name,
        const Vector<VertexType>& vertices,
        const Vector<uint32>&     indices,
        const String              material_name,
        bool                      auto_release = true
    );

    /**
     * @brief Releases geometry resource. Geometry system will automatically
     * release this geometry from memory if no other references to it are
     * detected and auto release flag is set to true.
     * @param geometry Geometry to release
     */
    void release(Geometry* geometry);

  private:
    struct GeometryRef {
        Geometry* handle;
        uint64    reference_count;
        bool      auto_release;
    };

    Renderer*       _renderer;
    MaterialSystem* _material_system;

    // Number of geometries that can be loaded at once.
    // NOTE: Should be significantly higher than the maximum number of static
    // meshes.
    const uint32 _max_geometry_count    = 1024 * 8;
    const String _default_geometry_name = "default";

    Geometry* _default_geometry    = nullptr;
    Geometry* _default_2d_geometry = nullptr;

    UnorderedMap<uint32, GeometryRef> _registered_geometries = {};

    void create_default_geometries();
};

inline uint32 generate_id() { // TODO: TEMP
    static uint32 id = 0;
    return id++;
}
template<typename VertexType>
Geometry* GeometrySystem::acquire(
    const String              name,
    const Vector<VertexType>& vertices,
    const Vector<uint32>&     indices,
    const String              material_name,
    bool                      auto_release
) {
    // Generate unique id
    auto id = generate_id();

    // Register new slot
    auto& ref           = _registered_geometries[id];
    ref.auto_release    = auto_release;
    ref.reference_count = 1;

    // Crete geometry
    ref.handle     = new (MemoryTag::Resource) Geometry(name);
    ref.handle->id = id;
    _renderer->create_geometry(ref.handle, vertices, indices);

    // Acquire material
    if (material_name != "") {
        ref.handle->material = _material_system->acquire(material_name);
        if (!ref.handle->material)
            ref.handle->material = _material_system->default_material();
    }

    return ref.handle;
}