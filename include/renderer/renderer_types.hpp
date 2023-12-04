#pragma once

#include "math_libs.hpp"
#include "serialization/serializer.hpp"
#include "resources/texture.hpp"

namespace ENGINE_NAMESPACE {

// -----------------------------------------------------------------------------
// Vertex
// -----------------------------------------------------------------------------

/**
 * @brief Generic representation of n-dimensional vertex
 * @tparam Dim Vertex dimension count
 */
template<uint8 Dim>
struct Vertex {
  public:
    typedef glm::vec<Dim, float32> Vector;

    Vector    position;
    glm::vec2 texture_coord {};

    bool operator==(const Vertex<Dim>& other) const {
        const auto same_position =
            glm::all(glm::epsilonEqual(other.position, position, Epsilon32));
        const auto same_texture_coord = glm::all(
            glm::epsilonEqual(other.texture_coord, texture_coord, Epsilon32)
        );
        return same_position && same_texture_coord;
    }
};

template<>
struct Vertex<3> {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec4 color;
    glm::vec2 texture_coord;

    Vertex() {}
    Vertex(
        const glm::vec3 position,
        const glm::vec3 normal,
        const glm::vec3 tangent,
        const glm::vec4 color,
        const glm::vec2 texture_coord
    )
        : position(position), normal(normal), tangent(tangent), color(color),
          texture_coord(texture_coord) {}
    ~Vertex() {}

    bool operator==(const Vertex& other) const {
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

/**
 * @brief Vertex in 2D plane
 *
 */
typedef Vertex<2> Vertex2D;
/**
 * @brief Vertex in 3D space
 *
 */
typedef Vertex<3> Vertex3D;

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

} // namespace ENGINE_NAMESPACE

namespace std {
template<>
struct hash<ENGINE_NAMESPACE::Vertex3D> {
    size_t operator()(ENGINE_NAMESPACE::Vertex3D const& vertex) const {
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

class Shader;
class Geometry;
class Mesh;
class RenderView;

/**
 * @brief Geometry render packet
 */
struct GeometryRenderData {
    Geometry* geometry;
    glm::mat4 model;
};

struct MeshRenderData {
    Vector<Mesh*> meshes;
};

struct RenderViewPacket {
    RenderView* const view;

    glm::vec3 view_position;
    glm::mat4 view_matrix;
    glm::mat4 proj_matrix;
    Shader*   shader;

    Vector<GeometryRenderData> geometry_data;
};

class DirectionalLightData;
class PointLightData;

/**
 * @brief Light render packet
 */
struct LightRenderData {
    DirectionalLightData*   directional_light;
    Vector<PointLightData*> point_lights;
};

/**
 * @brief A Structure generated by application and given to renderer for
 * rendering of one frame.
 */
struct RenderPacket {
    Vector<RenderViewPacket> view_data;
};

} // namespace ENGINE_NAMESPACE