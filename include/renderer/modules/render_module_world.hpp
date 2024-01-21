#pragma once

#include "render_module.hpp"
#include "renderer/views/render_view_perspective.hpp"
#include "systems/light_system.hpp"

namespace ENGINE_NAMESPACE {

class RenderModuleWorld : public RenderModule {
  public:
    struct Config : public RenderModule::Config {
        RenderViewPerspective* perspective_view;
        String                 ssao_texture;
        String                 shadow_texture;
        String                 volumetrics_texture;
        glm::vec4              ambient_color;
    };

  public:
    using RenderModule::RenderModule;

    void initialize(const Config& config) {
        _perspective_view = config.perspective_view;
        _ambient_color    = config.ambient_color;
        create_texture_maps(
            config.ssao_texture,
            config.shadow_texture,
            config.volumetrics_texture
        );

        setup_uniform_indices(_u_names.projection);
        setup_uniform_indices(_u_names.view);
        setup_uniform_indices(_u_names.ambient_color);
        setup_uniform_indices(_u_names.view_position);
        setup_uniform_indices(_u_names.mode);
        setup_uniform_indices(_u_names.model);
        setup_uniform_indices(_u_names.directional_light);
        setup_uniform_indices(_u_names.num_point_lights);
        setup_uniform_indices(_u_names.point_lights);
        setup_uniform_indices(_u_names.ssao_texture);
        setup_uniform_indices(_u_names.shadowmap_sampled_texture);
        setup_uniform_indices(_u_names.volumetrics_texture);
    }

    void set_mode(const DebugViewMode& mode) { _render_mode = mode; }

  protected:
    void on_render(const ModulePacket* const packet, const uint64 frame_number, uint32 rp_index)
        override {
        auto shader = _renderpasses.at(rp_index).shader;
        // Get visible geometries
        const auto geometry_data =
            _perspective_view->get_visible_render_data(frame_number);

        // Draw geometries
        for (const auto& geo_data : geometry_data) {
            // Update material instance
            geo_data.material->apply_instance();

            // Apply local
            shader->set_uniform(UNIFORM_ID(model), &geo_data.model);

            // Draw geometry
            _renderer->draw_geometry(geo_data.geometry);
        }
    }

    void apply_globals(uint32 rp_index) const override {
        auto shader = _renderpasses.at(rp_index).shader;
        // Apply globals update
        shader->set_uniform(
            UNIFORM_ID(projection), &_perspective_view->proj_matrix()
        );
        shader->set_uniform(
            UNIFORM_ID(view), &_perspective_view->camera()->view()
        );
        shader->set_uniform(UNIFORM_ID(ambient_color), &_ambient_color);
        shader->set_uniform(
            UNIFORM_ID(view_position),
            &_perspective_view->camera()->transform.position()
        );
        shader->set_uniform(UNIFORM_ID(mode), &_render_mode);
        shader->set_sampler(UNIFORM_ID(ssao_texture), _ssao_texture_map);
        shader->set_sampler(
            UNIFORM_ID(shadowmap_sampled_texture), _shadow_texture_map
        );
        shader->set_sampler(
            UNIFORM_ID(volumetrics_texture), _volumetrics_texture_map
        );

        // Apply lights
        auto point_light_data  = _light_system->get_point_data();
        auto directional_light = _light_system->get_directional_data();
        auto num_point_lights  = point_light_data.size();
        shader->set_uniform(UNIFORM_ID(directional_light), &directional_light);
        shader->set_uniform(UNIFORM_ID(num_point_lights), &num_point_lights);
        shader->set_uniform(UNIFORM_ID(point_lights), point_light_data.data());
    }

  private:
    RenderViewPerspective* _perspective_view;
    Texture::Map*          _ssao_texture_map;
    Texture::Map*          _shadow_texture_map;
    Texture::Map*          _volumetrics_texture_map;
    glm::vec4              _ambient_color;

    DebugViewMode _render_mode = DebugViewMode::Default;

    struct Uniforms {
        UNIFORM_NAME(projection);
        UNIFORM_NAME(view);
        UNIFORM_NAME(ambient_color);
        UNIFORM_NAME(view_position);
        UNIFORM_NAME(mode);
        UNIFORM_NAME(model);
        UNIFORM_NAME(directional_light);
        UNIFORM_NAME(num_point_lights);
        UNIFORM_NAME(point_lights);
        UNIFORM_NAME(ssao_texture);
        UNIFORM_NAME(shadowmap_sampled_texture);
        UNIFORM_NAME(volumetrics_texture);
    };
    Uniforms _u_names {};

    void create_texture_maps(
        const String& ssao_texture,
        const String& shadow_texture,
        const String& volumetrics_texture
    ) {
        _ssao_texture_map = create_texture_map(
            ssao_texture,
            Texture::Use::MapPassResult,
            Texture::Filter::BiLinear,
            Texture::Filter::BiLinear,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge
        );
        _shadow_texture_map = create_texture_map(
            shadow_texture,
            Texture::Use::MapPassResult,
            Texture::Filter::BiLinear,
            Texture::Filter::BiLinear,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge
        );
        _volumetrics_texture_map = create_texture_map(
            volumetrics_texture,
            Texture::Use::MapPassResult,
            Texture::Filter::BiLinear,
            Texture::Filter::BiLinear,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge
        );
    }
};

} // namespace ENGINE_NAMESPACE