#pragma once

#include "resources/geometry.hpp"

// Vertex
/**
 * @brief Vertex in 3D space
 *
 */
struct Vertex3D {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texture_coord;

    bool operator==(const Vertex3D& other) const {
        return position == other.position && normal == other.normal &&
               texture_coord == other.texture_coord;
    }
};

/**
 * @brief Vertex in 2D plane
 *
 */
struct Vertex2D {
    glm::vec2 position;
    glm::vec2 texture_coord;

    bool operator==(const Vertex2D& other) const {
        return position == other.position &&
               texture_coord == other.texture_coord;
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

// Render data
/**
 * @brief List of builtin render passes
 */
enum BuiltinRenderPass : uint8 { World = 0x1, UI = 0x2 };

/**
 * @brief Geometry render packet
 */
struct GeometryRenderData {
    Geometry* geometry;
};