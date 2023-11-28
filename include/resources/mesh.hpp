#pragma once

#include "geometry.hpp"
#include "component/transform.hpp"

namespace ENGINE_NAMESPACE {

class Mesh {
  public:
    Transform transform;

    /// @brief Geometries data of this mesh
    Property<Vector<Geometry*>> geometries {
        GET { return _geometries; }
    };

    Mesh(
        const Vector<Geometry*>& geometries,
        const Transform&         inital_transform = {}
    );
    Mesh(Geometry* const geometry, const Transform& inital_transform = {});
    ~Mesh();

  private:
    Vector<Geometry*> _geometries;
};

} // namespace ENGINE_NAMESPACE