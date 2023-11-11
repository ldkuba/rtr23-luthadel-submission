#pragma once

#include "component/axis_aligned_bbox.hpp"
#include "serialization/serializable.hpp"
#include "renderer/renderer_types.hpp"
#include "material.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Geometry resource. Represents a virtual geometry. Usually paired with
 * a material.
 */
class Geometry : public Resource {
  public:
    /**
     * @brief Configuration of single n-dimensional geometry.
     * @tparam Dim Geometry config dimension count
     */
    template<uint8 Dim>
    struct Config : public Serializable {
      public:
        uint8 dim_count = Dim;

        Vector<Vertex<Dim>>  vertices { { MemoryTag::Geometry } };
        Vector<uint32>       indices { { MemoryTag::Geometry } };
        AxisAlignedBBox<Dim> bbox;

        String name;
        String material_name;
        bool   auto_release;

        Config() {}
        Config(
            const String                name,
            const Vector<Vertex<Dim>>&  vertices,
            const Vector<uint32>&       indices,
            const AxisAlignedBBox<Dim>& bbox,
            const String                material_name = "",
            const bool                  auto_release  = true
        )
            : name(name), vertices(vertices), indices(indices), bbox(bbox),
              material_name(material_name), auto_release(auto_release) {}
        virtual ~Config() {}

        serializable_attributes(
            dim_count,
            vertices,
            indices,
            bbox,
            name,
            material_name,
            auto_release
        );
    };

    /**
     * @brief Geometry config variant for 2D geometries
     */
    typedef Config<2> Config2D;
    /**
     * @brief Geometry config variant for 3D geometries
     */
    typedef Config<3> Config3D;

  public:
    /// @brief Id used by the Renderer
    std::optional<uint64> internal_id;
    /// @brief Material used by the geometry
    Property<Material*>   material {
        GET { return _material; }
        SET { _material = value; }
    };

    Geometry(String name);
    ~Geometry();

    const static uint32 max_name_length = 256;

  private:
    Material* _material = nullptr;
};

class Geometry2D : public Geometry {
  public:
    typedef AxisAlignedBBox<2> BBox;

    Geometry2D(const String& name, const BBox& bbox)
        : Geometry(name), _bbox(bbox) {}

  private:
    BBox _bbox;
};

class Geometry3D : public Geometry {
  public:
    typedef AxisAlignedBBox<3> BBox;

    Geometry3D(const String& name, const BBox& bbox)
        : Geometry(name), _bbox(bbox) {}

  private:
    AxisAlignedBBox<3> _bbox;
};

/**
 * @brief Geometry configuration resource
 */
class GeometryConfigArray : public Resource {
  public:
    Vector<Geometry::Config<3>*> configs {};

    GeometryConfigArray(const String& name) : Resource(name) {}
    ~GeometryConfigArray() {
        for (auto& config : configs)
            del(config);
        configs.clear();
    }
};

} // namespace ENGINE_NAMESPACE