#pragma once

#include "render_module.hpp"
#include "renderer/views/render_view_perspective.hpp"
#include "systems/light_system.hpp"

namespace ENGINE_NAMESPACE {

class RenderModuleShadowmapPoint : public RenderModule {
  public:
    using RenderModule::RenderModule;

    void initialize(const RenderModule::Config& config) {
        setup_uniform_indices(_u_names.light_space);
        setup_uniform_indices(_u_names.model);
    }

    void render(const ModulePacket* const packet, const uint64 frame_number)
        override {

        // // Transition when necessary
        // for (const auto& map : _own_maps)
        //     if (map->texture->is_render_target())
        //         map->texture->transition_render_target(frame_number);

        // // Setup shader
        // _shader->use();
        // // Update render frame number
        // _shader->rendered_frame_number = frame_number;

        // uint32 light_index = 0;
        // for (auto* light : _light_system->get_point()) {
        //     if (!light->get_shadows_enabled()) continue;
        //     if (!light->recalculate_shadowmap) continue;

        //     // Disable for easier debugging, otherwise doesnt render when light
        //     // doens't move
        //     // light->recalculate_shadowmap = false;

        //     const std::array<glm::mat4, 6> light_spaces =
        //         light->get_light_space_matrices();

        //     uint32 camera_index = 0;
        //     for (auto* view : light->get_render_views()) {

        //         // Set viewport and scissors
        //         uint32    texture_index = light_index * 6 + camera_index;
        //         glm::vec2 texture_pos =
        //             glm::vec2(texture_index % 8, texture_index / 8) * 1024.0f;
        //         const glm::vec4 rect {
        //             texture_pos.x, texture_pos.y, 1024.0f, 1024.0f
        //         };

        //         // Start render pass
        //         _renderpass->begin(rect);

        //         // Update uniforms
        //         update_light_uniforms(camera_index, light_spaces);

        //         const auto geometry_data =
        //             view->get_visible_render_data(frame_number);
        //         for (const auto& geo_data : geometry_data) {
        //             // Apply local
        //             _shader->set_uniform(_u_index.model, &geo_data.model);

        //             // Draw geometry
        //             _renderer->draw_geometry(geo_data.geometry);
        //         }

        //         // End render pass
        //         _renderpass->end();

        //         camera_index++;
        //     }
        //     light_index++;
        // }
    }

  protected:
    void on_render(const ModulePacket* const packet, const uint64 frame_number, uint32 rp_index)
        override {
          // uint32 light_index = 0;
          // for (auto* light : _light_system->get_point()) {
          //     if (!light->get_shadows_enabled()) continue;
          //     if (!light->recalculate_shadowmap) continue;

          //     // Disable for easier debugging, otherwise doesnt render when light
          //     // doens't move
          //     // light->recalculate_shadowmap = false;

          //     const std::array<glm::mat4, 6> light_spaces =
          //         light->get_light_space_matrices();

          //     uint32 camera_index = 0;
          //     for (auto* view : light->get_render_views()) {

          //         // Set viewport and scissors
          //         uint32    texture_index = light_index * 6 + camera_index;
          //         glm::vec2 texture_pos =
          //             glm::vec2(texture_index % 8, texture_index / 8) * 1024.0f;
          //         const glm::vec4 rect {
          //             texture_pos.x, texture_pos.y, 1024.0f, 1024.0f
          //         };

          //         _renderpass->set_viewport(rect);
          //         _renderpass->set_scissors(rect);

          //         // Update uniforms
          //         update_light_uniforms(camera_index, light_spaces);

          //         const auto geometry_data =
          //             view->get_visible_render_data(frame_number);
          //         for (const auto& geo_data : geometry_data) {
          //             // Apply local
          //             _shader->set_uniform(_u_index.model, &geo_data.model);

          //             // Draw geometry
          //             _renderer->draw_geometry(geo_data.geometry);
          //         }

          //         camera_index++;
          //     }
          //     light_index++;
          // }
    }

    void update_light_uniforms(
        uint32 camera_index, const std::array<glm::mat4, 6>& light_spaces
    ) {

        // _shader->apply_global();
    }

    void apply_globals(uint32 rp_index) const override {}

  private:
    struct Uniforms {
        UNIFORM_NAME(light_space);
        UNIFORM_NAME(model);
    };
    Uniforms _u_names {};
};

} // namespace ENGINE_NAMESPACE