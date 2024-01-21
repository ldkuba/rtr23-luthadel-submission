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
        uint32 num_directional_cascades;

        Config(
            Vector<RenderModule::PassConfig> passes,
            RenderViewPerspective*           perspective_view,
            String                           depth_texture,
            String                           shadow_directional_texture,
            uint32                           num_directional_cascades
        )
            : RenderModuleFullScreen::Config(passes, perspective_view),
              depth_texture(depth_texture),
              shadow_directional_texture(shadow_directional_texture),
              num_directional_cascades(num_directional_cascades) {}
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

        _num_directional_cascades = config.num_directional_cascades;
        for (int i = 0; i < _num_directional_cascades; i++) {
            _directional_shadow_maps.push_back(create_texture_map(
                config.shadow_directional_texture + std::to_string(i),
                Texture::Use::MapPassResult,
                Texture::Filter::BiLinear,
                Texture::Filter::BiLinear,
                Texture::Repeat::ClampToEdge,
                Texture::Repeat::ClampToEdge,
                Texture::Repeat::ClampToEdge
            ));
            setup_uniform_indices(_u_names.shadowmap_directional_textures.at(i)
            );
        }

        setup_uniform_indices(_u_names.depth_texture);
        setup_uniform_indices(_u_names.projection_inverse);
        setup_uniform_indices(_u_names.view_inverse);
        setup_uniform_indices(_u_names.camera_position);
        setup_uniform_indices(_u_names.light_spaces_directional);
        setup_uniform_indices(_u_names.light_pos_directional);
        setup_uniform_indices(_u_names.light_color_directional);
        setup_uniform_indices(_u_names.animation_time);
        setup_uniform_indices(_u_names.num_directional_cascades);
    }

  protected:
    void on_render(
        const ModulePacket* const packet,
        const uint64              frame_number,
        uint32                    rp_index
    ) override {
        RenderModuleFullScreen::on_render(packet, frame_number, rp_index);
    }

    void apply_globals(uint32 rp_index) const override {
        auto shader = _renderpasses.at(rp_index).shader;
        shader->set_sampler(UNIFORM_ID(depth_texture), _depth_map);

        // Directional shadow maps (cascades)
        for (int i = 0; i < _num_directional_cascades; i++) {
            shader->set_sampler(
                UNIFORM_ID(shadowmap_directional_textures.at(i)),
                _directional_shadow_maps.at(i)
            );
        }
        shader->set_uniform(
            UNIFORM_ID(num_directional_cascades), &_num_directional_cascades
        );

        shader->set_uniform(
            UNIFORM_ID(projection_inverse),
            &_perspective_view->proj_inv_matrix()
        );

        glm::mat4 view_inverse =
            glm::inverse(_perspective_view->camera()->view());
        shader->set_uniform(UNIFORM_ID(view_inverse), &view_inverse);

        glm::vec4 camera_position =
            glm::vec4(_perspective_view->camera()->transform.position(), 1.0f);
        shader->set_uniform(UNIFORM_ID(camera_position), &camera_position);

        Vector<glm::mat4> light_spaces_directional =
            _light_system->get_directional()->get_light_space_matrices();
        shader->set_uniform(
            UNIFORM_ID(light_spaces_directional), light_spaces_directional.data()
        );

        glm::vec4 light_pos_directional =
            _light_system->get_directional()->get_light_camera_position();
        shader->set_uniform(
            UNIFORM_ID(light_pos_directional), &light_pos_directional
        );

        glm::vec4 light_color_directional =
            _light_system->get_directional()->data.color;
        shader->set_uniform(
            UNIFORM_ID(light_color_directional), &light_color_directional
        );

        auto  duration = std::chrono::system_clock::now() - _start_time;
        float time =
            std::chrono::duration_cast<std::chrono::milliseconds>(duration)
                .count();
        shader->set_uniform(UNIFORM_ID(animation_time), &time);
    }

  private:
    uint32                                _num_directional_cascades;
    Texture::Map*                         _depth_map = nullptr;
    Vector<Texture::Map*>                 _directional_shadow_maps;
    std::chrono::system_clock::time_point _start_time;

    struct Uniforms {
        UNIFORM_NAME(depth_texture);
        // Max 4 cascades
        Vector<String> shadowmap_directional_textures {
            "shadowmap_directional_texture0",
            "shadowmap_directional_texture1",
            "shadowmap_directional_texture2",
            "shadowmap_directional_texture3",
        };
        UNIFORM_NAME(projection_inverse);
        UNIFORM_NAME(view_inverse);
        UNIFORM_NAME(camera_position);
        UNIFORM_NAME(light_spaces_directional);
        UNIFORM_NAME(light_pos_directional);
        UNIFORM_NAME(light_color_directional);
        UNIFORM_NAME(animation_time);
        UNIFORM_NAME(num_directional_cascades);
    };
    Uniforms _u_names {};
};

} // namespace ENGINE_NAMESPACE
