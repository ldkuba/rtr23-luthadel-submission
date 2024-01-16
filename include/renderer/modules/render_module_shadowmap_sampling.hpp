#pragma once

#include "render_module_full_screen.hpp"
#include "renderer/views/render_view_perspective.hpp"
#include "systems/light_system.hpp"

namespace ENGINE_NAMESPACE {

class RenderModuleShadowmapSampling : public RenderModuleFullScreen {
  public:
    struct Config : public RenderModuleFullScreen::Config {
        String depth_texture;
        String directional_shadow_texture;
    };

  public:
    using RenderModuleFullScreen::RenderModuleFullScreen;

    void initialize(const Config& config) {
        RenderModuleFullScreen::initialize(config);
        _perspective_view = config.perspective_view;
        _depth_map        = create_texture_map(
            config.depth_texture,
            Texture::Use::MapPassResult,
            Texture::Filter::BiLinear,
            Texture::Filter::BiLinear,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge
        );
        _directional_shadow_map = create_texture_map(
            config.directional_shadow_texture,
            Texture::Use::MapPassResult,
            Texture::Filter::BiLinear,
            Texture::Filter::BiLinear,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge
        );

        SETUP_UNIFORM_INDEX(projection_inverse);
        SETUP_UNIFORM_INDEX(view_inverse);
        SETUP_UNIFORM_INDEX(light_space_directional);
        SETUP_UNIFORM_INDEX(depth_texture);
        SETUP_UNIFORM_INDEX(shadowmap_directional_texture);
    }

  protected:
    void apply_globals() const override {
        const auto camera = _perspective_view->camera();
        glm::mat4  light_space_directional =
            _light_system->get_directional()->get_light_space_matrix(
                camera->transform.position()
            );

        _shader->set_uniform(
            _u_index.projection_inverse, &_perspective_view->proj_inv_matrix()
        );
        _shader->set_uniform(_u_index.view_inverse, &camera->view_inverse());
        _shader->set_uniform(
            _u_index.light_space_directional, &light_space_directional
        );
        _shader->set_sampler(_u_index.depth_texture, _depth_map);
        _shader->set_sampler(
            _u_index.shadowmap_directional_texture, _directional_shadow_map
        );
    }

  private:
    RenderViewPerspective* _perspective_view;
    Texture::Map*          _depth_map;
    Texture::Map*          _directional_shadow_map;

    struct UIndex {
        uint16 projection_inverse            = -1;
        uint16 view_inverse                  = -1;
        uint16 light_space_directional       = -1;
        uint16 depth_texture                 = -1;
        uint16 shadowmap_directional_texture = -1;
    };
    UIndex _u_index {};
};

} // namespace ENGINE_NAMESPACE