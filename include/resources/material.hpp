#pragma once

#include "math_libs.hpp"
#include "texture.hpp"

struct TextureMap {
    const Texture* texture = nullptr;
    TextureUse     use;
};

enum class MaterialType { UI, World };

class MaterialConfig : public Resource {
  public:
    const MaterialType type;
    const String       diffuse_map_name;
    const glm::vec4    diffuse_color;
    const bool         auto_release;

    MaterialConfig(
        const String       name,
        const MaterialType type,
        const String       diffuse_map_name,
        const glm::vec4    diffuse_color,
        const bool         auto_release
    )
        : Resource(name), type(type), diffuse_map_name(diffuse_map_name),
          diffuse_color(diffuse_color), auto_release(auto_release) {}
    ~MaterialConfig() {}
};

class Material {
  public:
    /// @brief Unique identifier
    std::optional<uint64> id;
    /// @brief Id used by the Renderer
    std::optional<uint64> internal_id;
    /// @brief Material name
    Property<String>      name {
        Get { return _name; }
    };
    /// @brief Material type
    Property<MaterialType> type {
        Get { return _type; }
    };
    /// @brief Material's diffuse color
    Property<glm::vec4> diffuse_color {
        Get { return _diffuse_color; }
    };
    /// @brief Material's diffuse map
    Property<TextureMap> diffuse_map {
        Get { return _diffuse_map; }
        , Set { _diffuse_map = value; }
    };

    Material(
        const String       name,
        const MaterialType type,
        const glm::vec4    diffuse_color
    );
    ~Material();

    const static uint32 max_name_length = 256;

  private:
    String       _name = "";
    MaterialType _type;
    TextureMap   _diffuse_map;
    glm::vec4    _diffuse_color;
};