#include "resources/geometry_config.hpp"

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

String GeometryConfig::serialize(const Serializer* const serializer) const {
    auto out_string = serializer->serialize((uint32) indices.size());
    for (const auto& index : indices)
        out_string += serializer->serialize(index);
    out_string += serializer->serialize(
        // Center
        center.x,
        center.y,
        center.z,
        // Max extent
        max_extents.x,
        max_extents.y,
        max_extents.z,
        // Min extent
        min_extents.x,
        min_extents.y,
        min_extents.z,
        // Names
        name,
        material_name,
        // Auto release
        auto_release
    );
    return out_string;
}
Result<uint32, RuntimeError> GeometryConfig::deserialize(
    const String& data, const Serializer* const serializer
) {
    uint32 total_read = 0;

    // Index size
    uint32 index_count;
    auto   result = serializer->deserialize(data, index_count);
    total_read += check(result);

    // Indices
    indices.resize(index_count);
    for (auto& index : indices) {
        result = serializer->deserialize(data.substr(total_read), index);
        total_read += check(result);
    }

    // Rest
    result = serializer->deserialize(
        data.substr(total_read),
        // Center
        center.x,
        center.y,
        center.z,
        // Max extent
        max_extents.x,
        max_extents.y,
        max_extents.z,
        // Min extent
        min_extents.x,
        min_extents.y,
        min_extents.z,
        // Names
        name,
        material_name,
        // Auto release
        auto_release
    );
    total_read += check(result);
    return total_read;
}

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

String GeometryConfig2D::serialize(const Serializer* const serializer) const {
    String out_str {};

    // Vertex type
    out_str += serializer->serialize((uint8) 2);

    // Vertex count
    out_str += serializer->serialize((uint32) vertices.size());

    // Vertices
    for (const auto& vertex : vertices)
        out_str += serializer->serialize(vertex);

    // Rest
    out_str += GeometryConfig::serialize(serializer);

    return out_str;
}

Result<uint32, RuntimeError> GeometryConfig2D::deserialize(
    const String& data, const Serializer* const serializer
) {
    uint32 total_read = 0;

    // Vertex count
    uint32 vertex_count;
    auto   result = serializer->deserialize(data, vertex_count);
    total_read += check(result);

    // Vertices
    vertices.resize(vertex_count);
    for (auto& vertex : vertices) {
        result = serializer->deserialize(data.substr(total_read), vertex);
        total_read += check(result);
    }

    // Rest
    result = GeometryConfig::deserialize(data.substr(total_read), serializer);
    total_read += check(result);
    return total_read;
}

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

String GeometryConfig3D::serialize(const Serializer* const serializer) const {
    String out_str {};

    // Vertex type
    out_str += serializer->serialize((uint8) 3);

    // Vertex count
    out_str += serializer->serialize((uint32) vertices.size());

    // Vertices
    for (const auto& vertex : vertices)
        out_str += serializer->serialize(vertex);

    // Rest
    out_str += GeometryConfig::serialize(serializer);

    return out_str;
}

Result<uint32, RuntimeError> GeometryConfig3D::deserialize(
    const String& data, const Serializer* const serializer
) {
    uint32 total_read = 0;

    // Vertex count
    uint32 vertex_count;
    auto   result = serializer->deserialize(data, vertex_count);
    total_read += check(result);

    // Vertices
    vertices.resize(vertex_count);
    for (auto& vertex : vertices) {
        result = serializer->deserialize(data.substr(total_read), vertex);
        total_read += check(result);
    }

    // Rest
    result = GeometryConfig::deserialize(data.substr(total_read), serializer);
    total_read += check(result);
    return total_read;
}

// -----------------------------------------------------------------------------
// Geometry configuration array
// -----------------------------------------------------------------------------
GeometryConfigArray::GeometryConfigArray(const String& name) : Resource(name) {}
GeometryConfigArray::~GeometryConfigArray() {
    for (auto& config : configs)
        delete config;
    configs.clear();
}