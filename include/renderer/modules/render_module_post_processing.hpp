#pragma once

#include "render_module_full_screen.hpp"
#include "renderer/views/render_view_perspective.hpp"
#include "systems/geometry_system.hpp"

namespace ENGINE_NAMESPACE {

class RenderModulePostProcessing : public RenderModuleFullScreen {
  public:
    struct Config : public RenderModuleFullScreen::Config {
        String color_texture;
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
        SETUP_UNIFORM_INDEX(color_texture);
    }

  protected:
    void on_render(const ModulePacket* const packet, const uint64 frame_number)
        override {
        RenderModuleFullScreen::on_render(packet, frame_number);
    }

    void apply_globals() const override {
        _shader->set_sampler(_u_index.color_texture, _color_map);
    }

  private:
    Texture::Map* _color_map;

    struct UIndex {
        uint16 color_texture = -1;
    };
    UIndex _u_index {};
};

} // namespace ENGINE_NAMESPACE