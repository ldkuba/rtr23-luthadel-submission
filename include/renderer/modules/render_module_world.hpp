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
        glm::vec4              ambient_color;
    };

  public:
    using RenderModule::RenderModule;

    void initialize(const Config& config) {
        _perspective_view = config.perspective_view;
        _ambient_color    = config.ambient_color;
        create_texture_maps(config.ssao_texture, config.shadow_texture);

        SETUP_UNIFORM_INDEX(projection);
        SETUP_UNIFORM_INDEX(view);
        SETUP_UNIFORM_INDEX(ambient_color);
        SETUP_UNIFORM_INDEX(view_position);
        SETUP_UNIFORM_INDEX(mode);
        SETUP_UNIFORM_INDEX(model);
        SETUP_UNIFORM_INDEX(directional_light);
        SETUP_UNIFORM_INDEX(num_point_lights);
        SETUP_UNIFORM_INDEX(point_lights);
        SETUP_UNIFORM_INDEX(ssao_texture);
        SETUP_UNIFORM_INDEX(shadowmap_sampled_texture);
    }

    void set_mode(const DebugViewMode& mode) { _render_mode = mode; }

  protected:
    void on_render(const ModulePacket* const packet, const uint64 frame_number)
        override {
        // Get visible geometries
        const auto geometry_data =
            _perspective_view->get_visible_render_data(frame_number);

        // Draw geometries
        for (const auto& geo_data : geometry_data) {
            // Update material instance
            geo_data.material->apply_instance();

            // Apply local
            _shader->set_uniform(_u_index.model, &geo_data.model);

            // Draw geometry
            _renderer->draw_geometry(geo_data.geometry);
        }
    }

    void apply_globals() const override {
        // Apply globals update
        _shader->set_uniform(
            _u_index.projection, &_perspective_view->proj_matrix()
        );
        _shader->set_uniform(
            _u_index.view, &_perspective_view->camera()->view()
        );
        _shader->set_uniform(_u_index.ambient_color, &_ambient_color);
        _shader->set_uniform(
            _u_index.view_position,
            &_perspective_view->camera()->transform.position()
        );
        _shader->set_uniform(_u_index.mode, &_render_mode);
        _shader->set_sampler(_u_index.ssao_texture, _ssao_texture_map);
        _shader->set_sampler(
            _u_index.shadowmap_sampled_texture, _shadow_texture_map
        );

        // Apply lights
        auto point_light_data  = _light_system->get_point_data();
        auto directional_light = _light_system->get_directional_data();
        auto num_point_lights  = point_light_data.size();
        _shader->set_uniform(_u_index.directional_light, &directional_light);
        _shader->set_uniform(_u_index.num_point_lights, &num_point_lights);
        _shader->set_uniform(_u_index.point_lights, point_light_data.data());
    }

  private:
    RenderViewPerspective* _perspective_view;
    Texture::Map*          _ssao_texture_map;
    Texture::Map*          _shadow_texture_map;
    glm::vec4              _ambient_color;

    DebugViewMode _render_mode = DebugViewMode::Default;

    struct UIndex {
        uint16 projection                = -1;
        uint16 view                      = -1;
        uint16 ambient_color             = -1;
        uint16 view_position             = -1;
        uint16 mode                      = -1;
        uint16 model                     = -1;
        uint16 directional_light         = -1;
        uint16 num_point_lights          = -1;
        uint16 point_lights              = -1;
        uint16 ssao_texture              = -1;
        uint16 shadowmap_sampled_texture = -1;
    };
    UIndex _u_index {};

    void create_texture_maps(
        const String& ssao_texture, const String& shadow_texture
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
    }
};

} // namespace ENGINE_NAMESPACE