#pragma once

#include "render_module.hpp"
#include "renderer/views/render_view_perspective.hpp"

namespace ENGINE_NAMESPACE {

class RenderModuleGPrepass : public RenderModule {
  public:
    struct Config : public RenderModule::Config {
        RenderViewPerspective* perspective_view;
    };

  public:
    using RenderModule::RenderModule;

    void initialize(const Config& config) {
        _perspective_view = config.perspective_view;

        SETUP_UNIFORM_INDEX(projection);
        SETUP_UNIFORM_INDEX(view);
        SETUP_UNIFORM_INDEX(model);
    }

  protected:
    void on_render(const ModulePacket* const packet, const uint64 frame_number)
        override {
        // Get visible geometries
        const auto geometry_data =
            _perspective_view->get_visible_render_data(frame_number);

        // Draw geometries
        for (const auto& geo_data : geometry_data) {
            // Apply local
            _shader->set_uniform(_u_index.model, &geo_data.model);

            // Draw geometry
            _renderer->draw_geometry(geo_data.geometry);
        }
    }

    void apply_globals() const override {
        _shader->set_uniform(
            _u_index.projection, &_perspective_view->proj_matrix()
        );
        _shader->set_uniform(
            _u_index.view, &_perspective_view->camera()->view()
        );
    }

  private:
    RenderViewPerspective* _perspective_view;

    struct UIndex {
        uint16 projection = -1;
        uint16 view       = -1;
        uint16 model      = -1;
    };
    UIndex _u_index {};
};

} // namespace ENGINE_NAMESPACE