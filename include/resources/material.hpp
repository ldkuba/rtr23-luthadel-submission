#pragma once

#include "math_libs.hpp"
#include "texture.hpp"

struct TextureMap {
    const Texture* texture = nullptr;
    TextureUse use;
};

class Material {
public:
    std::optional<uint64> id;
    std::optional<uint64> internal_id;
    Property<String> name{ Get { return _name; } };
    Property<glm::vec4> diffuse_color{ Get { return _diffuse_color; } };
    Property<TextureMap> diffuse_map{
        Get { return _diffuse_map; },
        Set { _diffuse_map = value; }
    };

    Material(
        const String name,
        const glm::vec4 diffuse_color
    );
    ~Material();

    const static uint32 max_name_length = 256;

private:
    String _name;
    glm::vec4 _diffuse_color;
    TextureMap _diffuse_map;
};