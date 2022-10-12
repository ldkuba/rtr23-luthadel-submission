#pragma once

#include "resources/geometry.hpp"

// Vertex
struct Vertex3D {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texture_coord;

    bool operator==(const Vertex3D& other) const {
        return position == other.position &&
               texture_coord == other.texture_coord;
    }
};

struct Vertex2D {
    glm::vec3 position;
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
        return ((hash<glm::vec3>()(vertex.position) ^
                 (hash<glm::vec3>()(vertex.color) << 1)) >>
                1) ^
               (hash<glm::vec2>()(vertex.texture_coord) << 1);
    }
};
} // namespace std

// Render data
enum BuiltinRenderPass : uint8 { World = 0x1, UI = 0x2 };

struct GeometryRenderData {
    glm::mat4 model;
    Geometry* geometry;
};