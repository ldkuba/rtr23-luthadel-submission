#pragma once

#include "math_libs.hpp"
#include "texture.hpp"

class TextureSystem;

struct TextureMap {
    const Texture* texture;
    TextureUse use;
    bool is_default;
};

class Material {
public:
    std::optional<uint64> id;
    std::optional<uint64> internal_id;
    Property<String> name{ Get { return _name; } };
    Property<glm::vec4> diffuse_color{ Get { return _diffuse_color; } };
    Property<TextureMap> diffuse_map{ Get { return _diffuse_map; } };

    Material(
        const String name,
        const glm::vec4 diffuse_color,
        const String diffuse_map_name,
        TextureSystem* const texture_system,
        const bool create_as_default = false
    );
    ~Material();

    const static uint32 max_name_length = 256;

private:
    TextureSystem* _texture_system;

    String _name;
    glm::vec4 _diffuse_color;
    TextureMap _diffuse_map;
};