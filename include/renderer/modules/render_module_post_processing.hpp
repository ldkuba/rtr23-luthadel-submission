#pragma once

#include "render_module_full_screen.hpp"
#include "renderer/views/render_view_perspective.hpp"

namespace ENGINE_NAMESPACE {

class RenderModulePostProcessing : public RenderModuleFullScreen {
  public:
    struct Config : public RenderModuleFullScreen::Config {
        String color_texture;

        Config(
            Vector<RenderModule::PassConfig> passes,
            RenderViewPerspective*           perspective_view,
            String                           color_texture
        )
            : RenderModuleFullScreen::Config(passes, perspective_view),
              color_texture(color_texture) {
        }
    };

  public:
    using RenderModuleFullScreen::RenderModuleFullScreen;

    void initialize(const Config& config) {
        RenderModuleFullScreen::initialize(config);
        _color_map = create_texture_map(
            config.color_texture,
            Texture::Use::MapPassResult,
            Texture::Filter::BiLinear,
            Texture::Filter::BiLinear,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge
        );
        setup_uniform_indices(_u_names.color_texture);
    }

  protected:
    void on_render(const ModulePacket* const packet, const uint64 frame_number, uint32 rp_index)
        override {
        RenderModuleFullScreen::on_render(packet, frame_number, rp_index);
    }

    void apply_globals(uint32 rp_index) const override {
        auto shader = _renderpasses.at(rp_index).shader;
        shader->set_sampler(UNIFORM_ID(color_texture), _color_map);
    }

  private:
    Texture::Map* _color_map;

    struct Uniforms {
        UNIFORM_NAME(color_texture);
    };
    Uniforms _u_names {};
};

} // namespace ENGINE_NAMESPACE