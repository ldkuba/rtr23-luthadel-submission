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

#define set_uniform(uniform_name, uniform_value)                               \
    {                                                                          \
        auto uniform_id_res = _shader->get_uniform_index(uniform_name);        \
        if (uniform_id_res.has_error()) {                                      \
            Logger::error(                                                     \
                MATERIAL_LOG,                                                  \
                "Shader set_uniform method failed. No uniform is named \"",    \
                uniform_name,                                                  \
                "\". Nothing was done."                                        \
            );                                                                 \
            return;                                                            \
        }                                                                      \
        auto uniform_id = uniform_id_res.value();                              \
        auto set_result = _shader->set_uniform(uniform_id, &uniform_value);    \
        if (set_result.has_error()) {                                          \
            Logger::error(                                                     \
                MATERIAL_LOG,                                                  \
                "Shader set_uniform method failed for \"",                     \
                uniform_name,                                                  \
                "\". Nothing was done"                                         \
            );                                                                 \
            return;                                                            \
        }                                                                      \
    }
#define set_sampler(sampler_name, texture_map)                                 \
    {                                                                          \
        auto sampler_id_res = _shader->get_uniform_index(sampler_name);        \
        if (sampler_id_res.has_error()) {                                      \
            Logger::error(                                                     \
                MATERIAL_LOG,                                                  \
                "Shader set_sampler method failed. No sampler is named \"",    \
                sampler_name,                                                  \
                "\". Nothing was done."                                        \
            );                                                                 \
            return;                                                            \
        }                                                                      \
        auto sampler_id = sampler_id_res.value();                              \
        auto set_result = _shader->set_sampler(sampler_id, texture_map);       \
        if (set_result.has_error()) {                                          \
            Logger::error(                                                     \
                MATERIAL_LOG,                                                  \
                "Shader set_sampler method failed for \"",                     \
                sampler_name,                                                  \
                "\". Nothing was done"                                         \
            );                                                                 \
            return;                                                            \
        }                                                                      \
    }

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
        set_uniform("diffuse_color", _diffuse_color);
        set_sampler("diffuse_texture", _diffuse_map);
        // TODO: TEMP  CHECK
        if (_shader->get_name().compare_ci(Shader::BuiltIn::MaterialShader) ==
            0) {
            set_uniform("shininess", _shininess);
            set_sampler("specular_texture", _specular_map);
            set_sampler("normal_texture", _normal_map);
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