#pragma once

#include "render_module_post_processing.hpp"

namespace ENGINE_NAMESPACE {

class RenderModuleSSR : public RenderModulePostProcessing {
  public:
    struct Config : public RenderModulePostProcessing::Config {
        String g_pre_pass_texture;
        String depth_texture;

        Config(
            Vector<RenderModule::PassConfig> passes,
            RenderViewPerspective*           perspective_view,
            String                           color_texture,
            String                           g_pre_pass_texture,
            String                           depth_texture
        )
            : RenderModulePostProcessing::Config(
                  passes, perspective_view, color_texture
              ),
              g_pre_pass_texture(g_pre_pass_texture),
              depth_texture(depth_texture) {}
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

        setup_uniform_indices(_u_names.projection);
        setup_uniform_indices(_u_names.projection_inverse);
        setup_uniform_indices(_u_names.view);
        setup_uniform_indices(_u_names.view_inverse);
        setup_uniform_indices(_u_names.g_pre_pass_texture);
        setup_uniform_indices(_u_names.depth_texture);
        setup_uniform_indices(_u_names.view_origin);
        setup_uniform_indices(_u_names.enabled);
    }

    void toggle() { _enabled = 1.0 - _enabled; }

  protected:
    void apply_globals(uint32 rp_index) const override {
        RenderModulePostProcessing::apply_globals(rp_index);

        auto       shader = _renderpasses.at(rp_index).shader;
        const auto camera = _perspective_view->camera();

        shader->set_uniform(
            UNIFORM_ID(projection), &_perspective_view->proj_matrix()
        );
        shader->set_uniform(
            UNIFORM_ID(projection_inverse),
            &_perspective_view->proj_inv_matrix()
        );
        shader->set_uniform(UNIFORM_ID(view), &camera->view());
        shader->set_uniform(UNIFORM_ID(view_inverse), &camera->view_inverse());
        shader->set_uniform(
            UNIFORM_ID(view_origin), &camera->transform.position()
        );
        shader->set_uniform(UNIFORM_ID(enabled), &_enabled);
        shader->set_sampler(UNIFORM_ID(g_pre_pass_texture), _g_pass_map);
        shader->set_sampler(UNIFORM_ID(depth_texture), _depth_map);
    }

  private:
    Texture::Map* _g_pass_map;
    Texture::Map* _depth_map;
    float32       _enabled = 1.0;

    struct Uniforms {
        UNIFORM_NAME(g_pre_pass_texture);
        UNIFORM_NAME(depth_texture);
        UNIFORM_NAME(projection);
        UNIFORM_NAME(projection_inverse);
        UNIFORM_NAME(view);
        UNIFORM_NAME(view_inverse);
        UNIFORM_NAME(view_origin);
        UNIFORM_NAME(enabled);
    };
    Uniforms _u_names {};
};

} // namespace ENGINE_NAMESPACE