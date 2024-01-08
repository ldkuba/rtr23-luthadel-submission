#pragma once

#include "render_module.hpp"
#include "renderer/views/render_view_perspective.hpp"
#include "resources/skybox.hpp"
#include "systems/geometry_system.hpp"

namespace ENGINE_NAMESPACE {

class RenderModuleSkybox : public RenderModule {
  public:
    struct Config : public RenderModule::Config {
        RenderViewPerspective* perspective_view;
        String                 cube_texture;
    };

  public:
    using RenderModule::RenderModule;

    ~RenderModuleSkybox() {
        const auto cube_texture_name = _skybox.cube_map()->texture->name();
        _shader->release_instance_resources(_skybox.instance_id);
        _renderer->destroy_texture_map(_skybox.cube_map);
        _texture_system->release(cube_texture_name);
        _geometry_system->release(_skybox.geometry);
    }

    void initialize(const Config& config) {
        _perspective_view = config.perspective_view;
        create_skybox(config.cube_texture);

        SETUP_UNIFORM_INDEX(projection);
        SETUP_UNIFORM_INDEX(view);
        SETUP_UNIFORM_INDEX(cube_texture);
    }

  protected:
    void on_render(const ModulePacket* const packet, const uint64 frame_number)
        override {
        // Apply instance
        _shader->bind_instance(_skybox.instance_id);
        _shader->apply_instance();

        // Draw geometry
        _renderer->draw_geometry(_skybox.geometry());
    }

    void apply_globals() const override {
        // Zero out view position
        auto view_matrix  = _perspective_view->camera()->view();
        view_matrix[3][0] = 0;
        view_matrix[3][1] = 0;
        view_matrix[3][2] = 0;

        _shader->set_uniform(
            _u_index.projection, &_perspective_view->proj_matrix()
        );
        _shader->set_uniform(_u_index.view, &view_matrix);
        _shader->set_sampler(_u_index.cube_texture, _skybox.cube_map);
    }

  private:
    RenderViewPerspective* _perspective_view;
    Skybox                 _skybox;

    struct UIndex {
        uint16 projection   = -1;
        uint16 view         = -1;
        uint16 cube_texture = -1;
    };
    UIndex _u_index {};

    void create_skybox(const String& cube_texture_name) {
        const auto cube_texture =
            _texture_system->acquire_cube(cube_texture_name, true);
        const auto cube_map =
            _renderer->create_texture_map({ cube_texture,
                                            Texture::Use::MapCube,
                                            Texture::Filter::BiLinear,
                                            Texture::Filter::BiLinear,
                                            Texture::Repeat::Repeat,
                                            Texture::Repeat::Repeat,
                                            Texture::Repeat::Repeat });
        const auto skybox_geometry =
            _geometry_system->generate_cube("SkyboxCube", "");
        const auto skybox_instance_id =
            _shader->acquire_instance_resources({ cube_map });
        _skybox = { skybox_instance_id, cube_map, skybox_geometry };
    }
};

} // namespace ENGINE_NAMESPACE