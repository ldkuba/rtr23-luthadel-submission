#pragma once

#include "math_libs.hpp"
#include "shader.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief A material. Represents various properties of a in world surface
 * (texture, colour, bumpiness, shininess, etc...).
 */
class Material {
  public:
    /**
     * @brief Material configuration resource.
     *
     */
    class Config : public Resource {
      public:
        const String    shader;
        const String    diffuse_map_name;
        const String    specular_map_name;
        const String    normal_map_name;
        const glm::vec4 diffuse_color;
        const float32   shininess;
        const bool      auto_release;

        Config(
            const String    name,
            const String    shader,
            const String    diffuse_map_name,
            const String    specular_map_name,
            const String    normal_map_name,
            const glm::vec4 diffuse_color,
            const float32   shininess,
            const bool      auto_release
        )
            : Resource(name), shader(shader),
              diffuse_map_name(diffuse_map_name),
              specular_map_name(specular_map_name),
              normal_map_name(normal_map_name), diffuse_color(diffuse_color),
              shininess(shininess), auto_release(auto_release) {}
        ~Config() {}
    };

  public:
    /// @brief Unique identifier
    std::optional<uint64> id;
    /// @brief Id used by the Renderer
    std::optional<uint64> internal_id;
    /// @brief Material name
    Property<String>      name {
        GET { return _name; }
    };
    /// @brief Shader used
    Property<Shader*> shader {
        GET { return _shader; }
    };
    /// @brief Material's diffuse color
    Property<glm::vec4> diffuse_color {
        GET { return _diffuse_color; }
    };
    /// @brief Materials shininess. Controls concentration of specular light
    Property<float32> shininess {
        GET { return _shininess; }
    };
    /// @brief Material's diffuse map
    Property<Texture::Map*> diffuse_map {
        GET { return _diffuse_map; }
        SET {
            _diffuse_map     = value;
            _update_required = true;
        }
    };
    /// @brief Material's specular map
    Property<Texture::Map*> specular_map {
        GET { return _specular_map; }
        SET {
            _specular_map    = value;
            _update_required = true;
        }
    };
    /// @brief Material's normal map
    Property<Texture::Map*> normal_map {
        GET { return _normal_map; }
        SET {
            _normal_map      = value;
            _update_required = true;
        }
    };

    /**
     * @brief Construct a new Material object
     *
     * @param name Material name
     * @param shader Shader used by the material
     * @param diffuse_color Material diffuse color
     * @param shininess Default shininess
     */
    Material(
        const String    name,
        Shader* const   shader,
        const glm::vec4 diffuse_color,
        const float32   shininess
    );
    ~Material();

    /**
     * @brief Set instance uniform values of this material.
     *
     */
    void apply_instance();

    /**
     * @brief Acquires map resources from the GPU. Usually called after
     * initialization.
     */
    void acquire_map_resources();

    /**
     * @brief Release map resources from GPU. Usually called before destruction.
     */
    void release_map_resources();

    const static uint32 max_name_length = 256;

  private:
    String        _name = "";
    Shader* const _shader;
    Texture::Map* _diffuse_map  = nullptr;
    Texture::Map* _specular_map = nullptr;
    Texture::Map* _normal_map   = nullptr;
    glm::vec4     _diffuse_color;
    float32       _shininess;
    bool          _update_required = true;
};

} // namespace ENGINE_NAMESPACE