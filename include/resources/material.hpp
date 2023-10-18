#pragma once

#include "math_libs.hpp"
#include "shader.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Texture with its relevant properties
 */
struct TextureMap {
    const Texture* texture = nullptr;
    TextureUse     use;
};

/**
 * @brief Material configuration resource.
 *
 */
class MaterialConfig : public Resource {
  public:
    const String    shader;
    const String    diffuse_map_name;
    const String    specular_map_name;
    const String    normal_map_name;
    const glm::vec4 diffuse_color;
    const float32   shininess;
    const bool      auto_release;

    MaterialConfig(
        const String    name,
        const String    shader,
        const String    diffuse_map_name,
        const String    specular_map_name,
        const String    normal_map_name,
        const glm::vec4 diffuse_color,
        const float32   shininess,
        const bool      auto_release
    )
        : Resource(name), shader(shader), diffuse_map_name(diffuse_map_name),
          specular_map_name(specular_map_name),
          normal_map_name(normal_map_name), diffuse_color(diffuse_color),
          shininess(shininess), auto_release(auto_release) {}
    ~MaterialConfig() {}
};

/**
 * @brief A material. Represents various properties of a in world surface
 * (texture, colour, bumpiness, shininess, etc...).
 */
class Material {
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
    Property<TextureMap> diffuse_map {
        GET { return _diffuse_map; }
        SET {
            _diffuse_map     = value;
            _update_required = true;
        }
    };
    /// @brief Material's specular map
    Property<TextureMap> specular_map {
        GET { return _specular_map; }
        SET {
            _specular_map    = value;
            _update_required = true;
        }
    };
    /// @brief Material's normal map
    Property<TextureMap> normal_map {
        GET { return _normal_map; }
        SET {
            _normal_map      = value;
            _update_required = true;
        }
    };
    /// @brief Index of the frame on which this material has been last updated
    Property<uint64> last_update_frame {
        GET { return _last_update_frame; }
        SET { _last_update_frame = value; }
    };

    /**
     * @brief Construct a new Material object
     *
     * @param name Material name
     * @param shader Shader used by the material
     * @param diffuse_color Material diffuse color
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

    const static uint32 max_name_length = 256;

  private:
    String        _name = "";
    Shader* const _shader;
    TextureMap    _diffuse_map;
    TextureMap    _specular_map;
    TextureMap    _normal_map;
    glm::vec4     _diffuse_color;
    float32       _shininess;
    bool          _update_required = true;

    uint64 _last_update_frame = (uint64) -1;
};

} // namespace ENGINE_NAMESPACE