#pragma once

#include "math_libs.hpp"
#include "serialization/serializer.hpp"
#include "resources/texture.hpp"

namespace ENGINE_NAMESPACE {

// -----------------------------------------------------------------------------
// Vertex
// -----------------------------------------------------------------------------
/**
 * @brief Vertex in 3D space
 *
 */
struct Vertex3D {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec4 color;
    glm::vec2 texture_coord;

    Vertex3D() {}
    Vertex3D(
        const glm::vec3 position,
        const glm::vec3 normal,
        const glm::vec3 tangent,
        const glm::vec4 color,
        const glm::vec2 texture_coord
    )
        : position(position), normal(normal), tangent(tangent), color(color),
          texture_coord(texture_coord) {}
    ~Vertex3D() {}

    bool operator==(const Vertex3D& other) const {
        const auto same_position =
            glm::all(glm::epsilonEqual(other.position, position, Epsilon32));
        const auto same_normal =
            glm::all(glm::epsilonEqual(other.normal, normal, Epsilon32));
        const auto same_tangent =
            glm::all(glm::epsilonEqual(other.tangent, tangent, Epsilon32));
        const auto same_color =
            glm::all(glm::epsilonEqual(other.color, color, Epsilon32));
        const auto same_texture_coord = glm::all(
            glm::epsilonEqual(other.texture_coord, texture_coord, Epsilon32)
        );
        return same_position && same_normal && same_tangent && same_color &&
               same_texture_coord;
    }
};

template<>
inline String serialize_object<Vertex3D>(
    const Vertex3D& obj, const Serializer* const serializer
) {
    return serializer->serialize(
        obj.position, obj.normal, obj.tangent, obj.color, obj.texture_coord
    );
}
template<>
inline Result<uint32, RuntimeError> deserialize_object<Vertex3D>(
    Vertex3D&               obj,
    const Serializer* const serializer,
    const String&           data,
    const uint32            from_pos
) {
    return serializer->deserialize(
        data,
        from_pos,
        obj.position,
        obj.normal,
        obj.tangent,
        obj.color,
        obj.texture_coord
    );
}

/**
 * @brief Vertex in 2D plane
 *
 */
struct Vertex2D {
    glm::vec2 position;
    glm::vec2 texture_coord;

    Vertex2D() {}
    Vertex2D(const glm::vec2 position, const glm::vec2 texture_coord)
        : position(position), texture_coord(texture_coord) {}
    ~Vertex2D() {}

    bool operator==(const Vertex2D& other) const {
        const auto same_position =
            glm::all(glm::epsilonEqual(other.position, position, Epsilon32));
        const auto same_texture_coord = glm::all(
            glm::epsilonEqual(other.texture_coord, texture_coord, Epsilon32)
        );
        return same_position && same_texture_coord;
    }
};

template<>
inline String serialize_object<Vertex2D>(
    const Vertex2D& obj, const Serializer* const serializer
) {
    return serializer->serialize(obj.position, obj.texture_coord);
}
template<>
inline Result<uint32, RuntimeError> deserialize_object<Vertex2D>(
    Vertex2D&               obj,
    const Serializer* const serializer,
    const String&           data,
    const uint32            from_pos
) {
    return serializer->deserialize(
        data, from_pos, obj.position, obj.texture_coord
    );
}

typedef Vertex3D Vertex;

} // namespace ENGINE_NAMESPACE

namespace std {
template<>
struct hash<ENGINE_NAMESPACE::Vertex> {
    size_t operator()(ENGINE_NAMESPACE::Vertex const& vertex) const {
        return hash<glm::vec3>()(vertex.position) ^
               hash<glm::vec2>()(vertex.texture_coord);
    }
};
} // namespace std

namespace ENGINE_NAMESPACE {

// -----------------------------------------------------------------------------
// Frame buffer
// -----------------------------------------------------------------------------

/**
 * @brief Generic abstract representation of a framebuffer.
 */
class FrameBuffer {
  public:
    /**
     * @brief Recreates framebuffer object
     *
     * @param width New width in pixels
     * @param height New height in pixels
     * @param attachments New list of attached Textures
     */
    virtual void recreate(
        const uint32            width,
        const uint32            height,
        const Vector<Texture*>& attachments
    ) = 0;
};

// -----------------------------------------------------------------------------
// Render data
// -----------------------------------------------------------------------------

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
    glm::mat4 model;
};

class DirectionalLight;
class DirectionalLightData;
class PointLight;
class PointLightData;

/**
 * @brief Light render packet
 */
struct LightRenderData {
    DirectionalLightData*   directional_light;
    int                     num_point_lights;
    Vector<PointLightData*> point_lights;
};

struct RenderPacket {
    Vector<GeometryRenderData> geometry_data;
    GeometryRenderData         ui_geometry_data;

    LightRenderData light_data;
};

} // namespace ENGINE_NAMESPACE