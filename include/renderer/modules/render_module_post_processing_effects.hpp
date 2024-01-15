#pragma once

#include "render_module_post_processing.hpp"

namespace ENGINE_NAMESPACE {

class RenderModulePostProcessingEffects : public RenderModulePostProcessing {
  public:
    struct Config : public RenderModulePostProcessing::Config {
        String g_pre_pass_texture;
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

        SETUP_UNIFORM_INDEX(exposure);
        SETUP_UNIFORM_INDEX(max_blur);
        SETUP_UNIFORM_INDEX(aperture);
        SETUP_UNIFORM_INDEX(focus);
        SETUP_UNIFORM_INDEX(aspect);
        SETUP_UNIFORM_INDEX(g_pre_pass_texture);
    }

  protected:
    void apply_globals() const override {
        RenderModulePostProcessing::apply_globals();
        const auto camera = _perspective_view->camera();
        float32    aspect = _perspective_view->aspect_ratio();

        _shader->set_uniform(_u_index.exposure, &_exposure);
        _shader->set_uniform(_u_index.max_blur, &_max_blur);
        _shader->set_uniform(_u_index.aperture, &_aperture);
        _shader->set_uniform(_u_index.focus, &_focus);
        _shader->set_uniform(_u_index.aspect, &aspect);
        _shader->set_sampler(_u_index.g_pre_pass_texture, _g_pass_map);
    }

  private:
    Texture::Map* _g_pass_map;
    float32       _exposure = 1.0;
    float32       _max_blur = 0.6;
    float32       _aperture = 0.05;
    float32       _focus    = 0.985;

    struct UIndex {
        uint16 g_pre_pass_texture = -1;
        uint16 exposure           = -1;
        uint16 max_blur           = -1;
        uint16 aperture           = -1;
        uint16 focus              = -1;
        uint16 aspect             = -1;
    };
    UIndex _u_index {};
};

} // namespace ENGINE_NAMESPACE