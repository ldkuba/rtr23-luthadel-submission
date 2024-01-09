#pragma once

#include "render_module_full_screen.hpp"
#include "renderer/views/render_view_perspective.hpp"
#include "systems/geometry_system.hpp"

namespace ENGINE_NAMESPACE {

class RenderModuleVolumetrics : public RenderModuleFullScreen {
  public:
    struct Config : public RenderModuleFullScreen::Config {
        String color_texture;
        String depth_texture;
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

        _depth_map = create_texture_map(
            config.depth_texture,
            Texture::Use::MapPassResult,
            Texture::Filter::BiLinear,
            Texture::Filter::BiLinear,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge
        );

        SETUP_UNIFORM_INDEX(color_texture);
        SETUP_UNIFORM_INDEX(depth_texture);
        SETUP_UNIFORM_INDEX(projection_inverse);
        SETUP_UNIFORM_INDEX(view_inverse);
    }

  protected:
    void on_render(const ModulePacket* const packet, const uint64 frame_number)
        override {
        RenderModuleFullScreen::on_render(packet, frame_number);
    }

    void apply_globals() const override {
        _shader->set_sampler(_u_index.color_texture, _color_map);
        _shader->set_sampler(_u_index.depth_texture, _depth_map);

        _shader->set_uniform(
            _u_index.projection_inverse, &_perspective_view->proj_inv_matrix()
        );

        glm::mat4 view_inverse = glm::inverse(_perspective_view->camera()->view());
        _shader->set_uniform(_u_index.view_inverse, &view_inverse);
    }

  private:
    Texture::Map* _color_map = nullptr;
    Texture::Map* _depth_map = nullptr;

    struct UIndex {
        uint16 color_texture      = -1;
        uint16 depth_texture      = -1;
        uint16 projection_inverse = -1;
        uint16 view_inverse       = -1;
    };
    UIndex _u_index {};
};

} // namespace ENGINE_NAMESPACE