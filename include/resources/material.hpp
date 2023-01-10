#pragma once

#include "math_libs.hpp"
#include "shader.hpp"

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
    const glm::vec4 diffuse_color;
    const bool      auto_release;

    MaterialConfig(
        const String    name,
        const String    shader,
        const String    diffuse_map_name,
        const glm::vec4 diffuse_color,
        const bool      auto_release
    )
        : Resource(name), shader(shader), diffuse_map_name(diffuse_map_name),
          diffuse_color(diffuse_color), auto_release(auto_release) {}
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
    /// @brief Material's diffuse map
    Property<TextureMap> diffuse_map {
        GET { return _diffuse_map; }
        SET { _diffuse_map = value; }
    };

    /**
     * @brief Construct a new Material object
     *
     * @param name Material name
     * @param shader Shader used by the material
     * @param diffuse_color Material diffuse color
     */
    Material(
        const String name, Shader* const shader, const glm::vec4 diffuse_color
    );
    ~Material();

    /**
     * @brief Set global uniform values for all materials witch utilize this
     * shader.
     *
     * @param projection Projection matrix to set
     * @param view View matrix to set
     */
    void apply_global(const glm::mat4 projection, const glm::mat4 view);
    /**
     * @brief Set instance uniform values of this material.
     *
     */
    void apply_instance();
    /**
     * @brief Set local uniform values of this material
     *
     * @param model Model matrix to set
     */
    void apply_local(const glm::mat4 model);

    const static uint32 max_name_length = 256;

  private:
    String        _name = "";
    Shader* const _shader;
    TextureMap    _diffuse_map;
    glm::vec4     _diffuse_color;
};