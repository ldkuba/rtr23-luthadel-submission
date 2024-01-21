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

        setup_uniform_indices(_u_names.projection);
        setup_uniform_indices(_u_names.view);
        setup_uniform_indices(_u_names.model);
    }

  protected:
    void on_render(const ModulePacket* const packet, const uint64 frame_number, uint32 rp_index)
        override {
        auto shader = _renderpasses.at(rp_index).shader;
        // Get visible geometries
        const auto geometry_data =
            _orthographic_view->get_visible_render_data(frame_number);

        // Draw geometries
        for (const auto& geo_data : geometry_data) {
            // Update material instance
            geo_data.material->apply_instance();

            // Apply local
            shader->set_uniform(UNIFORM_ID(model), &geo_data.model);

            // Draw geometry
            _renderer->draw_geometry(geo_data.geometry);
        }
    }

    void apply_globals(uint32 rp_index) const override {
        auto shader = _renderpasses.at(rp_index).shader;
        shader->set_uniform(
            UNIFORM_ID(projection), &_orthographic_view->proj_matrix()
        );
        shader->set_uniform(UNIFORM_ID(view), &_view_matrix);
    }

  private:
    RenderViewOrthographic* _orthographic_view;
    glm::mat4               _view_matrix = glm::identity<glm::mat4>();

    struct Uniforms {
        UNIFORM_NAME(projection);
        UNIFORM_NAME(view);
        UNIFORM_NAME(model);
    };
    Uniforms _u_names {};
};

} // namespace ENGINE_NAMESPACE