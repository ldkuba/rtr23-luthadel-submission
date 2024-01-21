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
        uint32 num_directional_cascades;

        Config(
            Vector<RenderModule::PassConfig> passes,
            RenderViewPerspective*           perspective_view,
            String                           depth_texture,
            String                           directional_shadow_texture,
            uint32                           num_directional_cascades
        )
            : RenderModuleFullScreen::Config(passes, perspective_view),
              depth_texture(depth_texture),
              directional_shadow_texture(directional_shadow_texture),
              num_directional_cascades(num_directional_cascades) {
        }
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

        _num_directional_cascades = config.num_directional_cascades;
        for (int i = 0; i < _num_directional_cascades; i++) {
            _directional_shadow_maps.push_back(create_texture_map(
                config.directional_shadow_texture + std::to_string(i),
                Texture::Use::MapPassResult,
                Texture::Filter::BiLinear,
                Texture::Filter::BiLinear,
                Texture::Repeat::ClampToEdge,
                Texture::Repeat::ClampToEdge,
                Texture::Repeat::ClampToEdge
            ));
            setup_uniform_indices(_u_names.shadowmap_directional_textures.at(i));
        }

        setup_uniform_indices(_u_names.projection_inverse);
        setup_uniform_indices(_u_names.view_inverse);
        setup_uniform_indices(_u_names.light_spaces_directional);
        setup_uniform_indices(_u_names.num_directional_cascades);
        setup_uniform_indices(_u_names.depth_texture);
    }

  protected:
    void apply_globals(uint32 rp_index) const override {
        auto shader = _renderpasses.at(rp_index).shader;
        const auto        camera = _perspective_view->camera();
        shader->set_uniform(
            UNIFORM_ID(projection_inverse), &_perspective_view->proj_inv_matrix()
        );
        shader->set_uniform(UNIFORM_ID(view_inverse), &camera->view_inverse());

        // Directional light spaces (cascades)
        Vector<glm::mat4> light_spaces_directional =
            _light_system->get_directional()->get_light_space_matrices();
        shader->set_uniform(
            UNIFORM_ID(light_spaces_directional), light_spaces_directional.data()
        );

        // Depth map
        shader->set_sampler(UNIFORM_ID(depth_texture), _depth_map);

        // Directional shadow maps (cascades)
        for(int i = 0; i < _num_directional_cascades; i++) {
            shader->set_sampler(
                UNIFORM_ID(shadowmap_directional_textures.at(i)), _directional_shadow_maps.at(i)
            );
        }

        // Directional cascade number
        shader->set_uniform(UNIFORM_ID(num_directional_cascades), &_num_directional_cascades);
    }

  private:
    uint32                 _num_directional_cascades;
    RenderViewPerspective* _perspective_view;
    Texture::Map*          _depth_map;
    Vector<Texture::Map*>  _directional_shadow_maps;

    struct Uniforms {
        UNIFORM_NAME(projection_inverse);
        UNIFORM_NAME(view_inverse);
        UNIFORM_NAME(light_spaces_directional);
        UNIFORM_NAME(num_directional_cascades);
        UNIFORM_NAME(depth_texture);
        // Max 4 cascades
        Vector<String> shadowmap_directional_textures {
            "shadowmap_directional_texture0",
            "shadowmap_directional_texture1",
            "shadowmap_directional_texture2",
            "shadowmap_directional_texture3",
        };
    };
    Uniforms _u_names {};
};

} // namespace ENGINE_NAMESPACE
