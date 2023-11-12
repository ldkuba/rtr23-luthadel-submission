#pragma once

#include "render_view.hpp"
#include "systems/shader_system.hpp"

namespace ENGINE_NAMESPACE {

class RenderViewWorld : public RenderView {
  public:
    Property<DebugViewMode> render_mode {
        GET { return _render_mode; }
        SET { _render_mode = value; }
    };

    RenderViewWorld(
        const Config&       config,
        ShaderSystem* const shader_system,
        Camera* const       world_camera
    );
    ~RenderViewWorld();

    virtual RenderViewPacket on_build_pocket() override;
    virtual void on_resize(const uint32 width, const uint32 height) override;
    virtual void on_render(
        Renderer* const         renderer,
        const RenderViewPacket& packet,
        const uint64            frame_number,
        const uint64            render_target_index
    ) override;

  protected:
    Shader*   _shader;
    float32   _fov;
    float32   _near_clip;
    float32   _far_clip;
    glm::mat4 _proj_matrix;
    Camera*   _world_camera;
    glm::vec4 _ambient_color;

    DebugViewMode _render_mode = DebugViewMode::Default;

    virtual void apply_globals(const uint64 frame_number) const;
    virtual void apply_locals(const glm::mat4 model) const;
};

} // namespace ENGINE_NAMESPACE