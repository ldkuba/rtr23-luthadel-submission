#include "resources/material.hpp"

#define MATERIAL_LOG "Material :: "

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

#define BUILTIN_MATERIAL_SHADER_NAME "builtin.material_shader"
#define BUILTIN_UI_SHADER_NAME "builtin.ui_shader"

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
#define set_sampler(sampler_name, texture)                                     \
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
        auto set_result = _shader->set_sampler(sampler_id, texture);           \
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
#define set_uniform_s(uniform) set_uniform(#uniform, uniform)

void Material::apply_global(
    const glm::mat4 projection,
    const glm::mat4 view,
    const glm::vec4 ambient_color,
    const glm::vec3 view_position,
    const uint32    mode
) {
    // Apply globals
    _shader->bind_globals();
    set_uniform_s(projection);
    set_uniform_s(view);
    // TODO: Temp solution for type checking problem
    if (_shader->get_name().compare_ci("builtin.material_shader") == 0) {
        set_uniform_s(ambient_color);
        set_uniform_s(view_position);
        set_uniform_s(mode);
    }
    _shader->apply_global();
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

    // Apply locals
    _shader->bind_instance(internal_id.value());
    set_uniform("diffuse_color", _diffuse_color);
    set_sampler("diffuse_texture", _diffuse_map.texture);
    if (_shader->get_name().compare_ci("builtin.material_shader") == 0) {
        set_uniform("shininess", _shininess);
        set_sampler("specular_texture", _specular_map.texture);
        set_sampler("normal_texture", _normal_map.texture);
    }
    _shader->apply_instance();
}
void Material::apply_local(const glm::mat4 model) { set_uniform_s(model); }