#pragma once

#include "material.hpp"
#include "renderer/renderer_types.hpp"

/**
 * @brief Configuration of single geometry.
 */
class GeometryConfig : public Serializable {
  public:
    String         name;
    Vector<uint32> indices { { MemoryTag::Geometry } };
    glm::vec3      center;
    glm::vec3      max_extents;
    glm::vec3      min_extents;
    String         material_name;
    bool           auto_release;

    GeometryConfig();
    GeometryConfig(
        const String          name,
        const Vector<uint32>& indices,
        const glm::vec3       center,
        const glm::vec3       max_extents,
        const glm::vec3       min_extents,
        const String          material_name = "",
        const bool            auto_release  = true
    );
    virtual ~GeometryConfig();

    virtual String serialize(const Serializer* const serializer) const override;
    virtual Result<uint32, RuntimeError> deserialize(
        const String& data, const Serializer* const serializer
    ) override;
};

/**
 * @brief Geometry config variant for 2D geometries
 */
class GeometryConfig2D : public GeometryConfig {
  public:
    Vector<Vertex2D> vertices { { MemoryTag::Geometry } };

    GeometryConfig2D();
    GeometryConfig2D(
        const String            name,
        const Vector<Vertex2D>& vertices,
        const Vector<uint32>&   indices,
        const glm::vec3         center,
        const glm::vec3         max_extents,
        const glm::vec3         min_extents,
        const String            material_name = "",
        const bool              auto_release  = true
    );
    ~GeometryConfig2D();

    String serialize(const Serializer* const serializer) const override;
    Result<uint32, RuntimeError> deserialize(
        const String& data, const Serializer* const serializer
    ) override;
};

/**
 * @brief Geometry config variant for 3D geometries
 */
class GeometryConfig3D : public GeometryConfig {
  public:
    Vector<Vertex3D> vertices { { MemoryTag::Geometry } };

    GeometryConfig3D();
    GeometryConfig3D(
        const String            name,
        const Vector<Vertex3D>& vertices,
        const Vector<uint32>&   indices,
        const glm::vec3         center,
        const glm::vec3         max_extents,
        const glm::vec3         min_extents,
        const String            material_name = "",
        const bool              auto_release  = true
    );
    ~GeometryConfig3D() override;

    String serialize(const Serializer* const serializer) const override;

    Result<uint32, RuntimeError> deserialize(
        const String& data, const Serializer* const serializer
    ) override;
};

/**
 * @brief Geometry configuration resource
 */
class GeometryConfigArray : public Resource {
  public:
    Vector<GeometryConfig*> configs {};

    GeometryConfigArray(const String& name);
    ~GeometryConfigArray();
};
