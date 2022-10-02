#include "resources/material.hpp"

#include "systems/texture_system.hpp"

Material::Material(
    const String name,
    const glm::vec4 diffuse_color,
    const String diffuse_map_name,
    TextureSystem* const texture_system,
    const bool create_as_default
) : _name(name), _diffuse_color(diffuse_color), _texture_system(texture_system) {
    _diffuse_map = {};
    if (diffuse_map_name.length() > 0) {
        _diffuse_map.use = TextureUse::MapDiffuse;
        if (create_as_default) {
            _diffuse_map.texture = _texture_system->default_texture;
            _diffuse_map.is_default = true;
        } else
            _diffuse_map.texture = _texture_system->acquire(diffuse_map_name, true);
    } else {
        // NOTE: Set for clarity
        _diffuse_map.use = TextureUse::Unknown;
        _diffuse_map.texture = nullptr;
    }
    // TODO: Set other maps
}
Material::~Material() {
    if (_diffuse_map.texture && !_diffuse_map.is_default) {
        _texture_system->release(_diffuse_map.texture->name);
    }
}