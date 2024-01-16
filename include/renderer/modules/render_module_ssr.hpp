#pragma once

#include "render_module_post_processing.hpp"

namespace ENGINE_NAMESPACE {

class RenderModuleSSR : public RenderModulePostProcessing {
  public:
    struct Config : public RenderModulePostProcessing::Config {
        String g_pre_pass_texture;
        String depth_texture;
    };

  public:
    using RenderModulePostProcessing::RenderModulePostProcessing;

    void initialize(const Config& config) {
        RenderModulePostProcessing::initialize(config);
        _g_pass_map = create_texture_map(
            config.g_pre_pass_texture,
            Texture::Use::MapPassResult,
            Texture::Filter::NearestNeighbour,
            Texture::Filter::NearestNeighbour,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge
        );
        _depth_map = create_texture_map(
            config.depth_texture,
            Texture::Use::MapPassResult,
            Texture::Filter::BiLinear,
            Texture::Filter::BiLinear,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge
        );

        SETUP_UNIFORM_INDEX(projection);
        SETUP_UNIFORM_INDEX(projection_inverse);
        SETUP_UNIFORM_INDEX(view);
        SETUP_UNIFORM_INDEX(view_inverse);
        SETUP_UNIFORM_INDEX(g_pre_pass_texture);
        SETUP_UNIFORM_INDEX(depth_texture);
        SETUP_UNIFORM_INDEX(view_origin);
        SETUP_UNIFORM_INDEX(enabled);
    }

    void toggle() { _enabled = 1.0 - _enabled; }

  protected:
    void apply_globals() const override {
        RenderModulePostProcessing::apply_globals();

        const auto camera = _perspective_view->camera();

        _shader->set_uniform(
            _u_index.projection, &_perspective_view->proj_matrix()
        );
        _shader->set_uniform(
            _u_index.projection_inverse, &_perspective_view->proj_inv_matrix()
        );
        _shader->set_uniform(_u_index.view, &camera->view());
        _shader->set_uniform(_u_index.view_inverse, &camera->view_inverse());
        _shader->set_uniform(
            _u_index.view_origin, &camera->transform.position()
        );
        _shader->set_uniform(_u_index.enabled, &_enabled);
        _shader->set_sampler(_u_index.g_pre_pass_texture, _g_pass_map);
        _shader->set_sampler(_u_index.depth_texture, _depth_map);
    }

  private:
    Texture::Map* _g_pass_map;
    Texture::Map* _depth_map;
    float32       _enabled = 1.0;

    struct UIndex {
        uint16 g_pre_pass_texture = -1;
        uint16 depth_texture      = -1;
        uint16 projection         = -1;
        uint16 projection_inverse = -1;
        uint16 view               = -1;
        uint16 view_inverse       = -1;
        uint16 view_origin        = -1;
        uint16 enabled            = -1;
    };
    UIndex _u_index {};
};

} // namespace ENGINE_NAMESPACE