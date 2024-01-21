#pragma once

#include "render_module.hpp"
#include "renderer/views/render_view_perspective.hpp"
#include "systems/geometry_system.hpp"

namespace ENGINE_NAMESPACE {

class RenderModuleFullScreen : public RenderModule {
  public:
    struct Config : public RenderModule::Config {
        RenderViewPerspective* perspective_view;

        Config(
            Vector<RenderModule::PassConfig> passes,
            RenderViewPerspective*           perspective_view
        )
            : RenderModule::Config(passes), perspective_view(perspective_view) {
        }
    };

  public:
    using RenderModule::RenderModule;

    void initialize(const Config& config) {
        _perspective_view = config.perspective_view;

        // Create full screen render packet
        // Size is [2, 2], since shader converts positions: [0, 2] -> [-1, 1]
        _full_screen_geometry =
            _geometry_system->generate_ui_rectangle("full_screen_geom", 2, 2);
    }

  protected:
    RenderViewPerspective* _perspective_view;
    void                   on_render(
                          const ModulePacket* const packet,
                          const uint64              frame_number,
                          uint32                    rp_index
                      ) override {
        // Draw 1 geometry
        _renderer->draw_geometry(_full_screen_geometry);
    }

  private:
    Geometry* _full_screen_geometry {};
};

} // namespace ENGINE_NAMESPACE