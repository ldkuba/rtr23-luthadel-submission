#pragma once

#include "render_module_full_screen.hpp"
#include "renderer/views/render_view_perspective.hpp"
#include "systems/geometry_system.hpp"
#include <chrono>

namespace ENGINE_NAMESPACE {

class RenderModuleVolumetrics : public RenderModuleFullScreen {
  public:
    struct Config : public RenderModuleFullScreen::Config {
        String depth_texture;
        String shadow_directional_texture;
    };

  public:
    using RenderModuleFullScreen::RenderModuleFullScreen;

    void initialize(const Config& config) {
        RenderModuleFullScreen::initialize(config);

        _start_time = std::chrono::system_clock::now();

        _depth_map = create_texture_map(
            config.depth_texture,
            Texture::Use::MapPassResult,
            Texture::Filter::NearestNeighbour,
            Texture::Filter::NearestNeighbour,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge
        );

        _shadow_directional_map = create_texture_map(
            config.shadow_directional_texture,
            Texture::Use::MapPassResult,
            Texture::Filter::BiLinear,
            Texture::Filter::BiLinear,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge,
            Texture::Repeat::ClampToEdge
        );

        SETUP_UNIFORM_INDEX(depth_texture);
        SETUP_UNIFORM_INDEX(shadow_directional_texture);
        SETUP_UNIFORM_INDEX(projection_inverse);
        SETUP_UNIFORM_INDEX(view_inverse);
        SETUP_UNIFORM_INDEX(camera_position);
        SETUP_UNIFORM_INDEX(light_space_directional);
        SETUP_UNIFORM_INDEX(light_pos_directional);
        SETUP_UNIFORM_INDEX(light_color_directional);
        SETUP_UNIFORM_INDEX(animation_time);
    }

  protected:
    void on_render(const ModulePacket* const packet, const uint64 frame_number)
        override {
        RenderModuleFullScreen::on_render(packet, frame_number);
    }

    void apply_globals() const override {
        _shader->set_sampler(_u_index.depth_texture, _depth_map);
        _shader->set_sampler(
            _u_index.shadow_directional_texture, _shadow_directional_map
        );

        _shader->set_uniform(
            _u_index.projection_inverse, &_perspective_view->proj_inv_matrix()
        );

        glm::mat4 view_inverse =
            glm::inverse(_perspective_view->camera()->view());
        _shader->set_uniform(_u_index.view_inverse, &view_inverse);

        glm::vec4 camera_position =
            glm::vec4(_perspective_view->camera()->transform.position(), 1.0f);
        _shader->set_uniform(_u_index.camera_position, &camera_position);

        glm::mat4 light_space_directional =
            _light_system->get_directional()->get_light_space_matrix(
                _perspective_view->camera()->transform.position()
            );
        _shader->set_uniform(
            _u_index.light_space_directional, &light_space_directional
        );

        glm::vec4 light_pos_directional = glm::vec4(
            _light_system->get_directional()->get_light_camera_position(
                _perspective_view->camera()->transform.position()
            )
        );
        _shader->set_uniform(
            _u_index.light_pos_directional, &light_pos_directional
        );

        glm::vec4 light_color_directional =
            _light_system->get_directional()->data.color;
        _shader->set_uniform(
            _u_index.light_color_directional, &light_color_directional
        );

        auto  duration = std::chrono::system_clock::now() - _start_time;
        float time =
            std::chrono::duration_cast<std::chrono::milliseconds>(duration)
                .count();
        _shader->set_uniform(_u_index.animation_time, &time);
    }

  private:
    Texture::Map*                         _depth_map              = nullptr;
    Texture::Map*                         _shadow_directional_map = nullptr;
    std::chrono::system_clock::time_point _start_time;

    struct UIndex {
        uint16 depth_texture              = -1;
        uint16 shadow_directional_texture = -1;
        uint16 projection_inverse         = -1;
        uint16 view_inverse               = -1;
        uint16 camera_position            = -1;
        uint16 light_space_directional    = -1;
        uint16 light_pos_directional      = -1;
        uint16 light_color_directional    = -1;
        uint16 animation_time             = -1;
    };
    UIndex _u_index {};
};

} // namespace ENGINE_NAMESPACE