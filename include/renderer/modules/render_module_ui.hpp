#pragma once

#include "render_module.hpp"
#include "renderer/views/render_view_orthographic.hpp"

namespace ENGINE_NAMESPACE {

class RenderModuleUI : public RenderModule {
  public:
    struct Config : public RenderModule::Config {
        RenderViewOrthographic* orthographic_view;
    };

  public:
    using RenderModule::RenderModule;

    void initialize(const Config& config) {
        _orthographic_view = config.orthographic_view;

        SETUP_UNIFORM_INDEX(projection);
        SETUP_UNIFORM_INDEX(view);
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
            // Update material instance
            geo_data.material->apply_instance();

            // Apply local
            _shader->set_uniform(_u_index.model, &geo_data.model);

            // Draw geometry
            _renderer->draw_geometry(geo_data.geometry);
        }
    }

    void apply_globals() const override {
        _shader->set_uniform(
            _u_index.projection, &_orthographic_view->proj_matrix()
        );
        _shader->set_uniform(_u_index.view, &_view_matrix);
    }

  private:
    RenderViewOrthographic* _orthographic_view;
    glm::mat4               _view_matrix = glm::identity<glm::mat4>();

    struct UIndex {
        uint16 projection = -1;
        uint16 view       = -1;
        uint16 model      = -1;
    };
    UIndex _u_index {};
};

} // namespace ENGINE_NAMESPACE