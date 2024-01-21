#pragma once

#include "render_module_post_processing.hpp"

namespace ENGINE_NAMESPACE {

class RenderModulePostProcessingEffects : public RenderModulePostProcessing {
  public:
    struct Config : public RenderModulePostProcessing::Config {
        String depth_texture;

        Config(
            Vector<RenderModule::PassConfig> passes,
            RenderViewPerspective*           perspective_view,
            String                           color_texture,
            String                           depth_texture
        )
            : RenderModulePostProcessing::Config(
                  passes, perspective_view, color_texture
              ),
              depth_texture(depth_texture) {}
    };

  public:
    using RenderModulePostProcessing::RenderModulePostProcessing;

    void initialize(const Config& config) {
        RenderModulePostProcessing::initialize(config);
        _depth_map = create_texture_map(
            config.depth_texture,
            Texture::Use::MapPassResult,
            Texture::Filter::BiLinear,
            Texture::Filter::BiLinear,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge
        );

        setup_uniform_indices(_u_names.exposure);
        setup_uniform_indices(_u_names.max_blur);
        setup_uniform_indices(_u_names.aperture);
        setup_uniform_indices(_u_names.focus);
        setup_uniform_indices(_u_names.aspect);
        setup_uniform_indices(_u_names.depth_texture);
    }

  protected:
    void apply_globals(uint32 rp_index) const override {
        RenderModulePostProcessing::apply_globals(rp_index);
        const auto camera = _perspective_view->camera();
        float32    aspect = _perspective_view->aspect_ratio();

        auto shader = _renderpasses.at(rp_index).shader;

        shader->set_uniform(UNIFORM_ID(exposure), &_exposure);
        shader->set_uniform(UNIFORM_ID(max_blur), &_max_blur);
        shader->set_uniform(UNIFORM_ID(aperture), &_aperture);
        shader->set_uniform(UNIFORM_ID(focus), &_focus);
        shader->set_uniform(UNIFORM_ID(aspect), &aspect);
        shader->set_sampler(UNIFORM_ID(depth_texture), _depth_map);
    }

  private:
    Texture::Map* _depth_map;
    float32       _exposure = 0.9;
    float32       _max_blur = 0.6;
    float32       _aperture = 0.05;
    float32       _focus    = 0.985;

    struct Uniforms {
        UNIFORM_NAME(depth_texture);
        UNIFORM_NAME(exposure);
        UNIFORM_NAME(max_blur);
        UNIFORM_NAME(aperture);
        UNIFORM_NAME(focus);
        UNIFORM_NAME(aspect);
    };
    Uniforms _u_names {};
};

} // namespace ENGINE_NAMESPACE