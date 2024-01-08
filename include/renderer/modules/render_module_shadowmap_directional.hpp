#pragma once

#include "render_module.hpp"
#include "renderer/views/render_view_orthographic.hpp"
#include "systems/light_system.hpp"

namespace ENGINE_NAMESPACE {

class RenderModuleShadowmapDirectional : public RenderModule {
  public:
    struct Config : public RenderModule::Config {
        RenderViewOrthographic* orthographic_view;
    };

  public:
    using RenderModule::RenderModule;

    void initialize(const Config& config) {
        _orthographic_view = config.orthographic_view;

        SETUP_UNIFORM_INDEX(light_space);
        SETUP_UNIFORM_INDEX(model);
    }

  protected:
    void on_render(const ModulePacket* const packet, const uint64 frame_number)
        override {
        // Get visible geometries
        const auto geometry_data =
            _orthographic_view->get_visible_render_data(frame_number);

        // Draw geometries
        for (const auto& geo_data : geometry_data) {
            // Apply local
            _shader->set_uniform(_u_index.model, &geo_data.model);

            // Draw geometry
            _renderer->draw_geometry(geo_data.geometry);
        }
    }

    void apply_globals() const override {
        glm::mat4 light_space =
            _light_system->get_directional()->get_light_space_matrix(
                _orthographic_view->camera()->transform.position()
            );
        _shader->set_uniform(_u_index.light_space, &light_space);
    }

  private:
    RenderViewOrthographic* _orthographic_view;

    struct UIndex {
        uint16 light_space = -1;
        uint16 model       = -1;
    };
    UIndex _u_index {};
};

} // namespace ENGINE_NAMESPACE