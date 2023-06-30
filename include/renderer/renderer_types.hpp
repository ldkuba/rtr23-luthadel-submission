#pragma once

#include "math_libs.hpp"
#include "serialization/serializer.hpp"

// -----------------------------------------------------------------------------
// Vertex
// -----------------------------------------------------------------------------
/**
 * @brief Vertex in 3D space
 *
 */
struct Vertex3D : public Serializable {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 tangent;
    glm::vec4 color;
    glm::vec2 texture_coord;

    Vertex3D() {}
    Vertex3D(
        const glm::vec3 position,
        const glm::vec3 normal,
        const glm::vec4 tangent,
        const glm::vec4 color,
        const glm::vec2 texture_coord
    )
        : position(position), normal(normal), tangent(tangent), color(color),
          texture_coord(texture_coord) {}
    ~Vertex3D() {}

    serializable_attributes(
        position.x,
        position.y,
        position.z,
        normal.x,
        normal.y,
        normal.z,
        tangent.x,
        tangent.y,
        tangent.z,
        tangent.w,
        color.x,
        color.y,
        color.z,
        color.w,
        texture_coord.x,
        texture_coord.y
    );

    bool operator==(const Vertex3D& other) const {
        return other.position == position && //
               other.normal == normal &&     //
               other.tangent == tangent &&   //
               other.color == color &&       //
               other.texture_coord == texture_coord;
    }
};

/**
 * @brief Vertex in 2D plane
 *
 */
struct Vertex2D : public Serializable {
    glm::vec2 position;
    glm::vec2 texture_coord;

    serializable_attributes(
        position.x, position.y, texture_coord.x, texture_coord.y
    );

    Vertex2D() {}
    Vertex2D(const glm::vec2 position, const glm::vec2 texture_coord)
        : position(position), texture_coord(texture_coord) {}
    ~Vertex2D() {}

    bool operator==(const Vertex2D& other) const {
        return other.position == position &&
               other.texture_coord == texture_coord;
    }
};

typedef Vertex3D Vertex;

namespace std {
template<>
struct hash<Vertex> {
    size_t operator()(Vertex const& vertex) const {
        return hash<glm::vec3>()(vertex.position) ^
               hash<glm::vec2>()(vertex.texture_coord);
    }
};
} // namespace std

// -----------------------------------------------------------------------------
// Render data
// -----------------------------------------------------------------------------
/**
 * @brief List of builtin render passes
 */
enum BuiltinRenderPass : uint8 { World = 0x1, UI = 0x2 };

/**
 * @brief Render debug view modes
 */
enum DebugViewMode : uint8 { Default, Lighting, Normals };

class Geometry;

/**
 * @brief Geometry render packet
 */
struct GeometryRenderData {
    Geometry* geometry;
};