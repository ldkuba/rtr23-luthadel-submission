#include "resources/material.hpp"

#include "systems/shader_system.hpp"

namespace ENGINE_NAMESPACE {

#define MATERIAL_LOG "Material :: "

// Statics values
const uint32 Material::max_name_length;

// Constructor & Destructor
Material::Material(
    const String    name,
    Shader* const   shader,
    const glm::vec4 diffuse_color,
    const float32   shininess
)
    : _name(name), _shader(shader), _diffuse_color(diffuse_color),
      _shininess(shininess) {}
Material::~Material() {}

// /////////////////////// //
// MATERIAL PUBLIC METHODS //
// /////////////////////// //

#define uniform_set(uniform_name, uniform_value)                               \
    auto uniform_name##_res = _shader->get_uniform_index(#uniform_name);       \
    if (uniform_name##_res.has_error()) {                                      \
        Logger::error(                                                         \
            MATERIAL_LOG,                                                      \
            "Shader set_uniform method failed. No uniform is named \"",        \
            #uniform_name,                                                     \
            "\". Nothing was done."                                            \
        );                                                                     \
        return;                                                                \
    }                                                                          \
    _shader->set_uniform(uniform_name##_res.value(), &uniform_value);

#define sampler_set(sampler_name, texture_map)                                 \
    auto sampler_name##_res = _shader->get_uniform_index(#sampler_name);       \
    if (sampler_name##_res.has_error()) {                                      \
        Logger::error(                                                         \
            MATERIAL_LOG,                                                      \
            "Shader set_sampler method failed. No sampler is named \"",        \
            #sampler_name,                                                     \
            "\". Nothing was done."                                            \
        );                                                                     \
        return;                                                                \
    }                                                                          \
    _shader->set_sampler(sampler_name##_res.value(), texture_map);

void Material::apply_instance() {
    // Bind instance based on id
    if (!internal_id.has_value())
        Logger::fatal(
            MATERIAL_LOG,
            "Material \"",
            _name,
            "\" not properly initialized. Internal id not set."
        );

    // Apply instance level uniforms
    _shader->bind_instance(internal_id.value());
    if (_update_required) {
        uniform_set(diffuse_color, _diffuse_color);
        sampler_set(diffuse_texture, _diffuse_map);
        // TODO: TEMP  CHECK
        if (_shader->get_name().compare_ci(Shader::BuiltIn::MaterialShader) ==
            0) {
            uniform_set(shininess, _shininess);
            sampler_set(specular_texture, _specular_map);
            sampler_set(normal_texture, _normal_map);
        }
        _update_required = false;
    }
    _shader->apply_instance();
}

void Material::acquire_map_resources() {
    // Gather texture map pointer list
    Vector<Texture::Map*> texture_maps;
    if (_diffuse_map != nullptr) texture_maps.push_back(_diffuse_map);
    if (_specular_map != nullptr) texture_maps.push_back(_specular_map);
    if (_normal_map != nullptr) texture_maps.push_back(_normal_map);

    // Acquire shader instance
    internal_id = _shader->acquire_instance_resources(texture_maps);
}

void Material::release_map_resources() {
    _shader->release_instance_resources(internal_id.value());
}

} // namespace ENGINE_NAMESPACE