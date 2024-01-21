#pragma once

#include "render_module.hpp"
#include "renderer/views/render_view_directional_shadow.hpp"
#include "systems/light_system.hpp"

namespace ENGINE_NAMESPACE {

class RenderModuleShadowmapDirectional : public RenderModule {
  public:
    struct Config : public RenderModule::Config {};

  public:
    using RenderModule::RenderModule;

    void initialize(const Config& config) {
        setup_uniform_indices(_u_names.light_space);
        setup_uniform_indices(_u_names.model);
    }

  protected:
    void on_render(
        const ModulePacket* const packet,
        const uint64              frame_number,
        uint32                    rp_index
    ) override {

        auto shader = _renderpasses.at(rp_index).shader;

        // Get visible geometries
        const auto geometry_data = _light_system->get_directional()
                                       ->get_render_views()
                                       .at(rp_index)
                                       ->get_visible_render_data(frame_number);

        // Draw geometries
        for (const auto& geo_data : geometry_data) {
            // Apply local
            shader->set_uniform(UNIFORM_ID(model), &geo_data.model);

            // Draw geometry
            _renderer->draw_geometry(geo_data.geometry);
        }
    }

    void apply_globals(uint32 rp_index) const override {
        auto shader = _renderpasses.at(rp_index).shader;

        glm::mat4 light_space =
            _light_system->get_directional()->get_light_space_matrix(rp_index);
        shader->set_uniform(UNIFORM_ID(light_space), &light_space);
    }

  private:

    struct Uniforms {
        UNIFORM_NAME(light_space);
        UNIFORM_NAME(model);
    } _u_names;
};

} // namespace ENGINE_NAMESPACE