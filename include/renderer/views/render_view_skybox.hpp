#pragma once

#include "render_view.hpp"
#include "resources/skybox.hpp"
#include "systems/shader_system.hpp"

namespace ENGINE_NAMESPACE {

class RenderViewSkybox : public RenderView {
  public:
    RenderViewSkybox(
        const Config&       config,
        ShaderSystem* const shader_system,
        Camera* const       world_camera
    );
    virtual ~RenderViewSkybox() override;

    virtual Packet* on_build_pocket() override;
    virtual void    on_resize(const uint32 width, const uint32 height) override;
    virtual void    on_render(
           Renderer* const     renderer,
           const Packet* const packet,
           const uint64        frame_number,
           const uint64        render_target_index
       ) override;

    virtual void set_skybox_ref(const Skybox* const data) { _skybox = data; };

  protected:
    Shader*   _shader;
    float32   _fov;
    float32   _near_clip;
    float32   _far_clip;
    glm::mat4 _proj_matrix;
    Camera*   _world_camera;

    const Skybox* _skybox {};

    // Uniforms
    struct UIndex {
        uint16 projection   = -1;
        uint16 view         = -1;
        uint16 cube_texture = -1;

        UIndex() {}
        UIndex(const Shader* const shader);
    };
    UIndex _u_index {};

    virtual void apply_globals(const uint64 frame_number) const;
    virtual void apply_instance(const uint64 frame_number) const;
};

} // namespace ENGINE_NAMESPACE