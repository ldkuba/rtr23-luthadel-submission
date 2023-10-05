#pragma once

#include "material_system.hpp"
#include "resources/geometry.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Geometry system is responsible for the management of geometries, as
 * well as reference counting.
 */
class GeometrySystem {
  public:
    /// @brief Default fallback geometry
    Property<Geometry*> default_geometry {
        GET { return _default_geometry; }
    };
    /// @brief Default fallback 2D geometry
    Property<Geometry*> default_2d_geometry {
        GET { return _default_2d_geometry; }
    };

    /**
     * @brief Construct a new Geometry System object
     *
     * @param renderer Renderer used by the system
     * @param material_system Material system used
     */
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
     * @param config Geometry configuration
     * @returns Created geometry resource
     */
    Geometry* acquire(const GeometryConfig& config);

    /**
     * @brief Releases geometry resource. Geometry system will automatically
     * release this geometry from memory if no other references to it are
     * detected and auto release flag is set to true.
     * @param geometry Geometry to release
     */
    void release(Geometry* geometry);

    // Generators
    /**
     * @brief Generates 1 by 1 by 1 cube geometry
     *
     * @param name Geometry name
     * @param material_name Material name
     * @param auto_release If enabled geometry system will automaticaly release
     * the geometry resource from memory if no references to it are detected.
     * @returns Created cube geometry
     */
    Geometry* generate_cube(
        const String name,
        const String material_name,
        const bool   auto_release = true
    );

    // Utility methods
    static void generate_normals(
        Vector<Vertex3D>& vertices, Vector<uint32> const& indices
    );
    static void generate_tangents(
        Vector<Vertex3D>& vertices, Vector<uint32> const& indices
    );

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

} // namespace ENGINE_NAMESPACE