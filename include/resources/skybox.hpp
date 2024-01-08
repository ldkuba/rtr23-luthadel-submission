#pragma once

#include "geometry.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Skybox class representation.
 */
class Skybox {
  public:
    /// @brief Id of shader instance related to this skybox
    Property<uint32> instance_id {
        GET { return _instance_id; }
    };
    /// @brief Reference to skybox geometry
    Property<Geometry*> geometry {
        GET { return _geometry; }
    };
    /// @brief Reference to skybox cube map
    Property<Texture::Map*> cube_map {
        GET { return _cube_map; }
    };

    /**
     * @brief Construct a new Skybox object
     * @param instance_id Shader instance identifier
     * @param cube_map Cube map used
     * @param geometry Geometry used
     */
    Skybox(uint32 instance_id, Texture::Map* cube_map, Geometry* geometry)
        : _instance_id(instance_id), _cube_map(cube_map), _geometry(geometry) {}
    Skybox() {}
    ~Skybox() {}

    void operator=(Skybox const& value) {
        _instance_id = value._instance_id;
        _cube_map    = value._cube_map;
        _geometry    = value._geometry;
    }
    void operator=(Skybox&& value) {
        _instance_id = value._instance_id;
        _cube_map    = value._cube_map;
        _geometry    = value._geometry;
    }

  private:
    uint32        _instance_id {};
    Texture::Map* _cube_map {};
    Geometry*     _geometry {};
};

} // namespace ENGINE_NAMESPACE