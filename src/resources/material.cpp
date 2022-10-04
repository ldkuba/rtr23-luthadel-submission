#include "resources/material.hpp"

#include "systems/texture_system.hpp"

Material::Material(
    const String name,
    const glm::vec4 diffuse_color
) : Resource(name), _diffuse_color(diffuse_color) {}
Material::~Material() {}