#pragma once

#include "render_module.hpp"
#include "renderer/views/render_view_perspective.hpp"
#include "systems/light_system.hpp"

namespace ENGINE_NAMESPACE {

class RenderModuleShadowmapSampling : public RenderModule {
  public:
    struct Config : public RenderModule::Config {
        RenderViewPerspective* perspective_view;
        String                 directional_shadow_texture;
    };

  public:
    using RenderModule::RenderModule;

    void initialize(const Config& config) {
        _perspective_view       = config.perspective_view;
        _directional_shadow_map = create_texture_map(
            config.directional_shadow_texture,
            Texture::Use::MapPassResult,
            Texture::Filter::BiLinear,
            Texture::Filter::BiLinear,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge
        );

        SETUP_UNIFORM_INDEX(projection);
        SETUP_UNIFORM_INDEX(view);
        SETUP_UNIFORM_INDEX(light_space_directional);
        SETUP_UNIFORM_INDEX(model);
        SETUP_UNIFORM_INDEX(shadowmap_directional_texture);
    }

  protected:
    void on_render(const ModulePacket* const packet, const uint64 frame_number)
        override {
        // Get visible geometries
        const auto geometry_data =
            _perspective_view->get_visible_render_data(frame_number);

        // Draw geometries
        for (const auto& geo_data : geometry_data) {
            // Apply local
            _shader->set_uniform(_u_index.model, &geo_data.model);

            // Draw geometry
            _renderer->draw_geometry(geo_data.geometry);
        }
    }

    void apply_globals() const override {
        const auto camera = _perspective_view->camera();

        _shader->set_uniform(
            _u_index.projection, &_perspective_view->proj_matrix()
        );
        _shader->set_uniform(_u_index.view, &camera->view());

        glm::mat4 light_space_directional =
            _light_system->get_directional()->get_light_space_matrix(
                camera->transform.position()
            );
        _shader->set_uniform(
            _u_index.light_space_directional, &light_space_directional
        );
        _shader->set_sampler(
            _u_index.shadowmap_directional_texture, _directional_shadow_map
        );
    }

  private:
    RenderViewPerspective* _perspective_view;
    Texture::Map*          _directional_shadow_map;

    struct UIndex {
        uint16 projection                    = -1;
        uint16 view                          = -1;
        uint16 light_space_directional       = -1;
        uint16 model                         = -1;
        uint16 shadowmap_directional_texture = -1;
    };
    UIndex _u_index {};
};

} // namespace ENGINE_NAMESPACE