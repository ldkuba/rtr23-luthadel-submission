#include "resources/material.hpp"

Material::Material(
    const String name, const MaterialType type, const glm::vec4 diffuse_color
)
    : _name(name), _type(type), _diffuse_color(diffuse_color) {}
Material::~Material() {}