#include "resources/geometry_config.hpp"

namespace ENGINE_NAMESPACE {

// -----------------------------------------------------------------------------
// Geometry config
// -----------------------------------------------------------------------------

GeometryConfig::GeometryConfig() {}
GeometryConfig::GeometryConfig(
    const String          name,
    const Vector<uint32>& indices,
    const glm::vec3       center,
    const glm::vec3       max_extents,
    const glm::vec3       min_extents,
    const String          material_name,
    const bool            auto_release
)
    : name(name), indices(indices), center(center), max_extents(max_extents),
      min_extents(min_extents), material_name(material_name),
      auto_release(auto_release) {}
GeometryConfig::~GeometryConfig() {}

// -----------------------------------------------------------------------------
// Geometry config 2D
// -----------------------------------------------------------------------------

GeometryConfig2D::GeometryConfig2D() {}
GeometryConfig2D::GeometryConfig2D(
    const String            name,
    const Vector<Vertex2D>& vertices,
    const Vector<uint32>&   indices,
    const glm::vec3         center,
    const glm::vec3         max_extents,
    const glm::vec3         min_extents,
    const String            material_name,
    const bool              auto_release
)
    : GeometryConfig(
          name,
          indices,
          center,
          max_extents,
          min_extents,
          material_name,
          auto_release
      ),
      vertices(vertices) {}
GeometryConfig2D::~GeometryConfig2D() {}

// -----------------------------------------------------------------------------
// Geometry config 3D
// -----------------------------------------------------------------------------

GeometryConfig3D::GeometryConfig3D() {}
GeometryConfig3D::GeometryConfig3D(
    const String            name,
    const Vector<Vertex3D>& vertices,
    const Vector<uint32>&   indices,
    const glm::vec3         center,
    const glm::vec3         max_extents,
    const glm::vec3         min_extents,
    const String            material_name,
    const bool              auto_release
)
    : GeometryConfig(
          name,
          indices,
          center,
          max_extents,
          min_extents,
          material_name,
          auto_release
      ),
      vertices(vertices) {}
GeometryConfig3D::~GeometryConfig3D() {}

// -----------------------------------------------------------------------------
// Geometry configuration array
// -----------------------------------------------------------------------------
GeometryConfigArray::GeometryConfigArray(const String& name) : Resource(name) {}
GeometryConfigArray::~GeometryConfigArray() {
    for (auto& config : configs)
        delete config;
    configs.clear();
}

} // namespace ENGINE_NAMESPACE