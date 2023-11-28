#pragma once

#include "render_view.hpp"
#include "systems/shader_system.hpp"

namespace ENGINE_NAMESPACE {

class RenderViewUI : public RenderView {
  public:
    RenderViewUI(ShaderSystem* const shader_system, const Config& config);
    virtual ~RenderViewUI() override;

    virtual RenderViewPacket on_build_pocket() override;
    virtual void on_resize(const uint32 width, const uint32 height) override;
    virtual void on_render(
        Renderer* const         renderer,
        const RenderViewPacket& packet,
        const uint64            frame_number,
        const uint64            render_target_index
    ) override;

    virtual void set_render_data_ref(MeshRenderData* const data) {
        _render_data = data;
    };

  protected:
    Shader*   _shader;
    float32   _near_clip;
    float32   _far_clip;
    glm::mat4 _view_matrix;
    glm::mat4 _proj_matrix;
    // uint32 _render_mode; // TODO: Maybe

    DebugViewMode _render_mode = DebugViewMode::Default;

    const MeshRenderData* _render_data = nullptr;

    virtual void apply_globals(const uint64 frame_number) const;
    virtual void apply_locals(const glm::mat4 model) const;
};

} // namespace ENGINE_NAMESPACE