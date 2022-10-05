#ifndef __RENDERER_TYPES_H__
#define __RENDERER_TYPES_H__

#pragma once

#include "math_libs.hpp"
#include "resources/geometry.hpp"

// Vertex
struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texture_coord;

    bool operator==(const Vertex& other) const {
        return position == other.position && texture_coord == other.texture_coord;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texture_coord) << 1);
        }
    };
}

// UBO
struct GlobalUniformObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 project;
};

struct LocalUniformObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::vec4 diffuse_color;
};

struct GeometryRenderData {
    glm::mat4 model;
    Geometry* geometry;
};
#endif // __RENDERER_TYPES_H__