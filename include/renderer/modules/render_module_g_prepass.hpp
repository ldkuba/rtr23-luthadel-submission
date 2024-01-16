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
        SETUP_UNIFORM_INDEX(smoothness);
    }

    // TODO: TEMP
    void initialize_shader_data() {
        // Initialize material to geometry pass mapping
        const auto render_data = _perspective_view->get_all_render_data();
        for (const auto& geo_data : render_data) {
            const auto material_id = geo_data.material->internal_id.value();
            if (_material_to_g_pass_id.contains(material_id)) continue;
            _material_to_g_pass_id[material_id] =
                _shader->acquire_instance_resources({});
        }
    }

  protected:
    void on_render(const ModulePacket* const packet, const uint64 frame_number)
        override {
        // Get visible geometries
        const auto geometry_data =
            _perspective_view->get_visible_render_data(frame_number);

        // Draw geometries
        for (const auto& geo_data : geometry_data) {
            // Apply instance
            const auto material_id = geo_data.material->internal_id.value();
            const auto g_pass_id   = _material_to_g_pass_id[material_id];
            _shader->bind_instance(g_pass_id);
            _shader->set_uniform(
                _u_index.smoothness, &geo_data.material->smoothness()
            );
            _shader->apply_instance();

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
    Map<uint32, uint32>    _material_to_g_pass_id {};

    struct UIndex {
        uint16 projection = -1;
        uint16 view       = -1;
        uint16 model      = -1;
        uint16 smoothness = -1;
    };
    UIndex _u_index {};
};

} // namespace ENGINE_NAMESPACE