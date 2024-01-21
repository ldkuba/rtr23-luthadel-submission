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

        setup_uniform_indices(_u_names.projection);
        setup_uniform_indices(_u_names.view);
        setup_uniform_indices(_u_names.model);
        setup_uniform_indices(_u_names.smoothness);
    }

    // TODO: TEMP
    void initialize_shader_data() {
        // Initialize material to geometry pass mapping
        auto shader = _renderpasses.at(0).shader;
        const auto render_data = _perspective_view->get_all_render_data();
        for (const auto& geo_data : render_data) {
            const auto material_id = geo_data.material->internal_id.value();
            if (_material_to_g_pass_id.contains(material_id)) continue;
            _material_to_g_pass_id[material_id] =
                shader->acquire_instance_resources({});
        }
    }

  protected:
    void on_render(const ModulePacket* const packet, const uint64 frame_number, uint32 rp_index)
        override {

        auto shader = _renderpasses.at(rp_index).shader;

        // Get visible geometries
        const auto geometry_data =
            _perspective_view->get_visible_render_data(frame_number);

        // Draw geometries
        for (const auto& geo_data : geometry_data) {
            // Apply instance
            const auto material_id = geo_data.material->internal_id.value();
            const auto g_pass_id   = _material_to_g_pass_id[material_id];
            shader->bind_instance(g_pass_id);
            shader->set_uniform(
                UNIFORM_ID(smoothness), &geo_data.material->smoothness()
            );
            shader->apply_instance();

            // Apply local
            shader->set_uniform(UNIFORM_ID(model), &geo_data.model);

            // Draw geometry
            _renderer->draw_geometry(geo_data.geometry);
        }
    }

    void apply_globals(uint32 rp_index) const override {
        auto shader = _renderpasses.at(rp_index).shader;
        shader->set_uniform(
            UNIFORM_ID(projection), &_perspective_view->proj_matrix()
        );
        shader->set_uniform(
            UNIFORM_ID(view), &_perspective_view->camera()->view()
        );
    }

  private:
    RenderViewPerspective* _perspective_view;
    Map<uint32, uint32>    _material_to_g_pass_id {};

    struct Uniforms {
        UNIFORM_NAME(projection);
        UNIFORM_NAME(view);
        UNIFORM_NAME(model);
        UNIFORM_NAME(smoothness);
    };
    Uniforms _u_names {};
};

} // namespace ENGINE_NAMESPACE